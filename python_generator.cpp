#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>

class PythonGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "from typing import List, Optional, Any\n"
            << "from pydantic import BaseModel, Field\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "from enum import Enum\n\n"
                        << "class " << name << "(Enum):\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << value << " = \"" << value << "\"\n";
                    }
                    outFile << "\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "class " << className << "(BaseModel):\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "# " << schema["properties"][key]["description"] << "\n";
            }
            outFile << std::string(config.indentSize, ' ') << key << ": " << type << " = Field(...)\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        outFile << "\n";
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "import unittest\n"
            << "import json\n"
            << "from " << className.substr(0, 1) + className.substr(1) << " import " << className << "\n\n"
            << "class Test" << className << "(unittest.TestCase):\n"
            << std::string(config.indentSize, ' ') << "def test_serialization_deserialization(self):\n"
            << std::string(config.indentSize * 2, ' ') << "sample_json = " << sampleData.dump() << "\n"
            << std::string(config.indentSize * 2, ' ') << "obj = " << className << "(**sample_json)\n"
            << std::string(config.indentSize * 2, ' ') << "serialized = json.loads(obj.json())\n"
            << std::string(config.indentSize * 2, ' ') << "self.assertEqual(sample_json, serialized)\n\n"
            << "if __name__ == '__main__':\n"
            << std::string(config.indentSize, ' ') << "unittest.main()\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "Optional[Any]";
        if (value.is_boolean()) return "bool";
        if (value.is_number_integer()) return "int";
        if (value.is_number_float()) return "float";
        if (value.is_string()) return "str";
        if (value.is_array()) {
            if (!value.empty()) {
                return "List[" + toLanguageType(value[0], config) + "]";
            }
            return "List[Any]";
        }
        if (value.is_object()) return key.substr(0, 1) + key.substr(1);
        return "Any";
    }
};
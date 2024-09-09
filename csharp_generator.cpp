#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>

class CSharpGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "using System;\n"
            << "using System.Collections.Generic;\n"
            << "using Newtonsoft.Json;\n\n"
            << "namespace JsonModel\n{\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << std::string(config.indentSize, ' ') << "public enum " << name << "\n"
                        << std::string(config.indentSize, ' ') << "{\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize * 2, ' ') << value << ",\n";
                    }
                    outFile << std::string(config.indentSize, ' ') << "}\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << std::string(config.indentSize, ' ') << "public class " << className << "\n"
            << std::string(config.indentSize, ' ') << "{\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize * 2, ' ') << "/// <summary>\n"
                    << std::string(config.indentSize * 2, ' ') << "/// " << schema["properties"][key]["description"] << "\n"
                    << std::string(config.indentSize * 2, ' ') << "/// </summary>\n";
            }
            outFile << std::string(config.indentSize * 2, ' ') << "[JsonProperty(\"" << key << "\")]\n"
                << std::string(config.indentSize * 2, ' ') << "public " << type << " " << key << " { get; set; }\n\n";

            if (type.find("class") != std::string::npos) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }

        outFile << std::string(config.indentSize, ' ') << "}\n\n";
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "using NUnit.Framework;\n"
            << "using Newtonsoft.Json;\n\n"
            << "namespace JsonModel.Tests\n"
            << "{\n"
            << std::string(config.indentSize, ' ') << "[TestFixture]\n"
            << std::string(config.indentSize, ' ') << "public class " << className << "Tests\n"
            << std::string(config.indentSize, ' ') << "{\n"
            << std::string(config.indentSize * 2, ' ') << "[Test]\n"
            << std::string(config.indentSize * 2, ' ') << "public void SerializationDeserialization()\n"
            << std::string(config.indentSize * 2, ' ') << "{\n"
            << std::string(config.indentSize * 3, ' ') << "var sampleJson = " << sampleData.dump() << ";\n"
            << std::string(config.indentSize * 3, ' ') << "var obj = JsonConvert.DeserializeObject<" << className << ">(sampleJson);\n"
            << std::string(config.indentSize * 3, ' ') << "var serialized = JsonConvert.SerializeObject(obj);\n"
            << std::string(config.indentSize * 3, ' ') << "Assert.AreEqual(sampleJson, serialized);\n"
            << std::string(config.indentSize * 2, ' ') << "}\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "object";
        if (value.is_boolean()) return "bool";
        if (value.is_number_integer()) return "int";
        if (value.is_number_float()) return "double";
        if (value.is_string()) return "string";
        if (value.is_array()) {
            if (!value.empty()) {
                return "List<" + toLanguageType(value[0], config) + ">";
            }
            return "List<object>";
        }
        if (value.is_object()) return "class";
        return "object";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << std::string(config.indentSize * 2, ' ') << "public bool IsValid()\n"
            << std::string(config.indentSize * 2, ' ') << "{\n"
            << std::string(config.indentSize * 3, ' ') << "// Implement validation logic here\n"
            << std::string(config.indentSize * 3, ' ') << "return true;\n"
            << std::string(config.indentSize * 2, ' ') << "}\n";
    }
};
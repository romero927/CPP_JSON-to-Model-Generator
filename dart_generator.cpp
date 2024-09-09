#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

class DartGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "import 'package:json_annotation/json_annotation.dart';\n\n"
            << "part '" << config.outputFile.substr(0, config.outputFile.find_last_of('.')) << ".g.dart';\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "enum " << name << " {\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << "@JsonValue('" << value << "')\n"
                            << std::string(config.indentSize, ' ') << value << ",\n";
                    }
                    outFile << "}\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "@JsonSerializable()\n"
            << "class " << className << " {\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "/// " << schema["properties"][key]["description"] << "\n";
            }
            outFile << std::string(config.indentSize, ' ') << "@JsonKey(name: '" << key << "')\n"
                << std::string(config.indentSize, ' ') << "final " << type << " " << key << ";\n\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        // Constructor
        outFile << std::string(config.indentSize, ' ') << className << "({\n";
        for (auto& [key, value] : data.items()) {
            outFile << std::string(config.indentSize * 2, ' ') << "required this." << key << ",\n";
        }
        outFile << std::string(config.indentSize, ' ') << "});\n\n";

        // fromJson and toJson methods
        outFile << std::string(config.indentSize, ' ') << "factory " << className << ".fromJson(Map<String, dynamic> json) => _$" << className << "FromJson(json);\n\n"
            << std::string(config.indentSize, ' ') << "Map<String, dynamic> toJson() => _$" << className << "ToJson(this);\n";

        outFile << "}\n\n";

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "import 'package:test/test.dart';\n"
            << "import 'dart:convert';\n"
            << "import '" << config.outputFile << "';\n\n"
            << "void main() {\n"
            << std::string(config.indentSize, ' ') << "test('$" << className << " serialization and deserialization', () {\n"
            << std::string(config.indentSize * 2, ' ') << "final sampleJson = '" << sampleData.dump() << "';\n"
            << std::string(config.indentSize * 2, ' ') << "final jsonMap = json.decode(sampleJson) as Map<String, dynamic>;\n"
            << std::string(config.indentSize * 2, ' ') << "final obj = " << className << ".fromJson(jsonMap);\n"
            << std::string(config.indentSize * 2, ' ') << "final serialized = json.encode(obj.toJson());\n"
            << std::string(config.indentSize * 2, ' ') << "expect(json.decode(serialized), equals(jsonMap));\n"
            << std::string(config.indentSize, ' ') << "});\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "dynamic";
        if (value.is_boolean()) return "bool";
        if (value.is_number_integer()) return "int";
        if (value.is_number_float()) return "double";
        if (value.is_string()) return "String";
        if (value.is_array()) {
            if (!value.empty()) {
                return "List<" + toLanguageType(value[0], config) + ">";
            }
            return "List<dynamic>";
        }
        if (value.is_object()) return key.substr(0, 1) + key.substr(1);
        return "dynamic";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << "extension " << className << "Validator on " << className << " {\n"
            << std::string(config.indentSize, ' ') << "bool isValid() {\n"
            << std::string(config.indentSize * 2, ' ') << "// TODO: Implement validation logic\n"
            << std::string(config.indentSize * 2, ' ') << "return true;\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n\n";
    }
};
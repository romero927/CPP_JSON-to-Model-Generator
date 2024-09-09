#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>

class TypeScriptGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        // TypeScript doesn't need any special imports for basic types
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "export enum " << name << " {\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << value << " = \"" << value << "\",\n";
                    }
                    outFile << "}\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "export interface " << className << " {\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "/**\n"
                    << std::string(config.indentSize, ' ') << " * " << schema["properties"][key]["description"] << "\n"
                    << std::string(config.indentSize, ' ') << " */\n";
            }
            outFile << std::string(config.indentSize, ' ') << key << ": " << type << ";\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        outFile << "}\n\n";

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "import { " << className << " } from './" << className << "';\n\n"
            << "describe('" << className << "', () => {\n"
            << std::string(config.indentSize, ' ') << "it('should serialize and deserialize correctly', () => {\n"
            << std::string(config.indentSize * 2, ' ') << "const sampleData: " << className << " = " << sampleData.dump() << ";\n"
            << std::string(config.indentSize * 2, ' ') << "const serialized = JSON.stringify(sampleData);\n"
            << std::string(config.indentSize * 2, ' ') << "const deserialized: " << className << " = JSON.parse(serialized);\n"
            << std::string(config.indentSize * 2, ' ') << "expect(deserialized).toEqual(sampleData);\n"
            << std::string(config.indentSize, ' ') << "});\n"
            << "});\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "null";
        if (value.is_boolean()) return "boolean";
        if (value.is_number()) return "number";
        if (value.is_string()) return "string";
        if (value.is_array()) {
            if (!value.empty()) {
                return toLanguageType(value[0], config) + "[]";
            }
            return "any[]";
        }
        if (value.is_object()) return key.substr(0, 1) + key.substr(1);
        return "any";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << "export function is" << className << "Valid(obj: " << className << "): boolean {\n"
            << std::string(config.indentSize, ' ') << "// Implement validation logic here\n"
            << std::string(config.indentSize, ' ') << "return true;\n"
            << "}\n\n";
    }
};
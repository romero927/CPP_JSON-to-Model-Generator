#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

class RustGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "use serde::{Serialize, Deserialize};\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "#[derive(Debug, Serialize, Deserialize)]\n"
                        << "pub enum " << name << " {\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << value << ",\n";
                    }
                    outFile << "}\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "#[derive(Debug, Serialize, Deserialize)]\n"
            << "pub struct " << className << " {\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "/// " << schema["properties"][key]["description"] << "\n";
            }
            outFile << std::string(config.indentSize, ' ') << "#[serde(rename = \"" << key << "\")]\n"
                << std::string(config.indentSize, ' ') << "pub " << key << ": " << type << ",\n";

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
        testFile << "#[cfg(test)]\n"
            << "mod tests {\n"
            << std::string(config.indentSize, ' ') << "use super::*;\n"
            << std::string(config.indentSize, ' ') << "use serde_json;\n\n"
            << std::string(config.indentSize, ' ') << "#[test]\n"
            << std::string(config.indentSize, ' ') << "fn test_" << className << "_serialization_deserialization() {\n"
            << std::string(config.indentSize * 2, ' ') << "let sample_json = r#\"" << sampleData.dump() << "\"#;\n"
            << std::string(config.indentSize * 2, ' ') << "let obj: " << className << " = serde_json::from_str(sample_json).unwrap();\n"
            << std::string(config.indentSize * 2, ' ') << "let serialized = serde_json::to_string(&obj).unwrap();\n"
            << std::string(config.indentSize * 2, ' ') << "let deserialized: serde_json::Value = serde_json::from_str(&serialized).unwrap();\n"
            << std::string(config.indentSize * 2, ' ') << "let original: serde_json::Value = serde_json::from_str(sample_json).unwrap();\n"
            << std::string(config.indentSize * 2, ' ') << "assert_eq!(original, deserialized);\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "Option<serde_json::Value>";
        if (value.is_boolean()) return "bool";
        if (value.is_number_integer()) return "i64";
        if (value.is_number_float()) return "f64";
        if (value.is_string()) return "String";
        if (value.is_array()) {
            if (!value.empty()) {
                return "Vec<" + toLanguageType(value[0], config) + ">";
            }
            return "Vec<serde_json::Value>";
        }
        if (value.is_object()) return key.substr(0, 1) + key.substr(1);
        return "serde_json::Value";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << "impl " << className << " {\n"
            << std::string(config.indentSize, ' ') << "pub fn is_valid(&self) -> bool {\n"
            << std::string(config.indentSize * 2, ' ') << "// Implement validation logic here\n"
            << std::string(config.indentSize * 2, ' ') << "true\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n\n";
    }
};
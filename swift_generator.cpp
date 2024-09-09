#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

class SwiftGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "import Foundation\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "enum " << name << ": String, Codable {\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << "case " << value << " = \"" << value << "\"\n";
                    }
                    outFile << "}\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "struct " << className << ": Codable {\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "/// " << schema["properties"][key]["description"] << "\n";
            }
            outFile << std::string(config.indentSize, ' ') << "let " << key << ": " << type << "\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        // Generate CodingKeys enum
        outFile << "\n" << std::string(config.indentSize, ' ') << "enum CodingKeys: String, CodingKey {\n";
        for (auto& [key, value] : data.items()) {
            outFile << std::string(config.indentSize * 2, ' ') << "case " << key << "\n";
        }
        outFile << std::string(config.indentSize, ' ') << "}\n";

        outFile << "}\n\n";

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "import XCTest\n"
            << "@testable import YourModuleName\n\n"
            << "class " << className << "Tests: XCTestCase {\n\n"
            << std::string(config.indentSize, ' ') << "func testSerializationDeserialization() throws {\n"
            << std::string(config.indentSize * 2, ' ') << "let sampleJSON = \"\"\"\n"
            << sampleData.dump(4) << "\n"
            << std::string(config.indentSize * 2, ' ') << "\"\"\"\n\n"
            << std::string(config.indentSize * 2, ' ') << "let jsonData = sampleJSON.data(using: .utf8)!\n"
            << std::string(config.indentSize * 2, ' ') << "let decoder = JSONDecoder()\n"
            << std::string(config.indentSize * 2, ' ') << "let obj = try decoder.decode(" << className << ".self, from: jsonData)\n\n"
            << std::string(config.indentSize * 2, ' ') << "let encoder = JSONEncoder()\n"
            << std::string(config.indentSize * 2, ' ') << "encoder.outputFormatting = .prettyPrinted\n"
            << std::string(config.indentSize * 2, ' ') << "let encodedData = try encoder.encode(obj)\n"
            << std::string(config.indentSize * 2, ' ') << "let encodedJSON = String(data: encodedData, encoding: .utf8)!\n\n"
            << std::string(config.indentSize * 2, ' ') << "XCTAssertEqual(sampleJSON, encodedJSON)\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "Any?";
        if (value.is_boolean()) return "Bool";
        if (value.is_number_integer()) return "Int";
        if (value.is_number_float()) return "Double";
        if (value.is_string()) return "String";
        if (value.is_array()) {
            if (!value.empty()) {
                return "[" + toLanguageType(value[0], config) + "]";
            }
            return "[Any]";
        }
        if (value.is_object()) return key.substr(0, 1) + key.substr(1);
        return "Any";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << "extension " << className << " {\n"
            << std::string(config.indentSize, ' ') << "func isValid() -> Bool {\n"
            << std::string(config.indentSize * 2, ' ') << "// Implement validation logic here\n"
            << std::string(config.indentSize * 2, ' ') << "return true\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n\n";
    }
};
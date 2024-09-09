#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

class GoGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "package model\n\n"
            << "import (\n"
            << "\t\"encoding/json\"\n"
            << ")\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "type " << name << " string\n\n"
                        << "const (\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << name << value << " " << name << " = \"" << value << "\"\n";
                    }
                    outFile << ")\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "type " << className << " struct {\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            std::string capitalizedKey = key;
            capitalizedKey[0] = std::toupper(capitalizedKey[0]);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "// " << schema["properties"][key]["description"] << "\n";
            }
            outFile << std::string(config.indentSize, ' ') << capitalizedKey << " " << type << " `json:\"" << key << "\"`\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + capitalizedKey;
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
        testFile << "package model_test\n\n"
            << "import (\n"
            << "\t\"encoding/json\"\n"
            << "\t\"testing\"\n\n"
            << "\t\"github.com/stretchr/testify/assert\"\n"
            << "\t\"your_project/model\"\n"
            << ")\n\n"
            << "func Test" << className << "SerializationDeserialization(t *testing.T) {\n"
            << std::string(config.indentSize, ' ') << "sampleJSON := []byte(`" << sampleData.dump() << "`)\n"
            << std::string(config.indentSize, ' ') << "var obj model." << className << "\n"
            << std::string(config.indentSize, ' ') << "err := json.Unmarshal(sampleJSON, &obj)\n"
            << std::string(config.indentSize, ' ') << "assert.NoError(t, err)\n\n"
            << std::string(config.indentSize, ' ') << "serialized, err := json.Marshal(obj)\n"
            << std::string(config.indentSize, ' ') << "assert.NoError(t, err)\n\n"
            << std::string(config.indentSize, ' ') << "var deserialized map[string]interface{}\n"
            << std::string(config.indentSize, ' ') << "err = json.Unmarshal(serialized, &deserialized)\n"
            << std::string(config.indentSize, ' ') << "assert.NoError(t, err)\n\n"
            << std::string(config.indentSize, ' ') << "var original map[string]interface{}\n"
            << std::string(config.indentSize, ' ') << "err = json.Unmarshal(sampleJSON, &original)\n"
            << std::string(config.indentSize, ' ') << "assert.NoError(t, err)\n\n"
            << std::string(config.indentSize, ' ') << "assert.Equal(t, original, deserialized)\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "interface{}";
        if (value.is_boolean()) return "bool";
        if (value.is_number_integer()) return "int";
        if (value.is_number_float()) return "float64";
        if (value.is_string()) return "string";
        if (value.is_array()) {
            if (!value.empty()) {
                return "[]" + toLanguageType(value[0], config);
            }
            return "[]interface{}";
        }
        if (value.is_object()) {
            std::string capitalizedKey = key;
            capitalizedKey[0] = std::toupper(capitalizedKey[0]);
            return "*" + capitalizedKey;
        }
        return "interface{}";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << "func (m *" << className << ") IsValid() bool {\n"
            << std::string(config.indentSize, ' ') << "// Implement validation logic here\n"
            << std::string(config.indentSize, ' ') << "return true\n"
            << "}\n\n";
    }
};
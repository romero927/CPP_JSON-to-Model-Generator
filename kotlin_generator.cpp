#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

class KotlinGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "import kotlinx.serialization.*\n"
            << "import kotlinx.serialization.json.*\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "@Serializable\n"
                        << "enum class " << name << " {\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << "@SerialName(\"" << value << "\")\n"
                            << std::string(config.indentSize, ' ') << value << ",\n";
                    }
                    outFile << "}\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "@Serializable\n"
            << "data class " << className << "(\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "/** " << schema["properties"][key]["description"] << " */\n";
            }
            outFile << std::string(config.indentSize, ' ') << "@SerialName(\"" << key << "\")\n"
                << std::string(config.indentSize, ' ') << "val " << key << ": " << type << ",\n\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        outFile << ")\n\n";

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "import org.junit.jupiter.api.Test\n"
            << "import kotlinx.serialization.json.Json\n"
            << "import kotlin.test.assertEquals\n\n"
            << "class " << className << "Test {\n\n"
            << std::string(config.indentSize, ' ') << "@Test\n"
            << std::string(config.indentSize, ' ') << "fun testSerializationDeserialization() {\n"
            << std::string(config.indentSize * 2, ' ') << "val sampleJson = \"\"\"" << sampleData.dump() << "\"\"\"\n"
            << std::string(config.indentSize * 2, ' ') << "val obj = Json.decodeFromString<" << className << ">(sampleJson)\n"
            << std::string(config.indentSize * 2, ' ') << "val serialized = Json.encodeToString(obj)\n"
            << std::string(config.indentSize * 2, ' ') << "val deserialized = Json.decodeFromString<" << className << ">(serialized)\n"
            << std::string(config.indentSize * 2, ' ') << "assertEquals(obj, deserialized)\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "Any?";
        if (value.is_boolean()) return "Boolean";
        if (value.is_number_integer()) return "Int";
        if (value.is_number_float()) return "Double";
        if (value.is_string()) return "String";
        if (value.is_array()) {
            if (!value.empty()) {
                return "List<" + toLanguageType(value[0], config) + ">";
            }
            return "List<Any?>";
        }
        if (value.is_object()) return key.substr(0, 1) + key.substr(1);
        return "Any";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << "fun " << className << ".isValid(): Boolean {\n"
            << std::string(config.indentSize, ' ') << "// TODO: Implement validation logic\n"
            << std::string(config.indentSize, ' ') << "return true\n"
            << "}\n\n";
    }
};
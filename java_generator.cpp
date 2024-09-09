#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>

class JavaGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "import java.util.List;\n"
            << "import com.fasterxml.jackson.annotation.JsonProperty;\n"
            << "import com.fasterxml.jackson.databind.ObjectMapper;\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "public enum " << name << " {\n";
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
        outFile << "public class " << className << " {\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "/**\n"
                    << std::string(config.indentSize, ' ') << " * " << schema["properties"][key]["description"] << "\n"
                    << std::string(config.indentSize, ' ') << " */\n";
            }
            outFile << std::string(config.indentSize, ' ') << "@JsonProperty(\"" << key << "\")\n"
                << std::string(config.indentSize, ' ') << "private " << type << " " << key << ";\n\n";

            if (type == "class") {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        generateGettersAndSetters(data, outFile, config);

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }

        outFile << "}\n\n";
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "import org.junit.jupiter.api.Test;\n"
            << "import static org.junit.jupiter.api.Assertions.*;\n"
            << "import com.fasterxml.jackson.databind.ObjectMapper;\n\n"
            << "public class " << className << "Test {\n"
            << std::string(config.indentSize, ' ') << "@Test\n"
            << std::string(config.indentSize, ' ') << "public void testSerializationDeserialization() throws Exception {\n"
            << std::string(config.indentSize * 2, ' ') << "String sampleJson = " << sampleData.dump() << ";\n"
            << std::string(config.indentSize * 2, ' ') << "ObjectMapper objectMapper = new ObjectMapper();\n"
            << std::string(config.indentSize * 2, ' ') << className << " obj = objectMapper.readValue(sampleJson, " << className << ".class);\n"
            << std::string(config.indentSize * 2, ' ') << "String serialized = objectMapper.writeValueAsString(obj);\n"
            << std::string(config.indentSize * 2, ' ') << "assertEquals(sampleJson, serialized);\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "Object";
        if (value.is_boolean()) return "boolean";
        if (value.is_number_integer()) return "int";
        if (value.is_number_float()) return "double";
        if (value.is_string()) return "String";
        if (value.is_array()) {
            if (!value.empty()) {
                return "List<" + toLanguageType(value[0], config) + ">";
            }
            return "List<Object>";
        }
        if (value.is_object()) return "class";
        return "Object";
    }

private:
    void generateGettersAndSetters(const json& data, std::ofstream& outFile, const Config& config) {
        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            std::string capitalizedKey = key;
            capitalizedKey[0] = std::toupper(capitalizedKey[0]);

            outFile << std::string(config.indentSize, ' ') << "public " << type << " get" << capitalizedKey << "() {\n"
                << std::string(config.indentSize * 2, ' ') << "return " << key << ";\n"
                << std::string(config.indentSize, ' ') << "}\n\n"
                << std::string(config.indentSize, ' ') << "public void set" << capitalizedKey << "(" << type << " " << key << ") {\n"
                << std::string(config.indentSize * 2, ' ') << "this." << key << " = " << key << ";\n"
                << std::string(config.indentSize, ' ') << "}\n\n";
        }
    }

    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << std::string(config.indentSize, ' ') << "public boolean isValid() {\n"
            << std::string(config.indentSize * 2, ' ') << "// Implement validation logic here\n"
            << std::string(config.indentSize * 2, ' ') << "return true;\n"
            << std::string(config.indentSize, ' ') << "}\n";
    }
};
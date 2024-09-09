#include "json_model_generator.hpp"
#include <iostream>

class CppGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "#pragma once\n\n"
            << "#include <string>\n"
            << "#include <vector>\n"
            << "#include <nlohmann/json.hpp>\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "enum class " << name << " {\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << value << ",\n";
                    }
                    outFile << "};\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "class " << className << " {\n"
            << "public:\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "// " << schema["properties"][key]["description"] << "\n";
            }
            outFile << std::string(config.indentSize, ' ') << type << " " << key << ";\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        generateSerializationMethods(className, data, outFile, config);

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }

        outFile << "};\n\n";
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "#include <gtest/gtest.h>\n"
            << "#include \"" << className << ".hpp\"\n\n"
            << "TEST(" << className << "Test, SerializationDeserialization) {\n"
            << "    nlohmann::json sampleJson = " << sampleData.dump() << ";\n"
            << "    " << className << " obj = " << className << "::from_json(sampleJson);\n"
            << "    nlohmann::json serialized = obj.to_json();\n"
            << "    EXPECT_EQ(sampleJson, serialized);\n"
            << "}\n\n"
            << "TEST(" << className << "Test, Validation) {\n"
            << "    " << className << " validObj = " << className << "::from_json(" << sampleData.dump() << ");\n"
            << "    EXPECT_TRUE(validObj.is_valid());\n"
            << "    // Add invalid object test here\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "std::nullptr_t";
        if (value.is_boolean()) return "bool";
        if (value.is_number_integer()) return "int64_t";
        if (value.is_number_unsigned()) return "uint64_t";
        if (value.is_number_float()) return "double";
        if (value.is_string()) return "std::string";
        if (value.is_array()) {
            if (!value.empty()) {
                return "std::vector<" + toLanguageType(value[0], config) + ">";
            }
            return "std::vector<std::nullptr_t>";
        }
        if (value.is_object()) return "class";
        return "void*";
    }

private:
    void generateSerializationMethods(const std::string& className, const json& data, std::ofstream& outFile, const Config& config) {
        outFile << std::string(config.indentSize, ' ') << "nlohmann::json to_json() const {\n"
            << std::string(config.indentSize * 2, ' ') << "return nlohmann::json({\n";

        for (auto& [key, value] : data.items()) {
            outFile << std::string(config.indentSize * 3, ' ') << "{\"" << key << "\", " << key << "},\n";
        }

        outFile << std::string(config.indentSize * 2, ' ') << "});\n"
            << std::string(config.indentSize, ' ') << "}\n\n"
            << std::string(config.indentSize, ' ') << "static " << className << " from_json(const nlohmann::json& j) {\n"
            << std::string(config.indentSize * 2, ' ') << className << " obj;\n";

        for (auto& [key, value] : data.items()) {
            outFile << std::string(config.indentSize * 2, ' ') << "obj." << key << " = j.at(\"" << key << "\").get<" << toLanguageType(value, config, key) << ">();\n";
        }

        outFile << std::string(config.indentSize * 2, ' ') << "return obj;\n"
            << std::string(config.indentSize, ' ') << "}\n";
    }

    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << std::string(config.indentSize, ' ') << "bool is_valid() const {\n"
            << std::string(config.indentSize * 2, ' ') << "// TODO: Implement validation logic\n"
            << std::string(config.indentSize * 2, ' ') << "return true;\n"
            << std::string(config.indentSize, ' ') << "}\n";
    }
};
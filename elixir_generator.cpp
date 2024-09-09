#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

class ElixirGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "defmodule " << config.outputFile.substr(0, config.outputFile.find_last_of('.')) << " do\n"
            << std::string(config.indentSize, ' ') << "use Ecto.Schema\n"
            << std::string(config.indentSize, ' ') << "import Ecto.Changeset\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << std::string(config.indentSize, ' ') << "@type " << name << " :: ";
                    for (size_t i = 0; i < def["enum"].size(); ++i) {
                        outFile << ":" << def["enum"][i];
                        if (i < def["enum"].size() - 1) {
                            outFile << " | ";
                        }
                    }
                    outFile << "\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << std::string(config.indentSize, ' ') << "schema \"" << className.substr(0, 1) + className.substr(1) << "\" do\n";

        for (auto& [key, value] : data.items()) {
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize * 2, ' ') << "# " << schema["properties"][key]["description"] << "\n";
            }
            outFile << std::string(config.indentSize * 2, ' ') << "field :" << key << ", " << type << "\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        outFile << std::string(config.indentSize, ' ') << "end\n\n";

        generateChangeset(className, data, outFile, config);

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "defmodule " << className << "Test do\n"
            << std::string(config.indentSize, ' ') << "use ExUnit.Case\n"
            << std::string(config.indentSize, ' ') << "alias " << config.outputFile.substr(0, config.outputFile.find_last_of('.')) << "." << className << "\n\n"
            << std::string(config.indentSize, ' ') << "test \"serialization and deserialization\" do\n"
            << std::string(config.indentSize * 2, ' ') << "sample_json = \"\"\"" << sampleData.dump() << "\"\"\"\n"
            << std::string(config.indentSize * 2, ' ') << "{:ok, decoded} = Jason.decode(sample_json)\n"
            << std::string(config.indentSize * 2, ' ') << "changeset = " << className << ".changeset(%" << className << "{}, decoded)\n"
            << std::string(config.indentSize * 2, ' ') << "assert changeset.valid?\n"
            << std::string(config.indentSize * 2, ' ') << "obj = Ecto.Changeset.apply_changes(changeset)\n"
            << std::string(config.indentSize * 2, ' ') << "serialized = Jason.encode!(obj)\n"
            << std::string(config.indentSize * 2, ' ') << "assert Jason.decode!(serialized) == decoded\n"
            << std::string(config.indentSize, ' ') << "end\n"
            << "end\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return ":any";
        if (value.is_boolean()) return ":boolean";
        if (value.is_number_integer()) return ":integer";
        if (value.is_number_float()) return ":float";
        if (value.is_string()) return ":string";
        if (value.is_array()) {
            if (!value.empty()) {
                return "{:array, " + toLanguageType(value[0], config) + "}";
            }
            return "{:array, :any}";
        }
        if (value.is_object()) return ":map";
        return ":any";
    }

private:
    void generateChangeset(const std::string& className, const json& data, std::ofstream& outFile, const Config& config) {
        outFile << std::string(config.indentSize, ' ') << "def changeset(struct, params \\\\ %{}) do\n"
            << std::string(config.indentSize * 2, ' ') << "struct\n"
            << std::string(config.indentSize * 2, ' ') << "|> cast(params, [";

        bool first = true;
        for (auto& [key, value] : data.items()) {
            if (!first) {
                outFile << ", ";
            }
            outFile << ":" << key;
            first = false;
        }

        outFile << "])\n"
            << std::string(config.indentSize * 2, ' ') << "|> validate_required([";

        first = true;
        for (auto& [key, value] : data.items()) {
            if (!first) {
                outFile << ", ";
            }
            outFile << ":" << key;
            first = false;
        }

        outFile << "])\n"
            << std::string(config.indentSize, ' ') << "end\n\n";
    }

    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << std::string(config.indentSize, ' ') << "def valid?(%" << className << "{} = struct) do\n"
            << std::string(config.indentSize * 2, ' ') << "# TODO: Implement validation logic\n"
            << std::string(config.indentSize * 2, ' ') << "true\n"
            << std::string(config.indentSize, ' ') << "end\n\n";
    }
};
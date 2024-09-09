#include "json_model_generator.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

class ScalaGenerator : public LanguageGenerator {
public:
    void generateFileHeader(std::ofstream& outFile, const Config& config) override {
        outFile << "import io.circe.{Decoder, Encoder}\n"
            << "import io.circe.generic.semiauto.{deriveDecoder, deriveEncoder}\n\n";
    }

    void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) override {
        if (schema.contains("definitions")) {
            for (auto& [name, def] : schema["definitions"].items()) {
                if (def.contains("enum")) {
                    outFile << "sealed trait " << name << "\n"
                        << "object " << name << " {\n";
                    for (const auto& value : def["enum"]) {
                        outFile << std::string(config.indentSize, ' ') << "case object " << value << " extends " << name << "\n";
                    }
                    outFile << std::string(config.indentSize, ' ') << "implicit val encoder: Encoder[" << name << "] = Encoder.encodeString.contramap {\n"
                        << std::string(config.indentSize * 2, ' ') << "case ";
                    for (size_t i = 0; i < def["enum"].size(); ++i) {
                        outFile << def["enum"][i] << " => \"" << def["enum"][i] << "\"";
                        if (i < def["enum"].size() - 1) {
                            outFile << "\n" << std::string(config.indentSize * 2, ' ') << "case ";
                        }
                    }
                    outFile << "\n" << std::string(config.indentSize, ' ') << "}\n";
                    outFile << std::string(config.indentSize, ' ') << "implicit val decoder: Decoder[" << name << "] = Decoder.decodeString.emap {\n"
                        << std::string(config.indentSize * 2, ' ') << "case ";
                    for (size_t i = 0; i < def["enum"].size(); ++i) {
                        outFile << "\"" << def["enum"][i] << "\" => Right(" << def["enum"][i] << ")";
                        if (i < def["enum"].size() - 1) {
                            outFile << "\n" << std::string(config.indentSize * 2, ' ') << "case ";
                        }
                    }
                    outFile << "\n" << std::string(config.indentSize * 2, ' ') << "case _ => Left(\"Invalid " << name << "\")\n"
                        << std::string(config.indentSize, ' ') << "}\n";
                    outFile << "}\n\n";
                }
            }
        }
    }

    void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) override {
        outFile << "case class " << className << "(\n";

        for (auto it = data.begin(); it != data.end(); ++it) {
            std::string key = it.key();
            json value = it.value();
            std::string type = toLanguageType(value, config, key);
            if (config.generateDocs && schema.contains("properties") && schema["properties"].contains(key) && schema["properties"][key].contains("description")) {
                outFile << std::string(config.indentSize, ' ') << "/** " << schema["properties"][key]["description"] << " */\n";
            }
            outFile << std::string(config.indentSize, ' ') << key << ": " << type;
            if (std::next(it) != data.end()) {
                outFile << ",";
            }
            outFile << "\n";

            if (value.is_object()) {
                std::string newClassName = className + "_" + key;
                circHandler.addDependency(className, newClassName);
                generateClass(newClassName, value, schema["properties"][key], outFile, config, circHandler);
            }
        }

        outFile << ")\n\n";
        outFile << "object " << className << " {\n"
            << std::string(config.indentSize, ' ') << "implicit val encoder: Encoder[" << className << "] = deriveEncoder\n"
            << std::string(config.indentSize, ' ') << "implicit val decoder: Decoder[" << className << "] = deriveDecoder\n"
            << "}\n\n";

        if (config.generateValidation) {
            generateValidationMethod(className, schema, outFile, config);
        }
    }

    void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) override {
        testFile << "import org.scalatest.flatspec.AnyFlatSpec\n"
            << "import org.scalatest.matchers.should.Matchers\n"
            << "import io.circe.parser._\n"
            << "import io.circe.syntax._\n\n"
            << "class " << className << "Spec extends AnyFlatSpec with Matchers {\n\n"
            << std::string(config.indentSize, ' ') << "\"" << className << "\" should \"serialize and deserialize correctly\" in {\n"
            << std::string(config.indentSize * 2, ' ') << "val sampleJson = \"\"\"" << sampleData.dump() << "\"\"\"\n"
            << std::string(config.indentSize * 2, ' ') << "val decoded = decode[" << className << "](sampleJson)\n"
            << std::string(config.indentSize * 2, ' ') << "decoded.isRight shouldBe true\n"
            << std::string(config.indentSize * 2, ' ') << "val obj = decoded.right.get\n"
            << std::string(config.indentSize * 2, ' ') << "val encoded = obj.asJson.noSpaces\n"
            << std::string(config.indentSize * 2, ' ') << "val reDecoded = decode[" << className << "](encoded)\n"
            << std::string(config.indentSize * 2, ' ') << "reDecoded.isRight shouldBe true\n"
            << std::string(config.indentSize * 2, ' ') << "reDecoded.right.get shouldBe obj\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n";
    }

    std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") override {
        if (value.is_null()) return "Option[Any]";
        if (value.is_boolean()) return "Boolean";
        if (value.is_number_integer()) return "Int";
        if (value.is_number_float()) return "Double";
        if (value.is_string()) return "String";
        if (value.is_array()) {
            if (!value.empty()) {
                return "List[" + toLanguageType(value[0], config) + "]";
            }
            return "List[Any]";
        }
        if (value.is_object()) return key.substr(0, 1) + key.substr(1);
        return "Any";
    }

private:
    void generateValidationMethod(const std::string& className, const json& schema, std::ofstream& outFile, const Config& config) {
        outFile << "trait " << className << "Validator {\n"
            << std::string(config.indentSize, ' ') << "def isValid(obj: " << className << "): Boolean = {\n"
            << std::string(config.indentSize * 2, ' ') << "// TODO: Implement validation logic\n"
            << std::string(config.indentSize * 2, ' ') << "true\n"
            << std::string(config.indentSize, ' ') << "}\n"
            << "}\n\n";
    }
};
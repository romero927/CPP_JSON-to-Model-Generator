#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class Language { CPP, CSHARP, JAVA, PYTHON, GO, TYPESCRIPT, RUST, SWIFT, DART, KOTLIN, ELIXIR, SCALA };

struct Config {
    std::string inputFile;
    std::string schemaFile; // This will be optional now
    std::string outputFile;
    Language lang = Language::CPP;
    bool generateDocs = false;
    bool generateValidation = false;
    bool useBuilderPattern = false;
    bool generateImmutable = false;
    int indentSize = 4;
    std::string braceStyle = "same-line";
    std::string customTypeMappingsFile;
    bool showHelp = false;
    bool verbose = false;
    bool dryRun = false;
    bool useSchema = false; // New flag to indicate whether to use schema
};

class CircularReferenceHandler {
public:
    void addDependency(const std::string& className, const std::string& dependsOn);
    bool hasCyclicDependency(const std::string& className, std::set<std::string>& visited);
    void resolveCircularReferences(std::ofstream& outFile, const Config& config, class LanguageGenerator* generator);

private:
    std::set<std::string> generatedClasses;
    std::map<std::string, std::set<std::string>> dependencies;
};

class LanguageGenerator {
public:
    virtual ~LanguageGenerator() = default;
    virtual void generateFileHeader(std::ofstream& outFile, const Config& config) = 0;
    virtual void generateEnums(const json& schema, std::ofstream& outFile, const Config& config) = 0;
    virtual void generateClass(const std::string& className, const json& data, const json& schema,
        std::ofstream& outFile, const Config& config, CircularReferenceHandler& circHandler) = 0;
    virtual void generateUnitTests(const std::string& className, const json& sampleData,
        std::ofstream& testFile, const Config& config) = 0;
    virtual std::string toLanguageType(const json& value, const Config& config, const std::string& key = "") = 0;
};

Config parseConfig(int argc, char* argv[]);
void printUsage(const char* programName);
json readJsonFromFile(const std::string& filename);
json readSchemaFromFile(const std::string& filename);
json inferSchemaFromJson(const json& data);
LanguageGenerator* createLanguageGenerator(Language lang);
Language stringToLanguage(const std::string& lang);
std::string getFileExtension(Language lang);
std::string languageToString(Language lang);
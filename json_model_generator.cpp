#include "json_model_generator.hpp"
#include "cpp_generator.cpp"
#include "csharp_generator.cpp"
#include "dart_generator.cpp"
#include "elixir_generator.cpp"
#include "go_generator.cpp"
#include "java_generator.cpp"
#include "kotlin_generator.cpp"
#include "python_generator.cpp"
#include "rust_generator.cpp"
#include "scala_generator.cpp"
#include "swift_generator.cpp"
#include "typescript_generator.cpp"
#include <iostream>
#include <algorithm>
#include <cstring>

int main(int argc, char* argv[]) {
    Config config = parseConfig(argc, argv);

    if (config.showHelp) {
        printUsage(argv[0]);
        return 0;
    }

    try {
        json inputJson = readJsonFromFile(config.inputFile);
        json schema;

        if (config.useSchema) {
            schema = readSchemaFromFile(config.schemaFile);
        }
        else {
            schema = inferSchemaFromJson(inputJson);
        }

        if (config.verbose) {
            std::cout << "Input JSON file: " << config.inputFile << std::endl;
            if (config.useSchema) {
                std::cout << "JSON Schema file: " << config.schemaFile << std::endl;
            }
            else {
                std::cout << "Using inferred schema" << std::endl;
            }
            std::cout << "Output language: " << languageToString(config.lang) << std::endl;
            std::cout << "Output file: " << config.outputFile << std::endl;
        }

        if (config.dryRun) {
            std::cout << "Dry run mode. No files will be generated." << std::endl;
            return 0;
        }

        std::ofstream outFile(config.outputFile);
        if (!outFile.is_open()) {
            throw std::runtime_error("Unable to create output file: " + config.outputFile);
        }

        LanguageGenerator* generator = createLanguageGenerator(config.lang);
        CircularReferenceHandler circHandler;

        if (config.verbose) {
            std::cout << "Generating file header..." << std::endl;
        }
        generator->generateFileHeader(outFile, config);

        if (config.verbose) {
            std::cout << "Generating enums..." << std::endl;
        }
        generator->generateEnums(schema, outFile, config);

        if (config.verbose) {
            std::cout << "Resolving circular references..." << std::endl;
        }
        circHandler.resolveCircularReferences(outFile, config, generator);

        if (config.verbose) {
            std::cout << "Generating main class..." << std::endl;
        }
        generator->generateClass("RootModel", inputJson, schema, outFile, config, circHandler);

        outFile.close();

        if (config.verbose) {
            std::cout << "Generating unit tests..." << std::endl;
        }
        std::ofstream testFile(config.outputFile + "_test." + getFileExtension(config.lang));
        if (testFile.is_open()) {
            generator->generateUnitTests("RootModel", inputJson, testFile, config);
            testFile.close();
            if (config.verbose) {
                std::cout << "Test file '" << config.outputFile + "_test." + getFileExtension(config.lang) << "' has been generated." << std::endl;
            }
        }

        if (config.verbose) {
            std::cout << "Model file '" << config.outputFile << "' has been generated." << std::endl;
        }

        delete generator;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

Config parseConfig(int argc, char* argv[]) {
    Config config;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            config.showHelp = true;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (i + 1 < argc) config.inputFile = argv[++i];
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--schema") == 0) {
            if (i + 1 < argc) {
                config.schemaFile = argv[++i];
                config.useSchema = true;
            }
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--language") == 0) {
            if (i + 1 < argc) config.lang = stringToLanguage(argv[++i]);
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) config.outputFile = argv[++i];
        }
        else if (strcmp(argv[i], "--docs") == 0) {
            config.generateDocs = true;
        }
        else if (strcmp(argv[i], "--validation") == 0) {
            config.generateValidation = true;
        }
        else if (strcmp(argv[i], "--builder") == 0) {
            config.useBuilderPattern = true;
        }
        else if (strcmp(argv[i], "--immutable") == 0) {
            config.generateImmutable = true;
        }
        else if (strcmp(argv[i], "--indent") == 0) {
            if (i + 1 < argc) config.indentSize = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--brace-style") == 0) {
            if (i + 1 < argc) config.braceStyle = argv[++i];
        }
        else if (strcmp(argv[i], "--custom-types") == 0) {
            if (i + 1 < argc) config.customTypeMappingsFile = argv[++i];
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose = true;
        }
        else if (strcmp(argv[i], "--dry-run") == 0) {
            config.dryRun = true;
        }
    }
    return config;
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
        << "Options:\n"
        << "  -h, --help                 Show this help message\n"
        << "  -i, --input <file>         Input JSON file\n"
        << "  -s, --schema <file>        JSON Schema file (optional)\n"
        << "  -l, --language <lang>      Output language (cpp, csharp, java, python, go, typescript, rust, swift, dart, kotlin, elixir, scala)\n"
        << "  -o, --output <file>        Output file name\n"
        << "  --docs                     Generate documentation comments\n"
        << "  --validation               Generate validation methods\n"
        << "  --builder                  Use builder pattern (for supported languages)\n"
        << "  --immutable                Generate immutable objects\n"
        << "  --indent <size>            Indentation size (default: 4)\n"
        << "  --brace-style <style>      Brace style (same-line, new-line)\n"
        << "  --custom-types <file>      JSON file with custom type mappings\n"
        << "  --verbose                  Enable verbose output\n"
        << "  --dry-run                  Show what would be generated without creating files\n";
}

json readJsonFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }
    json j;
    file >> j;
    return j;
}

json readSchemaFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open schema file: " + filename);
    }
    json schema_json;
    file >> schema_json;
    return schema_json;
}

json inferSchemaFromJson(const json& data) {
    json schema;
    schema["type"] = "object";
    schema["properties"] = json::object();

    for (auto& [key, value] : data.items()) {
        if (value.is_null()) {
            schema["properties"][key]["type"] = "null";
        }
        else if (value.is_boolean()) {
            schema["properties"][key]["type"] = "boolean";
        }
        else if (value.is_number_integer()) {
            schema["properties"][key]["type"] = "integer";
        }
        else if (value.is_number_float()) {
            schema["properties"][key]["type"] = "number";
        }
        else if (value.is_string()) {
            schema["properties"][key]["type"] = "string";
        }
        else if (value.is_array()) {
            schema["properties"][key]["type"] = "array";
            if (!value.empty()) {
                schema["properties"][key]["items"] = inferSchemaFromJson(value[0]);
            }
        }
        else if (value.is_object()) {
            schema["properties"][key] = inferSchemaFromJson(value);
        }
    }

    return schema;
}

LanguageGenerator* createLanguageGenerator(Language lang) {
    switch (lang) {
    case Language::CPP: return new CppGenerator();
    case Language::CSHARP: return new CSharpGenerator();
    case Language::JAVA: return new JavaGenerator();
    case Language::PYTHON: return new PythonGenerator();
    case Language::GO: return new GoGenerator();
    case Language::TYPESCRIPT: return new TypeScriptGenerator();
    case Language::RUST: return new RustGenerator();
    case Language::SWIFT: return new SwiftGenerator();
    case Language::DART: return new DartGenerator();
    case Language::KOTLIN: return new KotlinGenerator();
    case Language::ELIXIR: return new ElixirGenerator();
    case Language::SCALA: return new ScalaGenerator();
    default: throw std::runtime_error("Unsupported language");
    }
}

std::string languageToString(Language lang) {
    switch (lang) {
    case Language::CPP: return "C++";
    case Language::CSHARP: return "C#";
    case Language::JAVA: return "Java";
    case Language::PYTHON: return "Python";
    case Language::GO: return "Go";
    case Language::TYPESCRIPT: return "TypeScript";
    case Language::RUST: return "Rust";
    case Language::SWIFT: return "Swift";
    case Language::DART: return "Dart";
    case Language::KOTLIN: return "Kotlin";
    case Language::ELIXIR: return "Elixir";
    case Language::SCALA: return "Scala";
    default: return "Unknown";
    }
}

Language stringToLanguage(const std::string& lang) {
    std::string lowerLang = lang;
    std::transform(lowerLang.begin(), lowerLang.end(), lowerLang.begin(), ::tolower);
    if (lowerLang == "cpp") return Language::CPP;
    if (lowerLang == "csharp") return Language::CSHARP;
    if (lowerLang == "java") return Language::JAVA;
    if (lowerLang == "python") return Language::PYTHON;
    if (lowerLang == "go") return Language::GO;
    if (lowerLang == "typescript") return Language::TYPESCRIPT;
    if (lowerLang == "rust") return Language::RUST;
    if (lowerLang == "swift") return Language::SWIFT;
    if (lowerLang == "dart") return Language::DART;
    if (lowerLang == "kotlin") return Language::KOTLIN;
    if (lowerLang == "elixir") return Language::ELIXIR;
    if (lowerLang == "scala") return Language::SCALA;
    throw std::runtime_error("Unsupported language: " + lang);
}

std::string getFileExtension(Language lang) {
    switch (lang) {
    case Language::CPP: return "hpp";
    case Language::CSHARP: return "cs";
    case Language::JAVA: return "java";
    case Language::PYTHON: return "py";
    case Language::GO: return "go";
    case Language::TYPESCRIPT: return "ts";
    case Language::RUST: return "rs";
    case Language::SWIFT: return "swift";
    case Language::DART: return "dart";
    case Language::KOTLIN: return "kt";
    case Language::ELIXIR: return "ex";
    case Language::SCALA: return "scala";
    default: return "txt";
    }
}
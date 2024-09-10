# JSON to Multiple Language Model Generator

This tool generates model classes/structs from JSON data and JSON Schema for multiple programming languages.

## Supported Languages

- C++
- C#
- Java
- Python
- Go
- TypeScript
- Rust
- Swift
- Dart
- Kotlin
- Elixir
- Scala

## Prerequisites

- CMake (version 3.14 or higher)
- C++17 compatible compiler
- nlohmann_json library (version 3.9.1 or higher)

## Project Structure

- `json_model_generator.cpp`: Main entry point of the application
- `json_model_generator.hpp`: Header file with declarations
- `circular_reference_handler.cpp`: Implementation of CircularReferenceHandler
- Language-specific generators (e.g., `cpp_generator.cpp`, `java_generator.cpp`, etc.)
- `CMakeLists.txt`: CMake configuration file

## Building the Project

1. Clone the repository:
   ```
   git clone https://github.com/your-username/json-to-model-generator.git
   cd json-to-model-generator
   ```

2. Create a build directory and run CMake:
   ```
   mkdir build && cd build
   cmake ..
   ```

3. Build the project:
   ```
   cmake --build .
   ```

## Usage

Run the generator with the following command:

```
./json_model_generator -i <input_json_file> -l <language> -o <output_file> [options]
```

Options:
- `-i, --input <file>`: Input JSON file (required)
- `-s, --schema <file>`: JSON Schema file (optional)
- `-l, --language <lang>`: Output language (cpp, csharp, java, python, go, typescript, rust, swift, dart, kotlin, elixir, scala) (required)
- `-o, --output <file>`: Output file name (required)
- `--docs`: Generate documentation comments
- `--validation`: Generate validation methods
- `--builder`: Use builder pattern (for supported languages)
- `--immutable`: Generate immutable objects
- `--indent <size>`: Indentation size (default: 4)
- `--brace-style <style>`: Brace style (same-line, new-line)
- `--custom-types <file>`: JSON file with custom type mappings
- `--verbose`: Enable verbose output
- `--dry-run`: Show what would be generated without creating files

If no schema file is provided, the tool will infer a basic schema from the input JSON.

Example:
```
./json_model_generator -i input.json -l csharp -o OutputModel.cs
```

This command will generate a C# model from `input.json` and save it as `OutputModel.cs`, using an inferred schema.

To use a specific schema file:
```
./json_model_generator -i input.json -s schema.json -l csharp -o OutputModel.cs
```

... (rest of the content remains the same)

## Custom Type Mappings

You can provide custom type mappings using a JSON file. Specify the file using the `--custom-types` option.

Example custom_type_mappings.json:
```json
{
  "cpp": {
    "timestamp": "std::chrono::system_clock::time_point",
    "uuid": "boost::uuids::uuid"
  },
  "java": {
    "timestamp": "java.time.Instant",
    "uuid": "java.util.UUID"
  }
}
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

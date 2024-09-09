cmake_minimum_required(VERSION 3.10)
project(JsonToModelGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
# Use FetchContent to get nlohmann/json
include(FetchContent)
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(json)

# Add executable
add_executable(json_model_generator
    json_model_generator.cpp
    circular_reference_handler.cpp
    cpp_generator.cpp
    csharp_generator.cpp
    java_generator.cpp
    python_generator.cpp
    go_generator.cpp
    typescript_generator.cpp
    rust_generator.cpp
    swift_generator.cpp
    dart_generator.cpp
    kotlin_generator.cpp
    elixir_generator.cpp
    scala_generator.cpp)

# Link libraries
target_link_libraries(json_model_generator PRIVATE
    nlohmann_json::nlohmann_json
)

# Include directories
target_include_directories(json_model_generator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
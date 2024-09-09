#include "json_model_generator.hpp"

void CircularReferenceHandler::addDependency(const std::string& className, const std::string& dependsOn) {
    dependencies[className].insert(dependsOn);
}

bool CircularReferenceHandler::hasCyclicDependency(const std::string& className, std::set<std::string>& visited) {
    if (visited.find(className) != visited.end()) {
        return true;
    }
    visited.insert(className);
    for (const auto& dep : dependencies[className]) {
        if (hasCyclicDependency(dep, visited)) {
            return true;
        }
    }
    visited.erase(className);
    return false;
}

void CircularReferenceHandler::resolveCircularReferences(std::ofstream& outFile, const Config& config, LanguageGenerator* generator) {
    for (const auto& [className, deps] : dependencies) {
        std::set<std::string> visited;
        if (hasCyclicDependency(className, visited)) {
            // Handle circular dependency here
            // This is a placeholder implementation; adjust as needed
            outFile << "// Circular dependency detected for " << className << std::endl;
        }
    }
}
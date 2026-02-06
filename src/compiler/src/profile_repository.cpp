#include "profile_repository.h"
#include <iostream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

ProfileRepository::ProfileRepository(const string& root)
    : root(root) {}

YAML::Node ProfileRepository::getComponentProfile(const string& type) {
    string path;
    
    // Handle special cases for component types
    if (type == "api" || type == "api/service") {
        path = root + "/components/api/service/default.yaml";
    } else if (type == "database") {
        path = root + "/components/database/default.yaml";
    } else if (type == "cache") {
        path = root + "/components/cache/default.yaml";
    } else {
        // Default to type/default.yaml
        path = root + "/components/" + type + "/default.yaml";
    }
    
    std::cout << "📖 Loading component profile for type '" << type << "': " << path << std::endl;
    
    if (!fs::exists(path)) {
        std::cerr << "❌ Profile file not found: " << path << std::endl;
        throw std::runtime_error("bad file: " + path);
    }
    
    try {
        YAML::Node profile = YAML::LoadFile(path);
        std::cout << "✅ Loaded profile successfully" << std::endl;
        return profile;
    } catch (const YAML::Exception& e) {
        std::cerr << "❌ Error loading YAML: " << e.what() << std::endl;
        throw std::runtime_error(string("YAML error in ") + path + ": " + e.what());
    }
}

YAML::Node ProfileRepository::getNetworkProfile(const string& type) {
    string path = root + "/networks/" + type + ".yaml";
    
    std::cout << "📖 Loading network profile: " << path << std::endl;
    
    if (!fs::exists(path)) {
        std::cerr << "❌ Profile file not found: " << path << std::endl;
        throw std::runtime_error("bad file: " + path);
    }
    
    try {
        YAML::Node profile = YAML::LoadFile(path);
        std::cout << "✅ Loaded profile successfully" << std::endl;
        return profile;
    } catch (const YAML::Exception& e) {
        std::cerr << "❌ Error loading YAML: " << e.what() << std::endl;
        throw std::runtime_error(string("YAML error in ") + path + ": " + e.what());
    }
}

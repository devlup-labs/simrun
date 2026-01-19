#pragma once
#include <yaml-cpp/yaml.h>
#include <string>
#include <unordered_map>

using namespace std;

class ProfileRepository {
public:
    ProfileRepository(const string& basePath);

    YAML::Node getComponentProfile(const string& name);
    YAML::Node getNetworkProfile(const string& name);

private:
    string basePath;
    unordered_map<string, YAML::Node> compCache;
    unordered_map<string, YAML::Node> netCache;
};

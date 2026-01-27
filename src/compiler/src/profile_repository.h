#pragma once
#include <yaml-cpp/yaml.h>
#include <string>

using namespace std;

class ProfileRepository {
public:
    explicit ProfileRepository(const string& root);
    YAML::Node getComponentProfile(const string& type);
    YAML::Node getNetworkProfile(const string& type);

private:
    string root;
};

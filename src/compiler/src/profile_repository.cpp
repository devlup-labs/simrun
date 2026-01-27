#include "profile_repository.h"

ProfileRepository::ProfileRepository(const string& root)
    : root(root) {}

YAML::Node ProfileRepository::getComponentProfile(const string& type) {
    string path = root + "/components/" + type + "/default.yaml";
    return YAML::LoadFile(path);
}

YAML::Node ProfileRepository::getNetworkProfile(const string& type) {
    string path = root + "/networks/" + type + ".yaml";
    return YAML::LoadFile(path);
}

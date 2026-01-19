#include "profile_repository.h"

ProfileRepository::ProfileRepository(const string& basePath)
    : basePath(basePath) {}

YAML::Node ProfileRepository::getComponentProfile(const string& name) {
    if (compCache.count(name))
        return compCache[name];

    auto node = YAML::LoadFile(basePath + "/components/" + name + ".yaml");
    compCache[name] = node;
    return node;
}

YAML::Node ProfileRepository::getNetworkProfile(const string& name) {
    if (netCache.count(name))
        return netCache[name];

    auto node = YAML::LoadFile(basePath + "/networks/" + name + ".yaml");
    netCache[name] = node;
    return node;
}

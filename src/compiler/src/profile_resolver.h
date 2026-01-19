#pragma once

#include "ir.h"
#include "profile_repository.h"
#include <yaml-cpp/yaml.h>

using namespace std;

class ProfileResolver {
public:
    ProfileResolver(ProfileRepository& repo);

    void resolve(IR& ir);

private:
    ProfileRepository& repo;

    // Converts a YAML scalar into IR Value safely
    Value parseYamlValue(const YAML::Node& node);
};

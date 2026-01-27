#pragma once
#include "ir.h"
#include "profile_repository.h"

class ProfileResolver {
public:
    explicit ProfileResolver(ProfileRepository& repo);
    void resolve(IR& ir);

private:
    ProfileRepository& repo;
    Value yamlToValue(const YAML::Node& node);
};

#include "profile_resolver.h"

ProfileResolver::ProfileResolver(ProfileRepository& repo)
    : repo(repo) {}

Value ProfileResolver::yamlToValue(const YAML::Node& n) {
    try { return n.as<int>(); } catch (...) {}
    try { return n.as<double>(); } catch (...) {}
    try { return n.as<bool>(); } catch (...) {}
    return n.as<string>();
}

void ProfileResolver::resolve(IR& ir) {

    for (auto& c : ir.context.components) {
        YAML::Node profile = repo.getComponentProfile(c.type);
        if (profile["defaults"]) {
            for (auto it : profile["defaults"]) {
                string k = it.first.as<string>();
                if (!c.config.count(k)) {
                    c.config[k] = yamlToValue(it.second);
                }
            }
        }
    }

    for (auto& l : ir.context.links) {
        YAML::Node profile = repo.getNetworkProfile("ethernet");
        if (profile["defaults"]) {
            for (auto it : profile["defaults"]) {
                string k = it.first.as<string>();
                if (!l.config.count(k)) {
                    l.config[k] = yamlToValue(it.second);
                }
            }
        }
    }
}

#include "profile_resolver.h"
#include <stdexcept>

using namespace std;

ProfileResolver::ProfileResolver(ProfileRepository& repo)
    : repo(repo) {}

/*
 * Safely convert YAML scalar → IR Value
 * Order matters: int → double → bool → string
 */
Value ProfileResolver::parseYamlValue(const YAML::Node& node) {

    if (!node.IsScalar()) {
        throw runtime_error("Only scalar YAML values are supported");
    }

    // Try int
    try {
        return node.as<int>();
    } catch (...) {}

    // Try double
    try {
        return node.as<double>();
    } catch (...) {}

    // Try bool
    try {
        return node.as<bool>();
    } catch (...) {}

    // Fallback: string
    return node.as<string>();
}

void ProfileResolver::resolve(IR& ir) {

    // ---------- COMPONENT PROFILES ----------
    for (auto& comp : ir.components) {

        YAML::Node profile = repo.getComponentProfile(comp.implementation);

        // Apply defaults from YAML
        if (profile["defaults"]) {
            for (auto it : profile["defaults"]) {
                string key = it.first.as<string>();
                comp.resolved_params[key] = parseYamlValue(it.second);
            }
        }

        // Override with user-provided parameters
        for (auto& [key, value] : comp.user_params) {
            comp.resolved_params[key] = value;
        }
    }

    // ---------- NETWORK PROFILES ----------
    for (auto& link : ir.links) {

        YAML::Node profile = repo.getNetworkProfile(link.type);

        if (profile["defaults"]) {
            for (auto it : profile["defaults"]) {
                string key = it.first.as<string>();
                link.resolved_params[key] = parseYamlValue(it.second);
            }
        }
    }
}

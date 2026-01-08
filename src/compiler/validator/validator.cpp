// validator/validator.cpp
#include "validator.h"
#include <unordered_map>
#include <unordered_set>

using namespace std;

static bool is_probability(double v) {
    return v >= 0.0 && v <= 1.0;
}

vector<ValidationError> validate_ast(const ArchitectureAST& ast) {
    vector<ValidationError> errors;

    unordered_map<string, vector<string>> outgoing;
    unordered_map<string, vector<string>> incoming;

    // Build graph + structural validation
    for (const auto& link : ast.links) {
        if (!ast.components.count(link.source)) {
            errors.push_back({"Link source does not exist", {link.source}});
            continue;
        }
        if (!ast.components.count(link.target)) {
            errors.push_back({"Link target does not exist", {link.target}});
            continue;
        }
        outgoing[link.source].push_back(link.target);
        incoming[link.target].push_back(link.source);
    }

    // Semantic rules
    for (const auto& entry : ast.components) {
        const auto& id   = entry.first;
        const auto& comp = entry.second;

        // Allowed topology rules
        if (comp.type == ComponentType::API) {
            for (const auto& to : outgoing[id]) {
                if (ast.components.at(to).type != ComponentType::DATABASE) {
                    errors.push_back({"API can only connect to DATABASE", {id, to}});
                }
            }
        }

        if (comp.type == ComponentType::DATABASE) {
            for (const auto& to : outgoing[id]) {
                if (ast.components.at(to).type != ComponentType::CACHE) {
                    errors.push_back({"DATABASE can only connect to CACHE", {id, to}});
                }
            }
        }

        if (comp.type == ComponentType::CACHE) {
            if (outgoing.count(id) && !outgoing[id].empty()) {
                errors.push_back({"CACHE cannot have outgoing edges", {id}});
            }
        }

        // Parameter checks
        for (const auto& param : comp.numeric_params) {
            const auto& k = param.first;
            const auto& v = param.second;

            if (k.find("prob") != string::npos && !is_probability(v)) {
                errors.push_back({"Probability parameter out of range", {id}});
            }
            if (v < 0) {
                errors.push_back({"Negative parameter value", {id}});
            }
        }
    }

    // Workload validation
    if (ast.workload.base_rps < 0) {
        errors.push_back({"Workload base_rps must be >= 0", {}});
    }
    if (ast.workload.duration_ms <= 0) {
        errors.push_back({"Workload duration must be positive", {}});
    }

    return errors;
}

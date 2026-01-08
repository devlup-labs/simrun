// parser/parser.cpp
#include "parser.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace std;
using json = nlohmann::json;

static ComponentType parse_type(const string& t) {
    if (t == "api") return ComponentType::API;
    if (t == "database") return ComponentType::DATABASE;
    if (t == "cache") return ComponentType::CACHE;
    return ComponentType::UNKNOWN;
}

ArchitectureAST parse_json_to_ast(const string& json_input) {
    ArchitectureAST ast;
    json j = json::parse(json_input);

    // Components
    for (const auto& c : j["components"]) {
        Component comp;
        comp.id = c["id"];
        comp.type = parse_type(c["type"]);
        comp.profile = c["profile"];

        if (c.contains("parameters")) {
            for (const auto& item : c["parameters"].items()) {
                const auto& k = item.key();
                const auto& v = item.value();

                if (v.is_number()) {
                    comp.numeric_params[k] = v.get<double>();
                }
            }
        }

        ast.components[comp.id] = comp;
    }

    // Links
    for (const auto& l : j["links"]) {
        Link link;
        link.source = l["source"];
        link.target = l["target"];

        if (l.contains("parameters")) {
            for (const auto& item : l["parameters"].items()) {
                const auto& k = item.key();
                const auto& v = item.value();

                if (v.is_number()) {
                    link.numeric_params[k] = v.get<double>();
                }
            }
        }

        ast.links.push_back(link);
    }

    // Workload
    if (j.contains("workload")) {
        ast.workload.type = j["workload"]["type"];
        ast.workload.base_rps = j["workload"]["base_rps"];
        ast.workload.duration_ms = j["workload"]["duration_ms"];
    }

    return ast;
}

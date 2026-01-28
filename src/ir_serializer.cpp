#include "ir_serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

static json valueToJson(const Value& v) {
    if (holds_alternative<int>(v)) return get<int>(v);
    if (holds_alternative<double>(v)) return get<double>(v);
    if (holds_alternative<bool>(v)) return get<bool>(v);
    return get<string>(v);
}

string writeIRToJsonFile(const IR& ir, const string& path) {

    json root;
    root["header"] = json::object();

    for (auto& [k, v] : ir.header) {
        root["header"][k] = valueToJson(v);
    }

    for (auto& c : ir.context.components) {
        json jc;
        jc["id"] = c.id;
        jc["type"] = c.type;
        for (auto& [k, v] : c.config) {
            jc["config"][k] = valueToJson(v);
        }
        root["context"]["components"].push_back(jc);
    }

    for (auto& l : ir.context.links) {
        json jl;
        jl["id"] = l.id;
        jl["from"] = l.from;
        jl["to"] = l.to;
        for (auto& [k, v] : l.config) {
            jl["config"][k] = valueToJson(v);
        }
        root["context"]["links"].push_back(jl);
    }

    ofstream out(path);
    out << root.dump(2);
    return path;
}

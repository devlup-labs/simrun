#include "ir_parser.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

static Value jsonToValue(const json& j) {
    if (j.is_number_integer()) return j.get<int>();
    if (j.is_number_float())   return j.get<double>();
    if (j.is_boolean())        return j.get<bool>();
    if (j.is_string())         return j.get<string>();
    throw runtime_error("Unsupported JSON value type");
}

IR parseIR(const string& jsonText) {
    json root = json::parse(jsonText);
    IR ir;

    for (auto& [k, v] : root["header"].items()) {
        ir.header[k] = jsonToValue(v);
    }

    for (const auto& c : root["context"]["components"]) {
        Component comp;
        comp.id = c["id"];
        comp.type = c["type"];
        for (auto& [k, v] : c["config"].items()) {
            comp.config[k] = jsonToValue(v);
        }
        ir.context.components.push_back(comp);
    }

    for (const auto& l : root["context"]["links"]) {
        Link link;
        link.id = l["id"];
        link.from = l["from"];
        link.to = l["to"];
        for (auto& [k, v] : l["config"].items()) {
            link.config[k] = jsonToValue(v);
        }
        ir.context.links.push_back(link);
    }

    return ir;
}

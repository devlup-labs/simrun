#include "ir_parser.h"
#include <fstream>

IR IRParser::parseFromFile(const std::string& path) {
    std::ifstream f(path);
    json j;
    f >> j;

    IR ir;

    /* ---------- Header ---------- */
    ir.header.ir_version = j["header"]["ir_version"];
    ir.header.engine_version = j["header"]["engine_version"];
    ir.header.seed = j["header"]["seed"];
    ir.header.time_unit = j["header"]["time_unit"];

    /* ---------- Components ---------- */
    for (auto& c : j["context"]["components"]) {
        ir.components.push_back({
            c["id"],
            ComponentType::UNKNOWN,
            c["config"]
        });
    }

    /* ---------- Links ---------- */
    for (auto& l : j["context"]["links"]) {
        ir.links.push_back({
            l["id"],
            l["from"],
            l["to"],
            l["config"]
        });
    }

    /* ---------- Initial State ---------- */
    for (auto& c : j["initial_state"]["components"]) {
        ir.components_context.push_back({
            c.value("id", 0),
            c.value("active", true),
            c.value("queue", 0)
        });
    }

    for (auto& l : j["initial_state"]["links"]) {
        ir.links_context.push_back({
            l.value("id", 0),
            l.value("current_bandwidth_mbps", 0.0),
            l.value("from", 0),
            l.value("to", 0)
        });
    }


    /* ---------- Request Types ---------- */
    for (auto& r : j["request_types"]) {
        ir.request_types.push_back({
            r["id"],
            r["payload"],
            r["routes"]
        });
    }

    /* ---------- Initial Events ---------- */
    for (auto& e : j["initial_events"]) {
        ir.initial_events.push_back({
            e["id"],
            e["time"],
            type,
            e["payload"]
        });
    }

    return ir;
}

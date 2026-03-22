#include "ir_parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

namespace sim::ir {
namespace {

std::string remove_underscores(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    return text;
}

std::int64_t parse_i64(const Json& value) {
    if (value.is_number_integer()) {
        return value.get<std::int64_t>();
    }
    if (value.is_number_unsigned()) {
        return static_cast<std::int64_t>(value.get<std::uint64_t>());
    }
    if (value.is_number_float()) {
        return static_cast<std::int64_t>(value.get<double>());
    }
    if (value.is_string()) {
        return std::stoll(remove_underscores(value.get<std::string>()));
    }
    throw std::runtime_error("Expected integer-compatible JSON value");
}

std::uint64_t parse_u64(const Json& value) {
    const std::int64_t parsed = parse_i64(value);
    if (parsed < 0) {
        throw std::runtime_error("Expected non-negative value");
    }
    return static_cast<std::uint64_t>(parsed);
}

double parse_double(const Json& value) {
    if (value.is_number()) {
        return value.get<double>();
    }
    if (value.is_string()) {
        return std::stod(remove_underscores(value.get<std::string>()));
    }
    throw std::runtime_error("Expected numeric JSON value");
}

bool parse_bool(const Json& value) {
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    if (value.is_number_integer() || value.is_number_unsigned()) {
        return parse_i64(value) != 0;
    }
    if (value.is_string()) {
        std::string lowered = value.get<std::string>();
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (lowered == "true" || lowered == "1") {
            return true;
        }
        if (lowered == "false" || lowered == "0") {
            return false;
        }
    }
    throw std::runtime_error("Expected bool-compatible JSON value");
}

}  // namespace

SimulationIR parse_ir_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open IR file: " + path);
    }

    Json root;
    in >> root;

    SimulationIR ir;

    const Json& header = root.at("header");
    ir.header.ir_version = static_cast<int>(parse_i64(header.at("ir_version")));
    if (header.contains("seed")) {
        ir.header.seed = parse_u64(header.at("seed"));
    }
    if (header.contains("time_unit")) {
        ir.header.time_unit = header.at("time_unit").get<std::string>();
    }
    if (header.contains("engine_version")) {
        ir.header.engine_version = header.at("engine_version").get<std::string>();
    }

    if (root.contains("model")) {
        const Json& model = root.at("model");

        if (model.contains("components")) {
            for (const Json& c : model.at("components")) {
                IR_Component component;
                component.id = static_cast<int>(parse_i64(c.at("id")));
                component.kind = c.value("kind", "");
                component.params = c.value("params", Json::object());
                ir.components.push_back(std::move(component));
            }
        }

        if (model.contains("links")) {
            for (const Json& l : model.at("links")) {
                IR_Link link;
                link.id = static_cast<int>(parse_i64(l.at("id")));
                link.from = static_cast<int>(parse_i64(l.at("from")));
                link.to = static_cast<int>(parse_i64(l.at("to")));
                link.params = l.value("params", Json::object());
                ir.links.push_back(std::move(link));
            }
        }

        if (model.contains("request_types")) {
            for (const Json& r : model.at("request_types")) {
                IR_RequestType request_type;
                request_type.id = static_cast<int>(parse_i64(r.at("id")));
                request_type.default_payload = r.value("default_payload", Json::object());

                if (r.contains("routes")) {
                    for (const Json& route_for_flag : r.at("routes")) {
                        std::vector<int> route;
                        for (const Json& link_id : route_for_flag) {
                            route.push_back(static_cast<int>(parse_i64(link_id)));
                        }
                        request_type.routes.push_back(std::move(route));
                    }
                }

                ir.request_types.push_back(std::move(request_type));
            }
        }
    }

    if (root.contains("behavior")) {
        for (const Json& b : root.at("behavior")) {
            IR_BehaviorEntry entry;
            entry.component_id = static_cast<int>(parse_i64(b.at("component_id")));
            entry.request_type = static_cast<int>(parse_i64(b.at("request_type")));
            entry.flag = static_cast<int>(parse_i64(b.at("flag")));

            for (const Json& op_json : b.at("sequence")) {
                IR_BehaviorOpSpec op;
                op.op = op_json.value("op", "");
                op.args = op_json.value("args", Json::object());
                entry.sequence.push_back(std::move(op));
            }

            ir.behavior.push_back(std::move(entry));
        }
    }

    if (root.contains("workloads")) {
        for (const Json& w : root.at("workloads")) {
            IR_WorkloadSpec workload;
            workload.id = static_cast<int>(parse_i64(w.at("id")));
            workload.request_type = static_cast<int>(parse_i64(w.at("request_type")));
            workload.target_component = static_cast<int>(parse_i64(w.at("target_component")));
            workload.total_requests = parse_u64(w.at("total_requests"));

            if (w.contains("arrival_distribution")) {
                const Json& arrival = w.at("arrival_distribution");
                workload.arrival_distribution.type = arrival.value("type", "");
                if (arrival.contains("p1")) {
                    workload.arrival_distribution.p1 = arrival.at("p1");
                }
                if (arrival.contains("p2")) {
                    workload.arrival_distribution.p2 = arrival.at("p2");
                }
            }

            ir.workloads.push_back(std::move(workload));
        }
    }

    if (root.contains("runtime_state")) {
        const Json& runtime_state = root.at("runtime_state");

        if (runtime_state.contains("components")) {
            for (const Json& c : runtime_state.at("components")) {
                IR_RuntimeComponentState state;
                state.id = static_cast<int>(parse_i64(c.at("id")));
                if (c.contains("active")) {
                    state.active = parse_bool(c.at("active"));
                }
                if (c.contains("queue_size")) {
                    state.queue_size = parse_u64(c.at("queue_size"));
                } else if (c.contains("queue")) {
                    if (c.at("queue").is_array()) {
                        state.queue_size = c.at("queue").size();
                    } else {
                        state.queue_size = parse_u64(c.at("queue"));
                    }
                }
                ir.runtime_state.components.push_back(std::move(state));
            }
        }

        if (runtime_state.contains("links")) {
            for (const Json& l : runtime_state.at("links")) {
                IR_RuntimeLinkState state;
                state.id = static_cast<int>(parse_i64(l.at("id")));
                if (l.contains("current_bandwidth")) {
                    state.current_bandwidth_mbps = parse_double(l.at("current_bandwidth"));
                } else if (l.contains("current_bandwidth_mbps")) {
                    state.current_bandwidth_mbps = parse_double(l.at("current_bandwidth_mbps"));
                }
                ir.runtime_state.links.push_back(std::move(state));
            }
        }
    }

    if (root.contains("bootstrap_events")) {
        for (const Json& event_json : root.at("bootstrap_events")) {
            IR_EventSpec event;
            event.time = parse_u64(event_json.at("time"));
            event.type = event_json.value("type", "");
            event.payload = event_json.value("payload", Json::object());
            ir.bootstrap_events.push_back(std::move(event));
        }
    }

    return ir;
}

}  // namespace sim::ir

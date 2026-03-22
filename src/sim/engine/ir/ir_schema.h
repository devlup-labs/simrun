#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace sim::ir {

using Json = nlohmann::json;

struct IR_Header {
    int ir_version = 0;
    std::uint64_t seed = 0;
    std::string time_unit;
    std::string engine_version;
};

struct IR_Component {
    int id = 0;
    std::string kind;
    Json params = Json::object();
};

struct IR_Link {
    int id = 0;
    int from = 0;
    int to = 0;
    Json params = Json::object();
};

struct IR_RequestType {
    int id = 0;
    Json default_payload = Json::object();
    std::vector<std::vector<int>> routes;
};

struct IR_DistributionSpec {
    std::string type;
    Json p1 = 0;
    Json p2 = 0;
};

struct IR_WorkloadSpec {
    int id = 0;
    int request_type = 0;
    int target_component = 0;
    std::uint64_t total_requests = 0;
    IR_DistributionSpec arrival_distribution;
};

struct IR_BehaviorOpSpec {
    std::string op;
    Json args = Json::object();
};

struct IR_BehaviorEntry {
    int component_id = 0;
    int request_type = 0;
    int flag = 0;
    std::vector<IR_BehaviorOpSpec> sequence;
};

struct IR_EventSpec {
    std::uint64_t time = 0;
    std::string type;
    Json payload = Json::object();
};

struct IR_RuntimeComponentState {
    int id = 0;
    std::optional<bool> active;
    std::optional<std::uint64_t> queue_size;
};

struct IR_RuntimeLinkState {
    int id = 0;
    std::optional<double> current_bandwidth_mbps;
};

struct IR_RuntimeState {
    std::vector<IR_RuntimeComponentState> components;
    std::vector<IR_RuntimeLinkState> links;
};

struct SimulationIR {
    IR_Header header;
    std::vector<IR_Component> components;
    std::vector<IR_Link> links;
    std::vector<IR_RequestType> request_types;
    std::vector<IR_WorkloadSpec> workloads;
    std::vector<IR_BehaviorEntry> behavior;
    IR_RuntimeState runtime_state;
    std::vector<IR_EventSpec> bootstrap_events;
};

}  // namespace sim::ir

#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "event.h"
#include "scheduler.h"

namespace sim {

enum class TimeUnit {
    NS,
    US,
    MS,
    S,
    UNKNOWN,
};

enum class ComponentKind {
    UNKNOWN,
    LOAD_BALANCER,
    SERVER,
    SERVICE,
    DATABASE,
    CACHE,
};

enum class BehaviorOpcode {
    CHANGE_FLAG,
    SEND_TO_NEXT_LINK,
};

enum class DistributionType {
    NORMAL,
};

struct Distribution {
    DistributionType type = DistributionType::NORMAL;
    double p1 = 0.0;
    double p2 = 0.0;
};

struct ComponentModel {
    int external_id = -1;
    ComponentKind kind = ComponentKind::UNKNOWN;
};

struct LinkModel {
    int external_id = -1;
    int from_component_idx = -1;
    int to_component_idx = -1;
    std::uint64_t latency_ns = 0;
    double base_bandwidth_mbps = 0.0;
};

struct RequestTypeModel {
    int external_id = -1;
    std::vector<std::vector<int>> routing_by_flag;
};

struct BehaviorOp {
    BehaviorOpcode opcode = BehaviorOpcode::SEND_TO_NEXT_LINK;
    std::vector<int> values;
};

struct WorkloadModel {
    int external_id = -1;
    int request_type_idx = -1;
    int target_component_idx = -1;
    Distribution arrival_dist;
    std::uint64_t total_requests = 0;
};

using BehaviorSequence = std::vector<BehaviorOp>;
using BehaviorByFlag = std::vector<BehaviorSequence>;
using BehaviorByRequestType = std::vector<BehaviorByFlag>;
using BehaviorTable = std::vector<BehaviorByRequestType>;

struct CompiledBootstrapEventSpec {
    SimTime time = 0;
    EventType type = EventType::WORKLOAD_ARRIVAL;
    int workload_idx = -1;
    int fault_entity_link_idx = -1;
    FaultEffect fault_effect = FaultEffect::NONE;
};

struct SimulationContext {
    std::uint64_t seed = 0;
    TimeUnit time_unit = TimeUnit::UNKNOWN;

    std::vector<ComponentModel> component_models;
    std::vector<LinkModel> link_models;
    std::vector<RequestTypeModel> request_types;
    std::vector<WorkloadModel> workloads;
    BehaviorTable behavior;
    std::vector<CompiledBootstrapEventSpec> bootstrap_events;

    std::vector<int> component_external_ids;
    std::vector<int> link_external_ids;
    std::vector<int> request_type_external_ids;
    std::vector<int> workload_external_ids;

    std::unordered_map<int, int> component_index_by_external_id;
    std::unordered_map<int, int> link_index_by_external_id;
    std::unordered_map<int, int> request_type_index_by_external_id;
    std::unordered_map<int, int> workload_index_by_external_id;
};

struct ComponentState {
    bool active = false;
    std::uint64_t queue_size = 0;
};

struct LinkState {
    double bandwidth_multiplier = 1.0;
    double effective_bandwidth_mbps = 0.0;
};

struct Request {
    int id = -1;
    int request_type_idx = -1;
    int flag = 0;
    int current_hop = -1;
    int current_component_idx = -1;
};

struct WorkloadRuntimeState {
    std::uint64_t generated_count = 0;
};

struct SimulationState {
    std::vector<ComponentState> components;
    std::vector<LinkState> links;
    std::vector<Request> requests;
    std::vector<WorkloadRuntimeState> workload_state;
};

class Simulator {
public:
    Simulator(const SimulationContext& context, SimulationState& state);

    void run();

    SimTime now() const;
    std::uint64_t next_seq();
    void schedule(std::unique_ptr<Event> event);

    int sample_equal(const std::vector<int>& values);
    SimTime sample_workload_arrival_delta(const Distribution& distribution);
    int create_request(int request_type_idx, int target_component_idx);

    const SimulationContext& context() const;
    SimulationState& state();
    const SimulationState& state() const;

    const std::vector<std::string>& event_trace() const;

private:
    const SimulationContext& context_;
    SimulationState& state_;
    Scheduler scheduler_;
    SimTime now_ = 0;
    std::uint64_t next_seq_ = 0;
    std::mt19937_64 rng_;
    std::vector<std::string> event_trace_;

    void record_trace(const Event& event);
};

}  // namespace sim

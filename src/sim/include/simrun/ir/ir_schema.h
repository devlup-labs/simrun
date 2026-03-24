#pragma once

#include <cstdint>
#include <vector>

#include "simrun/behavior/opcodes.h"
#include "simrun/core/enums.h"
#include "simrun/util/types.h"

namespace simrun {

struct GlobalParameters {
    SimTime time_unit = 1;
    SimTime horizon = 0;
    std::uint64_t seed = 0;
    std::int32_t max_requests = 0;
    std::int32_t payload_words = 0;
    std::int32_t max_tokens = 0;
};

struct ComponentSpec {
    ComponentId id = 0;
    std::int32_t capacity = 1;
    std::int32_t state_words = 0;
    std::int32_t waiting_queue_capacity = 0;
    SimTime service_time = 1;
    QueueDiscipline discipline = QueueDiscipline::kFifo;
};

struct LinkSpec {
    LinkId id = 0;
    ComponentId src_component_id = 0;
    ComponentId dst_component_id = 0;
    SimTime propagation_delay = 1;
    std::int32_t bandwidth = 1;
};

struct RequestTypeSpec {
    RequestTypeId id = 0;
    std::int16_t flag_count = 1;
    std::int16_t max_hops = 1;
};

struct RoutingEntry {
    RequestTypeId request_type = 0;
    Flag flag = 0;
    std::int16_t hop = 0;
    LinkId link_id = kInvalidLinkId;
};

struct BehaviorBinding {
    ComponentId component_id = 0;
    RequestTypeId request_type = 0;
    Flag flag = 0;
    EventPhase phase = EventPhase::kRequestArrival;
    std::int32_t instruction_pointer = 0;
};

struct WorkloadDefinition {
    RequestTypeId request_type = 0;
    Flag initial_flag = 0;
    ComponentId entry_component_id = 0;
    SimTime start_time = 0;
    SimTime interval = 0;
    std::int32_t count = 0;
};

struct BootstrapEventSpec {
    SimTime time = 0;
    RequestTypeId request_type = 0;
    Flag flag = 0;
    ComponentId component_id = 0;
    EventPhase phase = EventPhase::kRequestArrival;
};

struct SimulationIR {
    GlobalParameters global_parameters{};
    std::vector<ComponentSpec> components{};
    std::vector<LinkSpec> links{};
    std::vector<RequestTypeSpec> request_types{};
    std::vector<RoutingEntry> routing_entries{};
    std::vector<BehaviorBinding> behavior_bindings{};
    std::vector<Instruction> opcode_stream{};
    std::vector<WorkloadDefinition> workloads{};
    std::vector<BootstrapEventSpec> bootstrap_events{};
};

}  // namespace simrun

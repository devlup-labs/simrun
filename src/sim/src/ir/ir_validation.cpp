#include "simrun/ir/ir_validation.h"

#include <array>
#include <sstream>
#include <string>
#include <vector>

namespace simrun {
namespace {

constexpr std::array<EventPhase, 5> kAllPhases = {
    EventPhase::kRequestArrival,
    EventPhase::kServiceCompletion,
    EventPhase::kTimerExpiration,
    EventPhase::kResponseArrival,
    EventPhase::kWorkloadGeneration,
};

bool IsKnownPhase(const EventPhase phase) {
    for (const EventPhase candidate : kAllPhases) {
        if (candidate == phase) {
            return true;
        }
    }
    return false;
}

void AddError(ValidationResult* result, const std::string& message) {
    result->ok = false;
    result->errors.push_back(message);
}

std::size_t RoutingIndex(const RequestTypeId request_type_id, const Flag flag, const std::int16_t hop, const std::int32_t max_flags, const std::int32_t max_hops) {
    return static_cast<std::size_t>(
        (request_type_id * max_flags * max_hops) + (flag * max_hops) + hop);
}

std::size_t BehaviorIndex(const ComponentId component_id,
                          const RequestTypeId request_type_id,
                          const Flag flag,
                          const EventPhase phase,
                          const std::int32_t request_type_count,
                          const std::int32_t max_flags) {
    return static_cast<std::size_t>((((component_id * request_type_count + request_type_id) * max_flags + flag) * static_cast<std::int32_t>(kAllPhases.size())) +
                                    static_cast<std::int32_t>(phase));
}

}  // namespace

ValidationResult ValidateIR(const SimulationIR& ir) {
    ValidationResult result{};

    if (ir.global_parameters.max_requests <= 0) {
        AddError(&result, "global_parameters.max_requests must be > 0");
    }
    if (ir.global_parameters.payload_words <= 0) {
        AddError(&result, "global_parameters.payload_words must be > 0");
    }
    if (ir.global_parameters.max_tokens <= 0) {
        AddError(&result, "global_parameters.max_tokens must be > 0");
    }
    if (ir.global_parameters.time_unit <= 0) {
        AddError(&result, "global_parameters.time_unit must be > 0");
    }

    const std::int32_t component_count = static_cast<std::int32_t>(ir.components.size());
    const std::int32_t link_count = static_cast<std::int32_t>(ir.links.size());
    const std::int32_t request_type_count = static_cast<std::int32_t>(ir.request_types.size());

    if (component_count == 0) {
        AddError(&result, "components must not be empty");
    }
    if (link_count == 0) {
        AddError(&result, "links must not be empty");
    }
    if (request_type_count == 0) {
        AddError(&result, "request_types must not be empty");
    }

    std::int32_t max_flags = 1;
    std::int32_t max_hops = 1;
    for (std::int32_t i = 0; i < component_count; ++i) {
        const ComponentSpec& component = ir.components[static_cast<std::size_t>(i)];
        if (component.id != i) {
            std::ostringstream oss;
            oss << "component id mismatch: expected " << i << ", got " << component.id;
            AddError(&result, oss.str());
        }
        if (component.capacity <= 0) {
            std::ostringstream oss;
            oss << "component " << i << " has non-positive capacity";
            AddError(&result, oss.str());
        }
        if (component.state_words < 0) {
            std::ostringstream oss;
            oss << "component " << i << " has negative state_words";
            AddError(&result, oss.str());
        }
        if (component.waiting_queue_capacity < 0) {
            std::ostringstream oss;
            oss << "component " << i << " has negative waiting_queue_capacity";
            AddError(&result, oss.str());
        }
        if (component.service_time <= 0) {
            std::ostringstream oss;
            oss << "component " << i << " has non-positive service_time";
            AddError(&result, oss.str());
        }
    }

    for (std::int32_t i = 0; i < link_count; ++i) {
        const LinkSpec& link = ir.links[static_cast<std::size_t>(i)];
        if (link.id != i) {
            std::ostringstream oss;
            oss << "link id mismatch: expected " << i << ", got " << link.id;
            AddError(&result, oss.str());
        }
        if (link.src_component_id < 0 || link.src_component_id >= component_count) {
            std::ostringstream oss;
            oss << "link " << i << " has invalid src_component_id";
            AddError(&result, oss.str());
        }
        if (link.dst_component_id < 0 || link.dst_component_id >= component_count) {
            std::ostringstream oss;
            oss << "link " << i << " has invalid dst_component_id";
            AddError(&result, oss.str());
        }
        if (link.propagation_delay < 0) {
            std::ostringstream oss;
            oss << "link " << i << " has negative propagation_delay";
            AddError(&result, oss.str());
        }
        if (link.bandwidth <= 0) {
            std::ostringstream oss;
            oss << "link " << i << " has non-positive bandwidth";
            AddError(&result, oss.str());
        }
    }

    for (std::int32_t i = 0; i < request_type_count; ++i) {
        const RequestTypeSpec& request_type = ir.request_types[static_cast<std::size_t>(i)];
        if (request_type.id != i) {
            std::ostringstream oss;
            oss << "request type id mismatch: expected " << i << ", got " << request_type.id;
            AddError(&result, oss.str());
        }
        if (request_type.flag_count <= 0) {
            std::ostringstream oss;
            oss << "request type " << i << " has non-positive flag_count";
            AddError(&result, oss.str());
        }
        if (request_type.max_hops <= 0) {
            std::ostringstream oss;
            oss << "request type " << i << " has non-positive max_hops";
            AddError(&result, oss.str());
        }
        if (request_type.flag_count > max_flags) {
            max_flags = request_type.flag_count;
        }
        if (request_type.max_hops > max_hops) {
            max_hops = request_type.max_hops;
        }
    }

    const std::size_t routing_table_size = static_cast<std::size_t>(request_type_count * max_flags * max_hops);
    std::vector<std::int8_t> routing_seen(routing_table_size, 0);

    for (const RoutingEntry& entry : ir.routing_entries) {
        if (entry.request_type < 0 || entry.request_type >= request_type_count) {
            AddError(&result, "routing entry has invalid request_type");
            continue;
        }
        const RequestTypeSpec& request_type = ir.request_types[static_cast<std::size_t>(entry.request_type)];
        if (entry.flag < 0 || entry.flag >= request_type.flag_count) {
            AddError(&result, "routing entry has invalid flag for request type");
            continue;
        }
        if (entry.hop < 0 || entry.hop >= request_type.max_hops) {
            AddError(&result, "routing entry has invalid hop for request type");
            continue;
        }
        if (entry.link_id < 0 || entry.link_id >= link_count) {
            AddError(&result, "routing entry has invalid link_id");
            continue;
        }
        const std::size_t idx = RoutingIndex(entry.request_type, entry.flag, entry.hop, max_flags, max_hops);
        if (routing_seen[idx] != 0) {
            AddError(&result, "routing entry has duplicate (request_type, flag, hop) key");
            continue;
        }
        routing_seen[idx] = 1;
    }

    for (std::int32_t request_type_id = 0; request_type_id < request_type_count; ++request_type_id) {
        const RequestTypeSpec& request_type = ir.request_types[static_cast<std::size_t>(request_type_id)];
        for (Flag flag = 0; flag < request_type.flag_count; ++flag) {
            for (std::int16_t hop = 0; hop < request_type.max_hops; ++hop) {
                const std::size_t idx = RoutingIndex(request_type_id, flag, hop, max_flags, max_hops);
                if (routing_seen[idx] == 0) {
                    std::ostringstream oss;
                    oss << "missing routing entry for request_type=" << request_type_id << ", flag=" << flag << ", hop=" << hop;
                    AddError(&result, oss.str());
                }
            }
        }
    }

    if (ir.opcode_stream.empty()) {
        AddError(&result, "opcode_stream must not be empty");
    }

    const std::size_t behavior_table_size =
        static_cast<std::size_t>(component_count * request_type_count * max_flags * static_cast<std::int32_t>(kAllPhases.size()));
    std::vector<std::int8_t> behavior_seen(behavior_table_size, 0);

    for (const BehaviorBinding& binding : ir.behavior_bindings) {
        if (binding.component_id < 0 || binding.component_id >= component_count) {
            AddError(&result, "behavior binding has invalid component_id");
            continue;
        }
        if (binding.request_type < 0 || binding.request_type >= request_type_count) {
            AddError(&result, "behavior binding has invalid request_type");
            continue;
        }
        const RequestTypeSpec& request_type = ir.request_types[static_cast<std::size_t>(binding.request_type)];
        if (binding.flag < 0 || binding.flag >= request_type.flag_count) {
            AddError(&result, "behavior binding has invalid flag for request type");
            continue;
        }
        if (!IsKnownPhase(binding.phase)) {
            AddError(&result, "behavior binding has invalid phase");
            continue;
        }
        if (binding.instruction_pointer < 0 || binding.instruction_pointer >= static_cast<std::int32_t>(ir.opcode_stream.size())) {
            AddError(&result, "behavior binding has out-of-range instruction_pointer");
            continue;
        }
        const std::size_t idx = BehaviorIndex(binding.component_id, binding.request_type, binding.flag, binding.phase, request_type_count, max_flags);
        if (behavior_seen[idx] != 0) {
            AddError(&result, "duplicate behavior binding for (component, request_type, flag, phase)");
            continue;
        }
        behavior_seen[idx] = 1;
    }

    for (ComponentId component_id = 0; component_id < component_count; ++component_id) {
        for (RequestTypeId request_type_id = 0; request_type_id < request_type_count; ++request_type_id) {
            const RequestTypeSpec& request_type = ir.request_types[static_cast<std::size_t>(request_type_id)];
            for (Flag flag = 0; flag < request_type.flag_count; ++flag) {
                for (EventPhase phase : kAllPhases) {
                    const std::size_t idx = BehaviorIndex(component_id, request_type_id, flag, phase, request_type_count, max_flags);
                    if (behavior_seen[idx] == 0) {
                        std::ostringstream oss;
                        oss << "missing behavior binding for component=" << component_id << ", request_type=" << request_type_id
                            << ", flag=" << flag << ", phase=" << static_cast<std::int32_t>(phase);
                        AddError(&result, oss.str());
                    }
                }
            }
        }
    }

    for (const BootstrapEventSpec& bootstrap_event : ir.bootstrap_events) {
        if (bootstrap_event.time < 0) {
            AddError(&result, "bootstrap event has negative time");
        }
        if (bootstrap_event.request_type < 0 || bootstrap_event.request_type >= request_type_count) {
            AddError(&result, "bootstrap event has invalid request_type");
            continue;
        }
        const RequestTypeSpec& request_type = ir.request_types[static_cast<std::size_t>(bootstrap_event.request_type)];
        if (bootstrap_event.flag < 0 || bootstrap_event.flag >= request_type.flag_count) {
            AddError(&result, "bootstrap event has invalid flag for request type");
        }
        if (bootstrap_event.component_id < 0 || bootstrap_event.component_id >= component_count) {
            AddError(&result, "bootstrap event has invalid component_id");
        }
        if (!IsKnownPhase(bootstrap_event.phase)) {
            AddError(&result, "bootstrap event has invalid phase");
        }
    }

    for (const WorkloadDefinition& workload : ir.workloads) {
        if (workload.request_type < 0 || workload.request_type >= request_type_count) {
            AddError(&result, "workload has invalid request_type");
            continue;
        }
        if (workload.entry_component_id < 0 || workload.entry_component_id >= component_count) {
            AddError(&result, "workload has invalid entry_component_id");
        }
        const RequestTypeSpec& request_type = ir.request_types[static_cast<std::size_t>(workload.request_type)];
        if (workload.initial_flag < 0 || workload.initial_flag >= request_type.flag_count) {
            AddError(&result, "workload has invalid initial_flag for request type");
        }
        if (workload.start_time < 0) {
            AddError(&result, "workload has negative start_time");
        }
        if (workload.count < 0) {
            AddError(&result, "workload has negative count");
        }
        if (workload.count > 0 && workload.interval < 0) {
            AddError(&result, "workload has negative interval");
        }
    }

    return result;
}

}  // namespace simrun

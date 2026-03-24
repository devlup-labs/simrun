#include "simrun/behavior/behavior_executor.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>

#include "simrun/core/sim_context.h"

namespace simrun {
namespace {

constexpr std::int32_t kPhaseCount = 5;
constexpr std::size_t kMaxInstructionsPerEvent = 4096;

EventPhase EventPhaseFromInt(const std::int64_t value) {
    if (value < 0 || value >= kPhaseCount) {
        return EventPhase::kTimerExpiration;
    }
    return static_cast<EventPhase>(value);
}

}  // namespace

std::size_t BehaviorTables::BehaviorFlatIndex(const ComponentId component_id,
                                              const RequestTypeId request_type_id,
                                              const Flag flag,
                                              const EventPhase phase) const {
    return static_cast<std::size_t>((((component_id * request_type_count + request_type_id) * max_flags + flag) * kPhaseCount) +
                                    static_cast<std::int32_t>(phase));
}

std::size_t BehaviorTables::RoutingFlatIndex(const RequestTypeId request_type_id, const Flag flag, const std::int16_t hop) const {
    return static_cast<std::size_t>((request_type_id * max_flags * max_hops) + (flag * max_hops) + hop);
}

BehaviorTables BuildBehaviorTables(const SimulationIR& ir) {
    BehaviorTables tables{};
    tables.component_count = static_cast<std::int32_t>(ir.components.size());
    tables.request_type_count = static_cast<std::int32_t>(ir.request_types.size());
    tables.max_flags = 1;
    tables.max_hops = 1;

    for (const RequestTypeSpec& request_type : ir.request_types) {
        tables.max_flags = std::max(tables.max_flags, static_cast<std::int32_t>(request_type.flag_count));
        tables.max_hops = std::max(tables.max_hops, static_cast<std::int32_t>(request_type.max_hops));
    }

    const std::size_t behavior_table_size =
        static_cast<std::size_t>(tables.component_count * tables.request_type_count * tables.max_flags * kPhaseCount);
    tables.behavior_index.assign(behavior_table_size, -1);

    for (const BehaviorBinding& binding : ir.behavior_bindings) {
        const std::size_t index = tables.BehaviorFlatIndex(binding.component_id, binding.request_type, binding.flag, binding.phase);
        tables.behavior_index[index] = binding.instruction_pointer;
    }

    const std::size_t routing_table_size =
        static_cast<std::size_t>(tables.request_type_count * tables.max_flags * tables.max_hops);
    tables.routing_lookup.assign(routing_table_size, kInvalidLinkId);

    for (const RoutingEntry& entry : ir.routing_entries) {
        const std::size_t index = tables.RoutingFlatIndex(entry.request_type, entry.flag, entry.hop);
        tables.routing_lookup[index] = entry.link_id;
    }

    tables.opcode_stream = ir.opcode_stream;
    return tables;
}

void BehaviorExecutor::Execute(SimContext& context, ExecutionContext& execution_context) const {
    if (!context.request_table.IsAllocated(execution_context.current_request_id)) {
        execution_context.halted = true;
        return;
    }

    Request& request = context.request_table.At(execution_context.current_request_id);
    const ComponentId component_id = execution_context.current_component_id;
    if (component_id < 0 || component_id >= context.component_state.ComponentCount()) {
        execution_context.halted = true;
        return;
    }

    if (request.type_id < 0 || request.type_id >= context.behavior_tables.request_type_count) {
        execution_context.halted = true;
        return;
    }
    if (request.flag < 0 || request.flag >= context.behavior_tables.max_flags) {
        execution_context.halted = true;
        return;
    }

    const std::size_t behavior_index =
        context.behavior_tables.BehaviorFlatIndex(component_id, request.type_id, request.flag, execution_context.current_event.phase);
    if (behavior_index >= context.behavior_tables.behavior_index.size()) {
        execution_context.halted = true;
        return;
    }

    std::int32_t instruction_pointer = context.behavior_tables.behavior_index[behavior_index];
    if (instruction_pointer < 0) {
        execution_context.halted = true;
        return;
    }

    std::size_t executed = 0;
    while (!execution_context.halted && executed < kMaxInstructionsPerEvent) {
        if (instruction_pointer < 0 ||
            instruction_pointer >= static_cast<std::int32_t>(context.behavior_tables.opcode_stream.size())) {
            break;
        }

        const Instruction instruction = context.behavior_tables.opcode_stream[static_cast<std::size_t>(instruction_pointer)];
        executed += 1;

        switch (instruction.opcode) {
            case OpCode::kHalt:
                execution_context.halted = true;
                break;

            case OpCode::kSetFlag:
                request.flag = static_cast<Flag>(instruction.args[0]);
                instruction_pointer += 1;
                break;

            case OpCode::kIncrementHop:
                request.hop = static_cast<std::int16_t>(request.hop + 1);
                instruction_pointer += 1;
                break;

            case OpCode::kWritePayloadWord: {
                const std::int64_t payload_index = instruction.args[0];
                const PayloadWord value = static_cast<PayloadWord>(instruction.args[1]);
                if (payload_index >= 0 && payload_index < context.request_table.PayloadWords()) {
                    PayloadWord* payload = context.request_table.PayloadPtr(request.id);
                    payload[static_cast<std::size_t>(payload_index)] = value;
                }
                instruction_pointer += 1;
                break;
            }

            case OpCode::kAddComponentStateWord: {
                const std::int64_t state_index = instruction.args[0];
                const PayloadWord delta = static_cast<PayloadWord>(instruction.args[1]);
                ComponentRuntimeState& component_state = context.component_state.At(component_id);
                if (state_index >= 0 && static_cast<std::size_t>(state_index) < component_state.private_state.size()) {
                    component_state.private_state[static_cast<std::size_t>(state_index)] += delta;
                }
                instruction_pointer += 1;
                break;
            }

            case OpCode::kScheduleTimer: {
                const SimTime delay = static_cast<SimTime>(instruction.args[0]);
                const EventPhase phase = EventPhaseFromInt(instruction.args[1]);
                const SimTime event_time = execution_context.current_event.time + std::max<SimTime>(0, delay);
                if (event_time < context.sim_time) {
                    throw std::runtime_error("behavior attempted to schedule event in the past");
                }
                const TokenId token_id = context.token_table.AcquireActiveToken();
                context.event_heap.Push(Event{
                    .time = event_time,
                    .seq = 0,
                    .request_id = request.id,
                    .target_type = TargetType::kComponent,
                    .target_id = component_id,
                    .phase = phase,
                    .token_id = token_id,
                    .expected_generation = request.generation,
                });
                instruction_pointer += 1;
                break;
            }

            case OpCode::kSpawnRequest: {
                const RequestTypeId type_id = static_cast<RequestTypeId>(instruction.args[0]);
                const Flag flag = static_cast<Flag>(instruction.args[1]);
                const ComponentId destination_component = static_cast<ComponentId>(instruction.args[2]);
                const RequestId child_request_id = context.request_table.Allocate(type_id, flag);
                if (child_request_id == kInvalidRequestId) {
                    context.stats.OnRequestDropped();
                    instruction_pointer += 1;
                    break;
                }
                context.stats.OnRequestCreated(child_request_id, execution_context.current_event.time);
                context.request_table.SetLocation(child_request_id, LocationType::kExecution, destination_component, kInvalidSlotId);
                context.event_heap.Push(Event{
                    .time = execution_context.current_event.time,
                    .seq = 0,
                    .request_id = child_request_id,
                    .target_type = TargetType::kComponent,
                    .target_id = destination_component,
                    .phase = EventPhase::kRequestArrival,
                    .token_id = context.token_table.AcquireActiveToken(),
                    .expected_generation = context.request_table.At(child_request_id).generation,
                });
                instruction_pointer += 1;
                break;
            }

            case OpCode::kInvalidateToken: {
                TokenId token_id = static_cast<TokenId>(instruction.args[0]);
                if (token_id < 0) {
                    token_id = execution_context.current_event.token_id;
                }
                context.token_table.Invalidate(token_id);
                instruction_pointer += 1;
                break;
            }

            case OpCode::kRouteNextLink: {
                if (request.flag < 0 || request.flag >= context.behavior_tables.max_flags ||
                    request.hop < 0 || request.hop >= context.behavior_tables.max_hops) {
                    execution_context.halted = true;
                    break;
                }
                const std::size_t routing_index = context.behavior_tables.RoutingFlatIndex(request.type_id, request.flag, request.hop);
                if (routing_index >= context.behavior_tables.routing_lookup.size()) {
                    execution_context.halted = true;
                    break;
                }

                const LinkId link_id = context.behavior_tables.routing_lookup[routing_index];
                const std::optional<LinkDispatchResult> dispatch =
                    context.link_state.Dispatch(link_id, request.id, execution_context.current_event.time);
                if (!dispatch.has_value()) {
                    execution_context.halted = true;
                    break;
                }

                context.request_table.SetLocation(request.id, LocationType::kLinkQueue, link_id, kInvalidSlotId);
                context.event_heap.Push(Event{
                    .time = dispatch->arrival_time,
                    .seq = 0,
                    .request_id = request.id,
                    .target_type = TargetType::kComponent,
                    .target_id = dispatch->destination_component,
                    .phase = EventPhase::kRequestArrival,
                    .token_id = context.token_table.AcquireActiveToken(),
                    .expected_generation = request.generation,
                });
                instruction_pointer += 1;
                break;
            }

            case OpCode::kLog:
                if (context.design_trace_logging_enabled) {
                    context.stats.AppendDesignLog(LogEntry{
                        .time = execution_context.current_event.time,
                        .request_id = request.id,
                        .event_phase = execution_context.current_event.phase,
                        .component_id = component_id,
                        .custom_fields = {static_cast<PayloadWord>(instruction.args[0]), static_cast<PayloadWord>(instruction.args[1])},
                    });
                }
                instruction_pointer += 1;
                break;

            case OpCode::kPreemptCurrent: {
                SlotId slot_id = request.slot_id;
                if (instruction.args[0] >= 0) {
                    slot_id = static_cast<SlotId>(instruction.args[0]);
                }
                RequestId preempted_request_id = kInvalidRequestId;
                TokenId completion_token = kInvalidTokenId;
                if (context.component_state.Preempt(component_id, slot_id, static_cast<PayloadWord>(instruction.args[1]), &preempted_request_id, &completion_token)) {
                    context.token_table.Invalidate(completion_token);
                    Request& preempted = context.request_table.At(preempted_request_id);
                    preempted.generation += 1;
                    if (context.request_table.PayloadWords() > 0) {
                        PayloadWord* payload = context.request_table.PayloadPtr(preempted_request_id);
                        payload[0] = static_cast<PayloadWord>(instruction.args[1]);
                    }
                    const bool queued = context.component_state.EnqueueWaiting(component_id, preempted_request_id);
                    if (queued) {
                        context.request_table.SetLocation(preempted_request_id, LocationType::kComponentQueue, component_id, kInvalidSlotId);
                    } else {
                        context.stats.OnRequestDropped();
                        context.request_table.Release(preempted_request_id);
                    }
                }
                instruction_pointer += 1;
                break;
            }
        }
    }

    execution_context.halted = true;
}

}  // namespace simrun

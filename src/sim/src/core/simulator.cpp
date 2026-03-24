#include "simrun/core/simulator.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <sstream>

#include "simrun/behavior/behavior_executor.h"

namespace simrun {
namespace {

enum class DesignLogCode : PayloadWord {
    kBootstrapInjected = 1,
    kWorkloadGenerated = 2,
    kEventDiscarded = 3,
    kRequestArrived = 4,
    kRequestQueued = 5,
    kServiceStarted = 6,
    kServiceCompleted = 7,
    kRequestCompleted = 8,
    kRequestDropped = 9,
};

SimTime ServiceDelayFor(const SimContext& context, const ComponentId component_id, const RequestId request_id) {
    const SimTime base_service_time = context.structural_ir.components[static_cast<std::size_t>(component_id)].service_time;
    const PayloadWord* payload = context.request_table.PayloadPtr(request_id);
    const PayloadWord remaining_demand = context.request_table.PayloadWords() > 0 ? payload[0] : 0;
    if (remaining_demand > 0) {
        return static_cast<SimTime>(remaining_demand);
    }
    return std::max<SimTime>(1, base_service_time);
}

const char* PhaseName(const EventPhase phase) {
    switch (phase) {
        case EventPhase::kRequestArrival:
            return "RequestArrival";
        case EventPhase::kServiceCompletion:
            return "ServiceCompletion";
        case EventPhase::kTimerExpiration:
            return "TimerExpiration";
        case EventPhase::kResponseArrival:
            return "ResponseArrival";
        case EventPhase::kWorkloadGeneration:
            return "WorkloadGeneration";
    }
    return "UnknownPhase";
}

}  // namespace

Simulator::Simulator(SimContext context) : context_(std::move(context)) {
    InjectBootstrapEvents();
    InjectWorkloadEvents();
}

RunResult Simulator::Run(const std::size_t max_events) {
    RunResult result{};

    while (!context_.event_heap.Empty()) {
        if (result.executed_events >= max_events) {
            result.status = RunStatus::kStepLimitReached;
            result.final_time = context_.sim_time;
            FinalizeRun();
            return result;
        }

        const SimTime previous_time = context_.sim_time;
        Event event = context_.event_heap.Pop();
        context_.sim_time = event.time;
        {
            std::ostringstream oss;
            oss << "event pop: t=" << event.time << " seq=" << event.seq << " req=" << event.request_id
                << " phase=" << PhaseName(event.phase) << " target=" << event.target_id;
            DebugLog(oss.str());
        }

        const SimTime delta_time = std::max<SimTime>(0, event.time - previous_time);
        for (ComponentId component_id = 0; component_id < context_.component_state.ComponentCount(); ++component_id) {
            const ComponentRuntimeState& component = context_.component_state.At(component_id);
            context_.stats.IntegrateComponentBusy(component_id, component.busy_slots_counter, delta_time);
            context_.stats.IntegrateQueueLength(component_id, component.waiting_queue.size(), delta_time);
        }

        if (context_.horizon > 0 && event.time > context_.horizon) {
            result.status = RunStatus::kHorizonReached;
            result.final_time = context_.sim_time;
            FinalizeRun();
            return result;
        }
        if (context_.termination_predicate && context_.termination_predicate(context_)) {
            result.status = RunStatus::kPredicateTriggered;
            result.final_time = context_.sim_time;
            FinalizeRun();
            return result;
        }

        if (ShouldDiscardEvent(event)) {
            result.discarded_events += 1;
            EmitDesignLog(event.time, event.request_id, event.phase, static_cast<ComponentId>(event.target_id),
                          static_cast<PayloadWord>(DesignLogCode::kEventDiscarded), event.token_id);
            DebugLog("event discarded by logical cancellation or generation mismatch");
            continue;
        }

        DispatchEvent(event);
        result.executed_events += 1;
    }

    result.status = RunStatus::kCompleted;
    result.final_time = context_.sim_time;
    FinalizeRun();
    return result;
}

SimContext& Simulator::Context() noexcept {
    return context_;
}

const SimContext& Simulator::Context() const noexcept {
    return context_;
}

void Simulator::InjectBootstrapEvents() {
    for (const BootstrapEventSpec& bootstrap_event : context_.structural_ir.bootstrap_events) {
        const RequestId request_id = context_.request_table.Allocate(bootstrap_event.request_type, bootstrap_event.flag);
        if (request_id == kInvalidRequestId) {
            context_.stats.OnRequestDropped();
            EmitDesignLog(bootstrap_event.time, kInvalidRequestId, bootstrap_event.phase, bootstrap_event.component_id,
                          static_cast<PayloadWord>(DesignLogCode::kRequestDropped), 0);
            continue;
        }

        context_.stats.OnRequestCreated(request_id, bootstrap_event.time);
        context_.request_table.SetLocation(request_id, LocationType::kExecution, bootstrap_event.component_id, kInvalidSlotId);

        context_.event_heap.Push(Event{
            .time = bootstrap_event.time,
            .seq = 0,
            .request_id = request_id,
            .target_type = TargetType::kComponent,
            .target_id = bootstrap_event.component_id,
            .phase = bootstrap_event.phase,
            .token_id = context_.token_table.AcquireActiveToken(),
            .expected_generation = context_.request_table.At(request_id).generation,
        });

        EmitDesignLog(bootstrap_event.time, request_id, bootstrap_event.phase, bootstrap_event.component_id,
                      static_cast<PayloadWord>(DesignLogCode::kBootstrapInjected), 0);
        std::ostringstream oss;
        oss << "bootstrap injected: req=" << request_id << " component=" << bootstrap_event.component_id;
        DebugLog(oss.str());
    }
}

void Simulator::InjectWorkloadEvents() {
    for (std::size_t workload_index = 0; workload_index < context_.structural_ir.workloads.size(); ++workload_index) {
        const WorkloadDefinition& workload = context_.structural_ir.workloads[workload_index];
        for (std::int32_t i = 0; i < workload.count; ++i) {
            const SimTime time = workload.start_time + (workload.interval * i);
            context_.event_heap.Push(Event{
                .time = time,
                .seq = 0,
                .request_id = kInvalidRequestId,
                .target_type = TargetType::kGlobal,
                .target_id = static_cast<std::int32_t>(workload_index),
                .phase = EventPhase::kWorkloadGeneration,
                .token_id = kInvalidTokenId,
                .expected_generation = 0,
            });
            std::ostringstream oss;
            oss << "workload event scheduled: workload=" << workload_index << " time=" << time;
            DebugLog(oss.str());
        }
    }
}

bool Simulator::ShouldTerminate() const {
    if (context_.event_heap.Empty()) {
        return true;
    }
    if (context_.horizon > 0 && context_.sim_time > context_.horizon) {
        return true;
    }
    if (context_.termination_predicate && context_.termination_predicate(context_)) {
        return true;
    }
    return false;
}

bool Simulator::ShouldDiscardEvent(const Event& event) const {
    if (!context_.token_table.IsEventTokenValid(event.token_id)) {
        return true;
    }
    if (event.request_id == kInvalidRequestId) {
        return false;
    }
    if (!context_.request_table.IsAllocated(event.request_id)) {
        return true;
    }
    const Request& request = context_.request_table.At(event.request_id);
    return request.generation != event.expected_generation;
}

void Simulator::DispatchEvent(const Event& event) {
    if (event.phase == EventPhase::kWorkloadGeneration && event.target_type == TargetType::kGlobal) {
        if (event.target_id < 0 || static_cast<std::size_t>(event.target_id) >= context_.structural_ir.workloads.size()) {
            return;
        }
        const WorkloadDefinition& workload = context_.structural_ir.workloads[static_cast<std::size_t>(event.target_id)];
        const RequestId request_id = context_.request_table.Allocate(workload.request_type, workload.initial_flag);
        if (request_id == kInvalidRequestId) {
            context_.stats.OnRequestDropped();
            EmitDesignLog(event.time, kInvalidRequestId, event.phase, workload.entry_component_id,
                          static_cast<PayloadWord>(DesignLogCode::kRequestDropped), 0);
            return;
        }

        context_.stats.OnRequestCreated(request_id, event.time);
        context_.request_table.SetLocation(request_id, LocationType::kExecution, workload.entry_component_id, kInvalidSlotId);
        context_.event_heap.Push(Event{
            .time = event.time,
            .seq = 0,
            .request_id = request_id,
            .target_type = TargetType::kComponent,
            .target_id = workload.entry_component_id,
            .phase = EventPhase::kRequestArrival,
            .token_id = context_.token_table.AcquireActiveToken(),
            .expected_generation = context_.request_table.At(request_id).generation,
        });
        EmitDesignLog(event.time, request_id, event.phase, workload.entry_component_id,
                      static_cast<PayloadWord>(DesignLogCode::kWorkloadGenerated), event.target_id);
        std::ostringstream oss;
        oss << "workload generated request: req=" << request_id << " component=" << workload.entry_component_id;
        DebugLog(oss.str());
        return;
    }

    switch (event.phase) {
        case EventPhase::kRequestArrival:
            HandleRequestArrival(event);
            return;
        case EventPhase::kServiceCompletion:
            HandleServiceCompletion(event);
            return;
        case EventPhase::kTimerExpiration:
        case EventPhase::kResponseArrival:
        case EventPhase::kWorkloadGeneration:
            HandleBehaviorPhase(event);
            return;
    }
}

void Simulator::HandleRequestArrival(const Event& event) {
    if (!context_.request_table.IsAllocated(event.request_id)) {
        return;
    }
    const ComponentId component_id = static_cast<ComponentId>(event.target_id);
    Request& request = context_.request_table.At(event.request_id);

    if (request.location_type == LocationType::kLinkQueue && request.location_id >= 0) {
        context_.link_state.MarkArrived(static_cast<LinkId>(request.location_id), request.id);
    }

    EmitDesignLog(event.time, request.id, event.phase, component_id,
                  static_cast<PayloadWord>(DesignLogCode::kRequestArrived), request.flag);

    const TokenId completion_token = context_.token_table.AcquireActiveToken();
    SlotId assigned_slot = kInvalidSlotId;
    if (context_.component_state.AdmitIfPossible(component_id, request.id, completion_token, &assigned_slot)) {
        context_.request_table.SetLocation(request.id, LocationType::kComponentService, component_id, assigned_slot);
        const SimTime completion_time = event.time + ServiceDelayFor(context_, component_id, request.id);
        EmitDesignLog(event.time, request.id, EventPhase::kServiceCompletion, component_id,
                      static_cast<PayloadWord>(DesignLogCode::kServiceStarted), assigned_slot);
        context_.event_heap.Push(Event{
            .time = completion_time,
            .seq = 0,
            .request_id = request.id,
            .target_type = TargetType::kComponent,
            .target_id = component_id,
            .phase = EventPhase::kServiceCompletion,
            .token_id = completion_token,
            .expected_generation = request.generation,
        });
        std::ostringstream oss;
        oss << "request admitted to service: req=" << request.id << " component=" << component_id << " slot=" << assigned_slot;
        DebugLog(oss.str());
        return;
    }

    if (context_.component_state.EnqueueWaiting(component_id, request.id)) {
        context_.request_table.SetLocation(request.id, LocationType::kComponentQueue, component_id, kInvalidSlotId);
        EmitDesignLog(event.time, request.id, event.phase, component_id,
                      static_cast<PayloadWord>(DesignLogCode::kRequestQueued),
                      static_cast<PayloadWord>(context_.component_state.At(component_id).waiting_queue.size()));
        DebugLog("request queued due to full capacity");
        return;
    }

    context_.stats.OnRequestDropped();
    EmitDesignLog(event.time, request.id, event.phase, component_id,
                  static_cast<PayloadWord>(DesignLogCode::kRequestDropped), 0);
    DebugLog("request dropped due to queue admission failure");
    context_.request_table.Release(request.id);
}

void Simulator::HandleServiceCompletion(const Event& event) {
    if (!context_.request_table.IsAllocated(event.request_id)) {
        return;
    }
    const ComponentId component_id = static_cast<ComponentId>(event.target_id);
    Request& request = context_.request_table.At(event.request_id);
    const SlotId slot_id = request.slot_id;

    TokenId completion_token = kInvalidTokenId;
    const RequestId released_request = context_.component_state.ReleaseSlot(component_id, slot_id, &completion_token);
    if (released_request == kInvalidRequestId) {
        return;
    }

    context_.token_table.Invalidate(completion_token);
    context_.request_table.SetLocation(request.id, LocationType::kExecution, component_id, kInvalidSlotId);
    EmitDesignLog(event.time, request.id, event.phase, component_id,
                  static_cast<PayloadWord>(DesignLogCode::kServiceCompleted), slot_id);
    DebugLog("service completion event executed");

    ExecutionContext execution_context{};
    execution_context.current_event = event;
    execution_context.current_request_id = request.id;
    execution_context.current_component_id = component_id;
    BehaviorExecutor{}.Execute(context_, execution_context);

    if (context_.request_table.IsAllocated(request.id) &&
        context_.request_table.At(request.id).location_type == LocationType::kExecution) {
        context_.stats.OnRequestCompleted(request.id, event.time);
        EmitDesignLog(event.time, request.id, event.phase, component_id,
                      static_cast<PayloadWord>(DesignLogCode::kRequestCompleted), 0);
        DebugLog("request completed and released");
        context_.request_table.Release(request.id);
    }

    AdmitQueuedIfPossible(component_id);
}

void Simulator::HandleBehaviorPhase(const Event& event) {
    if (!context_.request_table.IsAllocated(event.request_id)) {
        return;
    }
    const ComponentId component_id = static_cast<ComponentId>(event.target_id);
    Request& request = context_.request_table.At(event.request_id);
    context_.request_table.SetLocation(request.id, LocationType::kExecution, component_id, kInvalidSlotId);

    ExecutionContext execution_context{};
    execution_context.current_event = event;
    execution_context.current_request_id = request.id;
    execution_context.current_component_id = component_id;
    BehaviorExecutor{}.Execute(context_, execution_context);

    if (context_.request_table.IsAllocated(request.id) &&
        context_.request_table.At(request.id).location_type == LocationType::kExecution) {
        context_.stats.OnRequestCompleted(request.id, event.time);
        EmitDesignLog(event.time, request.id, event.phase, component_id,
                      static_cast<PayloadWord>(DesignLogCode::kRequestCompleted), 0);
        DebugLog("request completed from behavior phase");
        context_.request_table.Release(request.id);
    }
}

void Simulator::AdmitQueuedIfPossible(const ComponentId component_id) {
    while (context_.component_state.HasFreeCapacity(component_id)) {
        const std::optional<RequestId> next_request = context_.component_state.PopWaiting(component_id);
        if (!next_request.has_value()) {
            return;
        }
        if (!context_.request_table.IsAllocated(next_request.value())) {
            continue;
        }

        SlotId slot_id = kInvalidSlotId;
        const TokenId completion_token = context_.token_table.AcquireActiveToken();
        if (!context_.component_state.AdmitIfPossible(component_id, next_request.value(), completion_token, &slot_id)) {
            const bool requeued = context_.component_state.EnqueueWaiting(component_id, next_request.value());
            if (!requeued) {
                context_.stats.OnRequestDropped();
                EmitDesignLog(context_.sim_time, next_request.value(), EventPhase::kRequestArrival, component_id,
                              static_cast<PayloadWord>(DesignLogCode::kRequestDropped), 0);
                context_.request_table.Release(next_request.value());
            }
            return;
        }

        Request& request = context_.request_table.At(next_request.value());
        context_.request_table.SetLocation(request.id, LocationType::kComponentService, component_id, slot_id);
        const SimTime completion_time = context_.sim_time + ServiceDelayFor(context_, component_id, request.id);

        context_.event_heap.Push(Event{
            .time = completion_time,
            .seq = 0,
            .request_id = request.id,
            .target_type = TargetType::kComponent,
            .target_id = component_id,
            .phase = EventPhase::kServiceCompletion,
            .token_id = completion_token,
            .expected_generation = request.generation,
        });
        std::ostringstream oss;
        oss << "queued request admitted: req=" << request.id << " component=" << component_id << " slot=" << slot_id;
        DebugLog(oss.str());
    }
}

void Simulator::EmitDesignLog(const SimTime time,
                              const RequestId request_id,
                              const EventPhase phase,
                              const ComponentId component_id,
                              const PayloadWord field0,
                              const PayloadWord field1) {
    if (!context_.design_trace_logging_enabled) {
        return;
    }
    context_.stats.AppendDesignLog(LogEntry{
        .time = time,
        .request_id = request_id,
        .event_phase = phase,
        .component_id = component_id,
        .custom_fields = {field0, field1},
    });
}

void Simulator::FinalizeRun() {
    if (!context_.dump_design_trace_to_file_on_finish) {
        return;
    }
    const bool dumped = context_.stats.DumpDesignTraceCsv(context_.design_trace_output_path);
    if (dumped) {
        std::ostringstream oss;
        oss << "design trace dumped to file: " << context_.design_trace_output_path;
        DebugLog(oss.str());
    } else {
        std::ostringstream oss;
        oss << "failed to dump design trace to file: " << context_.design_trace_output_path;
        DebugLog(oss.str());
    }
}

void Simulator::DebugLog(const std::string_view message) const {
    if (!context_.debug_console_logging_enabled) {
        return;
    }
    std::cout << "[simrun][t=" << context_.sim_time << "] " << message << '\n';
}

}  // namespace simrun

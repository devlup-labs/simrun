#include "dispatch.h"

#include <stdexcept>

#include "../behavior/behavior_executor.h"

namespace sim::events {
namespace {

void handle_workload_arrival(const Event& event, Simulator& sim) {
    if (
        event.workload_idx < 0 ||
        event.workload_idx >= static_cast<int>(sim.context().workloads.size())
    ) {
        throw std::runtime_error("WORKLOAD_ARRIVAL references unknown workload index");
    }

    const WorkloadModel& model = sim.context().workloads[event.workload_idx];
    WorkloadRuntimeState& runtime = sim.state().workload_state[event.workload_idx];

    if (runtime.generated_count >= model.total_requests) {
        return;
    }

    const int request_id = sim.create_request(model.request_type_idx, model.target_component_idx);
    runtime.generated_count += 1;

    sim.schedule(Event::make_component_event(
        sim.now(),
        sim.next_seq(),
        request_id,
        model.target_component_idx
    ));

    if (runtime.generated_count < model.total_requests) {
        const SimTime delta = sim.sample_workload_arrival_delta(model.arrival_dist);
        sim.schedule(Event::make_workload_arrival(
            sim.now() + delta,
            sim.next_seq(),
            event.workload_idx
        ));
    }
}

void handle_component_event(const Event& event, Simulator& sim) {
    auto& requests = sim.state().requests;
    if (event.request_id < 0 || event.request_id >= static_cast<int>(requests.size())) {
        throw std::runtime_error("COMPONENT_EVENT references unknown request_id");
    }

    Request& req = requests[event.request_id];
    if (event.component_idx < 0) {
        throw std::runtime_error("COMPONENT_EVENT missing component index");
    }
    req.current_component_idx = event.component_idx;

    behavior::run_behavior_sequence(sim, req, event.component_idx);
}

void handle_arrive_at_link(const Event& event, Simulator& sim) {
    auto& requests = sim.state().requests;
    const auto& context = sim.context();

    if (event.request_id < 0 || event.request_id >= static_cast<int>(requests.size())) {
        throw std::runtime_error("ARRIVE_AT_LINK references unknown request_id");
    }
    if (event.link_idx < 0 || event.link_idx >= static_cast<int>(context.link_models.size())) {
        throw std::runtime_error("ARRIVE_AT_LINK references unknown link index");
    }

    const LinkModel& link = context.link_models[event.link_idx];
    if (link.to_component_idx < 0) {
        throw std::runtime_error("ARRIVE_AT_LINK cannot deliver to external endpoint");
    }

    sim.schedule(Event::make_arrive_at_component(
        sim.now() + link.latency_ns,
        sim.next_seq(),
        event.request_id,
        link.to_component_idx
    ));
}

void handle_arrive_at_component(const Event& event, Simulator& sim) {
    auto& requests = sim.state().requests;
    if (event.request_id < 0 || event.request_id >= static_cast<int>(requests.size())) {
        throw std::runtime_error("ARRIVE_AT_COMPONENT references unknown request_id");
    }
    if (event.component_idx < 0 || event.component_idx >= static_cast<int>(sim.state().components.size())) {
        throw std::runtime_error("ARRIVE_AT_COMPONENT references unknown component index");
    }

    Request& req = requests[event.request_id];
    req.current_component_idx = event.component_idx;

    sim.schedule(Event::make_component_event(
        sim.now(),
        sim.next_seq(),
        event.request_id,
        event.component_idx
    ));
}

void handle_fault_start(const Event& event, Simulator& sim) {
    if (
        event.fault_entity_link_idx < 0 ||
        event.fault_entity_link_idx >= static_cast<int>(sim.state().links.size())
    ) {
        throw std::runtime_error("FAULT_START references unknown link index");
    }

    LinkState& link_state = sim.state().links[event.fault_entity_link_idx];
    const LinkModel& link_model = sim.context().link_models[event.fault_entity_link_idx];

    switch (event.fault_effect) {
        case FaultEffect::HALVE_BANDWIDTH:
            link_state.bandwidth_multiplier *= 0.5;
            link_state.effective_bandwidth_mbps =
                link_model.base_bandwidth_mbps * link_state.bandwidth_multiplier;
            break;
        default:
            throw std::runtime_error("Unsupported FAULT_START effect");
    }
}

}  // namespace

void dispatch(const Event& event, Simulator& sim) {
    switch (event.type) {
        case EventType::WORKLOAD_ARRIVAL:
            handle_workload_arrival(event, sim);
            break;
        case EventType::COMPONENT_EVENT:
            handle_component_event(event, sim);
            break;
        case EventType::ARRIVE_AT_LINK:
            handle_arrive_at_link(event, sim);
            break;
        case EventType::ARRIVE_AT_COMPONENT:
            handle_arrive_at_component(event, sim);
            break;
        case EventType::FAULT_START:
            handle_fault_start(event, sim);
            break;
    }
}

}  // namespace sim::events

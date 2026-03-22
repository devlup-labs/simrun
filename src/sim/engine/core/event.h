#pragma once

#include <cstdint>
#include <memory>

namespace sim {

using SimTime = std::uint64_t;

enum class EventType {
    WORKLOAD_ARRIVAL,
    COMPONENT_EVENT,
    ARRIVE_AT_LINK,
    ARRIVE_AT_COMPONENT,
    FAULT_START,
};

enum class FaultEffect {
    NONE,
    HALVE_BANDWIDTH,
};

struct Event {
    SimTime time = 0;
    std::uint64_t seq = 0;
    EventType type = EventType::COMPONENT_EVENT;

    int request_id = -1;
    int request_type_idx = -1;
    int component_idx = -1;
    int target_component_idx = -1;
    int workload_idx = -1;
    int link_idx = -1;
    int fault_entity_link_idx = -1;
    FaultEffect fault_effect = FaultEffect::NONE;

    static std::unique_ptr<Event> make_workload_arrival(
        SimTime time,
        std::uint64_t seq,
        int workload_idx
    ) {
        auto ev = std::make_unique<Event>();
        ev->time = time;
        ev->seq = seq;
        ev->type = EventType::WORKLOAD_ARRIVAL;
        ev->workload_idx = workload_idx;
        return ev;
    }

    static std::unique_ptr<Event> make_component_event(
        SimTime time,
        std::uint64_t seq,
        int request_id,
        int component_idx
    ) {
        auto ev = std::make_unique<Event>();
        ev->time = time;
        ev->seq = seq;
        ev->type = EventType::COMPONENT_EVENT;
        ev->request_id = request_id;
        ev->component_idx = component_idx;
        return ev;
    }

    static std::unique_ptr<Event> make_arrive_at_link(
        SimTime time,
        std::uint64_t seq,
        int request_id,
        int link_idx
    ) {
        auto ev = std::make_unique<Event>();
        ev->time = time;
        ev->seq = seq;
        ev->type = EventType::ARRIVE_AT_LINK;
        ev->request_id = request_id;
        ev->link_idx = link_idx;
        return ev;
    }

    static std::unique_ptr<Event> make_arrive_at_component(
        SimTime time,
        std::uint64_t seq,
        int request_id,
        int component_idx
    ) {
        auto ev = std::make_unique<Event>();
        ev->time = time;
        ev->seq = seq;
        ev->type = EventType::ARRIVE_AT_COMPONENT;
        ev->request_id = request_id;
        ev->component_idx = component_idx;
        return ev;
    }

    static std::unique_ptr<Event> make_fault_start(
        SimTime time,
        std::uint64_t seq,
        int link_idx,
        FaultEffect effect
    ) {
        auto ev = std::make_unique<Event>();
        ev->time = time;
        ev->seq = seq;
        ev->type = EventType::FAULT_START;
        ev->fault_entity_link_idx = link_idx;
        ev->fault_effect = effect;
        return ev;
    }
};

inline const char* event_type_name(EventType type) {
    switch (type) {
        case EventType::WORKLOAD_ARRIVAL:
            return "WORKLOAD_ARRIVAL";
        case EventType::COMPONENT_EVENT:
            return "COMPONENT_EVENT";
        case EventType::ARRIVE_AT_LINK:
            return "ARRIVE_AT_LINK";
        case EventType::ARRIVE_AT_COMPONENT:
            return "ARRIVE_AT_COMPONENT";
        case EventType::FAULT_START:
            return "FAULT_START";
    }
    return "UNKNOWN_EVENT";
}

}  // namespace sim

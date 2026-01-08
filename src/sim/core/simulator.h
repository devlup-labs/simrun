#pragma once
#include "sim_types.h"
#include "scheduler.h"
#include "event_loop.h"

#include "../entities/entity_context.h"
#include "../entities/entity_state.h"

using Context = SimulationContext;
using State   = SimulationState;

class EventQueue;

class Simulator final : public EventLoop {
private:
    SimTime current_time = 0;

    EventQueue& queue;
    EventScheduler scheduler;

    const Context& context;
    State& state;

public:
    Simulator(
        EventQueue& q,
        const Context& ctx,
        State& st
    );

    void run() override;
    SimTime now() const;
};

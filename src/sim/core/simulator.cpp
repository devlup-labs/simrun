#include "simulator.h"
#include "event_queue.h"
#include "../events/event.h"

Simulator::Simulator(
    EventQueue& q,
    const Context& ctx,
    State& st
)
    : queue(q),
      scheduler(q),
      context(ctx),
      state(st) {}

void Simulator::run() {
    while (!queue.empty()) {
        auto event = queue.pop();

        current_time = event->time;

        event->execute(context, state, scheduler);
    }
}

SimTime Simulator::now() const {
    return current_time;
}

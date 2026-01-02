//simulation componenets will only call scheduler.schedule(e) to schedule events onto queue
#pragma once
#include "../events/event_queue.h"

class EventScheduler {
private:
    EventQueue& queue; //private as components shouldnt manipulate event queue themselves

public:
    explicit EventScheduler(EventQueue& q) : queue(q) {}

    void schedule(Event* e) {
        queue.push(e); //this is NOT saying "push to queue, it says this event will occur in simulated future"
    }
};

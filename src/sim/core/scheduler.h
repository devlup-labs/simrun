#pragma once
#include <memory>

class Event;
class EventQueue;

class EventScheduler {
private:
    EventQueue& queue;  //private as components only schedule the events using scheduler, so they should not be able to inspect it

public:
    explicit EventScheduler(EventQueue& q) : queue(q) {}

    void schedule(std::unique_ptr<Event> e) {
        queue.push(std::move(e));
    }

};
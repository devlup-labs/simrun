//event_queue.h defines the abstract event queue API which will be used by event scheduler
#pragma once
#include <memory>

class Event;

class EventQueue {
public:
    virtual ~EventQueue() = default;

    virtual void push(std::unique_ptr<Event> e) = 0;
    virtual std::unique_ptr<Event> pop() = 0;
    virtual bool empty() const = 0;
};

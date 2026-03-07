// events/event.h
#pragma once
#include <cstdint>

using SimTime = uint64_t;

class Event {
public:
    virtual ~Event() = default;

    // When this event is scheduled to run
    virtual SimTime time() const = 0;

    // What happens when the event fires
    virtual void execute() = 0;
};

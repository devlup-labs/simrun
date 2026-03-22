#pragma once

#include <memory>
#include <queue>
#include <vector>

#include "event.h"

namespace sim {

class Scheduler {
public:
    void schedule(std::unique_ptr<Event> event);
    std::unique_ptr<Event> next();
    bool empty() const;
    std::size_t size() const;

private:
    struct EventCompare {
        bool operator()(const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b) const;
    };

    std::priority_queue<
        std::unique_ptr<Event>,
        std::vector<std::unique_ptr<Event>>,
        EventCompare
    >
        queue_;
};

}  // namespace sim

#include "scheduler.h"

#include <stdexcept>

namespace sim {

bool Scheduler::EventCompare::operator()(
    const std::unique_ptr<Event>& a,
    const std::unique_ptr<Event>& b
) const {
    if (a->time != b->time) {
        return a->time > b->time;
    }
    return a->seq > b->seq;
}

void Scheduler::schedule(std::unique_ptr<Event> event) {
    queue_.push(std::move(event));
}

std::unique_ptr<Event> Scheduler::next() {
    if (queue_.empty()) {
        throw std::runtime_error("Scheduler::next called on empty queue");
    }
    auto ev = std::move(const_cast<std::unique_ptr<Event>&>(queue_.top()));
    queue_.pop();
    return ev;
}

bool Scheduler::empty() const {
    return queue_.empty();
}

std::size_t Scheduler::size() const {
    return queue_.size();
}

}  // namespace sim

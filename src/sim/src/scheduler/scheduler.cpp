#include "simrun/scheduler/scheduler.h"

#include <algorithm>
#include <stdexcept>

namespace simrun {

bool Scheduler::MinHeapComparator::operator()(const Event& lhs, const Event& rhs) const noexcept {
    return EventLessThan(rhs, lhs);
}

void Scheduler::Reset() {
    event_heap_.clear();
    global_seq_counter_ = 0;
    sim_time_ = 0;
}

void Scheduler::Push(Event event) {
    event.seq = NextSequence();
    event_heap_.push_back(event);
    std::push_heap(event_heap_.begin(), event_heap_.end(), MinHeapComparator{});
}

Event Scheduler::Pop() {
    if (event_heap_.empty()) {
        throw std::runtime_error("cannot pop from empty scheduler");
    }
    std::pop_heap(event_heap_.begin(), event_heap_.end(), MinHeapComparator{});
    Event event = event_heap_.back();
    event_heap_.pop_back();
    sim_time_ = event.time;
    return event;
}

bool Scheduler::Empty() const noexcept {
    return event_heap_.empty();
}

std::size_t Scheduler::Size() const noexcept {
    return event_heap_.size();
}

SimTime Scheduler::SimTimeNow() const noexcept {
    return sim_time_;
}

void Scheduler::SetSimTime(const SimTime time) {
    if (time < sim_time_) {
        throw std::invalid_argument("sim_time cannot move backwards");
    }
    sim_time_ = time;
}

EventSeq Scheduler::NextSequence() noexcept {
    const EventSeq seq = global_seq_counter_;
    global_seq_counter_ += 1;
    return seq;
}

}  // namespace simrun

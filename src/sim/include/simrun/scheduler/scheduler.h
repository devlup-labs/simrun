#pragma once

#include <vector>

#include "simrun/core/event.h"

namespace simrun {

class Scheduler {
public:
    Scheduler() = default;

    void Reset();

    void Push(Event event);
    Event Pop();

    [[nodiscard]] bool Empty() const noexcept;
    [[nodiscard]] std::size_t Size() const noexcept;

    [[nodiscard]] SimTime SimTimeNow() const noexcept;
    void SetSimTime(SimTime time);

    [[nodiscard]] EventSeq NextSequence() noexcept;

private:
    struct MinHeapComparator {
        bool operator()(const Event& lhs, const Event& rhs) const noexcept;
    };

    std::vector<Event> event_heap_{};
    EventSeq global_seq_counter_ = 0;
    SimTime sim_time_ = 0;
};

}  // namespace simrun

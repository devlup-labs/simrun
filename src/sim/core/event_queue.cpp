//event_queue.cpp defines different priority/calender/ladder (for now just pq) queues
//at runtime EventQueue reference may point to any of them (depending on configuration)
#include "event_queue.h"
#include "../events/event.h"
#include <queue>
#include <vector>

using std::unique_ptr;
using std::priority_queue;
using std::vector;
using std::move;

namespace {

struct EventCompare {
    bool operator()(
        const unique_ptr<Event>& a,
        const unique_ptr<Event>& b
    ) const {
        return a->time > b->time;
    }
};

class PriorityEventQueue : public EventQueue {
private:
    priority_queue<
        unique_ptr<Event>,
        vector<unique_ptr<Event>>,
        EventCompare
    > pq;

public:
    void push(unique_ptr<Event> e) override {
        pq.push(move(e));
    }

    unique_ptr<Event> pop() override {
        auto e = move(pq.top());
        pq.pop();
        return e;
    }

    bool empty() const override {
        return pq.empty();
    }
};

}


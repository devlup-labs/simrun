// events/api_events.cpp
#include "api_events.h"
#include "api_service.h"   // your service model
#include "simulator.h"     // event scheduling, clock, RNG

/* ---------------- RequestArrivalEvent ---------------- */

RequestArrivalEvent::RequestArrivalEvent(
    SimTime t,
    ApiService* service,
    Simulator* sim
) : t(t), service(service), sim(sim) {}

SimTime RequestArrivalEvent::time() const {
    return t;
}

void RequestArrivalEvent::execute() {
    // Case 1: Can process immediately
    if (service -> active_requests < service -> concurrency_limit) {
        service -> active_requests++;

        sim -> schedule_event(
            new RequestProcessingEvent(sim -> now(), service , sim)
        );
        return;
    }

    // Case 2: Queue has space
    if (!service -> queue_full()) {
        service -> enqueue_request();
        return;
    }

    // Case 3: Reject request
    service -> rejected_requests++;
}

/* ---------------- RequestProcessingEvent ---------------- */

RequestProcessingEvent::RequestProcessingEvent(
    SimTime t,
    ApiService* service,
    Simulator* sim
) : t(t), service(service), sim(sim) {}

SimTime RequestProcessingEvent::time() const {
    return t;
}

void RequestProcessingEvent::execute() {
    // 1. Sample processing time (log-normal, etc.)
    SimTime processing_time = service -> sample_latency();

    // 2. Schedule completion / next network event
    sim -> schedule_event_at(
        sim -> now() + processing_time,
        service -> create_next_network_event()
    );

    // 3. Update concurrency
    service -> active_requests--;

    // 4. Pull from queue if exists
    if (!service -> queue_empty()) {
        service -> dequeue_request();
        service -> active_requests++;

        sim -> schedule_event(
            new RequestProcessingEvent(sim -> now(), service, sim)
        );
    }
}


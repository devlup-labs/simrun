// events/api_events.h
#pragma once

#include "events.h"

// Forward declarations (avoid heavy includes in headers)
class ApiService;
class Simulator;

/*
 * Event 1: RequestArrival
 * - Checks concurrency limit
 * - If possible → schedule RequestProcessing
 * - Else enqueue
 * - If queue full → reject
 */
class RequestArrivalEvent : public Event {
public:
    RequestArrivalEvent(
        SimTime t,
        ApiService* service,
        Simulator* sim
    );

    SimTime time() const override;
    void execute() override;

private:
    SimTime t;
    ApiService* service;
    Simulator* sim;
};


/*
 * Event 2: RequestProcessing
 * - Calculates processing time
 * - Updates concurrency
 * - Advances simulation by scheduling completion/network event
 */
class RequestProcessingEvent : public Event {
public:
    RequestProcessingEvent(
        SimTime t,
        ApiService* service,
        Simulator* sim
    );

    SimTime time() const override;
    void execute() override;

private:
    SimTime t_;
    ApiService* service_;
    Simulator* sim_;
};

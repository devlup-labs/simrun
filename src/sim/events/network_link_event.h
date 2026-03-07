#ifndef SIMRUN_NETWORK_LINK_EVENT_H
#define SIMRUN_NETWORK_LINK_EVENT_H

#include <cstdint>
#include <string>

/* ---------------- Forward Declarations ---------------- */

struct Request;
class EventScheduler;
struct Event; 
class NetworkLinkEntity;

/* ---------------- Network Events ---------------- */

// Event 1: Packet Arrival 
// Pushes the job to the queue. If link is free, starts transmission.
struct NetworkLinkArrivalEvent : public Event {
    NetworkLinkEntity* link;
    Request* request;
    
    // Context for this specific arrival (Fresh vs Retry)
    uint32_t bytes_to_transmit;
    int attempt_count;

    NetworkLinkArrivalEvent(
        double ts,
        uint64_t seed_,
        NetworkLinkEntity* link_,
        Request* req_,
        uint32_t bytes_ = 0, // 0 means "Use Request Size" (default)
        int attempts_ = 0
    );

    void execute(EventScheduler& scheduler) override;
};

// Event 2: Transmission Departure
// Fires when the wire finishes sending. Calculates Fate (Loss/Success).
// Triggers the next job in the queue.
struct NetworkLinkDepartureEvent : public Event {
    NetworkLinkEntity* link;
    
    // We store the job details here to perform fate calculation
    uint32_t transmitted_bytes;
    int attempt_count;
    Request* request;

    NetworkLinkDepartureEvent(
        double ts,
        uint64_t seed_,
        NetworkLinkEntity* link_,
        Request* req_,
        uint32_t bytes_,
        int attempts_
    );

    void execute(EventScheduler& scheduler) override;
};

// Event 3: Logging (Optional, keeps logic clean)
struct NetworkLogEvent : public Event {
    NetworkLinkEntity* link;
    Request* request;
    bool is_loss;
    std::string msg;

    NetworkLogEvent(double ts, NetworkLinkEntity* l, Request* r, bool loss, std::string m);
    void execute(EventScheduler& scheduler) override;
};

#endif // SIMRUN_NETWORK_LINK_EVENT_H
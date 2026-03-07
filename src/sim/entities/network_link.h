#ifndef SIMRUN_NETWORK_LINK_H
#define SIMRUN_NETWORK_LINK_H

#include <cstdint>
#include <string>
#include <deque>
#include <nlohmann/json.hpp>

#include "base_entity.h"
#include "request.h" // Needed for Request pointer

// A "Job" represents a specific transmission attempt (e.g., a retry of 5KB)
struct LinkJob {
    Request* request;
    uint32_t size_bytes; // Size for THIS transmission (shrinks on retry)
    int attempt_count;   // 0 for fresh, >0 for retries
};

class NetworkLinkEntity : public BaseEntity {
public:
    explicit NetworkLinkEntity(uint32_t id, const nlohmann::json& params);

    /* ---------- Topology (Immutable) ---------- */
    std::string from;
    std::string to;

    /* ---------- Physics Config ---------- */
    double bandwidth_mbps;       
    double base_latency_mean;    
    double base_packet_loss_rate;
    double tcp_rto_ms;           
    uint32_t mtu_bytes;          

    /* ---------- Runtime State ---------- */
    bool is_down;
    
    // The Queue: Holds jobs waiting for the wire
    std::deque<LinkJob> transmission_queue;

    // The Server State: Is the wire currently transmitting?
    bool is_busy;

    // Effective values (mutable by Chaos)
    double current_packet_loss_rate;

    /* ---------- Statistics ---------- */
    struct Stats {
        uint64_t packets_sent = 0;
        uint64_t packets_dropped = 0;
        uint64_t tcp_retransmits = 0;
        uint64_t bytes_transmitted = 0;
    } stats;
};

#endif // SIMRUN_NETWORK_LINK_H
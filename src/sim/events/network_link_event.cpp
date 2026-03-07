#include "network_link_events.h"
#include "network_link.h"
#include "event.h"
#include "scheduler.h"
#include "request.h" 
#include "rng.h"     

#include <iostream>  
#include <cmath>     // pow
#include <algorithm> // max, min

using namespace std;

/* ---------------- Helper Logic ---------------- */

// Starts the job at the front of the queue
void start_next_job(NetworkLinkEntity* link, EventScheduler& scheduler, double now, uint64_t seed) {
    if (link->transmission_queue.empty()) {
        link->is_busy = false; // Link goes idle
        return;
    }

    link->is_busy = true;
    const LinkJob& job = link->transmission_queue.front();

    // 1. Calculate Wire Time (Transmission)
    // Time = (Bits) / (Bits/ms)
    double transmission_ms = (static_cast<double>(job.size_bytes) * 8.0) / 
                             (link->bandwidth_mbps * 1000.0);

    // 2. Schedule Departure (End of Transmission)
    // Note: Latency (Propagation) is added at the END, upon delivery.
    double departure_time = now + transmission_ms;

    scheduler.schedule(new NetworkLinkDepartureEvent(
        departure_time,
        seed, // Pass seed forward
        link,
        job.request,
        job.size_bytes,
        job.attempt_count
    ));
}

bool nl_check_loss(NetworkLinkEntity* link, uint64_t& seed) {
    double r = uniform_dist(0.0, 1.0, seed);
    seed = next_seed(seed);
    return r < link->current_packet_loss_rate;
}

/* ---------------- Constructors ---------------- */

NetworkLinkArrivalEvent::NetworkLinkArrivalEvent(
    double ts, uint64_t seed_, NetworkLinkEntity* link_, Request* req_, 
    uint32_t bytes_, int attempts_
) {
    type = EventType::NETWORK_LINK_ARRIVAL;
    timestamp = ts;
    seed = seed_;
    link = link_;
    request = req_;
    // If bytes_ is 0 (default), use the full request size (Fresh Request)
    bytes_to_transmit = (bytes_ == 0) ? req_->packet_size_bytes : bytes_;
    attempt_count = attempts_;
}

NetworkLinkDepartureEvent::NetworkLinkDepartureEvent(
    double ts, uint64_t seed_, NetworkLinkEntity* link_, Request* req_,
    uint32_t bytes_, int attempts_
) {
    type = EventType::NETWORK_LINK_DEPARTURE; // Ensure this Enum exists
    timestamp = ts;
    seed = seed_;
    link = link_;
    request = req_;
    transmitted_bytes = bytes_;
    attempt_count = attempts_;
}

NetworkLogEvent::NetworkLogEvent(double ts, NetworkLinkEntity* l, Request* r, bool loss, string m) {
    // type = EventType::LOG; 
    timestamp = ts; link = l; request = r; is_loss = loss; msg = m;
}

/* ---------------- Event Execution ---------------- */

void NetworkLinkArrivalEvent::execute(EventScheduler& scheduler) {
    // 1. Hard Failure Check
    if (link->is_down) {
        link->stats.packets_dropped++;
        return; 
    }

    // 2. Enqueue the Job
    LinkJob job;
    job.request = request;
    job.size_bytes = bytes_to_transmit;
    job.attempt_count = attempt_count;
    
    link->transmission_queue.push_back(job);

    // 3. Try to Start Transmission
    // If the link is idle, we must wake it up. 
    // If it's busy, this job just sits in the queue and waits for a DepartureEvent to trigger it.
    if (!link->is_busy) {
        start_next_job(link, scheduler, timestamp, seed);
    }
}

void NetworkLinkDepartureEvent::execute(EventScheduler& scheduler) {
    // 1. Pop the finished job
    if (!link->transmission_queue.empty()) {
        link->transmission_queue.pop_front();
    }

    // 2. Fate Calculation (Physics)
    uint32_t safe_mtu = (link->mtu_bytes > 0) ? link->mtu_bytes : 1500;
    int packets_in_batch = (transmitted_bytes + safe_mtu - 1) / safe_mtu;
    int lost_packets = 0;

    for (int i = 0; i < packets_in_batch; ++i) {
        if (nl_check_loss(link, seed)) lost_packets++;
    }

    link->stats.bytes_transmitted += transmitted_bytes;

    // 3. Handle Outcome
    if (lost_packets == 0) {
        /* ---- SUCCESS ---- */
        double arrival_at_dest = timestamp + link->base_latency_mean;
        uint64_t next_seed_val = next_seed(seed);
        
        request->network_finish_time = arrival_at_dest;
        request->seed = next_seed_val;

        // Forward to Next Component
        Event* next = request->create_next_event(arrival_at_dest, next_seed_val);
        if (next) scheduler.schedule(next);

        scheduler.schedule(new NetworkLogEvent(arrival_at_dest, link, request, false, "Delivered"));
    }
    else {
        /* ---- FAILURE ---- */
        if (request->protocol == Protocol::UDP) {
            // UDP: Mark corrupted, but forward anyway
            link->stats.packets_dropped++;
            request->packets_lost += lost_packets; // Assuming Request has this field
            
            double arrival_at_dest = timestamp + link->base_latency_mean;
            uint64_t next_seed_val = next_seed(seed);

            Event* next = request->create_next_event(arrival_at_dest, next_seed_val);
            if (next) scheduler.schedule(next);

            scheduler.schedule(new NetworkLogEvent(arrival_at_dest, link, request, true, "UDP Corrupted"));
        }
        else {
            // TCP: Schedule Retry (Feedback Loop)
            link->stats.tcp_retransmits += lost_packets;

            // Exponential Backoff: RTO * 2^attempts
            double backoff = link->tcp_rto_ms * pow(2.0, min(attempt_count, 6));
            double retry_time = timestamp + backoff; 
            
            uint32_t retry_size = lost_packets * safe_mtu;

            // Schedule the Retry as a NEW Arrival
            // It will re-enter the queue at 'retry_time'
            scheduler.schedule(new NetworkLinkArrivalEvent(
                retry_time,
                seed, 
                link,
                request,
                retry_size,       // Smaller payload
                attempt_count + 1 // Increment attempt
            ));

            scheduler.schedule(new NetworkLogEvent(timestamp, link, request, true, "TCP Retry Scheduled"));
        }
    }

    // 4. Pump the Queue (Start the next waiting job)
    // The wire is now physically free (Departure done).
    start_next_job(link, scheduler, timestamp, seed);
}

void NetworkLogEvent::execute(EventScheduler& scheduler) {
    cout << "[Network] Time: " << timestamp 
         << " | " << link->from << "->" << link->to
         << " | ReqID: " << request->id
         << " | " << msg << endl;
}
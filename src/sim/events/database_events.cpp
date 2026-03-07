#include "database_events.h"

#include "database.h"
#include "scheduler.h"
#include "request.h"
#include "rng.h"

#include <algorithm>

/* ================= Utilities ================= */

void db_update_tokens(Database* db, double now) {
    if (now <= db->last_token_update_ts)
        return;

    double delta = now - db->last_token_update_ts;
    uint32_t new_tokens =
        static_cast<uint32_t>(delta * db->max_iops);

    if (new_tokens == 0)
        return;

    db->tokens = std::min(
        db->bucket_capacity,
        db->tokens + new_tokens
    );

    db->last_token_update_ts +=
        static_cast<double>(new_tokens) / db->max_iops;
}

double db_sample_seek_latency(
    Database* db,
    uint64_t& seed
) {
    double latency = 0.0;

    if (db->seek_model == "normal") {
        latency = normal_dist(
            db->base_median_seek_ms,
            db->base_variance_seek_ms,
            seed
        );
    }

    if (latency < 0.0)
        latency = 0.0;

    seed = next_seed(seed);
    return latency;
}

void db_reject_request(
    Database* db,
    Request* req,
    EventScheduler& scheduler
) {
    db->rejected_requests++;
    req->status = RequestStatus::REJECTED;
    req->finish_time = scheduler.now();
}

void db_try_dispatch(
    Database* db,
    EventScheduler& scheduler,
    double now,
    uint64_t& seed
) {
    if (db->queue.empty())
        return;

    Request* req = db->queue.front();
    int required_tokens = req->is_write ? 2 : 1;

    if (db->active_requests >= db->max_concurrency ||
        db->tokens < required_tokens)
        return;

    db->queue.pop();
    db->tokens -= required_tokens;
    db->active_requests++;

    double latency = db_sample_seek_latency(db, seed);

    scheduler.schedule(
        scheduler.entityFactory().createEvent(
            EventType::DB_REQUEST_SEND,
            now + latency,
            seed,
            req
        )
    );
}

/* ================= Constructors ================= */

DBRequestArrivalEvent::DBRequestArrivalEvent(
    double ts,
    uint64_t seed_,
    Database* db_,
    Request* req_
) {
    type = EventType::DB_REQUEST_ARRIVAL;
    timestamp = ts;
    seed = seed_;
    db = db_;
    request = req_;
}

DBRequestSendEvent::DBRequestSendEvent(
    double ts,
    uint64_t seed_,
    Database* db_,
    Request* req_
) {
    type = EventType::DB_REQUEST_SEND;
    timestamp = ts;
    seed = seed_;
    db = db_;
    request = req_;
}

/* ================= Event execution ================= */

void DBRequestArrivalEvent::execute(EventScheduler& scheduler) {
    db_update_tokens(db, timestamp);

    int required_tokens = request->is_write ? 2 : 1;

    if (db->active_requests < db->max_concurrency &&
        db->tokens >= required_tokens) {

        db->tokens -= required_tokens;
        db->active_requests++;

        double latency =
            db_sample_seek_latency(db, seed);

        scheduler.schedule(
            scheduler.entityFactory().createEvent(
                EventType::DB_REQUEST_SEND,
                timestamp + latency,
                seed,
                request
            )
        );
        return;
    }

    if (db->queue.size() < db->queue_capacity) {
        db->queue.push(request);
        return;
    }

    db_reject_request(db, request, scheduler);
}

//this event is sending req from QUEUE to
void DBRequestSendEvent::execute(EventScheduler& scheduler) {
    db->active_requests--;

    db_update_tokens(db, timestamp);

    db_try_dispatch(
        db,
        scheduler,
        timestamp,
        seed
    );

    request->db_finish_time = scheduler.now();  //fix this db_finish_time updating 
    request->seed = seed;

    if (Event* next =
        request->create_next_event(
            scheduler.now(),
            seed
        )) {
        scheduler.schedule(next);
    }
}

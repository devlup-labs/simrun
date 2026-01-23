#pragma once

#include <cstdint>
#include <queue>
#include <string>

#include "base_entity.h"

// Forward declaration
struct Request;

class Database : public BaseEntity {
public:
    explicit Database(uint32_t id);

    /* ---------- Configuration ---------- */
    std::string seek_model;
    double base_median_seek_ms;
    double base_variance_seek_ms;

    uint32_t max_iops;
    uint32_t bucket_capacity;

    uint32_t max_concurrency;
    uint32_t queue_capacity;

    /* ---------- Runtime state ---------- */
    uint32_t active_requests = 0;
    uint32_t tokens = 0;
    double last_token_update_ts = 0.0;

    std::queue<Request*> queue;

    /* ---------- Stats ---------- */
    uint64_t rejected_requests = 0;
};

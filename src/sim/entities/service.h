#pragma once

#include <cstdint>
#include <queue>
#include <string>
#include <nlohmann/json.hpp>

#include "base_entity.h"

// forward declaration
class Request;

class Service : public BaseEntity {
public:
    explicit Service(uint32_t id, const nlohmann::json& params);

private:
    /* ---------- latency config ---------- */
    std::string latency_dist;
    double base_median_latency;
    double base_variance_latency;

    /* ---------- capacity ---------- */
    uint32_t max_concurrency;
    uint32_t queue_capacity;

    /* ---------- runtime state ---------- */
    uint32_t active;
    std::queue<Request*> queue;
};

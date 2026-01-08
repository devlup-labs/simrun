#pragma once
#include "../core/base_entity.h"

class DatabaseEntity final : public BaseEntity {
public:
    // ---- context ----
    const int capacity;
    const double latency_mean;
    const double failure_prob;

    // ---- state ----
    bool is_down = false;
    int active_connections = 0;

    DatabaseEntity(
        std::string id,
        int capacity,
        double latency_mean,
        double failure_prob
    )
        : BaseEntity(std::move(id)),
          capacity(capacity),
          latency_mean(latency_mean),
          failure_prob(failure_prob) {}
};

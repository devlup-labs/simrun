#pragma once
#include "../core/base_entity.h"

class ServiceEntity final : public BaseEntity {
public:
    // ---- context (immutable) ----
    const int capacity;
    const double latency_mean;
    const double failure_prob;

    // ---- state (mutable) ----
    bool is_down = false;
    int active_requests = 0;
    int queued_requests = 0;

    ServiceEntity(
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

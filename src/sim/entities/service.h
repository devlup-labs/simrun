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
        int capacity_,
        double latency_mean_,
        double failure_prob_
    )
        : BaseEntity(std::move(id)),
          capacity(capacity_),
          latency_mean(latency_mean_),
          failure_prob(failure_prob_) {}
};

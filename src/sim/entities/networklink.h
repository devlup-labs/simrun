#pragma once
#include "../core/base_entity.h"
#include <string>

class NetworkLinkEntity final : public BaseEntity {
public:
    // ---- context ----
    const std::string from;
    const std::string to;
    const double latency_mean;
    const double failure_prob;

    // ---- state ----
    bool is_down = false;
    int in_flight = 0;

    NetworkLinkEntity(
        std::string id,
        std::string from_,
        std::string to_,
        double latency_mean_,
        double failure_prob_
    )
        : BaseEntity(std::move(id)),
          from(std::move(from_)),
          to(std::move(to_)),
          latency_mean(latency_mean_),
          failure_prob(failure_prob_) {}
};

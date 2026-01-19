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
        std::string from,
        std::string to,
        double latency_mean,
        double failure_prob
    )
        : BaseEntity(std::move(id)),
          from(std::move(from)),
          to(std::move(to)),
          latency_mean(latency_mean),
          failure_prob(failure_prob) {}
};

#pragma once
#include "../core/base_entity.h"
#include <string>

class NetworkLinkEntity final : public BaseEntity {
public:
    // ---- context ----
    const std::string from;
    const std::string to;
    const double base_latency_mean;
    const double base_failure_prob;
    const double bandwidth;

    // ---- state ----
    bool is_down = false;
    int in_flight = 0;
    double latency_mean = base_latency_mean;
    double failure_prob = base_failure_prob;

    NetworkLinkEntity(
        std::string id,
        std::string from,
        std::string to,
        double base_latency_mean,
        double base_failure_prob
    )
        : BaseEntity(std::move(id)),
          from(std::move(from)),
          to(std::move(to)),
          base_latency_mean(base_latency_mean),
          base_failure_prob(base_failure_prob) {}
};

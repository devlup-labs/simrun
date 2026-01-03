#pragma once
#include "../core/base_entity.h"

class DatabaseEntity final : public BaseEntity {
public:
    // ---- context ----
    const int max_connections;
    const double latency_mean;
    const double failure_prob;

    // ---- state ----
    bool is_down = false;
    int active_connections = 0;

    DatabaseEntity(
        std::string id,
        int max_conn,
        double latency_mean_,
        double failure_prob_
    )
        : BaseEntity(std::move(id)),
          max_connections(max_conn),
          latency_mean(latency_mean_),
          failure_prob(failure_prob_) {}
};

#include "service.h"

Service::Service(uint32_t id, const nlohmann::json& params)
    : BaseEntity(id)
{
    /* -------- Latency params -------- */
    latency_dist = params.at("dist_latency").get<std::string>();
    base_median_latency = params.at("base_median_latency").get<double>();
    base_variance_latency = params.at("base_variance_latency").get<double>();

    /* -------- Capacity params -------- */
    max_concurrency = params.at("max_concurrency").get<uint32_t>();
    queue_capacity = params.at("queue_capacity").get<uint32_t>();

    /* -------- Initial state -------- */
    active = params.value("active", 0);
}

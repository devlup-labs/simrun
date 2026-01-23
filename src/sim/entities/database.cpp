#include "database.h"
#include <nlohmann/json.hpp>

Database::Database(uint32_t id, const nlohmann::json& params)
    : BaseEntity(id)
{
    /* ---------- Seek latency config ---------- */
    seek_model = params.at("seek_model").get<std::string>();
    base_median_seek_ms =
        params.at("base_median_seek_ms").get<double>();
    base_variance_seek_ms =
        params.at("base_variance_seek_ms").get<double>();

    /* ---------- Token bucket config ---------- */
    max_iops =
        params.at("max_iops").get<uint32_t>();
    bucket_capacity =
        params.at("bucket_capacity").get<uint32_t>();

    /* ---------- Concurrency / queue config ---------- */
    max_concurrency =
        params.at("max_concurrency").get<uint32_t>();
    queue_capacity =
        params.at("queue_capacity").get<uint32_t>();

    /* ---------- Runtime state ---------- */
    active_requests = params.value("active", 0u);
    tokens = params.value("initial_tokens", bucket_capacity);
    last_token_update_ts = 0.0;

    rejected_requests = 0;
}

#include "cache.h"

Cache::Cache(uint32_t id, const nlohmann::json& params)
    : BaseEntity(id)
{
    /* -------- Cache config -------- */
    base_cache_hit_probability =
        params.at("base_cache_hit_probability").get<double>();

    base_cache_hit_latency =
        params.at("base_cache_hit_latency").get<double>();

    base_cache_miss_latency =
        params.at("base_cache_miss_latency").get<double>();

    /* -------- Initial runtime state -------- */
    hits = 0;
    misses = 0;
}

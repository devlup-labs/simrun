#pragma once

#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

#include "base_entity.h"

class Cache : public BaseEntity {
public:
    explicit Cache(uint32_t id, const nlohmann::json& params);

private:
    /* ---------- cache config ---------- */
    double base_cache_hit_probability;
    double base_cache_hit_latency;
    double base_cache_miss_latency;

    /* ---------- runtime state (optional, extensible) ---------- */
    uint64_t hits;
    uint64_t misses;
};

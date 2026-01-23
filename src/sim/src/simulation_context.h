#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>

#include "../entities/component.h"
#include "../network/network_link.h"
#include "../request/request.h"

class SimulationContext {
public:
    SimulationContext(uint64_t seed_, const std::string& time_unit_)
        : seed(seed_), time_unit(time_unit_) {}

    /* ---------- Global State ---------- */
    uint64_t seed;
    std::string time_unit;

    /* ---------- World ---------- */
    std::unordered_map<uint32_t, std::unique_ptr<Component>> components;
    std::unordered_map<uint32_t, std::unique_ptr<NetworkLink>> links;

    /* ---------- Requests ---------- */
    std::unordered_map<std::string, json> request_types;
};

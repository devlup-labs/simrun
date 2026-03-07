#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/* ---------- Enums ---------- */
enum class ComponentType {
    SERVICE,
    DATABASE,
    CACHE,
    UNKNOWN
};

enum class EventIRType {
    REQUEST_ARRIVAL,
    TIMER,
    UNKNOWN
};

/* ---------- Header ---------- */
struct IRHeader {
    std::string ir_version;
    std::string engine_version;
    uint64_t seed;
    std::string time_unit;
};

/* ---------- Components ---------- */
struct IRComponent {
    uint32_t id;
    ComponentType type;
    json config;
};

/* ---------- Links ---------- */
struct IRLink {
    uint32_t id;
    uint32_t from;
    uint32_t to;
    json config;
};

/* ---------- Component Context ---------- */
struct IRComponentContext {
    uint32_t id;
    bool active;
    uint32_t queue;
};

/* ---------- Link Context ---------- */
struct IRLinkContext {
    uint32_t id;
    double current_bandwidth_mbps;
    uint32_t from;
    uint32_t to;
};

/* ---------- Request Types ---------- */
struct IRRequestType {
    std::string id;
    json payload;
    json routes;
};

/* ---------- Initial Events ---------- */
struct IREvent {
    std::string id;
    double time;
    EventIRType type;
    json payload;
};

/* ---------- Root IR ---------- */
struct IR {
    IRHeader header;

    std::vector<IRComponent> components;
    std::vector<IRLink> links;

    std::vector<IRComponentContext> components_context;
    std::vector<IRLinkContext> links_context;

    std::vector<IRRequestType> request_types;
    std::vector<IREvent> initial_events;
};

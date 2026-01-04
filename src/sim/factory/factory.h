#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Entities
#include "../entities/service.h"
#include "../entities/database.h"
#include "../entities/network_link.h"

// IR (will be changed after UI -> frontend is finalised)

enum class IRType {
    SERVICE,
    DATABASE,
    NETWORK_LINK
};

struct IRNode {
    std::string id;
    IRType type;

    // For network links
    std::string from;
    std::string to;

    // Common parameters
    int capacity;
    double latency_mean;
    double failure_prob;
};

// ------------- Simulation Registry -------------

struct Simulation {
    std::unordered_map<std::string, std::unique_ptr<BaseEntity>> entities;
};

// Factory
class EntityFactory {
public:
    void build(
        const std::vector<IRNode>& ir,
        Simulation& simulation
    );

private:
    void create_entities(
        const std::vector<IRNode>& ir,
        Simulation& simulation
    );
};

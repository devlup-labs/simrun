#pragma once
#include <vector>
#include <string>
#include <unordered_map>

class Entity;

struct Context {
    std::unordered_map<std::string, Entity*> entities;
};

struct State {
    // intentionally empty for now
};

enum class IRType {
    SERVICE,
    DATABASE,
    NETWORK_LINK
};

struct IRNode {
    std::string id;
    IRType type;

    std::string from;
    std::string to;

    int capacity;
    double latency_mean;
    double failure_prob;
};

class EntityFactory {
public:
    void build(
        const std::vector<IRNode>& ir,
        Context& context,
        State& state
    );

private:
    void create_entities(
        const std::vector<IRNode>& ir,
        Context& context
    );

    void wire_links(
        const std::vector<IRNode>& ir,
        Context& context
    );
};

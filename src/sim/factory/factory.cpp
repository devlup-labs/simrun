#include "entity_factory.h"
#include <stdexcept>

using std::make_unique;
using std::runtime_error;
using std::vector;

// ---------------- Public ----------------

void EntityFactory::build(
    const vector<IRNode>& ir,
    Simulation& simulation
) {
    create_entities(ir, simulation);
}

// ---------------- Private ----------------

void EntityFactory::create_entities(
    const vector<IRNode>& ir,
    Simulation& simulation
) {
    for (const auto& node : ir) {

        switch (node.type) {

        case IRType::SERVICE:
            simulation.entities[node.id] =
                make_unique<ServiceEntity>(
                    node.id,
                    node.capacity,
                    node.latency_mean,
                    node.failure_prob
                );
            break;

        case IRType::DATABASE:
            simulation.entities[node.id] =
                make_unique<DatabaseEntity>(
                    node.id,
                    node.capacity,
                    node.latency_mean,
                    node.failure_prob
                );
            break;

        case IRType::NETWORK_LINK:
            simulation.entities[node.id] =
                make_unique<NetworkLinkEntity>(
                    node.id,
                    node.from,
                    node.to,
                    node.latency_mean,
                    node.failure_prob
                );
            break;

        default:
            throw runtime_error("Unknown IRType");
        }
    }
}

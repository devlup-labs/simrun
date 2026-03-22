#include "state_factory.h"

#include <stdexcept>

namespace sim::factory {

SimulationState build_initial_state(const ir::SimulationIR& ir, const SimulationContext& ctx) {
    SimulationState state;
    state.components.resize(ctx.component_models.size());
    state.links.resize(ctx.link_models.size());
    state.workload_state.resize(ctx.workloads.size());

    for (std::size_t i = 0; i < state.links.size(); ++i) {
        state.links[i].effective_bandwidth_mbps = ctx.link_models[i].base_bandwidth_mbps;
    }

    for (const auto& component_state : ir.runtime_state.components) {
        const auto it = ctx.component_index_by_external_id.find(component_state.id);
        if (it == ctx.component_index_by_external_id.end()) {
            throw std::runtime_error(
                "runtime_state.component references unknown component id: " +
                std::to_string(component_state.id)
            );
        }
        const int idx = it->second;
        if (component_state.active.has_value()) {
            state.components[idx].active = *component_state.active;
        }
        if (component_state.queue_size.has_value()) {
            state.components[idx].queue_size = *component_state.queue_size;
        }
    }

    for (const auto& link_state : ir.runtime_state.links) {
        const auto it = ctx.link_index_by_external_id.find(link_state.id);
        if (it == ctx.link_index_by_external_id.end()) {
            throw std::runtime_error(
                "runtime_state.link references unknown link id: " + std::to_string(link_state.id)
            );
        }
        const int idx = it->second;
        if (link_state.current_bandwidth_mbps.has_value()) {
            state.links[idx].effective_bandwidth_mbps = *link_state.current_bandwidth_mbps;
            const double base = ctx.link_models[idx].base_bandwidth_mbps;
            if (base > 0.0) {
                state.links[idx].bandwidth_multiplier = *link_state.current_bandwidth_mbps / base;
            } else {
                state.links[idx].bandwidth_multiplier = 1.0;
            }
        }
    }

    return state;
}

}  // namespace sim::factory

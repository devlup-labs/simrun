#include "behavior_executor.h"

#include <stdexcept>

namespace sim::behavior {

void run_behavior_sequence(Simulator& sim, Request& req, int component_idx) {
    const auto& context = sim.context();

    if (component_idx < 0 || component_idx >= static_cast<int>(context.behavior.size())) {
        throw std::runtime_error("Behavior lookup: component index out of range");
    }
    if (
        req.request_type_idx < 0 ||
        req.request_type_idx >= static_cast<int>(context.request_types.size())
    ) {
        throw std::runtime_error("Behavior lookup: request type index out of range");
    }

    const auto& by_flag = context.behavior[component_idx][req.request_type_idx];
    if (req.flag < 0 || req.flag >= static_cast<int>(by_flag.size())) {
        return;
    }
    const auto& sequence = by_flag[req.flag];

    for (const auto& op : sequence) {
        switch (op.opcode) {
            case BehaviorOpcode::CHANGE_FLAG: {
                req.flag = sim.sample_equal(op.values);
                break;
            }
            case BehaviorOpcode::SEND_TO_NEXT_LINK: {
                req.current_hop += 1;

                const auto& routes_by_flag = context.request_types[req.request_type_idx].routing_by_flag;
                if (req.flag < 0 || req.flag >= static_cast<int>(routes_by_flag.size())) {
                    throw std::runtime_error("Routing lookup: flag out of range");
                }
                const auto& route = routes_by_flag[req.flag];
                if (req.current_hop < 0 || req.current_hop >= static_cast<int>(route.size())) {
                    return;
                }

                const int link_idx = route[req.current_hop];
                sim.schedule(Event::make_arrive_at_link(
                    sim.now(),
                    sim.next_seq(),
                    req.id,
                    link_idx
                ));
                break;
            }
        }
    }
}

}  // namespace sim::behavior

#include "simrun/core/sim_context.h"

#include <stdexcept>

#include "simrun/ir/json_ir_parser.h"
#include "simrun/ir/ir_validation.h"

namespace simrun {

SimContext BuildSimContext(const SimulationIR& ir) {
    const ValidationResult validation = ValidateIR(ir);
    if (!validation.ok) {
        throw std::invalid_argument("SimulationIR failed validation");
    }

    SimContext context{};
    context.sim_time = 0;
    context.event_heap.Reset();
    context.request_table.Reset(ir.global_parameters.max_requests, ir.global_parameters.payload_words);
    context.component_state.Initialize(ir.components);
    context.link_state.Initialize(ir.links);
    context.token_table.Reset(ir.global_parameters.max_tokens);
    context.behavior_tables = BuildBehaviorTables(ir);
    context.rng_state.seed(ir.global_parameters.seed);
    context.stats.Initialize(
        static_cast<std::int32_t>(ir.components.size()),
        64,
        ir.global_parameters.time_unit);
    context.structural_ir = ir;
    context.horizon = ir.global_parameters.horizon;
    return context;
}

SimContext BuildSimContextFromJsonFile(const std::string& ir_json_path) {
    return BuildSimContext(LoadSimulationIRFromJsonFile(ir_json_path));
}

SimTime SampleDelay(std::mt19937_64& rng, const SimTime base_delay) {
    if (base_delay <= 0) {
        return 0;
    }
    if (base_delay == 1) {
        return 1;
    }
    std::uniform_int_distribution<SimTime> distribution(1, base_delay);
    return distribution(rng);
}

}  // namespace simrun

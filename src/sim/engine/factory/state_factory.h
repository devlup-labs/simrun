#pragma once

#include "../core/simulator.h"
#include "../ir/ir_schema.h"

namespace sim::factory {

SimulationState build_initial_state(const ir::SimulationIR& ir, const SimulationContext& ctx);

}  // namespace sim::factory

#pragma once

#include "../core/simulator.h"
#include "../ir/ir_schema.h"

namespace sim::factory {

SimulationContext build_context(const ir::SimulationIR& ir);

}  // namespace sim::factory

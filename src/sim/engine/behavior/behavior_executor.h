#pragma once

#include "../core/simulator.h"

namespace sim::behavior {

void run_behavior_sequence(Simulator& sim, Request& req, int component_idx);

}  // namespace sim::behavior

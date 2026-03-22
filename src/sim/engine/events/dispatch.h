#pragma once

#include "../core/event.h"
#include "../core/simulator.h"

namespace sim::events {

void dispatch(const Event& event, Simulator& sim);

}  // namespace sim::events

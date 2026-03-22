#pragma once

#include <memory>

#include "../core/event.h"
#include "../core/simulator.h"
#include "../ir/ir_schema.h"

namespace sim::factory {

std::unique_ptr<Event> create_event_from_spec(
    const ir::IR_EventSpec& spec,
    std::uint64_t seq,
    const SimulationContext& ctx
);

std::unique_ptr<Event> create_event_from_spec(
    const CompiledBootstrapEventSpec& spec,
    std::uint64_t seq
);

}  // namespace sim::factory

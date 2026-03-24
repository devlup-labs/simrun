#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "simrun/behavior/opcodes.h"
#include "simrun/core/enums.h"
#include "simrun/execution/execution_context.h"
#include "simrun/ir/ir_schema.h"
#include "simrun/util/types.h"

namespace simrun {

struct SimContext;

struct BehaviorTables {
    std::int32_t component_count = 0;
    std::int32_t request_type_count = 0;
    std::int32_t max_flags = 1;
    std::int32_t max_hops = 1;

    std::vector<std::int32_t> behavior_index{};
    std::vector<Instruction> opcode_stream{};
    std::vector<LinkId> routing_lookup{};

    [[nodiscard]] std::size_t BehaviorFlatIndex(ComponentId component_id,
                                                RequestTypeId request_type_id,
                                                Flag flag,
                                                EventPhase phase) const;

    [[nodiscard]] std::size_t RoutingFlatIndex(RequestTypeId request_type_id, Flag flag, std::int16_t hop) const;
};

BehaviorTables BuildBehaviorTables(const SimulationIR& ir);

class BehaviorExecutor {
public:
    void Execute(SimContext& context, ExecutionContext& execution_context) const;
};

}  // namespace simrun

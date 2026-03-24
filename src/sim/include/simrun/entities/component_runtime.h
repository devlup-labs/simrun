#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

#include "simrun/ir/ir_schema.h"
#include "simrun/util/types.h"

namespace simrun {

struct ComponentRuntimeState {
    std::int32_t capacity = 0;
    std::int32_t busy_slots_counter = 0;
    std::int32_t waiting_queue_capacity = 0;
    QueueDiscipline discipline = QueueDiscipline::kFifo;
    std::deque<RequestId> waiting_queue{};
    std::vector<RequestId> slots{};
    std::vector<TokenId> slot_tokens{};
    std::vector<PayloadWord> private_state{};
};

class ComponentRuntime {
public:
    void Initialize(const std::vector<ComponentSpec>& component_specs);

    [[nodiscard]] bool EnqueueWaiting(ComponentId component_id, RequestId request_id);
    [[nodiscard]] bool AdmitIfPossible(ComponentId component_id, RequestId request_id, TokenId completion_token, SlotId* out_slot);
    [[nodiscard]] RequestId ReleaseSlot(ComponentId component_id, SlotId slot_id, TokenId* out_completion_token);

    [[nodiscard]] std::optional<RequestId> PopWaiting(ComponentId component_id);
    [[nodiscard]] bool HasFreeCapacity(ComponentId component_id) const;
    [[nodiscard]] bool FindSlot(ComponentId component_id, RequestId request_id, SlotId* out_slot) const;

    [[nodiscard]] bool Preempt(ComponentId component_id,
                               SlotId slot_id,
                               PayloadWord remaining_demand,
                               RequestId* out_request_id,
                               TokenId* out_completion_token);

    [[nodiscard]] std::int32_t ComponentCount() const noexcept;
    ComponentRuntimeState& At(ComponentId component_id);
    const ComponentRuntimeState& At(ComponentId component_id) const;

private:
    bool IsValidComponent(ComponentId component_id) const noexcept;

    std::vector<ComponentRuntimeState> components_{};
};

}  // namespace simrun

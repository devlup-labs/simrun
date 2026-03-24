#include "simrun/entities/component_runtime.h"

#include <stdexcept>

namespace simrun {

void ComponentRuntime::Initialize(const std::vector<ComponentSpec>& component_specs) {
    components_.clear();
    components_.reserve(component_specs.size());

    for (const ComponentSpec& spec : component_specs) {
        if (spec.capacity <= 0) {
            throw std::invalid_argument("component capacity must be > 0");
        }
        if (spec.state_words < 0) {
            throw std::invalid_argument("component state_words must be >= 0");
        }

        ComponentRuntimeState state{};
        state.capacity = spec.capacity;
        state.busy_slots_counter = 0;
        state.waiting_queue_capacity = spec.waiting_queue_capacity;
        state.discipline = spec.discipline;
        state.slots.assign(static_cast<std::size_t>(spec.capacity), kInvalidRequestId);
        state.slot_tokens.assign(static_cast<std::size_t>(spec.capacity), kInvalidTokenId);
        state.private_state.assign(static_cast<std::size_t>(spec.state_words), 0);
        components_.push_back(std::move(state));
    }
}

bool ComponentRuntime::EnqueueWaiting(const ComponentId component_id, const RequestId request_id) {
    if (!IsValidComponent(component_id)) {
        return false;
    }
    ComponentRuntimeState& component = components_[static_cast<std::size_t>(component_id)];
    if (component.waiting_queue_capacity > 0 &&
        component.waiting_queue.size() >= static_cast<std::size_t>(component.waiting_queue_capacity)) {
        return false;
    }
    component.waiting_queue.push_back(request_id);
    return true;
}

bool ComponentRuntime::AdmitIfPossible(const ComponentId component_id, const RequestId request_id, const TokenId completion_token, SlotId* out_slot) {
    if (!IsValidComponent(component_id)) {
        return false;
    }
    ComponentRuntimeState& component = components_[static_cast<std::size_t>(component_id)];
    if (component.busy_slots_counter >= component.capacity) {
        return false;
    }

    for (std::size_t slot = 0; slot < component.slots.size(); ++slot) {
        if (component.slots[slot] == kInvalidRequestId) {
            component.slots[slot] = request_id;
            component.slot_tokens[slot] = completion_token;
            component.busy_slots_counter += 1;
            if (out_slot != nullptr) {
                *out_slot = static_cast<SlotId>(slot);
            }
            return true;
        }
    }
    return false;
}

RequestId ComponentRuntime::ReleaseSlot(const ComponentId component_id, const SlotId slot_id, TokenId* out_completion_token) {
    if (!IsValidComponent(component_id)) {
        return kInvalidRequestId;
    }
    ComponentRuntimeState& component = components_[static_cast<std::size_t>(component_id)];
    if (slot_id < 0 || static_cast<std::size_t>(slot_id) >= component.slots.size()) {
        return kInvalidRequestId;
    }
    const std::size_t slot_index = static_cast<std::size_t>(slot_id);
    const RequestId request_id = component.slots[slot_index];
    if (request_id == kInvalidRequestId) {
        return kInvalidRequestId;
    }

    if (out_completion_token != nullptr) {
        *out_completion_token = component.slot_tokens[slot_index];
    }
    component.slots[slot_index] = kInvalidRequestId;
    component.slot_tokens[slot_index] = kInvalidTokenId;
    component.busy_slots_counter -= 1;
    return request_id;
}

std::optional<RequestId> ComponentRuntime::PopWaiting(const ComponentId component_id) {
    if (!IsValidComponent(component_id)) {
        return std::nullopt;
    }
    ComponentRuntimeState& component = components_[static_cast<std::size_t>(component_id)];
    if (component.waiting_queue.empty()) {
        return std::nullopt;
    }
    const RequestId request_id = component.waiting_queue.front();
    component.waiting_queue.pop_front();
    return request_id;
}

bool ComponentRuntime::HasFreeCapacity(const ComponentId component_id) const {
    if (!IsValidComponent(component_id)) {
        return false;
    }
    const ComponentRuntimeState& component = components_[static_cast<std::size_t>(component_id)];
    return component.busy_slots_counter < component.capacity;
}

bool ComponentRuntime::FindSlot(const ComponentId component_id, const RequestId request_id, SlotId* out_slot) const {
    if (!IsValidComponent(component_id)) {
        return false;
    }
    const ComponentRuntimeState& component = components_[static_cast<std::size_t>(component_id)];
    for (std::size_t i = 0; i < component.slots.size(); ++i) {
        if (component.slots[i] == request_id) {
            if (out_slot != nullptr) {
                *out_slot = static_cast<SlotId>(i);
            }
            return true;
        }
    }
    return false;
}

bool ComponentRuntime::Preempt(const ComponentId component_id,
                               const SlotId slot_id,
                               const PayloadWord remaining_demand,
                               RequestId* out_request_id,
                               TokenId* out_completion_token) {
    (void)remaining_demand;
    if (!IsValidComponent(component_id)) {
        return false;
    }
    ComponentRuntimeState& component = components_[static_cast<std::size_t>(component_id)];
    if (slot_id < 0 || static_cast<std::size_t>(slot_id) >= component.slots.size()) {
        return false;
    }
    const std::size_t slot_index = static_cast<std::size_t>(slot_id);
    if (component.slots[slot_index] == kInvalidRequestId) {
        return false;
    }

    if (out_request_id != nullptr) {
        *out_request_id = component.slots[slot_index];
    }
    if (out_completion_token != nullptr) {
        *out_completion_token = component.slot_tokens[slot_index];
    }

    component.slots[slot_index] = kInvalidRequestId;
    component.slot_tokens[slot_index] = kInvalidTokenId;
    component.busy_slots_counter -= 1;
    return true;
}

std::int32_t ComponentRuntime::ComponentCount() const noexcept {
    return static_cast<std::int32_t>(components_.size());
}

ComponentRuntimeState& ComponentRuntime::At(const ComponentId component_id) {
    if (!IsValidComponent(component_id)) {
        throw std::out_of_range("component_id out of range");
    }
    return components_[static_cast<std::size_t>(component_id)];
}

const ComponentRuntimeState& ComponentRuntime::At(const ComponentId component_id) const {
    if (!IsValidComponent(component_id)) {
        throw std::out_of_range("component_id out of range");
    }
    return components_[static_cast<std::size_t>(component_id)];
}

bool ComponentRuntime::IsValidComponent(const ComponentId component_id) const noexcept {
    return component_id >= 0 && static_cast<std::size_t>(component_id) < components_.size();
}

}  // namespace simrun

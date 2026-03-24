#pragma once

#include <array>

#include "simrun/core/event.h"
#include "simrun/util/types.h"

namespace simrun {

struct ExecutionContext {
    Event current_event{};
    RequestId current_request_id = kInvalidRequestId;
    ComponentId current_component_id = kInvalidComponentId;
    LinkId current_link_id = kInvalidLinkId;
    std::array<PayloadWord, 8> registers{0, 0, 0, 0, 0, 0, 0, 0};
    bool halted = false;
};

}  // namespace simrun

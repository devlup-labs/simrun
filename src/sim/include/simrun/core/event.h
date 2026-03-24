#pragma once

#include "simrun/core/enums.h"
#include "simrun/util/types.h"

namespace simrun {

struct Event {
    SimTime time = 0;
    EventSeq seq = 0;
    RequestId request_id = kInvalidRequestId;
    TargetType target_type = TargetType::kGlobal;
    std::int32_t target_id = -1;
    EventPhase phase = EventPhase::kTimerExpiration;
    TokenId token_id = kInvalidTokenId;
    Generation expected_generation = 0;
};

inline bool EventLessThan(const Event& lhs, const Event& rhs) noexcept {
    if (lhs.time != rhs.time) {
        return lhs.time < rhs.time;
    }
    return lhs.seq < rhs.seq;
}

}  // namespace simrun

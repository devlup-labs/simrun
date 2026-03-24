#pragma once

#include <cstdint>

namespace simrun {

using SimTime = std::int64_t;
using RequestId = std::int64_t;
using EventSeq = std::int64_t;
using ComponentId = std::int32_t;
using LinkId = std::int32_t;
using RequestTypeId = std::int32_t;
using TokenId = std::int32_t;
using SlotId = std::int16_t;
using Flag = std::int16_t;
using Generation = std::int32_t;
using PayloadWord = std::int64_t;

inline constexpr RequestId kInvalidRequestId = static_cast<RequestId>(-1);
inline constexpr ComponentId kInvalidComponentId = static_cast<ComponentId>(-1);
inline constexpr LinkId kInvalidLinkId = static_cast<LinkId>(-1);
inline constexpr TokenId kInvalidTokenId = static_cast<TokenId>(-1);
inline constexpr SlotId kInvalidSlotId = static_cast<SlotId>(-1);

}  // namespace simrun

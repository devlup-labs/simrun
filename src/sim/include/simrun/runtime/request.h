#pragma once

#include <cstdint>

#include "simrun/core/enums.h"
#include "simrun/util/types.h"

namespace simrun {

struct Request {
    RequestId id = kInvalidRequestId;
    RequestTypeId type_id = 0;
    Flag flag = 0;
    std::int16_t hop = 0;
    Generation generation = 0;
    LocationType location_type = LocationType::kInvalid;
    std::int32_t location_id = -1;
    SlotId slot_id = kInvalidSlotId;
    std::int32_t payload_ptr = -1;
};

}  // namespace simrun

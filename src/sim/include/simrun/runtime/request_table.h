#pragma once

#include <cstddef>
#include <vector>

#include "simrun/runtime/request.h"

namespace simrun {

class RequestTable {
public:
    RequestTable() = default;
    RequestTable(std::int32_t max_requests, std::int32_t payload_words);

    void Reset(std::int32_t max_requests, std::int32_t payload_words);

    RequestId Allocate(RequestTypeId type_id, Flag flag);
    void Release(RequestId request_id);

    bool IsAllocated(RequestId request_id) const;

    Request& At(RequestId request_id);
    const Request& At(RequestId request_id) const;

    PayloadWord* PayloadPtr(RequestId request_id);
    const PayloadWord* PayloadPtr(RequestId request_id) const;

    std::int32_t Capacity() const noexcept;
    std::int32_t PayloadWords() const noexcept;

    void SetLocation(RequestId request_id, LocationType location_type, std::int32_t location_id, SlotId slot_id);
    bool ValidateLocationInvariant(const Request& request) const noexcept;

private:
    bool IsValidId(RequestId request_id) const noexcept;
    std::size_t PayloadOffset(RequestId request_id) const;

    std::vector<Request> requests_{};
    std::vector<std::uint8_t> allocated_{};
    std::vector<RequestId> free_ids_{};
    std::int32_t payload_words_ = 0;
    std::vector<PayloadWord> payload_storage_{};
};

}  // namespace simrun

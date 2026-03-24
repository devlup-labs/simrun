#include "simrun/runtime/request_table.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace simrun {

RequestTable::RequestTable(const std::int32_t max_requests, const std::int32_t payload_words) {
    Reset(max_requests, payload_words);
}

void RequestTable::Reset(const std::int32_t max_requests, const std::int32_t payload_words) {
    if (max_requests <= 0) {
        throw std::invalid_argument("max_requests must be > 0");
    }
    if (payload_words <= 0) {
        throw std::invalid_argument("payload_words must be > 0");
    }

    requests_.assign(static_cast<std::size_t>(max_requests), Request{});
    allocated_.assign(static_cast<std::size_t>(max_requests), 0U);
    free_ids_.clear();
    free_ids_.reserve(static_cast<std::size_t>(max_requests));
    payload_words_ = payload_words;
    payload_storage_.assign(static_cast<std::size_t>(max_requests) * static_cast<std::size_t>(payload_words_), 0);

    for (RequestId request_id = max_requests - 1; request_id >= 0; --request_id) {
        Request& request = requests_[static_cast<std::size_t>(request_id)];
        request.id = request_id;
        request.payload_ptr = static_cast<std::int32_t>(PayloadOffset(request_id));
        free_ids_.push_back(request_id);
        if (request_id == 0) {
            break;
        }
    }
}

RequestId RequestTable::Allocate(const RequestTypeId type_id, const Flag flag) {
    if (free_ids_.empty()) {
        return kInvalidRequestId;
    }

    const RequestId request_id = free_ids_.back();
    free_ids_.pop_back();

    Request& request = requests_[static_cast<std::size_t>(request_id)];
    allocated_[static_cast<std::size_t>(request_id)] = 1U;
    request.type_id = type_id;
    request.flag = flag;
    request.hop = 0;
    request.location_type = LocationType::kInvalid;
    request.location_id = -1;
    request.slot_id = kInvalidSlotId;
    std::fill_n(PayloadPtr(request_id), static_cast<std::size_t>(payload_words_), 0);
    return request_id;
}

void RequestTable::Release(const RequestId request_id) {
    if (!IsValidId(request_id)) {
        throw std::out_of_range("request_id out of range");
    }

    if (allocated_[static_cast<std::size_t>(request_id)] == 0U) {
        return;
    }

    Request& request = requests_[static_cast<std::size_t>(request_id)];
    request.generation += 1;
    request.location_type = LocationType::kInvalid;
    request.location_id = -1;
    request.slot_id = kInvalidSlotId;
    allocated_[static_cast<std::size_t>(request_id)] = 0U;
    std::fill_n(PayloadPtr(request_id), static_cast<std::size_t>(payload_words_), 0);
    free_ids_.push_back(request_id);
}

bool RequestTable::IsAllocated(const RequestId request_id) const {
    return IsValidId(request_id) && allocated_[static_cast<std::size_t>(request_id)] != 0U;
}

Request& RequestTable::At(const RequestId request_id) {
    if (!IsValidId(request_id)) {
        throw std::out_of_range("request_id out of range");
    }
    return requests_[static_cast<std::size_t>(request_id)];
}

const Request& RequestTable::At(const RequestId request_id) const {
    if (!IsValidId(request_id)) {
        throw std::out_of_range("request_id out of range");
    }
    return requests_[static_cast<std::size_t>(request_id)];
}

PayloadWord* RequestTable::PayloadPtr(const RequestId request_id) {
    if (!IsValidId(request_id)) {
        throw std::out_of_range("request_id out of range");
    }
    return payload_storage_.data() + static_cast<std::ptrdiff_t>(PayloadOffset(request_id));
}

const PayloadWord* RequestTable::PayloadPtr(const RequestId request_id) const {
    if (!IsValidId(request_id)) {
        throw std::out_of_range("request_id out of range");
    }
    return payload_storage_.data() + static_cast<std::ptrdiff_t>(PayloadOffset(request_id));
}

std::int32_t RequestTable::Capacity() const noexcept {
    return static_cast<std::int32_t>(requests_.size());
}

std::int32_t RequestTable::PayloadWords() const noexcept {
    return payload_words_;
}

void RequestTable::SetLocation(const RequestId request_id, const LocationType location_type, const std::int32_t location_id, const SlotId slot_id) {
    Request& request = At(request_id);
    request.location_type = location_type;
    request.location_id = location_id;
    request.slot_id = slot_id;
    assert(ValidateLocationInvariant(request));
}

bool RequestTable::ValidateLocationInvariant(const Request& request) const noexcept {
    switch (request.location_type) {
        case LocationType::kInvalid:
            return request.location_id == -1 && request.slot_id == kInvalidSlotId;
        case LocationType::kComponentService:
            return request.location_id >= 0 && request.slot_id >= 0;
        case LocationType::kComponentQueue:
            return request.location_id >= 0 && request.slot_id == kInvalidSlotId;
        case LocationType::kLinkQueue:
            return request.location_id >= 0 && request.slot_id == kInvalidSlotId;
        case LocationType::kExecution:
            return request.slot_id == kInvalidSlotId;
        default:
            return false;
    }
}

bool RequestTable::IsValidId(const RequestId request_id) const noexcept {
    return request_id >= 0 && static_cast<std::size_t>(request_id) < requests_.size();
}

std::size_t RequestTable::PayloadOffset(const RequestId request_id) const {
    return static_cast<std::size_t>(request_id) * static_cast<std::size_t>(payload_words_);
}

}  // namespace simrun

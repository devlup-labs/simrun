#include "simrun/entities/link_runtime.h"

#include <algorithm>
#include <stdexcept>

namespace simrun {

void LinkRuntime::Initialize(const std::vector<LinkSpec>& link_specs) {
    links_.clear();
    links_.reserve(link_specs.size());

    for (const LinkSpec& spec : link_specs) {
        if (spec.bandwidth <= 0) {
            throw std::invalid_argument("link bandwidth must be > 0");
        }
        if (spec.propagation_delay < 0) {
            throw std::invalid_argument("link propagation_delay must be >= 0");
        }

        LinkRuntimeState state{};
        state.bandwidth_state = spec.bandwidth;
        state.next_available_time = 0;
        state.in_flight_count = 0;
        state.src_component = spec.src_component_id;
        state.dst_component = spec.dst_component_id;
        state.propagation_delay = spec.propagation_delay;
        links_.push_back(std::move(state));
    }
}

std::optional<LinkDispatchResult> LinkRuntime::Dispatch(const LinkId link_id, const RequestId request_id, const SimTime now) {
    if (!IsValidLink(link_id)) {
        return std::nullopt;
    }
    LinkRuntimeState& link = links_[static_cast<std::size_t>(link_id)];
    link.transmission_queue.push_back(request_id);
    link.in_flight_count += 1;

    const SimTime dispatch_time = std::max(now, link.next_available_time);
    const SimTime arrival_time = dispatch_time + link.propagation_delay;

    const SimTime serialization_delay = std::max<SimTime>(1, static_cast<SimTime>(link.bandwidth_state));
    link.next_available_time = dispatch_time + serialization_delay;

    return LinkDispatchResult{
        .arrival_time = arrival_time,
        .destination_component = link.dst_component,
    };
}

void LinkRuntime::MarkArrived(const LinkId link_id, const RequestId request_id) {
    if (!IsValidLink(link_id)) {
        return;
    }
    LinkRuntimeState& link = links_[static_cast<std::size_t>(link_id)];
    const auto it = std::find(link.transmission_queue.begin(), link.transmission_queue.end(), request_id);
    if (it != link.transmission_queue.end()) {
        link.transmission_queue.erase(it);
    }
    if (link.in_flight_count > 0) {
        link.in_flight_count -= 1;
    }
}

std::int32_t LinkRuntime::LinkCount() const noexcept {
    return static_cast<std::int32_t>(links_.size());
}

LinkRuntimeState& LinkRuntime::At(const LinkId link_id) {
    if (!IsValidLink(link_id)) {
        throw std::out_of_range("link_id out of range");
    }
    return links_[static_cast<std::size_t>(link_id)];
}

const LinkRuntimeState& LinkRuntime::At(const LinkId link_id) const {
    if (!IsValidLink(link_id)) {
        throw std::out_of_range("link_id out of range");
    }
    return links_[static_cast<std::size_t>(link_id)];
}

bool LinkRuntime::IsValidLink(const LinkId link_id) const noexcept {
    return link_id >= 0 && static_cast<std::size_t>(link_id) < links_.size();
}

}  // namespace simrun

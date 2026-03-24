#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

#include "simrun/ir/ir_schema.h"
#include "simrun/util/types.h"

namespace simrun {

struct LinkRuntimeState {
    std::deque<RequestId> transmission_queue{};
    std::int32_t bandwidth_state = 1;
    SimTime next_available_time = 0;
    std::int32_t in_flight_count = 0;
    ComponentId src_component = 0;
    ComponentId dst_component = 0;
    SimTime propagation_delay = 1;
};

struct LinkDispatchResult {
    SimTime arrival_time = 0;
    ComponentId destination_component = 0;
};

class LinkRuntime {
public:
    void Initialize(const std::vector<LinkSpec>& link_specs);

    [[nodiscard]] std::optional<LinkDispatchResult> Dispatch(LinkId link_id, RequestId request_id, SimTime now);
    void MarkArrived(LinkId link_id, RequestId request_id);

    [[nodiscard]] std::int32_t LinkCount() const noexcept;
    LinkRuntimeState& At(LinkId link_id);
    const LinkRuntimeState& At(LinkId link_id) const;

private:
    bool IsValidLink(LinkId link_id) const noexcept;

    std::vector<LinkRuntimeState> links_{};
};

}  // namespace simrun

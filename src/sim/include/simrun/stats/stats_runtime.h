#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "simrun/core/enums.h"
#include "simrun/util/types.h"

namespace simrun {

struct LogEntry {
    SimTime time = 0;
    RequestId request_id = kInvalidRequestId;
    EventPhase event_phase = EventPhase::kTimerExpiration;
    ComponentId component_id = kInvalidComponentId;
    std::array<PayloadWord, 2> custom_fields{0, 0};
};

struct StatsSnapshot {
    std::vector<std::uint64_t> latency_histogram{};
    std::vector<long double> component_busy_time_integral{};
    std::vector<long double> queue_length_time_integral{};
    std::uint64_t dropped_requests = 0;
};

class StatsRuntime {
public:
    void Initialize(std::int32_t component_count, std::size_t latency_buckets, SimTime latency_bucket_width);

    void OnRequestCreated(RequestId request_id, SimTime at_time);
    void OnRequestCompleted(RequestId request_id, SimTime at_time);
    void OnRequestDropped();

    void IntegrateComponentBusy(ComponentId component_id, std::int32_t busy_slots, SimTime delta_time);
    void IntegrateQueueLength(ComponentId component_id, std::size_t queue_length, SimTime delta_time);

    void AppendDesignLog(LogEntry entry);
    void AppendLog(LogEntry entry);

    [[nodiscard]] const std::vector<LogEntry>& DesignTraceBuffer() const noexcept;
    [[nodiscard]] const std::vector<LogEntry>& TraceBuffer() const noexcept;
    [[nodiscard]] const StatsSnapshot& Snapshot() const noexcept;
    [[nodiscard]] bool DumpDesignTraceCsv(const std::string& output_path) const;

private:
    std::size_t LatencyBucket(SimTime latency) const;

    std::vector<LogEntry> trace_buffer_{};
    StatsSnapshot snapshot_{};
    std::unordered_map<RequestId, SimTime> request_start_time_{};
    SimTime latency_bucket_width_ = 1;
};

}  // namespace simrun

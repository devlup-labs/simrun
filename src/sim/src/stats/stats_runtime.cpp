#include "simrun/stats/stats_runtime.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace simrun {

void StatsRuntime::Initialize(const std::int32_t component_count, const std::size_t latency_buckets, const SimTime latency_bucket_width) {
    if (component_count < 0) {
        throw std::invalid_argument("component_count must be >= 0");
    }
    if (latency_buckets == 0) {
        throw std::invalid_argument("latency_buckets must be > 0");
    }
    if (latency_bucket_width <= 0) {
        throw std::invalid_argument("latency_bucket_width must be > 0");
    }

    trace_buffer_.clear();
    request_start_time_.clear();

    snapshot_.latency_histogram.assign(latency_buckets, 0);
    snapshot_.component_busy_time_integral.assign(static_cast<std::size_t>(component_count), 0.0L);
    snapshot_.queue_length_time_integral.assign(static_cast<std::size_t>(component_count), 0.0L);
    snapshot_.dropped_requests = 0;

    latency_bucket_width_ = latency_bucket_width;
}

void StatsRuntime::OnRequestCreated(const RequestId request_id, const SimTime at_time) {
    request_start_time_[request_id] = at_time;
}

void StatsRuntime::OnRequestCompleted(const RequestId request_id, const SimTime at_time) {
    const auto it = request_start_time_.find(request_id);
    if (it == request_start_time_.end()) {
        return;
    }
    const SimTime latency = std::max<SimTime>(0, at_time - it->second);
    const std::size_t bucket = LatencyBucket(latency);
    snapshot_.latency_histogram[bucket] += 1;
    request_start_time_.erase(it);
}

void StatsRuntime::OnRequestDropped() {
    snapshot_.dropped_requests += 1;
}

void StatsRuntime::IntegrateComponentBusy(const ComponentId component_id, const std::int32_t busy_slots, const SimTime delta_time) {
    if (component_id < 0 || static_cast<std::size_t>(component_id) >= snapshot_.component_busy_time_integral.size()) {
        return;
    }
    snapshot_.component_busy_time_integral[static_cast<std::size_t>(component_id)] +=
        static_cast<long double>(busy_slots) * static_cast<long double>(delta_time);
}

void StatsRuntime::IntegrateQueueLength(const ComponentId component_id, const std::size_t queue_length, const SimTime delta_time) {
    if (component_id < 0 || static_cast<std::size_t>(component_id) >= snapshot_.queue_length_time_integral.size()) {
        return;
    }
    snapshot_.queue_length_time_integral[static_cast<std::size_t>(component_id)] +=
        static_cast<long double>(queue_length) * static_cast<long double>(delta_time);
}

void StatsRuntime::AppendLog(LogEntry entry) {
    AppendDesignLog(std::move(entry));
}

void StatsRuntime::AppendDesignLog(LogEntry entry) {
    trace_buffer_.push_back(std::move(entry));
}

const std::vector<LogEntry>& StatsRuntime::DesignTraceBuffer() const noexcept {
    return trace_buffer_;
}

const std::vector<LogEntry>& StatsRuntime::TraceBuffer() const noexcept {
    return DesignTraceBuffer();
}

const StatsSnapshot& StatsRuntime::Snapshot() const noexcept {
    return snapshot_;
}

bool StatsRuntime::DumpDesignTraceCsv(const std::string& output_path) const {
    std::ofstream out(output_path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "time,request_id,event_phase,component_id,field0,field1\n";
    for (const LogEntry& entry : trace_buffer_) {
        out << entry.time << ','
            << entry.request_id << ','
            << static_cast<std::int32_t>(entry.event_phase) << ','
            << entry.component_id << ','
            << entry.custom_fields[0] << ','
            << entry.custom_fields[1] << '\n';
    }
    return true;
}

std::size_t StatsRuntime::LatencyBucket(const SimTime latency) const {
    const std::size_t bucket =
        static_cast<std::size_t>(latency / latency_bucket_width_);
    if (bucket >= snapshot_.latency_histogram.size()) {
        return snapshot_.latency_histogram.size() - 1;
    }
    return bucket;
}

}  // namespace simrun

#pragma once

#include <cstddef>
#include <string_view>

#include "simrun/core/sim_context.h"

namespace simrun {

enum class RunStatus {
    kCompleted = 0,
    kHorizonReached = 1,
    kPredicateTriggered = 2,
    kStepLimitReached = 3,
};

struct RunResult {
    RunStatus status = RunStatus::kCompleted;
    std::size_t executed_events = 0;
    std::size_t discarded_events = 0;
    SimTime final_time = 0;
};

class Simulator {
public:
    explicit Simulator(SimContext context);

    RunResult Run(std::size_t max_events);
    SimContext& Context() noexcept;
    const SimContext& Context() const noexcept;

private:
    void InjectBootstrapEvents();
    void InjectWorkloadEvents();
    bool ShouldTerminate() const;

    bool ShouldDiscardEvent(const Event& event) const;
    void DispatchEvent(const Event& event);
    void HandleRequestArrival(const Event& event);
    void HandleServiceCompletion(const Event& event);
    void HandleBehaviorPhase(const Event& event);
    void AdmitQueuedIfPossible(ComponentId component_id);
    void FinalizeRun();
    void EmitDesignLog(SimTime time, RequestId request_id, EventPhase phase, ComponentId component_id, PayloadWord field0, PayloadWord field1);
    void DebugLog(std::string_view message) const;

    SimContext context_{};
};

}  // namespace simrun

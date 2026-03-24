#pragma once

#include <functional>
#include <random>
#include <string>

#include "simrun/behavior/behavior_executor.h"
#include "simrun/entities/component_runtime.h"
#include "simrun/entities/link_runtime.h"
#include "simrun/ir/ir_schema.h"
#include "simrun/runtime/request_table.h"
#include "simrun/runtime/token_table.h"
#include "simrun/scheduler/scheduler.h"
#include "simrun/stats/stats_runtime.h"

namespace simrun {

struct SimContext {
    SimTime sim_time = 0;
    Scheduler event_heap{};
    RequestTable request_table{};
    ComponentRuntime component_state{};
    LinkRuntime link_state{};
    TokenTable token_table{};
    BehaviorTables behavior_tables{};
    std::mt19937_64 rng_state{};
    StatsRuntime stats{};

    SimulationIR structural_ir{};
    SimTime horizon = 0;
    std::function<bool(const SimContext&)> termination_predicate{};
    bool design_trace_logging_enabled = true;
    bool debug_console_logging_enabled = true;
    bool dump_design_trace_to_file_on_finish = false;
    std::string design_trace_output_path = "simrun_design_trace.csv";
};

SimContext BuildSimContext(const SimulationIR& ir);
SimContext BuildSimContextFromJsonFile(const std::string& ir_json_path);

SimTime SampleDelay(std::mt19937_64& rng, SimTime base_delay);

}  // namespace simrun

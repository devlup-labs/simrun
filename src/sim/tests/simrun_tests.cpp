#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "simrun/behavior/opcodes.h"
#include "simrun/core/event.h"
#include "simrun/core/sim_context.h"
#include "simrun/core/simulator.h"
#include "simrun/ir/json_ir_parser.h"
#include "simrun/ir/ir_validation.h"

namespace {

simrun::SimulationIR BuildBaseIr() {
    simrun::SimulationIR ir{};
    ir.global_parameters = simrun::GlobalParameters{
        .time_unit = 1,
        .horizon = 100,
        .seed = 42,
        .max_requests = 64,
        .payload_words = 8,
        .max_tokens = 256,
    };

    ir.components = {
        simrun::ComponentSpec{
            .id = 0,
            .capacity = 1,
            .state_words = 4,
            .waiting_queue_capacity = 64,
            .service_time = 1,
            .discipline = simrun::QueueDiscipline::kFifo,
        },
    };

    ir.links = {
        simrun::LinkSpec{
            .id = 0,
            .src_component_id = 0,
            .dst_component_id = 0,
            .propagation_delay = 1,
            .bandwidth = 1,
        },
    };

    ir.request_types = {
        simrun::RequestTypeSpec{
            .id = 0,
            .flag_count = 1,
            .max_hops = 1,
        },
    };

    ir.routing_entries = {
        simrun::RoutingEntry{
            .request_type = 0,
            .flag = 0,
            .hop = 0,
            .link_id = 0,
        },
    };

    ir.opcode_stream = {
        simrun::Instruction{.opcode = simrun::OpCode::kHalt},
    };

    ir.behavior_bindings = {
        simrun::BehaviorBinding{.component_id = 0, .request_type = 0, .flag = 0, .phase = simrun::EventPhase::kRequestArrival, .instruction_pointer = 0},
        simrun::BehaviorBinding{.component_id = 0, .request_type = 0, .flag = 0, .phase = simrun::EventPhase::kServiceCompletion, .instruction_pointer = 0},
        simrun::BehaviorBinding{.component_id = 0, .request_type = 0, .flag = 0, .phase = simrun::EventPhase::kTimerExpiration, .instruction_pointer = 0},
        simrun::BehaviorBinding{.component_id = 0, .request_type = 0, .flag = 0, .phase = simrun::EventPhase::kResponseArrival, .instruction_pointer = 0},
        simrun::BehaviorBinding{.component_id = 0, .request_type = 0, .flag = 0, .phase = simrun::EventPhase::kWorkloadGeneration, .instruction_pointer = 0},
    };

    return ir;
}

std::int32_t AddScript(std::vector<simrun::Instruction>* stream, std::vector<simrun::Instruction> script) {
    if (script.empty() || script.back().opcode != simrun::OpCode::kHalt) {
        script.push_back(simrun::Instruction{.opcode = simrun::OpCode::kHalt});
    }
    const std::int32_t ip = static_cast<std::int32_t>(stream->size());
    stream->insert(stream->end(), script.begin(), script.end());
    return ip;
}

simrun::SimulationIR BuildComplexSystemIr() {
    simrun::SimulationIR ir{};
    ir.global_parameters = simrun::GlobalParameters{
        .time_unit = 1,
        .horizon = 2000,
        .seed = 987654321ULL,
        .max_requests = 512,
        .payload_words = 16,
        .max_tokens = 8192,
    };

    ir.components = {
        simrun::ComponentSpec{.id = 0, .capacity = 2, .state_words = 4, .waiting_queue_capacity = 128, .service_time = 2, .discipline = simrun::QueueDiscipline::kFifo},
        simrun::ComponentSpec{.id = 1, .capacity = 2, .state_words = 4, .waiting_queue_capacity = 128, .service_time = 3, .discipline = simrun::QueueDiscipline::kFifo},
        simrun::ComponentSpec{.id = 2, .capacity = 2, .state_words = 4, .waiting_queue_capacity = 128, .service_time = 1, .discipline = simrun::QueueDiscipline::kFifo},
    };

    ir.links = {
        simrun::LinkSpec{.id = 0, .src_component_id = 0, .dst_component_id = 1, .propagation_delay = 1, .bandwidth = 1},
        simrun::LinkSpec{.id = 1, .src_component_id = 1, .dst_component_id = 2, .propagation_delay = 2, .bandwidth = 1},
        simrun::LinkSpec{.id = 2, .src_component_id = 2, .dst_component_id = 0, .propagation_delay = 1, .bandwidth = 1},
        simrun::LinkSpec{.id = 3, .src_component_id = 1, .dst_component_id = 0, .propagation_delay = 3, .bandwidth = 1},
        simrun::LinkSpec{.id = 4, .src_component_id = 0, .dst_component_id = 2, .propagation_delay = 2, .bandwidth = 1},
    };

    ir.request_types = {
        simrun::RequestTypeSpec{.id = 0, .flag_count = 2, .max_hops = 3},
        simrun::RequestTypeSpec{.id = 1, .flag_count = 2, .max_hops = 2},
    };

    for (simrun::Flag flag = 0; flag < 2; ++flag) {
        const std::array<simrun::LinkId, 3> links_for_type0 =
            (flag == 0) ? std::array<simrun::LinkId, 3>{0, 1, 2} : std::array<simrun::LinkId, 3>{4, 2, 3};
        for (std::int16_t hop = 0; hop < 3; ++hop) {
            ir.routing_entries.push_back(simrun::RoutingEntry{
                .request_type = 0,
                .flag = flag,
                .hop = hop,
                .link_id = links_for_type0[static_cast<std::size_t>(hop)],
            });
        }
    }

    for (simrun::Flag flag = 0; flag < 2; ++flag) {
        const std::array<simrun::LinkId, 2> links_for_type1 =
            (flag == 0) ? std::array<simrun::LinkId, 2>{1, 2} : std::array<simrun::LinkId, 2>{0, 4};
        for (std::int16_t hop = 0; hop < 2; ++hop) {
            ir.routing_entries.push_back(simrun::RoutingEntry{
                .request_type = 1,
                .flag = flag,
                .hop = hop,
                .link_id = links_for_type1[static_cast<std::size_t>(hop)],
            });
        }
    }

    const std::int32_t inert_phase_script = AddScript(&ir.opcode_stream, {
        simrun::Instruction{.opcode = simrun::OpCode::kLog, .args = {9000, 0, 0, 0}},
    });

    std::int32_t service_script[3][2][2] = {};
    for (simrun::ComponentId component_id = 0; component_id < 3; ++component_id) {
        for (simrun::RequestTypeId type_id = 0; type_id < 2; ++type_id) {
            for (simrun::Flag flag = 0; flag < 2; ++flag) {
                std::vector<simrun::Instruction> script{};
                script.push_back(simrun::Instruction{
                    .opcode = simrun::OpCode::kAddComponentStateWord,
                    .args = {static_cast<std::int64_t>(type_id), type_id == 0 ? 1 : 2, 0, 0},
                });
                script.push_back(simrun::Instruction{
                    .opcode = simrun::OpCode::kWritePayloadWord,
                    .args = {2, static_cast<std::int64_t>(10000 + component_id * 100 + type_id * 10 + flag), 0, 0},
                });
                script.push_back(simrun::Instruction{
                    .opcode = simrun::OpCode::kLog,
                    .args = {static_cast<std::int64_t>(2000 + component_id * 100 + type_id * 10 + flag), 0, 0, 0},
                });

                if (component_id == 1 && type_id == 0 && flag == 0) {
                    script.push_back(simrun::Instruction{
                        .opcode = simrun::OpCode::kSpawnRequest,
                        .args = {1, 1, 2, 0},
                    });
                }

                script.push_back(simrun::Instruction{.opcode = simrun::OpCode::kRouteNextLink});
                script.push_back(simrun::Instruction{.opcode = simrun::OpCode::kIncrementHop});
                service_script[component_id][type_id][flag] = AddScript(&ir.opcode_stream, std::move(script));
            }
        }
    }

    for (simrun::ComponentId component_id = 0; component_id < 3; ++component_id) {
        for (simrun::RequestTypeId type_id = 0; type_id < 2; ++type_id) {
            for (simrun::Flag flag = 0; flag < 2; ++flag) {
                ir.behavior_bindings.push_back(simrun::BehaviorBinding{
                    .component_id = component_id,
                    .request_type = type_id,
                    .flag = flag,
                    .phase = simrun::EventPhase::kRequestArrival,
                    .instruction_pointer = inert_phase_script,
                });
                ir.behavior_bindings.push_back(simrun::BehaviorBinding{
                    .component_id = component_id,
                    .request_type = type_id,
                    .flag = flag,
                    .phase = simrun::EventPhase::kServiceCompletion,
                    .instruction_pointer = service_script[component_id][type_id][flag],
                });
                ir.behavior_bindings.push_back(simrun::BehaviorBinding{
                    .component_id = component_id,
                    .request_type = type_id,
                    .flag = flag,
                    .phase = simrun::EventPhase::kTimerExpiration,
                    .instruction_pointer = inert_phase_script,
                });
                ir.behavior_bindings.push_back(simrun::BehaviorBinding{
                    .component_id = component_id,
                    .request_type = type_id,
                    .flag = flag,
                    .phase = simrun::EventPhase::kResponseArrival,
                    .instruction_pointer = inert_phase_script,
                });
                ir.behavior_bindings.push_back(simrun::BehaviorBinding{
                    .component_id = component_id,
                    .request_type = type_id,
                    .flag = flag,
                    .phase = simrun::EventPhase::kWorkloadGeneration,
                    .instruction_pointer = inert_phase_script,
                });
            }
        }
    }

    ir.workloads = {
        simrun::WorkloadDefinition{.request_type = 0, .initial_flag = 0, .entry_component_id = 0, .start_time = 0, .interval = 2, .count = 6},
        simrun::WorkloadDefinition{.request_type = 0, .initial_flag = 1, .entry_component_id = 2, .start_time = 1, .interval = 3, .count = 4},
    };

    ir.bootstrap_events = {
        simrun::BootstrapEventSpec{.time = 0, .request_type = 0, .flag = 1, .component_id = 0, .phase = simrun::EventPhase::kRequestArrival},
        simrun::BootstrapEventSpec{.time = 1, .request_type = 1, .flag = 0, .component_id = 1, .phase = simrun::EventPhase::kRequestArrival},
    };

    return ir;
}

void Assert(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void TestIrValidation() {
    simrun::SimulationIR ir = BuildBaseIr();
    simrun::ValidationResult valid_result = simrun::ValidateIR(ir);
    Assert(valid_result.ok, "expected base IR to validate");

    ir.routing_entries.clear();
    simrun::ValidationResult invalid_result = simrun::ValidateIR(ir);
    Assert(!invalid_result.ok, "expected missing routing entries to fail validation");
}

void TestSchedulerOrdering() {
    simrun::Scheduler scheduler{};
    scheduler.Reset();
    scheduler.Push(simrun::Event{.time = 5});
    scheduler.Push(simrun::Event{.time = 5});
    scheduler.Push(simrun::Event{.time = 3});

    const simrun::Event first = scheduler.Pop();
    const simrun::Event second = scheduler.Pop();
    const simrun::Event third = scheduler.Pop();

    Assert(first.time == 3, "expected earliest time first");
    Assert(second.time == 5 && third.time == 5, "expected later events after earliest");
    Assert(second.seq < third.seq, "expected lexicographic ordering by sequence within same time");
}

void TestInvalidTokenDiscard() {
    simrun::SimulationIR ir = BuildBaseIr();
    simrun::SimContext context = simrun::BuildSimContext(ir);

    const simrun::RequestId request_id = context.request_table.Allocate(0, 0);
    Assert(request_id != simrun::kInvalidRequestId, "expected request allocation");

    const simrun::TokenId token_id = context.token_table.AcquireActiveToken();
    context.event_heap.Push(simrun::Event{
        .time = 1,
        .seq = 0,
        .request_id = request_id,
        .target_type = simrun::TargetType::kComponent,
        .target_id = 0,
        .phase = simrun::EventPhase::kTimerExpiration,
        .token_id = token_id,
        .expected_generation = context.request_table.At(request_id).generation,
    });
    context.token_table.Invalidate(token_id);

    simrun::Simulator simulator(std::move(context));
    const simrun::RunResult result = simulator.Run(100);
    Assert(result.discarded_events == 1, "expected canceled event to be discarded");
}

void TestDeterministicExecution() {
    simrun::SimulationIR ir = BuildBaseIr();
    ir.bootstrap_events = {
        simrun::BootstrapEventSpec{
            .time = 0,
            .request_type = 0,
            .flag = 0,
            .component_id = 0,
            .phase = simrun::EventPhase::kRequestArrival,
        },
    };

    simrun::Simulator sim_a(simrun::BuildSimContext(ir));
    const simrun::RunResult run_a = sim_a.Run(100);

    simrun::Simulator sim_b(simrun::BuildSimContext(ir));
    const simrun::RunResult run_b = sim_b.Run(100);

    Assert(run_a.status == simrun::RunStatus::kCompleted, "expected completed run");
    Assert(run_b.status == simrun::RunStatus::kCompleted, "expected completed run");
    Assert(run_a.executed_events == run_b.executed_events, "expected stable event count");
    Assert(run_a.discarded_events == run_b.discarded_events, "expected stable discard count");
    Assert(run_a.final_time == run_b.final_time, "expected stable final time");
}

void TestDesignAndDebugLogging() {
    simrun::SimulationIR ir = BuildBaseIr();
    ir.bootstrap_events = {
        simrun::BootstrapEventSpec{
            .time = 0,
            .request_type = 0,
            .flag = 0,
            .component_id = 0,
            .phase = simrun::EventPhase::kRequestArrival,
        },
    };

    simrun::SimContext context = simrun::BuildSimContext(ir);
    context.design_trace_logging_enabled = true;
    context.debug_console_logging_enabled = true;

    std::ostringstream captured_stdout{};
    std::streambuf* old_buf = std::cout.rdbuf(captured_stdout.rdbuf());

    simrun::Simulator simulator(std::move(context));
    const simrun::RunResult run = simulator.Run(100);

    std::cout.rdbuf(old_buf);

    Assert(run.status == simrun::RunStatus::kCompleted, "expected completed run for logging test");
    Assert(!simulator.Context().stats.DesignTraceBuffer().empty(), "expected design trace logs");
    Assert(captured_stdout.str().find("[simrun]") != std::string::npos, "expected debug console logs");
}

void TestDesignLogDumpOnFinish() {
    simrun::SimulationIR ir = BuildBaseIr();
    ir.bootstrap_events = {
        simrun::BootstrapEventSpec{
            .time = 0,
            .request_type = 0,
            .flag = 0,
            .component_id = 0,
            .phase = simrun::EventPhase::kRequestArrival,
        },
    };

    const std::filesystem::path output_path = std::filesystem::path("build") / "design_trace_test.csv";
    std::error_code remove_error{};
    std::filesystem::remove(output_path, remove_error);

    simrun::SimContext context = simrun::BuildSimContext(ir);
    context.design_trace_logging_enabled = true;
    context.debug_console_logging_enabled = true;
    context.dump_design_trace_to_file_on_finish = true;
    context.design_trace_output_path = output_path.string();

    simrun::Simulator simulator(std::move(context));
    const simrun::RunResult run = simulator.Run(100);
    Assert(run.status == simrun::RunStatus::kCompleted, "expected completed run for dump test");

    Assert(std::filesystem::exists(output_path), "expected design trace file to be created");
    std::ifstream in(output_path);
    std::string first_line{};
    std::getline(in, first_line);
    Assert(first_line == "time,request_id,event_phase,component_id,field0,field1", "expected csv header");
    std::string second_line{};
    std::getline(in, second_line);
    Assert(!second_line.empty(), "expected at least one log row in dumped file");
}

void TestComplexSystemDesignIr() {
    simrun::SimulationIR ir = BuildComplexSystemIr();
    const simrun::ValidationResult validation = simrun::ValidateIR(ir);
    Assert(validation.ok, "expected complex IR to validate");

    const std::filesystem::path output_path = std::filesystem::path("build") / "complex_design_trace.csv";
    std::error_code remove_error{};
    std::filesystem::remove(output_path, remove_error);

    simrun::SimContext context = simrun::BuildSimContext(ir);
    context.design_trace_logging_enabled = true;
    context.debug_console_logging_enabled = true;
    context.dump_design_trace_to_file_on_finish = true;
    context.design_trace_output_path = output_path.string();

    simrun::Simulator simulator(std::move(context));
    const simrun::RunResult run = simulator.Run(50000);

    Assert(run.status == simrun::RunStatus::kCompleted, "expected complex scenario to complete");
    Assert(run.executed_events > 80, "expected many executed events for complex scenario");
    Assert(run.final_time > 0, "expected non-zero final time in complex scenario");

    const auto& snapshot = simulator.Context().stats.Snapshot();
    Assert(snapshot.dropped_requests == 0, "expected no dropped requests in complex scenario");

    const auto& trace = simulator.Context().stats.DesignTraceBuffer();
    Assert(trace.size() > 120, "expected rich design trace for complex scenario");

    bool saw_opcode_log_marker = false;
    for (const simrun::LogEntry& entry : trace) {
        if (entry.custom_fields[0] >= 2000 && entry.custom_fields[0] < 3000) {
            saw_opcode_log_marker = true;
            break;
        }
    }
    Assert(saw_opcode_log_marker, "expected opcode-driven design log markers in trace");

    const simrun::ComponentRuntimeState& c0 = simulator.Context().component_state.At(0);
    const simrun::ComponentRuntimeState& c1 = simulator.Context().component_state.At(1);
    const simrun::ComponentRuntimeState& c2 = simulator.Context().component_state.At(2);
    Assert(!c0.private_state.empty() && c0.private_state[0] > 0, "expected component 0 state mutations");
    Assert(!c1.private_state.empty() && c1.private_state[0] > 0, "expected component 1 state mutations");
    Assert(!c2.private_state.empty() && c2.private_state[0] > 0, "expected component 2 state mutations");

    Assert(std::filesystem::exists(output_path), "expected complex trace csv output");
    std::ifstream in(output_path);
    std::string header{};
    std::getline(in, header);
    Assert(header == "time,request_id,event_phase,component_id,field0,field1", "expected complex csv header");
    std::string first_row{};
    std::getline(in, first_row);
    Assert(!first_row.empty(), "expected complex csv to have data rows");
}

struct ScenarioSummary {
    simrun::RunResult run{};
    std::size_t trace_size = 0;
    std::vector<simrun::PayloadWord> component_0_state{};
    std::vector<simrun::PayloadWord> component_1_state{};
    std::vector<simrun::PayloadWord> component_2_state{};
};

ScenarioSummary RunScenario(const simrun::SimulationIR& ir, const bool enable_debug_logs) {
    simrun::SimContext context = simrun::BuildSimContext(ir);
    context.design_trace_logging_enabled = true;
    context.debug_console_logging_enabled = enable_debug_logs;

    simrun::Simulator simulator(std::move(context));
    const simrun::RunResult run = simulator.Run(50000);

    return ScenarioSummary{
        .run = run,
        .trace_size = simulator.Context().stats.DesignTraceBuffer().size(),
        .component_0_state = simulator.Context().component_state.At(0).private_state,
        .component_1_state = simulator.Context().component_state.At(1).private_state,
        .component_2_state = simulator.Context().component_state.At(2).private_state,
    };
}

void TestComplexSystemDesignIrFromJson() {
    const std::filesystem::path json_ir_path = std::filesystem::path("src") / "sim" / "tests" / "fixtures" / "complex_system_ir.json";
    Assert(std::filesystem::exists(json_ir_path), "expected complex IR JSON fixture to exist");

    const simrun::SimulationIR parsed_ir = simrun::LoadSimulationIRFromJsonFile(json_ir_path.string());
    const simrun::ValidationResult validation = simrun::ValidateIR(parsed_ir);
    Assert(validation.ok, "expected parsed complex IR JSON to validate");

    const ScenarioSummary from_json = RunScenario(parsed_ir, false);
    Assert(from_json.run.status == simrun::RunStatus::kCompleted,
           "expected parsed JSON scenario to complete (status=" + std::to_string(static_cast<int>(from_json.run.status)) + ")");
    Assert(from_json.run.executed_events > 80, "expected parsed JSON scenario to execute many events");

    const ScenarioSummary from_builder = RunScenario(BuildComplexSystemIr(), false);
    Assert(from_json.run.status == from_builder.run.status, "expected JSON run status to match builder IR run");
    Assert(from_json.run.executed_events == from_builder.run.executed_events, "expected JSON executed event count to match builder IR run");
    Assert(from_json.run.discarded_events == from_builder.run.discarded_events, "expected JSON discarded event count to match builder IR run");
    Assert(from_json.run.final_time == from_builder.run.final_time, "expected JSON final time to match builder IR run");
    Assert(from_json.trace_size == from_builder.trace_size, "expected JSON trace size to match builder IR run");
    Assert(from_json.component_0_state == from_builder.component_0_state, "expected component 0 state to match builder IR run");
    Assert(from_json.component_1_state == from_builder.component_1_state, "expected component 1 state to match builder IR run");
    Assert(from_json.component_2_state == from_builder.component_2_state, "expected component 2 state to match builder IR run");
}

}  // namespace

int main() {
    try {
        TestIrValidation();
        TestSchedulerOrdering();
        TestInvalidTokenDiscard();
        TestDeterministicExecution();
        TestDesignAndDebugLogging();
        TestDesignLogDumpOnFinish();
        TestComplexSystemDesignIr();
        TestComplexSystemDesignIrFromJson();
        std::cout << "simrun_tests: OK\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "simrun_tests: FAIL: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}

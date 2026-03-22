#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../engine/factory/context_factory.h"
#include "../engine/factory/event_factory.h"
#include "../engine/factory/state_factory.h"
#include "../engine/ir/ir_parser.h"

namespace fs = std::filesystem;

namespace {

void log_test(const std::string& msg) {
    std::cout << "[TEST] " << msg << "\n";
}

void log_info(const std::string& msg) {
    std::cout << "[INFO] " << msg << "\n";
}

void log_pass(const std::string& msg) {
    std::cout << "[PASS] " << msg << "\n";
}

sim::ir::SimulationIR make_minimal_ir_with_workload(
    std::uint64_t total_requests,
    const sim::ir::Json& p1,
    const sim::ir::Json& p2,
    bool include_fault
) {
    sim::ir::SimulationIR ir;
    ir.header.ir_version = 2;
    ir.header.seed = 7;
    ir.header.time_unit = "ns";

    ir.components.push_back(sim::ir::IR_Component{1, "SERVICE", sim::ir::Json::object()});
    ir.components.push_back(sim::ir::IR_Component{2, "SERVICE", sim::ir::Json::object()});
    ir.links.push_back(sim::ir::IR_Link{
        10,
        1,
        2,
        sim::ir::Json{{"bandwidth_mbps", 1000}, {"latency_ns", 5}},
    });

    sim::ir::IR_RequestType request_type;
    request_type.id = 1;
    request_type.routes = {{10}};
    ir.request_types.push_back(request_type);

    sim::ir::IR_BehaviorEntry behavior_entry;
    behavior_entry.component_id = 1;
    behavior_entry.request_type = 1;
    behavior_entry.flag = 0;
    behavior_entry.sequence.push_back(sim::ir::IR_BehaviorOpSpec{
        "SEND_TO_NEXT_LINK",
        sim::ir::Json::object(),
    });
    ir.behavior.push_back(behavior_entry);

    sim::ir::IR_WorkloadSpec workload;
    workload.id = 1;
    workload.request_type = 1;
    workload.target_component = 1;
    workload.total_requests = total_requests;
    workload.arrival_distribution.type = "NORMAL";
    workload.arrival_distribution.p1 = p1;
    workload.arrival_distribution.p2 = p2;
    ir.workloads.push_back(workload);

    ir.bootstrap_events.push_back(sim::ir::IR_EventSpec{
        0,
        "WORKLOAD_ARRIVAL",
        sim::ir::Json{{"workload_id", 1}},
    });
    if (include_fault) {
        ir.bootstrap_events.push_back(sim::ir::IR_EventSpec{
            0,
            "FAULT_START",
            sim::ir::Json{{"entity_id", 10}, {"effect", "HALVE_BANDWIDTH"}},
        });
    }

    return ir;
}

std::vector<std::string> run_trace_for_ir(const fs::path& ir_path) {
    log_info("Loading IR for trace run: " + ir_path.string());
    sim::ir::SimulationIR ir = sim::ir::parse_ir_file(ir_path.string());
    sim::SimulationContext context = sim::factory::build_context(ir);
    sim::SimulationState state = sim::factory::build_initial_state(ir, context);
    sim::Simulator simulator(context, state);

    for (const auto& bootstrap_spec : context.bootstrap_events) {
        simulator.schedule(sim::factory::create_event_from_spec(bootstrap_spec, simulator.next_seq()));
    }

    simulator.run();
    const auto trace = simulator.event_trace();
    log_info("Trace run complete. events=" + std::to_string(trace.size()));
    return trace;
}

void test_parse_examples(const fs::path& repo_root) {
    log_test("test_parse_examples");
    const fs::path ir1 = repo_root / "docs" / "ir.json";
    const fs::path ir2 = repo_root / "docs" / "ir2.json";
    log_info("Parsing: " + ir1.string());
    const sim::ir::SimulationIR parsed1 = sim::ir::parse_ir_file(ir1.string());
    log_info("Parsing: " + ir2.string());
    const sim::ir::SimulationIR parsed2 = sim::ir::parse_ir_file(ir2.string());

    assert(parsed1.header.ir_version == 2);
    assert(!parsed1.workloads.empty());
    assert(!parsed1.bootstrap_events.empty());
    assert(parsed2.header.ir_version == 2);
    assert(!parsed2.components.empty());
    assert(!parsed2.workloads.empty());
    log_pass("test_parse_examples");
}

void test_canonicalization_and_reference_resolution(const fs::path& repo_root) {
    log_test("test_canonicalization_and_reference_resolution");
    const fs::path ir2 = repo_root / "docs" / "ir2.json";
    log_info("Building context from: " + ir2.string());
    sim::ir::SimulationIR ir = sim::ir::parse_ir_file(ir2.string());
    sim::SimulationContext context = sim::factory::build_context(ir);

    assert(context.component_models.size() == 3U);
    assert(context.link_models.size() == 3U);
    assert(context.request_types.size() == 1U);
    assert(context.workloads.size() == 1U);

    assert(context.component_index_by_external_id.at(1) == 0);
    assert(context.component_index_by_external_id.at(2) == 1);
    assert(context.component_index_by_external_id.at(3) == 2);

    assert(context.link_index_by_external_id.at(10) == 0);
    assert(context.link_index_by_external_id.at(11) == 1);
    assert(context.link_index_by_external_id.at(12) == 2);

    assert(context.link_models[0].from_component_idx == -1);
    assert(context.link_models[0].to_component_idx == context.component_index_by_external_id.at(1));
    assert(context.workload_index_by_external_id.at(1) == 0);
    log_info("component_count=" + std::to_string(context.component_models.size()));
    log_info("link_count=" + std::to_string(context.link_models.size()));
    log_info("workload_count=" + std::to_string(context.workloads.size()));
    log_pass("test_canonicalization_and_reference_resolution");
}

void test_determinism_and_flow(const fs::path& repo_root) {
    log_test("test_determinism_and_flow");
    const fs::path ir2 = repo_root / "docs" / "ir2.json";
    const std::vector<std::string> trace1 = run_trace_for_ir(ir2);
    const std::vector<std::string> trace2 = run_trace_for_ir(ir2);

    assert(!trace1.empty());
    assert(trace1 == trace2);
    log_info("deterministic_trace_size=" + std::to_string(trace1.size()));
    if (!trace1.empty()) {
        log_info("first_event=" + trace1.front());
        log_info("last_event=" + trace1.back());
    }
    log_pass("test_determinism_and_flow");
}

void test_workload_validation_errors() {
    log_test("test_workload_validation_errors");

    {
        sim::ir::SimulationIR ir = make_minimal_ir_with_workload(1, 1, 1, false);
        ir.workloads[0].request_type = 999;
        bool threw = false;
        try {
            (void)sim::factory::build_context(ir);
        } catch (const std::exception&) {
            threw = true;
        }
        assert(threw);
    }

    {
        sim::ir::SimulationIR ir = make_minimal_ir_with_workload(1, 1, 1, false);
        ir.bootstrap_events[0].payload = sim::ir::Json{{"workload_id", 999}};
        bool threw = false;
        try {
            (void)sim::factory::build_context(ir);
        } catch (const std::exception&) {
            threw = true;
        }
        assert(threw);
    }

    {
        sim::ir::SimulationIR ir = make_minimal_ir_with_workload(1, 1, 1, false);
        ir.workloads[0].arrival_distribution.type = "LOGNORMAL";
        bool threw = false;
        try {
            (void)sim::factory::build_context(ir);
        } catch (const std::exception&) {
            threw = true;
        }
        assert(threw);
    }

    {
        sim::ir::SimulationIR ir = make_minimal_ir_with_workload(1, 1, -1, false);
        bool threw = false;
        try {
            (void)sim::factory::build_context(ir);
        } catch (const std::exception&) {
            threw = true;
        }
        assert(threw);
    }

    log_pass("test_workload_validation_errors");
}

void test_workload_exact_request_count() {
    log_test("test_workload_exact_request_count");
    const std::uint64_t quota = 7;
    sim::ir::SimulationIR ir = make_minimal_ir_with_workload(quota, 2, 1, false);
    sim::SimulationContext context = sim::factory::build_context(ir);
    sim::SimulationState state = sim::factory::build_initial_state(ir, context);
    sim::Simulator simulator(context, state);

    for (const auto& bootstrap_spec : context.bootstrap_events) {
        simulator.schedule(sim::factory::create_event_from_spec(bootstrap_spec, simulator.next_seq()));
    }
    simulator.run();

    assert(simulator.state().requests.size() == quota);
    assert(simulator.state().workload_state.size() == 1U);
    assert(simulator.state().workload_state[0].generated_count == quota);
    log_info("generated_count=" + std::to_string(simulator.state().workload_state[0].generated_count));
    log_pass("test_workload_exact_request_count");
}

void test_workload_zero_requests() {
    log_test("test_workload_zero_requests");
    sim::ir::SimulationIR ir = make_minimal_ir_with_workload(0, 2, 1, false);
    sim::SimulationContext context = sim::factory::build_context(ir);
    sim::SimulationState state = sim::factory::build_initial_state(ir, context);
    sim::Simulator simulator(context, state);

    for (const auto& bootstrap_spec : context.bootstrap_events) {
        simulator.schedule(sim::factory::create_event_from_spec(bootstrap_spec, simulator.next_seq()));
    }
    simulator.run();

    assert(simulator.state().requests.empty());
    assert(simulator.state().workload_state[0].generated_count == 0U);
    log_pass("test_workload_zero_requests");
}

void test_negative_arrival_clamp() {
    log_test("test_negative_arrival_clamp");
    sim::ir::SimulationIR ir = make_minimal_ir_with_workload(4, -10, 0, false);
    sim::SimulationContext context = sim::factory::build_context(ir);
    sim::SimulationState state = sim::factory::build_initial_state(ir, context);
    sim::Simulator simulator(context, state);

    for (const auto& bootstrap_spec : context.bootstrap_events) {
        simulator.schedule(sim::factory::create_event_from_spec(bootstrap_spec, simulator.next_seq()));
    }
    simulator.run();

    assert(simulator.state().requests.size() == 4U);
    assert(simulator.state().workload_state[0].generated_count == 4U);

    for (const auto& trace_line : simulator.event_trace()) {
        if (trace_line.find("|WORKLOAD_ARRIVAL|") != std::string::npos) {
            const std::size_t sep = trace_line.find('|');
            assert(sep != std::string::npos);
            const std::uint64_t time = static_cast<std::uint64_t>(std::stoull(trace_line.substr(0, sep)));
            assert(time == 0U);
        }
    }
    log_pass("test_negative_arrival_clamp");
}

void test_fault_flow_with_workload() {
    log_test("test_fault_flow_with_workload");
    sim::ir::SimulationIR ir = make_minimal_ir_with_workload(2, 0, 0, true);
    sim::SimulationContext context = sim::factory::build_context(ir);
    sim::SimulationState state = sim::factory::build_initial_state(ir, context);
    sim::Simulator simulator(context, state);

    for (const auto& bootstrap_spec : context.bootstrap_events) {
        simulator.schedule(sim::factory::create_event_from_spec(bootstrap_spec, simulator.next_seq()));
    }
    simulator.run();

    const int link_idx = context.link_index_by_external_id.at(10);
    assert(simulator.state().links[link_idx].effective_bandwidth_mbps == 500.0);
    log_info(
        "faulted_bandwidth_mbps=" +
        std::to_string(simulator.state().links[link_idx].effective_bandwidth_mbps)
    );
    log_pass("test_fault_flow_with_workload");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "simrun_tests usage: simrun_tests <repo_root>\n";
        return 1;
    }
    const fs::path repo_root = fs::path(argv[1]);

    log_info("simrun_tests starting");
    log_info("repo_root=" + repo_root.string());

    test_parse_examples(repo_root);
    test_canonicalization_and_reference_resolution(repo_root);
    test_determinism_and_flow(repo_root);
    test_workload_validation_errors();
    test_workload_exact_request_count();
    test_workload_zero_requests();
    test_negative_arrival_clamp();
    test_fault_flow_with_workload();

    log_pass("simrun_tests passed");
    return 0;
}

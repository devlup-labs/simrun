#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "engine/factory/context_factory.h"
#include "engine/factory/event_factory.h"
#include "engine/factory/state_factory.h"
#include "engine/ir/ir_parser.h"

namespace {

std::string load_ir_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open IR file: " + path);
    }
    return path;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: simrun <ir.json>\n";
            return 1;
        }

        const std::string ir_path = load_ir_file(argv[1]);
        sim::ir::SimulationIR ir = sim::ir::parse_ir_file(ir_path);
        sim::SimulationContext context = sim::factory::build_context(ir);
        sim::SimulationState state = sim::factory::build_initial_state(ir, context);
        sim::Simulator simulator(context, state);

        for (const auto& bootstrap_spec : context.bootstrap_events) {
            simulator.schedule(
                sim::factory::create_event_from_spec(bootstrap_spec, simulator.next_seq())
            );
        }

        simulator.run();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "simrun error: " << ex.what() << "\n";
        return 2;
    }
}

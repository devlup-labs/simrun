#include <iostream>
#include "ir_parser.h"
#include "event_initializer.h"

#include "simulation_context.h"
#include "entity_factory.h"
#include "event_factory.h"
#include "scheduler.h"
#include "simulator.h"

int main(int argc, char** argv) {

    /* ---------- Parse IR ---------- */
    IR ir = IRParser::parseFromFile(argv[1]);

    /* ---------- Global Context ---------- */
    SimulationContext ctx(
        ir.header.seed,
        ir.header.time_unit
    );

    /* ---------- Factories ---------- */
    EntityFactory entityFactory(ctx);
    EventFactory eventFactory(ctx);

    /* ---------- Build Static World ---------- */
    entityFactory.createComponents(ir.components);
    entityFactory.createLinks(ir.links);


    entityFactory.applyInitialComponentState(ir.initial_components);
    entityFactory.applyInitialLinkState(ir.initial_links);

    entityFactory.registerRequestTypes(ir.request_types);

    /* ---------- Scheduler ---------- */
    Scheduler scheduler;

    /* ---------- Seed Initial Events ---------- */
    EventInitializer::seedInitialEvents(
        ir,
        scheduler,
        eventFactory
    );

    /* ---------- Run Simulation ---------- */
    Simulator sim(ctx, scheduler);
    sim.run();

    return 0;
}
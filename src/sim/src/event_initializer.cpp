#include "event_initializer.h"
#include "scheduler.h"
#include "entity_factory.h"
#include "../request/request.h"

void EventInitializer::seedInitialEvents(
    const IR& ir,
    Scheduler& scheduler,
    EntityFactory& factory
) {
    for (const auto& e : ir.initial_events) {

        /* ---------- Build request object ---------- */
        Request* req = createRequestObject(
            e.type,
            e.time,
            e.payload
        );

        /* ---------- Create event ---------- */
        Event* ev = factory.createEvent(
            e.type,
            e.time,
            e.seed,
            req
        );

        scheduler.schedule(ev);
    }
}

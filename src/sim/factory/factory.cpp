#include "entity_factory.h"
#include <stdexcept>

using std::make_unique;
using std::runtime_error;

/* ---------------- Constructor ---------------- */

EntityFactory::EntityFactory(SimulationContext& ctx)
    : ctx(ctx) {}

/* ---------------- Components ---------------- */

void EntityFactory::createComponents(
    const std::vector<IRComponent>& components
) {
    for (const auto& c : components) {

        switch (c.type) {
            case ComponentType::SERVICE:
                createService(c);
                break;

            case ComponentType::DATABASE:
                createDatabase(c);
                break;

            case ComponentType::CACHE:
                createCache(c);
                break;

            default:
                throw runtime_error("Unknown component type");
        }
    }
}


void EntityFactory::createService(const IRComponent& c) {
    ctx.components[c.id] = make_unique<Service>(
        c.id,   
        c.config
    );
}

void EntityFactory::createDatabase(const IRComponent& c) {
    ctx.components[c.id] = make_unique<Database>(
        c.id,
        c.config
    );
}

void EntityFactory::createCache(const IRComponent& c) {
    ctx.components[c.id] = make_unique<Cache>(
        c.id,
        c.config
    );
}

/* ---------------- Links ---------------- */

void EntityFactory::createLinks(
    const std::vector<IRLink>& links
) {
    for (const auto& l : links) {
        ctx.links[l.id] = make_unique<NetworkLink>(
            l.id,
            l.from,
            l.to,
            l.config
        );
    }
}

/* ---------------- Apply Initial Component State ---------------- */
void EntityFactory::applyComponentContext(
    const std::vector<IRComponentContext>& context
) {
    for (const auto& c : context) {
        auto it = ctx.components.find(c.id);
        if (it == ctx.components.end()) {
            throw runtime_error("Component not found for initial state");
        }

        it->second->setActive(c.active);
        it->second->setQueueSize(c.queue);
    }
}


/* ---------------- Apply Initial Link State ---------------- */
void EntityFactory::applyLinkContext(
    const std::vector<IRLinkContext>& context
) {
    for (const auto& c : context) {
        auto it = ctx.links.find(c.id);
        if (it == ctx.links.end()) {
            throw runtime_error("Link not found for initial state");
        }

        it->second->setBandwidthMbps(c.current_bandwidth_mbps);
    }
}


/* ---------------- Register Request Types ---------------- */
void EntityFactory::registerRequestTypes(
    const std::vector<IRRequestType>& request_types
) {
    for (const auto& r : request_types) {
        ctx.request_types[r.id] = r.payload;
    }
}


/* ================= Events ================= */

Event* EntityFactory::createEvent(
    EventType type,
    double timestamp,
    uint64_t seed,
    Request* req
) {
    switch (type) {

        case EventType::DB_REQUEST_ARRIVAL:
            return createDBRequestArrival(
                timestamp,
                seed,
                req
            );

        case EventType::DB_REQUEST_SEND:
            return createDBRequestSend(
                timestamp,
                seed,
                req
            );

        default:
            throw runtime_error("Unknown event type");
    }
}


/* -------- DB events -------- */

Event* EntityFactory::createDBRequestArrival(
    double ts,
    uint64_t seed,
    Request* req
) {
    uint32_t db_id = req->target_id;

    auto it = ctx.components.find(db_id);
    if (it == ctx.components.end())
        throw runtime_error("Database not found");

    Database* db =
        static_cast<Database*>(it->second.get());

    return new DBRequestArrivalEvent(
        ts,
        seed,
        db,
        req
    );
}

Event* EntityFactory::createDBRequestSend(
    double ts,
    uint64_t seed,
    Request* req
) {
    uint32_t db_id = req->target_id;

    auto it = ctx.components.find(db_id);
    if (it == ctx.components.end())
        throw runtime_error("Database not found");

    Database* db =
        static_cast<Database*>(it->second.get());

    return new DBRequestSendEvent(
        ts,
        seed,
        db,
        req
    );
}

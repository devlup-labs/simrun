#pragma once

#include <unordered_map>
#include <memory>
#include <vector>

#include "../context/simulation_context.h"
#include "../ir/ir_types.h"

// Components
#include "../entities/service.h"
#include "../entities/database.h"
#include "../entities/cache.h"

// Links
#include "../network/network_link.h"

class EntityFactory {
public:
    explicit EntityFactory(SimulationContext& ctx);

    void createComponents(const std::vector<IRComponent>& components);
    void createLinks(const std::vector<IRLink>& links);

    /* Initial state */
    void applyComponentContext(
        const std::vector<IRComponentContext>& context
    );

    void applyLinkContext(
        const std::vector<IRLinkContext>& context
    );

    /* Request types */
    void registerRequestTypes(
        const std::vector<IRRequestType>& request_types
    );

private:
    SimulationContext& ctx;

    /* Componenet Helpers */
    void createService(const IRComponent& c);
    void createDatabase(const IRComponent& c);
    void createCache(const IRComponent& c);


    /* -------- Events -------- */
    Event* createEvent(
        EventType type,
        double timestamp,
        uint64_t seed,
        Request* req
    );

    Event* createDBRequestArrival(
        double ts,
        uint64_t seed,
        Request* req
    );
    
    Event* createDBRequestSend(
        double ts,
        uint64_t seed,
        Request* req
    );

};

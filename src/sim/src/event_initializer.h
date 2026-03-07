#pragma once
#include "ir_types.h"

class Scheduler;
class EventFactory;

class EventInitializer {
public:
    static void seedInitialEvents(
        const IR& ir,
        Scheduler& scheduler,
        EventFactory& eventFactory
    );

    // fix @aradhya
    Request* createRequestObject(
        EventType type,
        double timestamp,
        const nlohmann::json& payload
    );

};

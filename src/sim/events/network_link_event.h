#pragma once

#include "event.h"

class Request;
class Simulator;

class RequestArrivalAtLinkEvent : public Event {
public:
    RequestArrivalAtLinkEvent(
        double time,
        uint32_t link_id,
        uint32_t dst_component_id,
        Request* req
    );

    void execute(Simulator& sim) override;

private:
    uint32_t link_id;
    uint32_t dst_component_id;
    Request* request;
};

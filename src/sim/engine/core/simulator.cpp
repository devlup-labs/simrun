#include "simulator.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

#include "../events/dispatch.h"

namespace sim {

Simulator::Simulator(const SimulationContext& context, SimulationState& state)
    : context_(context), state_(state), rng_(context.seed) {}

void Simulator::run() {
    while (!scheduler_.empty()) {
        std::unique_ptr<Event> event = scheduler_.next();
        now_ = event->time;
        record_trace(*event);
        events::dispatch(*event, *this);
    }
}

SimTime Simulator::now() const {
    return now_;
}

std::uint64_t Simulator::next_seq() {
    return next_seq_++;
}

void Simulator::schedule(std::unique_ptr<Event> event) {
    scheduler_.schedule(std::move(event));
}

int Simulator::sample_equal(const std::vector<int>& values) {
    if (values.empty()) {
        throw std::runtime_error("CHANGE_FLAG called with empty values");
    }
    std::uniform_int_distribution<std::size_t> dist(0, values.size() - 1);
    return values[dist(rng_)];
}

SimTime Simulator::sample_workload_arrival_delta(const Distribution& distribution) {
    switch (distribution.type) {
        case DistributionType::NORMAL: {
            double sampled = distribution.p1;
            if (distribution.p2 > 0.0) {
                std::normal_distribution<double> dist(distribution.p1, distribution.p2);
                sampled = dist(rng_);
            }
            if (!std::isfinite(sampled)) {
                throw std::runtime_error("Sampled non-finite NORMAL inter-arrival delta");
            }
            if (sampled < 0.0) {
                sampled = 0.0;
            }
            return static_cast<SimTime>(std::floor(sampled));
        }
    }

    throw std::runtime_error("Unsupported distribution type");
}

int Simulator::create_request(int request_type_idx, int target_component_idx) {
    if (request_type_idx < 0 || request_type_idx >= static_cast<int>(context_.request_types.size())) {
        throw std::runtime_error("Request type index out of bounds");
    }
    if (
        target_component_idx < 0 ||
        target_component_idx >= static_cast<int>(context_.component_models.size())
    ) {
        throw std::runtime_error("Target component index out of bounds");
    }

    Request req;
    req.id = static_cast<int>(state_.requests.size());
    req.request_type_idx = request_type_idx;
    req.current_component_idx = target_component_idx;
    state_.requests.push_back(req);
    return req.id;
}

const SimulationContext& Simulator::context() const {
    return context_;
}

SimulationState& Simulator::state() {
    return state_;
}

const SimulationState& Simulator::state() const {
    return state_;
}

const std::vector<std::string>& Simulator::event_trace() const {
    return event_trace_;
}

void Simulator::record_trace(const Event& event) {
    std::ostringstream out;
    out << event.time << "|" << event.seq << "|" << event_type_name(event.type) << "|"
        << event.request_id << "|" << event.component_idx << "|" << event.link_idx;
    event_trace_.push_back(out.str());
}

}  // namespace sim

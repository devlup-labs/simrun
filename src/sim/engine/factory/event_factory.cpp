#include "event_factory.h"

#include <algorithm>
#include <stdexcept>

namespace sim::factory {
namespace {

std::string remove_underscores(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    return text;
}

std::int64_t parse_i64_flexible(const ir::Json& value) {
    if (value.is_number_integer()) {
        return value.get<std::int64_t>();
    }
    if (value.is_number_unsigned()) {
        return static_cast<std::int64_t>(value.get<std::uint64_t>());
    }
    if (value.is_number_float()) {
        return static_cast<std::int64_t>(value.get<double>());
    }
    if (value.is_string()) {
        return std::stoll(remove_underscores(value.get<std::string>()));
    }
    throw std::runtime_error("Expected integer JSON value in event payload");
}

FaultEffect parse_fault_effect(const std::string& effect) {
    if (effect == "HALVE_BANDWIDTH") {
        return FaultEffect::HALVE_BANDWIDTH;
    }
    throw std::runtime_error("Unsupported fault effect: " + effect);
}

int resolve_id_or_throw(
    const std::unordered_map<int, int>& index_by_external_id,
    int external_id,
    const char* label
) {
    const auto it = index_by_external_id.find(external_id);
    if (it == index_by_external_id.end()) {
        throw std::runtime_error(
            std::string("Unknown ") + label + " external id in bootstrap event: " +
            std::to_string(external_id)
        );
    }
    return it->second;
}

}  // namespace

std::unique_ptr<Event> create_event_from_spec(
    const ir::IR_EventSpec& spec,
    std::uint64_t seq,
    const SimulationContext& ctx
) {
    if (spec.type == "WORKLOAD_ARRIVAL") {
        if (!spec.payload.contains("workload_id")) {
            throw std::runtime_error(
                "WORKLOAD_ARRIVAL bootstrap payload must contain workload_id"
            );
        }
        const int workload_idx = resolve_id_or_throw(
            ctx.workload_index_by_external_id,
            static_cast<int>(parse_i64_flexible(spec.payload.at("workload_id"))),
            "workload"
        );
        return Event::make_workload_arrival(spec.time, seq, workload_idx);
    }

    if (spec.type == "FAULT_START") {
        const int link_idx = resolve_id_or_throw(
            ctx.link_index_by_external_id,
            static_cast<int>(parse_i64_flexible(spec.payload.at("entity_id"))),
            "link"
        );
        const FaultEffect effect = parse_fault_effect(spec.payload.at("effect").get<std::string>());
        return Event::make_fault_start(spec.time, seq, link_idx, effect);
    }

    throw std::runtime_error("Unsupported bootstrap event type: " + spec.type);
}

std::unique_ptr<Event> create_event_from_spec(
    const CompiledBootstrapEventSpec& spec,
    std::uint64_t seq
) {
    switch (spec.type) {
        case EventType::WORKLOAD_ARRIVAL:
            return Event::make_workload_arrival(spec.time, seq, spec.workload_idx);
        case EventType::FAULT_START:
            return Event::make_fault_start(
                spec.time,
                seq,
                spec.fault_entity_link_idx,
                spec.fault_effect
            );
        default:
            break;
    }
    throw std::runtime_error("Unsupported compiled bootstrap event type");
}

}  // namespace sim::factory

#include "context_factory.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace sim::factory {
namespace {

std::string lower_copy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string remove_underscores(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());
    return text;
}

double parse_double_flexible(const ir::Json& value) {
    if (value.is_number()) {
        return value.get<double>();
    }
    if (value.is_string()) {
        return std::stod(remove_underscores(value.get<std::string>()));
    }
    throw std::runtime_error("Expected numeric value during context build");
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
    throw std::runtime_error("Expected integer value during context build");
}

TimeUnit parse_time_unit(const std::string& text) {
    const std::string unit = lower_copy(text);
    if (unit == "ns" || unit == "nanoseconds") {
        return TimeUnit::NS;
    }
    if (unit == "us" || unit == "microseconds") {
        return TimeUnit::US;
    }
    if (unit == "ms" || unit == "milliseconds") {
        return TimeUnit::MS;
    }
    if (unit == "s" || unit == "seconds") {
        return TimeUnit::S;
    }
    return TimeUnit::UNKNOWN;
}

ComponentKind parse_component_kind(const std::string& kind) {
    const std::string k = lower_copy(kind);
    if (k == "load_balancer") {
        return ComponentKind::LOAD_BALANCER;
    }
    if (k == "server") {
        return ComponentKind::SERVER;
    }
    if (k == "service") {
        return ComponentKind::SERVICE;
    }
    if (k == "database") {
        return ComponentKind::DATABASE;
    }
    if (k == "cache") {
        return ComponentKind::CACHE;
    }
    return ComponentKind::UNKNOWN;
}

BehaviorOpcode parse_opcode(const std::string& op) {
    if (op == "CHANGE_FLAG") {
        return BehaviorOpcode::CHANGE_FLAG;
    }
    if (op == "SEND_TO_NEXT_LINK") {
        return BehaviorOpcode::SEND_TO_NEXT_LINK;
    }
    throw std::runtime_error("Unsupported behavior op: " + op);
}

DistributionType parse_distribution_type(const std::string& type) {
    if (type == "NORMAL") {
        return DistributionType::NORMAL;
    }
    throw std::runtime_error("Unsupported arrival distribution type: " + type);
}

FaultEffect parse_fault_effect(const std::string& effect) {
    if (effect == "HALVE_BANDWIDTH") {
        return FaultEffect::HALVE_BANDWIDTH;
    }
    throw std::runtime_error("Unsupported fault effect: " + effect);
}

int resolve_component_external_id(const SimulationContext& ctx, int external_id) {
    const auto it = ctx.component_index_by_external_id.find(external_id);
    if (it == ctx.component_index_by_external_id.end()) {
        if (external_id == 0) {
            return -1;
        }
        throw std::runtime_error("Unknown component external id: " + std::to_string(external_id));
    }
    return it->second;
}

int resolve_link_external_id(const SimulationContext& ctx, int external_id) {
    const auto it = ctx.link_index_by_external_id.find(external_id);
    if (it == ctx.link_index_by_external_id.end()) {
        throw std::runtime_error("Unknown link external id: " + std::to_string(external_id));
    }
    return it->second;
}

int resolve_request_type_external_id(const SimulationContext& ctx, int external_id) {
    const auto it = ctx.request_type_index_by_external_id.find(external_id);
    if (it == ctx.request_type_index_by_external_id.end()) {
        throw std::runtime_error("Unknown request type external id: " + std::to_string(external_id));
    }
    return it->second;
}

int resolve_workload_external_id(const SimulationContext& ctx, int external_id) {
    const auto it = ctx.workload_index_by_external_id.find(external_id);
    if (it == ctx.workload_index_by_external_id.end()) {
        throw std::runtime_error("Unknown workload external id: " + std::to_string(external_id));
    }
    return it->second;
}

std::uint64_t compile_link_latency_ns(const ir::IR_Link& link) {
    if (!link.params.is_object()) {
        return 0;
    }
    if (link.params.contains("latency_ns")) {
        return static_cast<std::uint64_t>(parse_i64_flexible(link.params.at("latency_ns")));
    }
    if (link.params.contains("latency_dist")) {
        const ir::Json& dist = link.params.at("latency_dist");
        if (dist.is_object() && dist.contains("p1")) {
            return static_cast<std::uint64_t>(parse_i64_flexible(dist.at("p1")));
        }
    }
    return 0;
}

double compile_base_bandwidth_mbps(const ir::IR_Link& link) {
    if (!link.params.is_object()) {
        return 0.0;
    }
    if (link.params.contains("bandwidth_mbps")) {
        return parse_double_flexible(link.params.at("bandwidth_mbps"));
    }
    return 0.0;
}

BehaviorOp compile_behavior_op(const ir::IR_BehaviorOpSpec& spec) {
    BehaviorOp op;
    op.opcode = parse_opcode(spec.op);

    if (op.opcode == BehaviorOpcode::CHANGE_FLAG) {
        if (!spec.args.contains("values") || !spec.args.at("values").is_array()) {
            throw std::runtime_error("CHANGE_FLAG requires args.values array");
        }
        for (const ir::Json& value : spec.args.at("values")) {
            op.values.push_back(static_cast<int>(parse_i64_flexible(value)));
        }
        if (op.values.empty()) {
            throw std::runtime_error("CHANGE_FLAG args.values cannot be empty");
        }
    }

    return op;
}

Distribution compile_distribution(const ir::IR_DistributionSpec& spec) {
    Distribution distribution;
    distribution.type = parse_distribution_type(spec.type);
    distribution.p1 = parse_double_flexible(spec.p1);
    distribution.p2 = parse_double_flexible(spec.p2);
    if (distribution.p2 < 0.0) {
        throw std::runtime_error("NORMAL distribution p2 (stddev) must be >= 0");
    }
    return distribution;
}

}  // namespace

SimulationContext build_context(const ir::SimulationIR& ir) {
    SimulationContext ctx;
    ctx.seed = ir.header.seed;
    ctx.time_unit = parse_time_unit(ir.header.time_unit);

    ctx.component_models.resize(ir.components.size());
    ctx.component_external_ids.resize(ir.components.size());
    for (std::size_t i = 0; i < ir.components.size(); ++i) {
        const auto& component = ir.components[i];
        ctx.component_external_ids[i] = component.id;
        ctx.component_index_by_external_id[component.id] = static_cast<int>(i);
        ctx.component_models[i] = ComponentModel{
            component.id,
            parse_component_kind(component.kind),
        };
    }

    ctx.link_models.resize(ir.links.size());
    ctx.link_external_ids.resize(ir.links.size());
    for (std::size_t i = 0; i < ir.links.size(); ++i) {
        const auto& link = ir.links[i];
        ctx.link_external_ids[i] = link.id;
        ctx.link_index_by_external_id[link.id] = static_cast<int>(i);
    }

    for (std::size_t i = 0; i < ir.links.size(); ++i) {
        const auto& link = ir.links[i];
        const int from_idx = resolve_component_external_id(ctx, link.from);
        const int to_idx = resolve_component_external_id(ctx, link.to);
        ctx.link_models[i] = LinkModel{
            link.id,
            from_idx,
            to_idx,
            compile_link_latency_ns(link),
            compile_base_bandwidth_mbps(link),
        };
    }

    ctx.request_types.resize(ir.request_types.size());
    ctx.request_type_external_ids.resize(ir.request_types.size());
    for (std::size_t i = 0; i < ir.request_types.size(); ++i) {
        const auto& request_type = ir.request_types[i];
        ctx.request_type_external_ids[i] = request_type.id;
        ctx.request_type_index_by_external_id[request_type.id] = static_cast<int>(i);
    }

    for (std::size_t i = 0; i < ir.request_types.size(); ++i) {
        const auto& request_type = ir.request_types[i];
        RequestTypeModel compiled;
        compiled.external_id = request_type.id;
        compiled.routing_by_flag.resize(request_type.routes.size());
        for (std::size_t flag = 0; flag < request_type.routes.size(); ++flag) {
            const auto& route = request_type.routes[flag];
            auto& compiled_route = compiled.routing_by_flag[flag];
            compiled_route.reserve(route.size());
            for (const int external_link_id : route) {
                compiled_route.push_back(resolve_link_external_id(ctx, external_link_id));
            }
        }
        ctx.request_types[i] = std::move(compiled);
    }

    ctx.workloads.resize(ir.workloads.size());
    ctx.workload_external_ids.resize(ir.workloads.size());
    for (std::size_t i = 0; i < ir.workloads.size(); ++i) {
        const auto& workload = ir.workloads[i];
        ctx.workload_external_ids[i] = workload.id;
        ctx.workload_index_by_external_id[workload.id] = static_cast<int>(i);
    }

    for (std::size_t i = 0; i < ir.workloads.size(); ++i) {
        const auto& workload = ir.workloads[i];
        const int request_type_idx = resolve_request_type_external_id(ctx, workload.request_type);
        const int target_component_idx = resolve_component_external_id(ctx, workload.target_component);
        if (target_component_idx < 0) {
            throw std::runtime_error(
                "Workload target_component cannot be external endpoint: " +
                std::to_string(workload.target_component)
            );
        }

        WorkloadModel compiled_workload;
        compiled_workload.external_id = workload.id;
        compiled_workload.request_type_idx = request_type_idx;
        compiled_workload.target_component_idx = target_component_idx;
        compiled_workload.total_requests = workload.total_requests;
        compiled_workload.arrival_dist = compile_distribution(workload.arrival_distribution);
        ctx.workloads[i] = compiled_workload;
    }

    ctx.behavior.resize(ctx.component_models.size());
    for (auto& by_component : ctx.behavior) {
        by_component.resize(ctx.request_types.size());
    }

    for (const auto& entry : ir.behavior) {
        if (entry.flag < 0) {
            throw std::runtime_error("Behavior flag must be non-negative");
        }
        const int component_idx = resolve_component_external_id(ctx, entry.component_id);
        const int request_type_idx = resolve_request_type_external_id(ctx, entry.request_type);

        auto& by_flag = ctx.behavior[component_idx][request_type_idx];
        if (static_cast<int>(by_flag.size()) <= entry.flag) {
            by_flag.resize(static_cast<std::size_t>(entry.flag) + 1U);
        }

        auto& sequence = by_flag[entry.flag];
        sequence.reserve(entry.sequence.size());
        for (const auto& op_spec : entry.sequence) {
            sequence.push_back(compile_behavior_op(op_spec));
        }
    }

    ctx.bootstrap_events.reserve(ir.bootstrap_events.size());
    for (const auto& spec : ir.bootstrap_events) {
        CompiledBootstrapEventSpec compiled;
        compiled.time = spec.time;

        if (spec.type == "WORKLOAD_ARRIVAL") {
            compiled.type = EventType::WORKLOAD_ARRIVAL;
            if (!spec.payload.contains("workload_id")) {
                throw std::runtime_error(
                    "WORKLOAD_ARRIVAL bootstrap payload must contain workload_id"
                );
            }
            compiled.workload_idx = resolve_workload_external_id(
                ctx,
                static_cast<int>(parse_i64_flexible(spec.payload.at("workload_id")))
            );
        } else if (spec.type == "FAULT_START") {
            compiled.type = EventType::FAULT_START;
            compiled.fault_entity_link_idx = resolve_link_external_id(
                ctx,
                static_cast<int>(parse_i64_flexible(spec.payload.at("entity_id")))
            );
            compiled.fault_effect = parse_fault_effect(spec.payload.at("effect").get<std::string>());
        } else {
            throw std::runtime_error("Unsupported bootstrap event type: " + spec.type);
        }

        ctx.bootstrap_events.push_back(compiled);
    }

    return ctx;
}

}  // namespace sim::factory

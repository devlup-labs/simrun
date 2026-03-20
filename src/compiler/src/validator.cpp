#include "validator.h"

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <limits>

namespace simrun {

// ============================================================
// Shared adjacency helpers used by multiple modules
// ============================================================

using AdjSet = std::unordered_map<std::string, std::unordered_set<std::string>>;

static void buildAdj(const DiagramIR& ir, AdjSet& out, AdjSet& in) {
    for (auto& [id, _] : ir.components) { out[id]; in[id]; }
    for (auto& c : ir.connections) {
        if (ir.components.count(c.from_component_id) && ir.components.count(c.to_component_id)) {
            out[c.from_component_id].insert(c.to_component_id);
            in[c.to_component_id].insert(c.from_component_id);
        }
    }
}

// ============================================================
// StructuralValidator
// ============================================================

class StructuralValidator : public IValidatorModule {
public:
    std::string name() const override { return "StructuralValidator"; }

    std::vector<ValidationIssue> validate(const DiagramIR& ir) const override {
        std::vector<ValidationIssue> issues;

        for (auto& [id, comp] : ir.components) {
            if (id.empty())
                issues.push_back(ValidationIssue::error("A component has an empty ID.", name()));
            if (comp.id != id)
                issues.push_back(ValidationIssue::error(
                    "Component map key \"" + id + "\" does not match stored id \"" + comp.id + "\".",
                    name(), {id}));
        }

        std::unordered_set<std::string> seenConn;
        for (auto& conn : ir.connections) {
            if (conn.id.empty()) {
                issues.push_back(ValidationIssue::error("A connection has an empty ID.", name()));
                continue;
            }
            if (!seenConn.insert(conn.id).second)
                issues.push_back(ValidationIssue::error(
                    "Duplicate connection ID \"" + conn.id + "\".", name(), {}, {conn.id}));

            if (!ir.components.count(conn.from_component_id))
                issues.push_back(ValidationIssue::error(
                    "Connection \"" + conn.id + "\" references non-existent source component \""
                    + conn.from_component_id + "\".", name(), {conn.from_component_id}, {conn.id}));

            if (!ir.components.count(conn.to_component_id))
                issues.push_back(ValidationIssue::error(
                    "Connection \"" + conn.id + "\" references non-existent target component \""
                    + conn.to_component_id + "\".", name(), {conn.to_component_id}, {conn.id}));

            if (!conn.from_component_id.empty() && conn.from_component_id == conn.to_component_id)
                issues.push_back(ValidationIssue::error(
                    "Connection \"" + conn.id + "\" is a self-loop on component \""
                    + conn.from_component_id + "\". Self-loops are not permitted.",
                    name(), {conn.from_component_id}, {conn.id}));
        }

        std::unordered_set<std::string> hasOut, hasIn;
        for (auto& c : ir.connections) {
            if (ir.components.count(c.from_component_id) && ir.components.count(c.to_component_id)) {
                hasOut.insert(c.from_component_id);
                hasIn.insert(c.to_component_id);
            }
        }
        for (auto& [id, comp] : ir.components) {
            if (!hasOut.count(id) && !hasIn.count(id))
                issues.push_back(ValidationIssue::warning(
                    "Component \"" + id + "\" (" + componentTypeStr(comp.type)
                    + ") is completely isolated — no connections. It has no effect on simulation.",
                    name(), {id}));
        }

        return issues;
    }
};

// ============================================================
// ConfigValidator
// ============================================================

struct NumericParam {
    std::string key;
    bool   required  = false;
    double minVal    = 0.0;
    double maxVal    = std::numeric_limits<double>::max();
    bool   allowZero = true;
    std::function<std::string(double)> customCheck = nullptr;
};

struct StringParam {
    std::string key;
    bool required = false;
    std::unordered_set<std::string> allowed;
};

struct ComponentSchema {
    std::vector<NumericParam> numeric;
    std::vector<StringParam>  strings;
};

static std::unordered_map<ComponentType, ComponentSchema> buildSchemas() {
    std::unordered_map<ComponentType, ComponentSchema> s;

    s[ComponentType::SERVICE] = {
        {
            {"latency",    false, 0.0, 1e9,  true},
            {"error_rate", false, 0.0, 1.0,  true},
            {"capacity",   false, 0.0, 1e9,  false,
                [](double v) -> std::string {
                    return v != std::floor(v) ? "capacity must be an integer, got " + std::to_string(v) : "";
                }},
            {"timeout_ms", false, 0.0, 1e6,  true},
            {"instances",  false, 1.0, 1e4,  false,
                [](double v) -> std::string {
                    return v != std::floor(v) ? "instances must be an integer, got " + std::to_string(v) : "";
                }},
        },
        { {"protocol", false, {"HTTP","GRPC","TCP","UDP"}} }
    };

    s[ComponentType::CACHE] = {
        {
            {"latency",    false, 0.0, 1e9, true},
            {"error_rate", false, 0.0, 1.0, true},
            {"capacity",   false, 0.0, 1e9, false},
            {"ttl_ms",     false, 0.0, std::numeric_limits<double>::max(), true},
            {"hit_rate",   false, 0.0, 1.0, true},
        },
        { {"eviction_policy", false, {"LRU","LFU","FIFO","RANDOM"}} }
    };

    s[ComponentType::DATABASE] = {
        {
            {"latency",           false, 0.0, 1e9,  true},
            {"error_rate",        false, 0.0, 1.0,  true},
            {"capacity",          false, 0.0, 1e9,  false},
            {"replication_factor",false, 1.0, 100.0,false,
                [](double v) -> std::string {
                    return v != std::floor(v) ? "replication_factor must be an integer, got " + std::to_string(v) : "";
                }},
            {"query_timeout_ms",  false, 0.0, 1e6,  true},
        },
        {
            {"engine",      false, {"POSTGRES","MYSQL","MONGODB","CASSANDRA","REDIS"}},
            {"consistency", false, {"STRONG","EVENTUAL","CAUSAL"}}
        }
    };

    s[ComponentType::LOAD_BALANCER] = {
        {
            {"latency",    false, 0.0, 1e9, true},
            {"error_rate", false, 0.0, 1.0, true},
            {"capacity",   false, 0.0, 1e9, false},
        },
        {
            {"algorithm",     false, {"ROUND_ROBIN","LEAST_CONNECTIONS","IP_HASH","RANDOM","WEIGHTED_ROUND_ROBIN"}},
            {"health_check",  false, {"ENABLED","DISABLED"}}
        }
    };

    return s;
}

class ConfigValidator : public IValidatorModule {
public:
    std::string name() const override { return "ConfigValidator"; }

    std::vector<ValidationIssue> validate(const DiagramIR& ir) const override {
        std::vector<ValidationIssue> issues;
        static const auto schemas = buildSchemas();

        for (auto& [id, comp] : ir.components) {
            auto sit = schemas.find(comp.type);
            if (sit == schemas.end()) {
                if (comp.type != ComponentType::UNKNOWN)
                    issues.push_back(ValidationIssue::warning(
                        "No config schema for type \"" + componentTypeStr(comp.type)
                        + "\" on component \"" + id + "\". Skipping config validation.",
                        name(), {id}));
                continue;
            }
            const auto& schema = sit->second;

            // numeric
            std::unordered_set<std::string> knownNum;
            for (auto& p : schema.numeric) {
                knownNum.insert(p.key);
                auto it = comp.numericConfig.find(p.key);
                if (it == comp.numericConfig.end()) {
                    if (p.required)
                        issues.push_back(ValidationIssue::error(
                            "Component \"" + id + "\" missing required numeric param \"" + p.key + "\".",
                            name(), {id}));
                    continue;
                }
                double v = it->second;
                if (std::isnan(v) || std::isinf(v)) {
                    issues.push_back(ValidationIssue::error(
                        "Component \"" + id + "\": param \"" + p.key + "\" is non-finite.",
                        name(), {id}));
                    continue;
                }
                if (!p.allowZero && v <= p.minVal)
                    issues.push_back(ValidationIssue::error(
                        "Component \"" + id + "\": param \"" + p.key + "\" must be > "
                        + std::to_string(p.minVal) + " but is " + std::to_string(v) + ".",
                        name(), {id}));
                else if (v < p.minVal || v > p.maxVal)
                    issues.push_back(ValidationIssue::error(
                        "Component \"" + id + "\": param \"" + p.key + "\" = " + std::to_string(v)
                        + " is outside [" + std::to_string(p.minVal) + ", " + std::to_string(p.maxVal) + "].",
                        name(), {id}));
                if (p.customCheck) {
                    std::string msg = p.customCheck(v);
                    if (!msg.empty())
                        issues.push_back(ValidationIssue::error(
                            "Component \"" + id + "\": param \"" + p.key + "\": " + msg,
                            name(), {id}));
                }
            }

            // string / enum
            std::unordered_set<std::string> knownStr;
            for (auto& p : schema.strings) {
                knownStr.insert(p.key);
                auto it = comp.stringConfig.find(p.key);
                if (it == comp.stringConfig.end()) {
                    if (p.required)
                        issues.push_back(ValidationIssue::error(
                            "Component \"" + id + "\" missing required string param \"" + p.key + "\".",
                            name(), {id}));
                    continue;
                }
                const std::string& val = it->second;
                if (val.empty()) {
                    issues.push_back(ValidationIssue::error(
                        "Component \"" + id + "\": string param \"" + p.key + "\" must not be empty.",
                        name(), {id}));
                    continue;
                }
                if (!p.allowed.empty() && !p.allowed.count(val)) {
                    std::string opts;
                    for (auto& o : p.allowed) opts += "\"" + o + "\" ";
                    issues.push_back(ValidationIssue::error(
                        "Component \"" + id + "\": param \"" + p.key + "\" has unsupported value \""
                        + val + "\". Allowed: " + opts, name(), {id}));
                }
            }

            // unknown keys
            for (auto& [k, _] : comp.numericConfig)
                if (!knownNum.count(k))
                    issues.push_back(ValidationIssue::warning(
                        "Component \"" + id + "\" has unrecognised numeric key \"" + k
                        + "\". It will be ignored. Check for typos.", name(), {id}));
            for (auto& [k, _] : comp.stringConfig)
                if (!knownStr.count(k))
                    issues.push_back(ValidationIssue::warning(
                        "Component \"" + id + "\" has unrecognised string key \"" + k
                        + "\". It will be ignored. Check for typos.", name(), {id}));
        }

        return issues;
    }
};

// ============================================================
// SemanticValidator
// ============================================================

class SemanticValidator : public IValidatorModule {
public:
    std::string name() const override { return "SemanticValidator"; }

    std::vector<ValidationIssue> validate(const DiagramIR& ir) const override {
        std::vector<ValidationIssue> issues;
        if (ir.components.empty()) return issues;

        AdjSet out, in;
        buildAdj(ir, out, in);

        for (auto& [id, comp] : ir.components) {
            if (comp.type == ComponentType::DATABASE && !out.at(id).empty()) {
                std::string targets;
                for (auto& t : out.at(id)) targets += "\"" + t + "\" ";
                issues.push_back(ValidationIssue::error(
                    "DATABASE \"" + id + "\" has " + std::to_string(out.at(id).size())
                    + " outgoing connection(s) to: " + targets
                    + "— databases must not initiate outbound requests.",
                    name(), {id}));
            }

            if (comp.type == ComponentType::CACHE) {
                std::size_t n = out.at(id).size();
                if (n != 1)
                    issues.push_back(ValidationIssue::error(
                        "CACHE \"" + id + "\" must have exactly one backend but has "
                        + std::to_string(n) + ". A cache must point to exactly one data source.",
                        name(), {id}));
            }

            if (comp.type == ComponentType::LOAD_BALANCER && out.at(id).empty())
                issues.push_back(ValidationIssue::error(
                    "LOAD_BALANCER \"" + id + "\" has no backend connections. "
                    "It must route traffic to at least one backend.",
                    name(), {id}));

            if (comp.type == ComponentType::LOAD_BALANCER) {
                for (auto& tgt : out.at(id)) {
                    if (ir.components.count(tgt) && ir.components.at(tgt).type == ComponentType::DATABASE)
                        issues.push_back(ValidationIssue::warning(
                            "LOAD_BALANCER \"" + id + "\" routes directly to DATABASE \""
                            + tgt + "\". LBs typically front services, not databases.",
                            name(), {id, tgt}));
                }
            }
        }

        std::vector<std::string> entries;
        for (auto& [id, comp] : ir.components)
            if (comp.type != ComponentType::DATABASE && in.at(id).empty())
                entries.push_back(id);

        if (entries.empty())
            issues.push_back(ValidationIssue::error(
                "The diagram has no entry point. At least one non-DATABASE component "
                "must have no incoming connections to serve as request ingress.",
                name()));
        else if (entries.size() > 1) {
            std::string list;
            for (auto& ep : entries) list += "\"" + ep + "\" ";
            issues.push_back(ValidationIssue::warning(
                "The diagram has " + std::to_string(entries.size()) + " entry points: "
                + list + "— multiple ingress points may indicate a missing top-level load balancer.",
                name(), entries));
        }

        for (auto& [id, comp] : ir.components) {
            bool hasIn = !in.at(id).empty(), hasOut = !out.at(id).empty();
            if (!hasIn && hasOut
                && comp.type != ComponentType::LOAD_BALANCER
                && comp.type != ComponentType::SERVICE)
            {
                issues.push_back(ValidationIssue::warning(
                    componentTypeStr(comp.type) + " component \"" + id
                    + "\" has outgoing connections but no incoming — unexpected ingress type.",
                    name(), {id}));
            }
        }

        return issues;
    }
};

// ============================================================
// TopologyValidator
// ============================================================

class TopologyValidator : public IValidatorModule {
public:
    static constexpr int DEFAULT_DEPTH_THRESHOLD = 8;
    explicit TopologyValidator(int dt = DEFAULT_DEPTH_THRESHOLD) : depthThreshold_(dt) {}
    std::string name() const override { return "TopologyValidator"; }

    std::vector<ValidationIssue> validate(const DiagramIR& ir) const override {
        std::vector<ValidationIssue> issues;
        if (ir.components.empty()) return issues;

        std::unordered_map<std::string, std::vector<std::string>> adj;
        for (auto& [id, _] : ir.components) adj[id];
        for (auto& c : ir.connections)
            if (ir.components.count(c.from_component_id) && ir.components.count(c.to_component_id))
                adj[c.from_component_id].push_back(c.to_component_id);

        detectCycles(ir, adj, issues);
        checkReachability(ir, adj, issues);
        checkDepth(ir, adj, issues);
        return issues;
    }

private:
    int depthThreshold_;

    using Adj = std::unordered_map<std::string, std::vector<std::string>>;

    void detectCycles(const DiagramIR& ir, const Adj& adj,
                      std::vector<ValidationIssue>& issues) const
    {
        enum Color { WHITE, GRAY, BLACK };
        std::unordered_map<std::string, Color> color;
        for (auto& [id, _] : ir.components) color[id] = WHITE;

        for (auto& [startId, _] : ir.components) {
            if (color[startId] != WHITE) continue;
            std::vector<std::pair<std::string, std::size_t>> stack;
            stack.push_back({startId, 0});
            color[startId] = GRAY;

            while (!stack.empty()) {
                auto& [node, idx] = stack.back();
                const auto& nbrs = adj.at(node);
                if (idx < nbrs.size()) {
                    const std::string& next = nbrs[idx++];
                    if (color[next] == GRAY) {
                        std::string path;
                        bool found = false;
                        for (auto& [n, _] : stack) {
                            if (n == next) found = true;
                            if (found) path += "\"" + n + "\" -> ";
                        }
                        path += "\"" + next + "\" (cycle)";
                        issues.push_back(ValidationIssue::error(
                            "Cycle detected: " + path
                            + ". Cycles cause infinite request loops during simulation.",
                            name(), {node, next}));
                    } else if (color[next] == WHITE) {
                        color[next] = GRAY;
                        stack.push_back({next, 0});
                    }
                } else {
                    color[node] = BLACK;
                    stack.pop_back();
                }
            }
        }
    }

    void checkReachability(const DiagramIR& ir, const Adj& adj,
                           std::vector<ValidationIssue>& issues) const
    {
        std::unordered_set<std::string> hasIn;
        for (auto& [id, nbrs] : adj) for (auto& n : nbrs) hasIn.insert(n);

        std::unordered_set<std::string> visited;
        std::vector<std::string> queue;
        for (auto& [id, _] : ir.components)
            if (!hasIn.count(id)) { queue.push_back(id); visited.insert(id); }

        while (!queue.empty()) {
            std::string cur = queue.back(); queue.pop_back();
            for (auto& next : adj.at(cur))
                if (!visited.count(next)) { visited.insert(next); queue.push_back(next); }
        }

        for (auto& [id, comp] : ir.components)
            if (!visited.count(id))
                issues.push_back(ValidationIssue::warning(
                    "Component \"" + id + "\" (" + componentTypeStr(comp.type)
                    + ") is unreachable from all entry points — it will never process requests.",
                    name(), {id}));
    }

    void checkDepth(const DiagramIR& ir, const Adj& adj,
                    std::vector<ValidationIssue>& issues) const
    {
        std::unordered_map<std::string, int> memo;
        std::unordered_map<std::string, bool> inStack;

        std::function<int(const std::string&)> dfs = [&](const std::string& id) -> int {
            auto it = memo.find(id);
            if (it != memo.end()) return it->second;
            if (inStack[id]) return 0;
            inStack[id] = true;
            int d = 0;
            for (auto& next : adj.at(id)) d = std::max(d, 1 + dfs(next));
            inStack[id] = false;
            return memo[id] = d;
        };

        std::unordered_set<std::string> hasIn;
        for (auto& [id, nbrs] : adj) for (auto& n : nbrs) hasIn.insert(n);

        int globalMax = 0;
        std::string deepestEntry;
        for (auto& [id, _] : ir.components) {
            if (!hasIn.count(id)) {
                int d = dfs(id);
                if (d > globalMax) { globalMax = d; deepestEntry = id; }
            }
        }

        if (globalMax >= depthThreshold_)
            issues.push_back(ValidationIssue::warning(
                "Longest request chain from \"" + deepestEntry + "\" has depth "
                + std::to_string(globalMax) + " (threshold: " + std::to_string(depthThreshold_)
                + "). Deep chains increase tail latency. Consider adding caching or flattening.",
                name(), {deepestEntry}));
    }
};

// ============================================================
// CapacityValidator
// ============================================================

class CapacityValidator : public IValidatorModule {
public:
    std::string name() const override { return "CapacityValidator"; }

    std::vector<ValidationIssue> validate(const DiagramIR& ir) const override {
        std::vector<ValidationIssue> issues;

        for (auto& [id, comp] : ir.components) {
            auto cap = comp.getNum("capacity");
            if (cap && *cap == 0.0)
                issues.push_back(ValidationIssue::error(
                    "Component \"" + id + "\" has capacity = 0. "
                    "It will never process requests. Set a positive capacity or remove it.",
                    name(), {id}));

            auto qs = comp.getNum("queue_size");
            if (qs && *qs < 0.0)
                issues.push_back(ValidationIssue::error(
                    "Component \"" + id + "\" has negative queue_size (" + std::to_string(*qs) + ").",
                    name(), {id}));

            auto tp = comp.getNum("throughput");
            if (tp && *tp < 0.0)
                issues.push_back(ValidationIssue::error(
                    "Component \"" + id + "\" has negative throughput (" + std::to_string(*tp) + ").",
                    name(), {id}));

            if (comp.type == ComponentType::CACHE) {
                auto hr = comp.getNum("hit_rate");
                if (hr) {
                    if (*hr == 0.0)
                        issues.push_back(ValidationIssue::warning(
                            "CACHE \"" + id + "\" has hit_rate = 0 — the cache never serves a hit. "
                            "All requests fall through to the backend.", name(), {id}));
                    else if (*hr == 1.0)
                        issues.push_back(ValidationIssue::warning(
                            "CACHE \"" + id + "\" has hit_rate = 1 — the backend will never receive "
                            "requests and is effectively dead code.", name(), {id}));
                }
            }

            auto er = comp.getNum("error_rate");
            if (er && *er == 1.0)
                issues.push_back(ValidationIssue::warning(
                    "Component \"" + id + "\" has error_rate = 1.0 (always fails). "
                    "All downstream components will be starved of successful requests.",
                    name(), {id}));

            if (comp.type == ComponentType::SERVICE) {
                auto lat = comp.getNum("latency");
                if (lat && *lat > 10000.0)
                    issues.push_back(ValidationIssue::warning(
                        "SERVICE \"" + id + "\" has very high latency ("
                        + std::to_string(static_cast<int>(*lat)) + " ms). "
                        "This will dominate tail latency in any chain through it.",
                        name(), {id}));
            }

            if (comp.type == ComponentType::DATABASE) {
                auto rf = comp.getNum("replication_factor");
                if (rf && *rf == 1.0)
                    issues.push_back(ValidationIssue::warning(
                        "DATABASE \"" + id + "\" has replication_factor = 1 (no replicas). "
                        "This is a single point of failure. Consider replication_factor >= 2.",
                        name(), {id}));
            }
        }

        return issues;
    }
};

// ============================================================
// DesignAdvisor
// ============================================================

class DesignAdvisor : public IValidatorModule {
public:
    std::string name() const override { return "DesignAdvisor"; }

    std::vector<ValidationIssue> validate(const DiagramIR& ir) const override {
        std::vector<ValidationIssue> issues;
        if (ir.components.empty()) return issues;

        AdjSet out, in;
        buildAdj(ir, out, in);

        // Service -> Database with no cache
        for (auto& [id, comp] : ir.components) {
            if (comp.type != ComponentType::SERVICE) continue;
            for (auto& tgt : out.at(id))
                if (ir.components.count(tgt) && ir.components.at(tgt).type == ComponentType::DATABASE)
                    issues.push_back(ValidationIssue::warning(
                        "SERVICE \"" + id + "\" connects directly to DATABASE \"" + tgt
                        + "\" without a CACHE. Every read hits the database.",
                        name(), {id, tgt}));
        }

        // Single-instance service with no fronting LB
        for (auto& [id, comp] : ir.components) {
            if (comp.type != ComponentType::SERVICE) continue;
            auto inst = comp.getNum("instances");
            if (!inst || *inst > 1) continue;
            bool hasFrontingLB = false;
            for (auto& caller : in.at(id))
                if (ir.components.count(caller) && ir.components.at(caller).type == ComponentType::LOAD_BALANCER)
                    { hasFrontingLB = true; break; }
            if (!hasFrontingLB)
                issues.push_back(ValidationIssue::warning(
                    "SERVICE \"" + id + "\" has instances = 1 and no upstream LOAD_BALANCER. "
                    "This is a single point of failure (SPOF).",
                    name(), {id}));
        }

        // LB with only one backend
        for (auto& [id, comp] : ir.components)
            if (comp.type == ComponentType::LOAD_BALANCER && out.at(id).size() == 1)
                issues.push_back(ValidationIssue::warning(
                    "LOAD_BALANCER \"" + id + "\" has only one backend — "
                    "no load distribution benefit. Add backends or remove the LB.",
                    name(), {id}));

        // No cache in system
        bool hasDB = false, hasCache = false;
        for (auto& [_, comp] : ir.components) {
            if (comp.type == ComponentType::DATABASE) hasDB = true;
            if (comp.type == ComponentType::CACHE)    hasCache = true;
        }
        if (hasDB && !hasCache)
            issues.push_back(ValidationIssue::warning(
                "The system has no CACHE components. All reads hit the database directly.",
                name()));

        // No LB with multiple services
        int svcCount = 0;
        bool hasLB = false;
        for (auto& [_, comp] : ir.components) {
            if (comp.type == ComponentType::SERVICE)       ++svcCount;
            if (comp.type == ComponentType::LOAD_BALANCER) hasLB = true;
        }
        if (svcCount >= 2 && !hasLB)
            issues.push_back(ValidationIssue::warning(
                "The system has " + std::to_string(svcCount)
                + " SERVICE components but no LOAD_BALANCER. "
                "Horizontal scaling will have no effect without one.",
                name()));

        // Shared-database anti-pattern
        for (auto& [id, comp] : ir.components) {
            if (comp.type != ComponentType::DATABASE) continue;
            std::vector<std::string> directCallers;
            for (auto& caller : in.at(id))
                if (ir.components.count(caller) && ir.components.at(caller).type == ComponentType::SERVICE)
                    directCallers.push_back(caller);
            if (directCallers.size() >= 2) {
                std::string list;
                for (auto& c : directCallers) list += "\"" + c + "\" ";
                issues.push_back(ValidationIssue::warning(
                    "DATABASE \"" + id + "\" is called directly by "
                    + std::to_string(directCallers.size()) + " SERVICE(s): " + list
                    + "— shared-database anti-pattern. Route via a single service or use separate DBs.",
                    name(), directCallers));
            }
        }

        // Mutual service dependency
        std::unordered_set<std::string> reported;
        for (auto& [id, comp] : ir.components) {
            if (comp.type != ComponentType::SERVICE) continue;
            for (auto& tgt : out.at(id)) {
                if (!ir.components.count(tgt)) continue;
                if (ir.components.at(tgt).type != ComponentType::SERVICE) continue;
                if (out.count(tgt) && out.at(tgt).count(id)) {
                    std::string key = id < tgt ? id + ":" + tgt : tgt + ":" + id;
                    if (!reported.insert(key).second) continue;
                    issues.push_back(ValidationIssue::warning(
                        "SERVICE \"" + id + "\" and SERVICE \"" + tgt + "\" call each other "
                        "(mutual dependency). This tight coupling risks cascading failures.",
                        name(), {id, tgt}));
                }
            }
        }

        return issues;
    }
};

// ============================================================
// Validator — orchestrator
// ============================================================

Validator Validator::createDefault(int depthThreshold) {
    Validator v;
    v.addModule(std::make_unique<StructuralValidator>());
    v.addModule(std::make_unique<ConfigValidator>());
    v.addModule(std::make_unique<SemanticValidator>());
    v.addModule(std::make_unique<TopologyValidator>(depthThreshold));
    v.addModule(std::make_unique<CapacityValidator>());
    v.addModule(std::make_unique<DesignAdvisor>());
    return v;
}

ValidationResult Validator::validate(const DiagramIR& ir) const {
    ValidationResult result;
    result.canProceed = true;
    for (auto& module : modules_) {
        for (auto& issue : module->validate(ir)) {
            if (issue.severity == Severity::ERROR) result.canProceed = false;
            result.issues.push_back(std::move(issue));
        }
    }
    return result;
}

void Validator::printResult(const ValidationResult& r, std::ostream& os) {
    os << "\n=== SimRUN Validator Report ===\n\n";
    if (r.issues.empty()) {
        os << "  No issues found. IR is valid.\n";
    } else {
        for (auto& issue : r.issues) {
            os << "  [" << severityStr(issue.severity) << "] " << issue.message << "\n";
            if (!issue.related_components.empty()) {
                os << "    Components : ";
                for (auto& c : issue.related_components) os << "\"" << c << "\" ";
                os << "\n";
            }
            if (!issue.related_connections.empty()) {
                os << "    Connections: ";
                for (auto& c : issue.related_connections) os << "\"" << c << "\" ";
                os << "\n";
            }
            os << "    Module     : " << issue.source_module << "\n\n";
        }
    }
    os << "------------------------------\n";
    os << "  Errors  : " << r.errorCount()   << "\n";
    os << "  Warnings: " << r.warningCount() << "\n";
    os << "  Status  : " << (r.canProceed ? "PASS - safe to proceed" : "FAIL - pipeline halted") << "\n\n";
}

} // namespace simrun

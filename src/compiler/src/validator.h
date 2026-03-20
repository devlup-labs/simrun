#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <iostream>

namespace simrun {

// ---- IR ----

enum class ComponentType { SERVICE, CACHE, DATABASE, LOAD_BALANCER, UNKNOWN };

inline std::string componentTypeStr(ComponentType t) {
    switch (t) {
        case ComponentType::SERVICE:       return "SERVICE";
        case ComponentType::CACHE:         return "CACHE";
        case ComponentType::DATABASE:      return "DATABASE";
        case ComponentType::LOAD_BALANCER: return "LOAD_BALANCER";
        default:                           return "UNKNOWN";
    }
}

struct Component {
    std::string id;
    ComponentType type = ComponentType::UNKNOWN;
    std::map<std::string, double>      numericConfig;
    std::map<std::string, std::string> stringConfig;

    std::optional<double>      getNum(const std::string& k) const {
        auto it = numericConfig.find(k);
        return it != numericConfig.end() ? std::optional<double>(it->second) : std::nullopt;
    }
    std::optional<std::string> getStr(const std::string& k) const {
        auto it = stringConfig.find(k);
        return it != stringConfig.end() ? std::optional<std::string>(it->second) : std::nullopt;
    }
};

struct Connection {
    std::string id;
    std::string from_component_id;
    std::string to_component_id;
};

struct DiagramIR {
    std::map<std::string, Component> components;
    std::vector<Connection>          connections;
};

// ---- Validation output ----

enum class Severity { ERROR, WARNING, INFO };

inline std::string severityStr(Severity s) {
    switch (s) {
        case Severity::ERROR:   return "ERROR";
        case Severity::WARNING: return "WARNING";
        default:                return "INFO";
    }
}

struct ValidationIssue {
    Severity                 severity;
    std::string              message;
    std::vector<std::string> related_components;
    std::vector<std::string> related_connections;
    std::string              source_module;

    static ValidationIssue error(const std::string& msg, const std::string& mod,
        std::vector<std::string> comps = {}, std::vector<std::string> conns = {})
    { return { Severity::ERROR, msg, std::move(comps), std::move(conns), mod }; }

    static ValidationIssue warning(const std::string& msg, const std::string& mod,
        std::vector<std::string> comps = {}, std::vector<std::string> conns = {})
    { return { Severity::WARNING, msg, std::move(comps), std::move(conns), mod }; }
};

struct ValidationResult {
    std::vector<ValidationIssue> issues;
    bool canProceed = true;

    bool hasErrors()   const { for (auto& i:issues) if (i.severity==Severity::ERROR)   return true; return false; }
    bool hasWarnings() const { for (auto& i:issues) if (i.severity==Severity::WARNING) return true; return false; }
    std::size_t errorCount()   const { std::size_t n=0; for (auto& i:issues) if (i.severity==Severity::ERROR)   ++n; return n; }
    std::size_t warningCount() const { std::size_t n=0; for (auto& i:issues) if (i.severity==Severity::WARNING) ++n; return n; }
};

// ---- Module interface ----

class IValidatorModule {
public:
    virtual ~IValidatorModule() = default;
    virtual std::vector<ValidationIssue> validate(const DiagramIR&) const = 0;
    virtual std::string name() const = 0;
};

// ---- Top-level Validator ----

class Validator {
public:
    void addModule(std::unique_ptr<IValidatorModule> m) { modules_.push_back(std::move(m)); }

    static Validator createDefault(int depthThreshold = 8);
    ValidationResult validate(const DiagramIR& ir) const;
    static void printResult(const ValidationResult& r, std::ostream& os = std::cout);

private:
    std::vector<std::unique_ptr<IValidatorModule>> modules_;
};

} // namespace simrun

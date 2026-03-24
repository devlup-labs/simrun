#pragma once

#include <string>
#include <vector>

#include "simrun/ir/ir_schema.h"

namespace simrun {

struct ValidationResult {
    bool ok = true;
    std::vector<std::string> errors{};
};

ValidationResult ValidateIR(const SimulationIR& ir);

}  // namespace simrun

#pragma once

#include <string>

#include "ir_schema.h"

namespace sim::ir {

SimulationIR parse_ir_file(const std::string& path);

}  // namespace sim::ir

#pragma once

#include <string>
#include <string_view>

#include "simrun/ir/ir_schema.h"

namespace simrun {

SimulationIR ParseSimulationIRJson(std::string_view json_text);
SimulationIR LoadSimulationIRFromJsonFile(const std::string& path);

}  // namespace simrun

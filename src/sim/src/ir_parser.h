#pragma once
#include <string>
#include "ir_types.h"

class IRParser {
public:
    static IR parseFromFile(const std::string& path);
};

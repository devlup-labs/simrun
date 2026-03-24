#pragma once

#include <array>
#include <cstdint>

namespace simrun {

enum class OpCode : std::uint8_t {
    kHalt = 0,
    kSetFlag = 1,
    kIncrementHop = 2,
    kWritePayloadWord = 3,
    kAddComponentStateWord = 4,
    kScheduleTimer = 5,
    kSpawnRequest = 6,
    kInvalidateToken = 7,
    kRouteNextLink = 8,
    kLog = 9,
    kPreemptCurrent = 10,
};

struct Instruction {
    OpCode opcode = OpCode::kHalt;
    std::array<std::int64_t, 4> args{0, 0, 0, 0};
};

}  // namespace simrun

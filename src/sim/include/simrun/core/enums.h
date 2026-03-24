#pragma once

#include <cstdint>

namespace simrun {

enum class LocationType : std::uint8_t {
    kInvalid = 0,
    kComponentService = 1,
    kComponentQueue = 2,
    kLinkQueue = 3,
    kExecution = 4,
};

enum class TargetType : std::uint8_t {
    kComponent = 0,
    kLink = 1,
    kGlobal = 2,
};

enum class EventPhase : std::int16_t {
    kRequestArrival = 0,
    kServiceCompletion = 1,
    kTimerExpiration = 2,
    kResponseArrival = 3,
    kWorkloadGeneration = 4,
};

enum class TokenState : std::uint8_t {
    kInvalid = 0,
    kActive = 1,
};

enum class QueueDiscipline : std::uint8_t {
    kFifo = 0,
};

}  // namespace simrun

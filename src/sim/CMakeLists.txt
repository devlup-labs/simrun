cmake_minimum_required(VERSION 3.20)
project(simrun LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

set(SIMRUN_SOURCES
  main.cpp
  engine/core/scheduler.cpp
  engine/core/simulator.cpp
  engine/ir/ir_parser.cpp
  engine/factory/context_factory.cpp
  engine/factory/state_factory.cpp
  engine/factory/event_factory.cpp
  engine/behavior/behavior_executor.cpp
  engine/events/dispatch.cpp
)

add_executable(simrun ${SIMRUN_SOURCES})
target_link_libraries(simrun PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(simrun PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
add_executable(simrun_tests
  tests/simrun_tests.cpp
  engine/core/scheduler.cpp
  engine/core/simulator.cpp
  engine/ir/ir_parser.cpp
  engine/factory/context_factory.cpp
  engine/factory/state_factory.cpp
  engine/factory/event_factory.cpp
  engine/behavior/behavior_executor.cpp
  engine/events/dispatch.cpp
)
target_link_libraries(simrun_tests PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(simrun_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(
  NAME simrun_tests
  COMMAND simrun_tests ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

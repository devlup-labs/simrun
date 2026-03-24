cmake_minimum_required(VERSION 3.23)

project(simrun LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIMRUN_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SIMRUN_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(SIMRUN_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sanitizers)

add_library(simrun_engine STATIC
    src/core/sim_context.cpp
    src/core/simulator.cpp
    src/ir/json_ir_parser.cpp
    src/ir/ir_validation.cpp
    src/runtime/request_table.cpp
    src/runtime/token_table.cpp
    src/scheduler/scheduler.cpp
    src/entities/component_runtime.cpp
    src/entities/link_runtime.cpp
    src/behavior/behavior_executor.cpp
    src/stats/stats_runtime.cpp
)

target_include_directories(simrun_engine
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(MSVC)
    target_compile_options(simrun_engine PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(simrun_engine PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

simrun_configure_sanitizers(simrun_engine)

add_executable(simrun_tests
    tests/simrun_tests.cpp
)

target_link_libraries(simrun_tests PRIVATE simrun_engine)

if(MSVC)
    target_compile_options(simrun_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(simrun_tests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

simrun_configure_sanitizers(simrun_tests)

enable_testing()
add_test(NAME simrun_tests COMMAND simrun_tests)

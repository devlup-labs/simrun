cmake_minimum_required(VERSION 3.15)
project(compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------
# Find packages (vcpkg CONFIG mode)
# ---------------------------
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
# Uncomment if this project also uses Crow
# find_package(Crow CONFIG REQUIRED)

# ---------------------------
# Source files
# ---------------------------
set(SOURCES
    src/server.cpp
    src/compiler_driver.cpp
    src/ir_parser.cpp
    src/validator.cpp
    src/profile_repository.cpp
    src/profile_resolver.cpp
    src/ir_serializer.cpp
)

# ---------------------------
# Create executable
# ---------------------------
add_executable(compiler ${SOURCES})
# Put the final .exe in the project source/compiler folder
set_target_properties(compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}"
)


# ---------------------------
# Link libraries
# ---------------------------
target_link_libraries(compiler
    PRIVATE
    nlohmann_json::nlohmann_json
    yaml-cpp
    # Crow::Crow   # Uncomment if using Crow
)

# ---------------------------
# Include directories
# ---------------------------
target_include_directories(compiler
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------
# Output directory
# ---------------------------
set_target_properties(compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ---------------------------
# Compiler warnings
# ---------------------------
if(MSVC)
    target_compile_options(compiler PRIVATE /W4)
else()
    target_compile_options(compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

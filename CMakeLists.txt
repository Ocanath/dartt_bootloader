cmake_minimum_required(VERSION 3.22)
project(dartt_flashing_tool C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/dartt-protocol)
add_subdirectory(external/serial-cross-platform)
add_subdirectory(external/byte-stuffing)

add_executable(dartt_flashing_tool
    flashing_tool/src/main.cpp
	flashing_tool/src/args.cpp
)

target_include_directories(dartt_flashing_tool PRIVATE
    flashing_tool/src
    external/dartt-protocol/src
	external/byte-stuffing/C-COBS
	shared
	external/ELFIO
)

target_link_libraries(dartt_flashing_tool PRIVATE
    dartt_protocol
    serial
    cobs
)

add_custom_target(generate_version ALL
    COMMAND ruby ${CMAKE_SOURCE_DIR}/scripts/run_get_version.rb
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts
    COMMENT "Generating version.h"
)

add_dependencies(dartt_flashing_tool generate_version)

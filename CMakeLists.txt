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
)

target_link_libraries(dartt_flashing_tool PRIVATE
    dartt_protocol
    serial
    cobs
)

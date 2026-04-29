cmake_minimum_required(VERSION 3.22)
project(dartt_flash C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/dartt-protocol)
add_subdirectory(external/byte-stuffing)
add_subdirectory(external/serial-cross-platform)

add_executable(dartt_flash
    flashing_tool/src/main.cpp
	flashing_tool/src/args.cpp
	flashing_tool/src/dartt_flasher.cpp
	flashing_tool/src/callbacks.cpp
	flashing_tool/src/milliseconds.cpp
	flashing_tool/src/stubs.c
	shared/dartt_bl.c
)

target_include_directories(dartt_flash PRIVATE
    flashing_tool/src
    external/dartt-protocol/src
	external/byte-stuffing/C-COBS
	# external/ELFIO
	shared
)

target_link_libraries(dartt_flash PRIVATE
    dartt_protocol
	dartt_checksum
    cobs
	PPP
	bytestream
    serial
)

add_custom_target(generate_version ALL
    COMMAND ruby ${CMAKE_SOURCE_DIR}/scripts/run_get_version.rb
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts
    COMMENT "Generating version.h"
)

add_dependencies(dartt_flash generate_version)

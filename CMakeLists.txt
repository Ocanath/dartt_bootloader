cmake_minimum_required(VERSION 3.22)
project(dartt_flash C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET dartt_protocol)
    add_subdirectory(external/dartt-protocol)
endif()
if(NOT TARGET cobs)
    add_subdirectory(external/byte-stuffing)
endif()
if(NOT TARGET serial)
    add_subdirectory(external/serial-cross-platform)
endif()

add_library(dartt_flasher_lib STATIC
    flashing_tool/src/dartt_flasher.cpp
    flashing_tool/src/callbacks.cpp
    flashing_tool/src/milliseconds.cpp
    flashing_tool/src/stubs.c
    shared/dartt_bl.c
)

target_include_directories(dartt_flasher_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/flashing_tool/src
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
)

target_link_libraries(dartt_flasher_lib PUBLIC
    dartt_protocol
    dartt_checksum
    cobs
    PPP
    bytestream
    serial
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_executable(dartt_flash
        flashing_tool/src/main.cpp
        flashing_tool/src/args.cpp
    )

    target_link_libraries(dartt_flash PRIVATE dartt_flasher_lib)

    add_custom_target(generate_version ALL
        COMMAND ruby ${CMAKE_SOURCE_DIR}/scripts/run_get_version.rb
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts
        COMMENT "Generating version.h"
    )

    add_dependencies(dartt_flash generate_version)
endif()

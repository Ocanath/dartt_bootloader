# ARM Cortex-M bare-metal toolchain for arm-none-eabi-gcc
#
# Requires arm-none-eabi-gcc to be on PATH.
# Download: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
#   Windows: installer or zip, add <install>/bin to PATH
#   Ubuntu:  sudo apt install gcc-arm-none-eabi

set(CMAKE_SYSTEM_NAME      Generic)   # bare-metal, no OS
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc     REQUIRED)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++     REQUIRED)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc     REQUIRED)
find_program(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy REQUIRED)
find_program(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size    REQUIRED)

# Without this CMake tries to link a test executable, which fails for bare-metal
# targets because there is no OS runtime or standard entry point.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

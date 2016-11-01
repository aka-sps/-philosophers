# Cross-compilation setup
#
# This file is passed to cmake on the command line via
# -DCMAKE_TOOLCHAIN_FILE.  It gets read first, prior to any of cmake's
# system tests.
#
# This is the place to configure your cross-compiling environment.
# An example for using the gumstix arm-linux toolchain is given below.
# Uncomment to try it out.
#
set(TARGET_PREFIX riscv32-unknown-linux-gnu-)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER ${TARGET_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TARGET_PREFIX}g++)
set(CMAKE_FIND_ROOT_PATH /opt/riscv32)
set(CMAKE_EXE_LINKER_FLAGS "-static -pthread" CACHE STRING "Flags used by the linker")

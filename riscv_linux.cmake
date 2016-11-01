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
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_FIND_ROOT_PATH /opt/riscv32)
set(TARGET_PREFIX riscv32-unknown-linux-gnu-)
set(CMAKE_SYSTEM_PROCESSOR riscv32)
set(CMAKE_C_COMPILER /opt/riscv32/bin/${TARGET_PREFIX}gcc)
set(CMAKE_CXX_COMPILER /opt/riscv32/bin/${TARGET_PREFIX}g++)
# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# set(CMAKE_EXE_LINKER_FLAGS -static CACHE STRING "Flags used by the linker")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

#
# Freestanding x86_64 kernel toolchain (replaces CrossFiles/kernel-target-clang-x86_64.cross-file)
#
# C++20 modules are required, which in turn requires clang >= 17 together with
# `clang-scan-deps` and CMake >= 3.28 driving Ninja.
#
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_AR           llvm-ar)
set(CMAKE_RANLIB       llvm-ranlib)
set(CMAKE_STRIP        llvm-strip)
set(CMAKE_NM           llvm-nm)
set(CMAKE_OBJCOPY      llvm-objcopy)

set(CMAKE_C_COMPILER_TARGET   x86_64-linux-elf)
set(CMAKE_CXX_COMPILER_TARGET x86_64-linux-elf)
set(CMAKE_ASM_COMPILER_TARGET x86_64-linux-elf)

include(${CMAKE_CURRENT_LIST_DIR}/common.cmake)

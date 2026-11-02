#
# Freestanding aarch64 kernel toolchain (replaces CrossFiles/kernel-target-clang-aarch64.cross-file)
#
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_AR           llvm-ar)
set(CMAKE_RANLIB       llvm-ranlib)
set(CMAKE_STRIP        llvm-strip)
set(CMAKE_NM           llvm-nm)
set(CMAKE_OBJCOPY      llvm-objcopy)

set(CMAKE_C_COMPILER_TARGET   aarch64-none-elf)
set(CMAKE_CXX_COMPILER_TARGET aarch64-none-elf)
set(CMAKE_ASM_COMPILER_TARGET aarch64-none-elf)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

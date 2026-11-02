#
# Settings shared by the kernel toolchain files.
#

# CMake needs `clang-scan-deps` to work out the order in which the module units have to be
# compiled.  Many distributions only ship it with a version suffix.
if(NOT CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS)
  find_program(CRYPTIX_CLANG_SCAN_DEPS
    NAMES clang-scan-deps clang-scan-deps-21 clang-scan-deps-20 clang-scan-deps-19
          clang-scan-deps-18 clang-scan-deps-17)
  if(CRYPTIX_CLANG_SCAN_DEPS)
    set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "${CRYPTIX_CLANG_SCAN_DEPS}" CACHE FILEPATH
        "clang-scan-deps used for C++ module dependency scanning")
  endif()
endif()

# There is no hosted environment to link a test program against.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

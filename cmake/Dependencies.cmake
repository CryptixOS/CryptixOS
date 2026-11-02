#
# Third party dependencies (replaces subprojects/*.wrap).
#
# Every dependency is fetched with FetchContent.  To build offline, or against a
# local checkout, point CMake at it with the standard override, e.g.
#
#   cmake ... -DFETCHCONTENT_SOURCE_DIR_UACPI=/path/to/uACPI
#
include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# --- fmt (header only; stripped down by Meta/fmt-10.2.0.patch) ----------------
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.2.0
  GIT_SHALLOW    TRUE
  PATCH_COMMAND  patch -p1 -N -s -i ${PROJECT_SOURCE_DIR}/Meta/fmt-10.2.0.patch
  UPDATE_DISCONNECTED TRUE
)

# --- magic_enum ---------------------------------------------------------------
FetchContent_Declare(magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.9.5
  GIT_SHALLOW    TRUE
)

# --- uACPI --------------------------------------------------------------------
FetchContent_Declare(uacpi
  GIT_REPOSITORY https://github.com/UltraOS/uACPI.git
  GIT_TAG        b36af9df8ea6b49142389a232ed0c2fd50209e2b
)

# --- Limine (protocol header + bootloader binaries) ---------------------------
FetchContent_Declare(limine
  GIT_REPOSITORY https://github.com/limine-bootloader/limine.git
  GIT_TAG        v9.x-binary
  GIT_SHALLOW    TRUE
)

# --- small utility libraries --------------------------------------------------
FetchContent_Declare(nostd_string
  GIT_REPOSITORY https://github.com/ilobilo/string.git
  GIT_TAG        07e01c7258a48dd0fe9b53c482212e81ae543e37
)
FetchContent_Declare(demangler
  GIT_REPOSITORY https://github.com/ilobilo/demangler.git
  GIT_TAG        0717ca6ad7b234786199a752e0c6d14dd54970ff
)
FetchContent_Declare(compiler_rt_builtins
  GIT_REPOSITORY https://github.com/ilobilo/compiler-rt-builtins.git
  GIT_TAG        c0c56bf5cfd0ffbc9ce14074e4410b1d40bea6ed
)

# --- freestanding C++ standard library headers --------------------------------
FetchContent_Declare(freestnd_cxx_hdrs
  GIT_REPOSITORY https://codeberg.org/osdev/freestnd-cxx-hdrs.git
  GIT_TAG        trunk
  GIT_SHALLOW    TRUE
)

# --- OVMF firmware images (only needed for `run_uefi*`) -----------------------
FetchContent_Declare(ovmf_binaries
  GIT_REPOSITORY https://github.com/ilobilo/ovmf-binaries.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)

# These projects ship their own (Meson / CMake) build files, which we do not use.
foreach(dep fmt magic_enum uacpi limine nostd_string demangler compiler_rt_builtins
            freestnd_cxx_hdrs ovmf_binaries)
  FetchContent_GetProperties(${dep})
  if(NOT ${dep}_POPULATED)
    FetchContent_Populate(${dep})
  endif()
endforeach()

# ------------------------------------------------------------------------------
# Imported targets
# ------------------------------------------------------------------------------
# Headers only.
add_library(cryptix_headers INTERFACE)
target_include_directories(cryptix_headers SYSTEM INTERFACE
  ${fmt_SOURCE_DIR}/include
  ${magic_enum_SOURCE_DIR}/include
  ${limine_SOURCE_DIR}
  ${uacpi_SOURCE_DIR}/include
  ${nostd_string_SOURCE_DIR}/include
  ${demangler_SOURCE_DIR}/include
  ${freestnd_cxx_hdrs_SOURCE_DIR}/${CRYPTIX_FREESTND_ARCH}/include
)

# uACPI is plain C.
file(GLOB UACPI_SOURCES CONFIGURE_DEPENDS ${uacpi_SOURCE_DIR}/source/*.c)
add_library(uacpi OBJECT ${UACPI_SOURCES})
target_include_directories(uacpi PUBLIC ${uacpi_SOURCE_DIR}/include)
target_link_libraries(uacpi PRIVATE cryptix_c_flags cryptix_includes cryptix_headers cryptix_kernel_arch)

# The Itanium demangler used for stack traces.
add_library(demangler OBJECT
  ${demangler_SOURCE_DIR}/source/ItaniumDemangle.cpp
  ${demangler_SOURCE_DIR}/source/cxa_demangle.cpp
  ${demangler_SOURCE_DIR}/source/Demangle.cpp
  ${demangler_SOURCE_DIR}/source/DLangDemangle.cpp
  ${demangler_SOURCE_DIR}/source/MicrosoftDemangle.cpp
  ${demangler_SOURCE_DIR}/source/MicrosoftDemangleNodes.cpp
  ${demangler_SOURCE_DIR}/source/RustDemangle.cpp
)
target_include_directories(demangler PUBLIC ${demangler_SOURCE_DIR}/include)
target_link_libraries(demangler PRIVATE cryptix_cxx_flags cryptix_includes cryptix_headers cryptix_kernel_arch)
set_target_properties(demangler PROPERTIES CXX_SCAN_FOR_MODULES OFF)

include(${CMAKE_CURRENT_LIST_DIR}/CompilerRT.cmake)

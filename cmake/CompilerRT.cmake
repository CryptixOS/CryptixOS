#
# compiler-rt builtins (replaces the `compiler_rt_builtins` Meson subproject).
#
# The file lists below mirror subprojects/compiler_rt_builtins/meson.build at the
# revision pinned in Dependencies.cmake.  Paths are relative to the checkout.
#
set(COMPILER_RT_GENERIC_SOURCES
  builtins/absvdi2.c
  builtins/absvsi2.c
  builtins/absvti2.c
  builtins/adddf3.c
  builtins/addsf3.c
  builtins/addtf3.c
  builtins/addvdi3.c
  builtins/addvsi3.c
  builtins/addvti3.c
  builtins/apple_versioning.c
  builtins/ashldi3.c
  builtins/ashlti3.c
  builtins/ashrdi3.c
  builtins/ashrti3.c
  builtins/clear_cache.c
  builtins/clzti2.c
  builtins/cmpdi2.c
  builtins/cmpti2.c
  builtins/comparedf2.c
  builtins/ctzdi2.c
  builtins/ctzsi2.c
  builtins/ctzti2.c
  builtins/divdc3.c
  builtins/divdf3.c
  builtins/divdi3.c
  builtins/divmoddi4.c
  builtins/divsc3.c
  builtins/divsf3.c
  builtins/divtc3.c
  builtins/divti3.c
  builtins/divtf3.c
  builtins/extendsfdf2.c
  builtins/extendhfsf2.c
  builtins/ffsdi2.c
  builtins/ffssi2.c
  builtins/ffsti2.c
  builtins/fixdfdi.c
  builtins/fixdfsi.c
  builtins/fixdfti.c
  builtins/fixsfdi.c
  builtins/fixsfsi.c
  builtins/fixsfti.c
  builtins/fixunsdfdi.c
  builtins/fixunsdfsi.c
  builtins/fixunsdfti.c
  builtins/fixunssfdi.c
  builtins/fixunssfsi.c
  builtins/fixunssfti.c
  builtins/floatsidf.c
  builtins/floatsisf.c
  builtins/floattidf.c
  builtins/floattisf.c
  builtins/floatunsidf.c
  builtins/floatunsisf.c
  builtins/floatuntidf.c
  builtins/floatuntisf.c
  builtins/int_util.c
  builtins/lshrdi3.c
  builtins/lshrti3.c
  builtins/moddi3.c
  builtins/modti3.c
  builtins/muldc3.c
  builtins/muldf3.c
  builtins/muldi3.c
  builtins/mulodi4.c
  builtins/mulosi4.c
  builtins/muloti4.c
  builtins/mulsc3.c
  builtins/mulsf3.c
  builtins/multi3.c
  builtins/multf3.c
  builtins/mulvdi3.c
  builtins/mulvsi3.c
  builtins/mulvti3.c
  builtins/negdf2.c
  builtins/negdi2.c
  builtins/negsf2.c
  builtins/negti2.c
  builtins/negvdi2.c
  builtins/negvsi2.c
  builtins/negvti2.c
  builtins/paritydi2.c
  builtins/paritysi2.c
  builtins/parityti2.c
  builtins/popcountdi2.c
  builtins/popcountsi2.c
  builtins/popcountti2.c
  builtins/powidf2.c
  builtins/powisf2.c
  builtins/powitf2.c
  builtins/subdf3.c
  builtins/subsf3.c
  builtins/subvdi3.c
  builtins/subvsi3.c
  builtins/subvti3.c
  builtins/subtf3.c
  builtins/trampoline_setup.c
  builtins/truncdfhf2.c
  builtins/truncdfsf2.c
  builtins/truncsfhf2.c
  builtins/ucmpdi2.c
  builtins/ucmpti2.c
  builtins/udivdi3.c
  builtins/udivmoddi4.c
  builtins/udivmodti4.c
  builtins/udivti3.c
  builtins/umoddi3.c
  builtins/umodti3.c
)

set(COMPILER_RT_GENERIC_NONARM_SOURCES
  builtins/bswapdi2.c
  builtins/bswapsi2.c
  builtins/clzdi2.c
  builtins/clzsi2.c
  builtins/comparesf2.c
  builtins/divmodsi4.c
  builtins/divsi3.c
  builtins/modsi3.c
  builtins/udivmodsi4.c
  builtins/udivsi3.c
  builtins/umodsi3.c
)

set(COMPILER_RT_GENERIC_NONX86_64_SOURCES
  builtins/floatdidf.c
  builtins/floatdisf.c
  builtins/floatundidf.c
  builtins/floatundisf.c
)

set(COMPILER_RT_GENERIC_TF_SOURCES
  builtins/comparetf2.c
  builtins/extenddftf2.c
  builtins/extendsftf2.c
  builtins/fixtfdi.c
  builtins/fixtfsi.c
  builtins/fixtfti.c
  builtins/fixunstfdi.c
  builtins/fixunstfsi.c
  builtins/fixunstfti.c
  builtins/floatditf.c
  builtins/floatsitf.c
  builtins/floattitf.c
  builtins/floatunditf.c
  builtins/floatunsitf.c
  builtins/floatuntitf.c
  builtins/multc3.c
  builtins/trunctfdf2.c
  builtins/trunctfsf2.c
)

set(COMPILER_RT_X86_FILES
  builtins/divxc3.c
  builtins/fixxfdi.c
  builtins/fixxfti.c
  builtins/fixunsxfdi.c
  builtins/fixunsxfsi.c
  builtins/fixunsxfti.c
  builtins/floatdixf.c
  builtins/floattixf.c
  builtins/floatundixf.c
  builtins/floatuntixf.c
  builtins/mulxc3.c
  builtins/powixf2.c
)

set(COMPILER_RT_X86_64_FILES
  builtins/x86_64/floatdidf.c
  builtins/x86_64/floatdisf.c
  builtins/x86_64/floatdixf.c
  builtins/x86_64/floatundidf.S
  builtins/x86_64/floatundisf.S
  builtins/x86_64/floatundixf.S
)

set(COMPILER_RT_AARCH64_FILES
  builtins/aarch64/fp_mode.c
)

set(_rt_sources ${COMPILER_RT_GENERIC_SOURCES})
if(CRYPTIX_ARCH STREQUAL "x86_64")
  list(APPEND _rt_sources ${COMPILER_RT_X86_FILES} ${COMPILER_RT_X86_64_FILES}
                          ${COMPILER_RT_GENERIC_NONARM_SOURCES})
elseif(CRYPTIX_ARCH STREQUAL "aarch64")
  list(APPEND _rt_sources ${COMPILER_RT_AARCH64_FILES} ${COMPILER_RT_GENERIC_TF_SOURCES}
                          ${COMPILER_RT_GENERIC_NONX86_64_SOURCES}
                          ${COMPILER_RT_GENERIC_NONARM_SOURCES})
endif()
list(TRANSFORM _rt_sources PREPEND "${compiler_rt_builtins_SOURCE_DIR}/")

add_library(compiler_rt_builtins STATIC ${_rt_sources})
target_compile_options(compiler_rt_builtins PRIVATE
  -fno-builtin -fvisibility=hidden -fomit-frame-pointer -ffreestanding
  -Wno-pedantic -Wno-undef -Wno-sign-conversion -Wno-double-promotion -Wno-float-equal
  -Wno-float-conversion -Wno-conversion -Wno-missing-noreturn -Wno-unused-parameter
  -Wno-format-nonliteral -Wno-unused-macros -Wno-shift-sign-overflow -Wno-shorten-64-to-32
  -Wno-unreachable-code-break -Wno-conditional-uninitialized -Wno-missing-variable-declarations
  -Wno-reserved-id-macro -Wno-missing-prototypes
  -D__ELF__)
# The builtins are built against the kernel's own libc headers.
target_include_directories(compiler_rt_builtins PRIVATE ${PROJECT_SOURCE_DIR}/Kernel/Library/KLibC)
target_link_libraries(compiler_rt_builtins PRIVATE cryptix_c_flags)

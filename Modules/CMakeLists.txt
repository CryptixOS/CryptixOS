#
# Created by v1tr10l7 on 04.03.2025.
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#
# Kernel modules.  They are compiled against the kernel's module interface (`import Cryptix;`);
# a module that has headers of its own is a small named module itself (Cryptix.Modules.<Name>).
#
#   EXTERNAL   linked into a stand-alone <name>.ko that the kernel loads at run time
#   BUILTIN    compiled into the kernel image
#
set(CRYPTIX_MODULE_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)

function(cryptix_add_module name)
  cmake_parse_arguments(MOD "BUILTIN" "" "SOURCES;INTERFACES;LINK" ${ARGN})

  add_library(${name} OBJECT)
  target_sources(${name} PRIVATE ${MOD_SOURCES})
  if(MOD_INTERFACES)
    target_sources(${name} PUBLIC FILE_SET CXX_MODULES FILES ${MOD_INTERFACES})
  endif()
  # Linking the kernel library only provides the module interface (BMI) and usage requirements; the
  # kernel's own object files are not pulled into the module.
  target_link_libraries(${name} PUBLIC cryptix_kernel ${MOD_LINK})

  if(MOD_BUILTIN)
    target_link_libraries(${name} PRIVATE cryptix_kernel_arch)
    target_link_libraries(Cryptix.elf PRIVATE ${name})
    return()
  endif()

  target_compile_options(${name} PRIVATE
    -fPIC -fsanitize=undefined -fmacro-backtrace-limit=0 ${CRYPTIX_MODULE_ARCH_FLAGS})

  set(ko ${CMAKE_CURRENT_BINARY_DIR}/${name}.ko)
  add_custom_command(
    OUTPUT  ${ko}
    COMMAND ld.lld -znoexecstack -shared -T ${CRYPTIX_MODULE_LINKER_SCRIPT}
            $<TARGET_OBJECTS:${name}> -o ${ko}
    DEPENDS ${name} $<TARGET_OBJECTS:${name}> ${CRYPTIX_MODULE_LINKER_SCRIPT}
    COMMAND_EXPAND_LISTS VERBATIM)
  add_custom_target(${name}_ko ALL DEPENDS ${ko})
  set_property(GLOBAL APPEND PROPERTY CRYPTIX_MODULE_FILES   ${ko})
  set_property(GLOBAL APPEND PROPERTY CRYPTIX_MODULE_TARGETS ${name}_ko)
endfunction()

# --- serio: the bus both PS/2 style drivers talk to --------------------------------------------------
cryptix_add_module(serio
  SOURCES    serio/main.cpp
  INTERFACES serio/SerioController.cppm)

# --- input -----------------------------------------------------------------------------------------
if(CRYPTIX_ARCH STREQUAL "x86_64")
  cryptix_add_module(i8042
    SOURCES    x86_64/input/i8042/main.cpp
    INTERFACES x86_64/input/i8042/I8042.cppm
               x86_64/input/i8042/PNP_IDs.cppm
    LINK       serio)
endif()

set(_atkbd_link serio)
if(CRYPTIX_ARCH STREQUAL "x86_64")
  list(APPEND _atkbd_link i8042)
endif()
cryptix_add_module(atkbd
  SOURCES    input/atkbd/main.cpp
  INTERFACES input/atkbd/AtKeyboard.cppm
             input/atkbd/Scancodes.cppm
  LINK       ${_atkbd_link})

# --- x86_64 only ------------------------------------------------------------------------------------
if(CRYPTIX_ARCH STREQUAL "x86_64")
  cryptix_add_module(pcspk SOURCES x86_64/pcspk/main.cpp)
endif()

# --- network ----------------------------------------------------------------------------------------
cryptix_add_module(e1000e SOURCES noarch/net/e1000e/main.cpp)
cryptix_add_module(rtl8139 BUILTIN
  SOURCES    noarch/net/rtl8139/main.cpp
  INTERFACES noarch/net/rtl8139/RTL8139.cppm)

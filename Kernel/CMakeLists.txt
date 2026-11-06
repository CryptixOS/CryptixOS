#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
include(${CMAKE_SOURCE_DIR}/CMake/ConfigureMesonTemplate.cmake)

# ==============================================================================
### Sources, in the same order as meson.build, so the link order (and with it
### the order of global constructors) is the same.
set(CTOS_PRISM_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Prism/Source)
set(CTOS_PRISM_SOURCES)
foreach(src
    Algorithm/Hash.cpp

    Containers/Bitmap.cpp
    Containers/RingBuffer.cpp

    Debug/Assertions.cpp
    Debug/Log.cpp
    Debug/Logger.cpp
    Debug/LogSink.cpp
    Debug/Ubsan.cpp
    Debug/Stacktrace.cpp

    Memory/Memory.cpp
    String/Printf.cpp

    Utility/Path.cpp
    Utility/PathView.cpp
    Utility/Time.cpp
)
    list(APPEND CTOS_PRISM_SOURCES ${CTOS_PRISM_SOURCE_DIR}/Prism/${src})
endforeach()

set(CTOS_KERNEL_SOURCES)
list(APPEND CTOS_KERNEL_SOURCES
    KernelStart.cpp

    Network/NetworkAdapter.cpp

    API/MM.cpp
    API/Process.cpp
    API/Syscall.cpp
    API/System.cpp
    API/Time.cpp
    API/VFS.cpp

    Arch/PowerManager.cpp
    Arch/User.cpp
)
if(CTOS_ARCH STREQUAL "x86_64")
    list(APPEND CTOS_KERNEL_SOURCES
        Arch/x86_64/Arch.cpp
        Arch/x86_64/CPU.cpp
        Arch/x86_64/CMOS.cpp
        Arch/x86_64/GDT.cpp
        Arch/x86_64/IDT.cpp
        Arch/x86_64/Syscall.cpp
        Arch/x86_64/VMM.cpp
        Arch/x86_64/idt.S
        Arch/x86_64/memory.S
        Arch/x86_64/signal_trampoline.S
        Arch/x86_64/syscall.S
        Arch/x86_64/Drivers/IoApic.cpp
        Arch/x86_64/Drivers/PCI.cpp
        Arch/x86_64/Drivers/PCSpeaker.cpp
        Arch/x86_64/Drivers/PIC.cpp
        Arch/x86_64/Drivers/Serial.cpp
        Arch/x86_64/Drivers/Time/HPET.cpp
        Arch/x86_64/Drivers/Time/KVMClock.cpp
        Arch/x86_64/Drivers/Time/Lapic.cpp
        Arch/x86_64/Drivers/Time/PIT.cpp
        Arch/x86_64/Drivers/Time/RTC.cpp
    )
endif()
if(CTOS_ARCH STREQUAL "aarch64")
    list(APPEND CTOS_KERNEL_SOURCES
        Arch/aarch64/Arch.cpp
        Arch/aarch64/Exceptions.cpp
        Arch/aarch64/CPU.cpp
        Arch/aarch64/Syscall.cpp
        Arch/aarch64/VMM.cpp
        Arch/aarch64/interrupts.S
        Arch/aarch64/signal_trampoline.S
        Arch/aarch64/Drivers/GIC.cpp
        Arch/aarch64/Drivers/Serial.cpp
    )
endif()
list(APPEND CTOS_KERNEL_SOURCES
    Boot/BootInfo.cpp
    Boot/CommandLine.cpp

    Debug/Kasan.cpp
    Debug/Panic.cpp

    Drivers/KernelMessageDevice.cpp
    Drivers/ACPI/Bus.cpp
    Drivers/ACPI/Device.cpp
    Drivers/Core/CharacterDevice.cpp
    Drivers/Core/Device.cpp
    Drivers/Core/DeviceManager.cpp
    Drivers/Core/GenericDriver.cpp
    Drivers/Input/InputDevice.cpp
    Drivers/PCI/Access.cpp
    Drivers/PCI/Bus.cpp
    Drivers/PCI/Device.cpp
    Drivers/PCI/HostController.cpp
    Drivers/PCI/PCI.cpp
    Drivers/Storage/PartitionTable.cpp
    Drivers/Storage/StorageDevice.cpp
    Drivers/Storage/StorageDevicePartition.cpp
    Drivers/Storage/NVMe/NVMeController.cpp
    Drivers/Storage/NVMe/NVMeNameSpace.cpp
    Drivers/Storage/NVMe/NVMeQueue.cpp
    Drivers/USB/USB.cpp
    Drivers/USB/UHCI/UHCiController.cpp
    Drivers/TTY/MasterPTY.cpp
    Drivers/TTY/PTY.cpp
    Drivers/TTY/PTYMultiplexer.cpp
    Drivers/TTY/SlavePTY.cpp
    Drivers/TTY/TTY.cpp
    Drivers/TTY/VirtualConsole.cpp
    Drivers/Video/FramebufferDevice.cpp
    Drivers/Video/FramebufferConsole.cpp

    Firmware/ACPI/ACPI.cpp
    Firmware/ACPI/MADT.cpp
    Firmware/ACPI/UACPI.cpp
    Firmware/ACPI/Interpreter/Interpreter.cpp
    Firmware/ACPI/Interpreter/NameSpace.cpp
    Firmware/DMI/SMBIOS.cpp
    Firmware/DeviceTree/DeviceTree.cpp
    Firmware/DeviceTree/Node.cpp
    Firmware/EFI/Initialize.cpp

    Library/ExecutableProgram.cpp
    Library/ICxxAbi.cpp
    Library/Image.cpp
    Library/KernelObject.cpp
    Library/Logger.cpp
    Library/Module.cpp
    Library/Stacktrace.cpp
    Library/ZLib.cpp
    Library/ELF/Image.cpp
    Library/ELF/Loader.cpp
    Library/KLibC/assert.cpp
    Library/KLibC/ctype.cpp
    Library/KLibC/errno.cpp
    Library/KLibC/stdio.cpp
    Library/KLibC/stdlib.cpp
    Library/KLibC/string.cpp
    Library/Locking/ConditionalVariable.cpp
    Library/Locking/Mutex.cpp
    Library/Locking/RecursiveMutex.cpp
    Library/Locking/Semaphore.cpp
    Library/Locking/SequenceLock.cpp
    Library/Locking/Spinlock.cpp

    Memory/AddressSpace.cpp
    Memory/MM.cpp
    Memory/PMM.cpp
    Memory/PageMap.cpp
    Memory/Region.cpp
    Memory/ScopedMapping.cpp
    Memory/VMM.cpp
    Memory/Allocator/BitmapAllocator.cpp
    Memory/Allocator/BuddyAllocator.cpp
    Memory/Allocator/KernelHeap.cpp

    Network/LocalSocket.cpp
    Network/NetLinkSocket.cpp
    Network/Socket.cpp

    Scheduler/Event.cpp
    Scheduler/Process.cpp
    Scheduler/Scheduler.cpp
    Scheduler/Thread.cpp

    System/InterruptManager.cpp
    System/System.cpp

    Time/Time.cpp
    Time/Timer.cpp

    VFS/DirectoryEntry.cpp
    VFS/Pipe.cpp
    VFS/File.cpp
    VFS/FileDescriptor.cpp
    VFS/FileDescriptorTable.cpp
    VFS/INode.cpp
    VFS/MountPoint.cpp
    VFS/PathResolver.cpp
    VFS/SynthFsINode.cpp
    VFS/VFS.cpp
    VFS/DevPtsFs/DevPtsFs.cpp
    VFS/DevPtsFs/DevPtsFsINode.cpp
    VFS/DevTmpFs/DevTmpFs.cpp
    VFS/DevTmpFs/DevTmpFsINode.cpp
    VFS/EchFs/EchFs.cpp
    VFS/EchFs/EchFsINode.cpp
    VFS/Ext2Fs/Ext2Fs.cpp
    VFS/Ext2Fs/Ext2FsAllocator.cpp
    VFS/Ext2Fs/Ext2FsINode.cpp
    VFS/Fat32Fs/Fat32Fs.cpp
    VFS/Fat32Fs/Fat32FsINode.cpp
    VFS/Initrd/Initrd.cpp
    VFS/Initrd/Ustar.cpp
    VFS/ProcFs/ProcFs.cpp
    VFS/ProcFs/ProcFsINode.cpp
    VFS/TmpFs/TmpFs.cpp
    VFS/TmpFs/TmpFsINode.cpp
)

# ==============================================================================
### Generated headers
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_tag
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(kernel_name cryptix)
set(CTOS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CTOS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CTOS_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(arch ${CTOS_ARCH})
string(TOLOWER ${CMAKE_CXX_COMPILER_ID} compiler)
if(compiler STREQUAL "gnu")
    set(compiler gcc)
endif()
set(compiler_version ${CMAKE_CXX_COMPILER_VERSION})
set(kernel_version_string "${PROJECT_VERSION}-${git_tag}")
ctos_configure_meson_template(Version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Version.hpp)

set(KERNEL_VIRTUAL_RANGE_BASE "0xffff'0000'0000'0000zu")
# Kernel virtual allocations end where the kernel image starts.
set(KERNEL_VIRTUAL_RANGE_TOP "0xffff'ffff'8000'0000zu")
set(USER_VIRTUAL_RANGE_BASE "0x0000'0000'0010'0000zu")
set(USER_VIRTUAL_RANGE_TOP "0x0000'7fff'ffff'ffffzu")
set(CTOS_ACPI_DISABLE 0)
set(CTOS_DEVICE_TREE_DISABLE 0)
ctos_configure_meson_template(Config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Config.hpp)

set(CTOS_MM_DUMP_MEMMAP 0)
set(CTOS_PCI_DUMP_IRQ_ROUTES 0)
set(CTOS_DTB_DUMP 0)
set(CTOS_EFI_DUMP_MEMMAP 0)
set(CTOS_TTY_LOG_TERMIOS 0)
set(CTOS_DUMP_INIT_ARRAY 1)
set(CTOS_ELF_DEBUG 0)
set(CTOS_DEVICE_TREE_DUMP 0)
ctos_configure_meson_template(Debug/Debug.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/Debug/Debug.hpp)

# ==============================================================================
### Compiler arguments
set(CTOS_KERNEL_C_ARGS)
set(CTOS_KERNEL_CXX_ARGS)
check_cxx_compiler_flag(-Wno-changes-meaning CTOS_HAS_WNO_CHANGES_MEANING)
if(CTOS_HAS_WNO_CHANGES_MEANING)
    list(APPEND CTOS_KERNEL_C_ARGS -Wglobal-constructors)
    list(APPEND CTOS_KERNEL_CXX_ARGS -Wglobal-constructors)
endif()

set(CTOS_KERNEL_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.${CTOS_ARCH}.ld)
set(CTOS_KERNEL_LINK_ARGS
    -nostdlib
    -static
    -pie
    -Wl,--fatal-warnings
    -Wl,--as-needed
    -Wl,--no-undefined
    -Wl,-pie
    -Wl,-z,max-page-size=0x1000
    -Wl,-T,${CTOS_KERNEL_LINKER_SCRIPT}
)

if(CTOS_ARCH STREQUAL "x86_64")
    list(APPEND CTOS_KERNEL_CXX_ARGS -DCTOS_ARCH=CTOS_ARCH_X86_64 -DCTOS_TARGET_X86_64)
    list(APPEND CTOS_KERNEL_C_ARGS
        -march=x86-64
        -msoft-float
        -mno-mmx
        -mno-sse
        -mno-sse2
        -mno-red-zone
        -mcmodel=kernel
    )
    list(APPEND CTOS_KERNEL_CXX_ARGS
        -march=x86-64
        -m64
        -msoft-float
        -mno-mmx
        -mno-sse
        -mno-sse2
        -mno-red-zone
        -mno-avx
        -mcmodel=kernel
        -mrdrnd
        -mrdseed
    )
    list(APPEND CTOS_KERNEL_LINK_ARGS -Wl,-z,noexecstack)
elseif(CTOS_ARCH STREQUAL "aarch64")
    list(APPEND CTOS_KERNEL_CXX_ARGS -DCTOS_ARCH=CTOS_ARCH_AARCH64 -DCTOS_TARGET_AARCH64)
    set(args -mgeneral-regs-only -march=armv8-a+nofp+nosimd -mcmodel=small)
    list(APPEND CTOS_KERNEL_C_ARGS ${args})
    list(APPEND CTOS_KERNEL_CXX_ARGS ${args})
endif()

# ==============================================================================
### C++ modules. Their interface units are compiled once, with the kernel's
### flags, and imported by the kernel and by the .ko modules.
set(CTOS_KERNEL_INCLUDES
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Library/KLibC
    ${CMAKE_CURRENT_SOURCE_DIR}/Library/KLibC++
    ${CTOS_PRISM_SOURCE_DIR}
    ${CTOS_DEPENDENCY_INCLUDES}
)

add_library(KernelModules OBJECT)
target_sources(KernelModules
    PUBLIC FILE_SET CXX_MODULES FILES
        Arch/PowerManager.cppm
)
target_include_directories(KernelModules PRIVATE ${CTOS_KERNEL_INCLUDES})
target_compile_options(KernelModules PRIVATE -fPIE ${CTOS_KERNEL_CXX_ARGS})

# ==============================================================================
### Cryptix.elf
add_executable(Kernel
    ${CTOS_PRISM_SOURCES}
    ${CTOS_KERNEL_SOURCES}
    ${CTOS_DEPENDENCY_SOURCES}
    ${CTOS_BUILTIN_MODULE_SOURCES}
)
set_target_properties(Kernel PROPERTIES
    OUTPUT_NAME Cryptix.elf
    SUFFIX ""
    LINK_DEPENDS ${CTOS_KERNEL_LINKER_SCRIPT}
)
target_include_directories(Kernel PRIVATE ${CTOS_KERNEL_INCLUDES})
target_compile_options(Kernel PRIVATE
    -fPIE
    "$<$<COMPILE_LANGUAGE:C,ASM>:${CTOS_KERNEL_C_ARGS}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${CTOS_KERNEL_CXX_ARGS}>"
)
target_precompile_headers(Kernel PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Library/PrecompiledHeader.hpp>"
)
target_link_options(Kernel PRIVATE ${CTOS_KERNEL_LINK_ARGS})
target_link_libraries(Kernel PRIVATE KernelModules compiler_rt_builtins)

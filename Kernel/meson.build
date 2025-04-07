#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'KernelStart.cpp',

  'API/MM.cpp',
  'API/Process.cpp',
  'API/Syscall.cpp',
  'API/SyscallEntryPoints.cpp',
  'API/System.cpp',
  'API/Time.cpp',
  'API/VFS.cpp',

  'Arch/PowerManager.cpp',

  'Boot/BootInfo.cpp',
  'Boot/CommandLine.cpp',
  'Debug/Panic.cpp',

  'Drivers/Terminal.cpp',
  'Drivers/TTY.cpp',

  'Drivers/HID/Ps2KeyboardDevice.cpp',

  'Drivers/PCI/Access.cpp',
  'Drivers/PCI/Bus.cpp',
  'Drivers/PCI/Device.cpp',
  'Drivers/PCI/PCI.cpp',
  'Drivers/PCI/HostController.cpp',

  'Drivers/Storage/PartitionTable.cpp',
  'Drivers/Storage/StorageDevice.cpp',

  'Drivers/Storage/NVMe/NVMeController.cpp',
  'Drivers/Storage/NVMe/NVMeNameSpace.cpp',
  'Drivers/Storage/NVMe/NVMeQueue.cpp',
  
  'Firmware/ACPI/ACPI.cpp',
  'Firmware/ACPI/MADT.cpp',
  'Firmware/ACPI/UACPI.cpp',

  'Firmware/DeviceTree/DeviceTree.cpp',

  'Firmware/SMBIOS/SMBIOS.cpp',

  'Memory/KernelHeap.cpp',
  'Memory/PMM.cpp',
  'Memory/VMM.cpp',

  'Network/NetworkAdapter.cpp',

  'Scheduler/Event.cpp',
  'Scheduler/Process.cpp',
  'Scheduler/Scheduler.cpp',
  'Scheduler/Thread.cpp',
    
  'Time/Time.cpp',

  'Library/ELF.cpp',
  'Library/ICxxAbi.cpp',
  'Library/Kasan.cpp',
  'Library/Logger.cpp',
  'Library/Module.cpp',
  'Library/Stacktrace.cpp',
  'Library/ZLib.cpp',

  'VFS/Fifo.cpp',
  'VFS/FileDescriptor.cpp',
  'VFS/FileDescriptorTable.cpp',
  'VFS/INode.cpp',
  'VFS/VFS.cpp',

  'VFS/DevTmpFs/DevTmpFs.cpp',
  'VFS/DevTmpFs/DevTmpFsINode.cpp',

  'VFS/EchFs/EchFs.cpp',
  'VFS/EchFs/EchFsINode.cpp',

  'VFS/Ext2Fs/Ext2Fs.cpp',
  'VFS/Ext2Fs/Ext2FsINode.cpp',

  'VFS/Fat32Fs/Fat32Fs.cpp',
  'VFS/Fat32Fs/Fat32FsINode.cpp',

  'VFS/Initrd/Initrd.cpp',
  'VFS/Initrd/Ustar.cpp',

  'VFS/ProcFs/ProcFs.cpp',
  'VFS/ProcFs/ProcFsINode.cpp',

  'VFS/TmpFs/TmpFs.cpp',
  'VFS/TmpFs/TmpFsINode.cpp',
)

c_args = []
cxx_args = []

ld_args = [
  '-nostdlib',
  '-static',
  '-fuse-ld=lld',
  '-Wl,-pie',
  '-Wl,-z,max-page-size=0x1000',
  '-Wl,-T,' + meson.current_source_dir() + '/linker.' + arch + '.ld',
]

args = []
if arch == 'x86_64'
  cxx_args += ['-DCTOS_ARCH=CTOS_ARCH_X86_64', '-DCTOS_TARGET_X86_64']
  subdir('Arch/x86_64')
elif arch == 'aarch64'
  subdir('Arch/aarch64')
  cxx_args += ['-DCTOS_ARCH=CTOS_ARCH_AARCH64', '-DCTOS_TARGET_AARCH64']
else
  error('Unknown architecture: ' + arch)
endif

subdir('Library/KLibC')
subdir('Library/KLibC++')

kernel_elf = executable(
  'Cryptix.elf',
  srcs,
  dependencies: [deps, mod_deps],
  link_depends: [meson.current_source_dir() / 'linker.' + arch + '.ld'],
  include_directories: [include_directories, incs],
  c_args: [c_args, args],
  cpp_args: [cxx_args, args],
  #cpp_pch: 'Library/PrecompiledHeader.hpp',
  link_args: ld_args,
  install: false,
)

#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'Arch.cpp',
  'CPU.cpp',
  'CMOS.cpp',
  'GDT.cpp',
  'IDT.cpp',
  'InterruptManager.cpp',
  'Syscall.cpp',
  'VMM.cpp',
  'idt.S',
  'memory.S',
  'syscall.S',

  'Drivers/I8042Controller.cpp',
  'Drivers/IoApic.cpp',
  'Drivers/PCI.cpp',
  'Drivers/PCSpeaker.cpp',
  'Drivers/PIC.cpp',
  'Drivers/Serial.cpp',

  'Drivers/Timers/HPET.cpp',
  'Drivers/Timers/Lapic.cpp',
  'Drivers/Timers/PIT.cpp',
  'Drivers/Timers/RTC.cpp',
)
asm_sources = files(
  'idt.S',
  'memory.S',
  'syscall.S',
)

c_args += [
  '-march=x86-64',
  '-msoft-float',
  '-mno-mmx',
  '-mno-sse',
  '-mno-sse2',
  '-mno-red-zone',
  '-mcmodel=kernel',
]
cxx_args += [
  '-march=x86-64',
  '-m64',
  '-msoft-float',
  #'-mno-80387',
  '-mno-mmx',
  '-mno-sse',
  '-mno-sse2',
  '-mno-red-zone',
  '-mno-avx',
  '-mcmodel=kernel',
  '-mrdrnd',
  '-mrdseed',
]

#foreach source : asm_sources
#  segments = source.full_path().split('/')
#  basename = segments[segments.length() - 1]
#  objs += custom_target(
#    basename,
#    input: source,
#    output: basename + '.o',
#    command: [
#      'fasm', '@INPUT@', '@OUTPUT@',
#    ],
#    build_by_default: true,
#    console: true,
#    install: false,
#  )
#endforeach
ld_args += ['-Wl,-z,noexecstack']

#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'Arch.cpp',
  'CPU.cpp',
  'GDT.cpp',
  'IDT.cpp',
  'InterruptManager.cpp',
  'Syscall.cpp',
  'VMM.cpp',
  'interrupts.x86_64.S',
  'syscall.S',

  'Drivers/PIC.cpp',
  'Drivers/Serial.cpp',

  'Drivers/Timers/PIT.cpp',
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
  #'-m64',
  '-msoft-float',
  #'-mno-80387',
  '-mno-mmx',
  '-mno-sse',
  '-mno-sse2',
  '-mno-red-zone',
  '-mno-avx',
  '-mcmodel=kernel',
  '-mrdrnd',
]

ld_args += ['-Wl,-z,noexecstack']

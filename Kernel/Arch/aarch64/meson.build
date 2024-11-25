#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'Arch.cpp',
  'InterruptManager.cpp',
  'interrupts.S',

  'Drivers/Serial.cpp'
)

ld_args += ['-Wl,-maarch64elf']
cxx_args += [
  '-mgeneral-regs-only',
  '-mcmodel=small',
]

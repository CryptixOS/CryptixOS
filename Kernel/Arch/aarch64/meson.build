#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'Arch.cpp',
  'CPU.cpp',
  'InterruptManager.cpp',
  'Syscall.cpp',
  'VMM.cpp',
  'interrupts.S',

  'Drivers/Serial.cpp'
)

#ld_args += ['-Wl,-maarch64elf']
args += [
  '-mgeneral-regs-only',
  '-march=armv8-a+nofp+nosimd',
  '-mcmodel=small',
]

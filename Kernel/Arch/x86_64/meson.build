#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'GDT.cpp',
  'Drivers/Serial.cpp'
)

cxx_args += [
  '-m64',
  '-march=x86-64',
  '-msoft-float',
  #'-mno-80387',
  '-mno-mmx',
  '-mno-sse',
  '-mno-sse2',
  '-mno-red-zone',
  '-mcmodel=kernel',
  '-mrdrnd',
]

ld_args += ['-Wl,-z,noexecstack', '-Wl,-m,elf_x86_64', '-Wl,-static']

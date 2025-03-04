#*
#* Created by v1tr10l7 on 04.03.2025.
#* Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
modules = [
  'ps2'
]

args += [
  '-march=x86_64',
  '-msoft-float',
  '-mno-mmx',
  '-mno-sse',
  '-mno-sse2',
  '-mno-red-zone',
  '-mc-model=large',
]

foreach name : modules
  external = false
  module = []
  subdir(name)
  all_modules += { name: [external, module] }
endforeach

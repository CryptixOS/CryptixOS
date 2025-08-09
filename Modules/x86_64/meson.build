#*
#* Created by v1tr10l7 on 04.03.2025.
#* Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
x86_64_modules = [
  'ps2',
  'pcspk'
]

args += [
  #'-march=x86_64',
  '-msoft-float',
  '-mno-mmx',
  '-mno-sse',
  '-mno-sse2',
  '-mno-red-zone',
  '-mcmodel=large',
]

foreach name : x86_64_modules
  external = false
  module = []
  subdir(name)
  all_modules += { name: [external, module] }
endforeach

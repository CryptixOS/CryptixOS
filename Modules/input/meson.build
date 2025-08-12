#*
#* Created by v1tr10l7 on 10.08.2025.
#* Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
modules = [
  'atkbd',
]

foreach name : modules
  external = false
  module = []
  subdir(name)
  all_modules += { name: [external, module] }
endforeach

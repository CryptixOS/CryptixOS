#*
#* Created by v1tr10l7 on 20.04.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
modules = [
  'serial_pnp',
]

foreach name : modules
  external = false
  module = []
  subdir(name)
  all_modules += { name: [external, module] }
endforeach

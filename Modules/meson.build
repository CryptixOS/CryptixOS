#*
#* Created by v1tr10l7 on 04.03.2025.
#* Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/

all_modules = {}

args = [ '-fsanitize=undefined']

if arch == 'aarch64'
  args += [
    '-mgeneral-regs-only', '-march=armv8-a+nofp+nosimd',
    '-mcmodel=small'
]
endif

ld_args = [
  '-nostdlib',
  '-static',
  '-relocatable',
]

subdir(arch)
subdir('noarch')

foreach name, mod : all_modules
  external = mod[0]
  dep = mod[1]

  if external == true
    lib = static_library(name, 
      dependencies: [inc_deps, dep],
      link_depends: [meson.current_source_dir() / 'linker.' + arch + '.ld'],
      include_directories: incs,
      c_args: args,
      cpp_args: args,
      link_args: ld_args,
      install: false,
      pic: false,
      build_by_default: true,
  )
  ext_mods += custom_target(name.split('/')[1] + '.ko',
    input: lib.extract_all_objects(recursive: true),
    output: name.split('/')[1] + '.ko',
    command: ['ld.lld', ld_args, '-T', meson.current_source_dir() / 'linker.' + arch + '.ld', '@INPUT@', '-o', '@OUTPUT@'],
    build_by_default: true,
    install: false,
    depends: lib
  )
  else
    mod_deps += dep
  endif
endforeach


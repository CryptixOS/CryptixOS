#*
#* Created by v1tr10l7 on 04.03.2025.
#* Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/

all_modules = {}

args = [ 
  '-fsanitize=undefined', 
  '-fmacro-backtrace-limit=0', 
  '-I' + meson.project_source_root(),
  # '-fno-pic',
  # '-fno-pie',
  '-fPIC',
]

if arch == 'x86_64'
  args += ['-DCTOS_ARCH=CTOS_ARCH_X86_64', '-DCTOS_TARGET_X86_64']
elif arch == 'aarch64'
  args += ['-DCTOS_ARCH=CTOS_ARCH_AARCH64', '-DCTOS_TARGET_AARCH64']
  args += [
    '-mgeneral-regs-only', '-march=armv8-a+nofp+nosimd',
    '-mcmodel=small',
]
endif

incdirs = [include_directories('.'), incs]

ld_args = [
  # '-nostdlib',
  # '-static',
  # '-relocatable',
  '-znoexecstack',
  '-shared',
  # '-Wl,--whole-archive',
  # '-fno-eliminate-unused-debug-symbols',
]

subdir(arch)
subdir('noarch')

modules = ['serio', 'input']

foreach name : modules
  external = false
  module = []
  subdir(name)
  all_modules += { name: [external, module] }
endforeach

foreach name, mod : all_modules
  external = mod[0]
  dep = mod[1]

  if external == true
    lib = static_library(name, 
      dependencies: [inc_deps, dep],
      link_depends: [meson.current_source_dir() / 'linker.ld'],
      include_directories: incdirs,
      c_args: args,
      cpp_args: args,
      link_args: ld_args,
      install: false,
      pic: false,
      build_by_default: true,
  )
  ext_mods += custom_target(name + '.ko',
    input: lib.extract_all_objects(recursive: true),
    output: name + '.ko',
    command: ['ld.lld', ld_args, '-T', meson.current_source_dir() / 'linker.ld', '@INPUT@', '-o', '@OUTPUT@'],
    build_by_default: true,
    install: false,
    depends: lib
  )
  else
    mod_deps += dep
  endif
endforeach


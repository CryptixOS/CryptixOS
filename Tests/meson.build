#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# Host-side unit tests, run with Meta/run-tests.sh (and in CI):
#  - Prism's own suite (containers, allocators, strings, Path/PathView ...),
#    built from the Prism submodule with -Dbuild_tests=true
#  - kernel code that doesn't need a running kernel, compiled for the host
#    against Prism with the headers in Kernel/Shims standing in for the
#    kernel's logger and assertions
project('cryptix-tests', ['c', 'cpp'],
  version: 'v0.0.1', license: 'GPL-3.0',
  default_options: [
    'c_std=c17',
    'cpp_std=c++23',
    'warning_level=2',
    'werror=false',
    'buildtype=debug',
    'b_sanitize=address,undefined',
  ],
)

prism = subproject('prism', default_options: [
  'target=app',
  'build_tests=true',
])
prism_dep = prism.get_variable('prism_dep')
prism_lib = prism.get_variable('prism')

# The shims come first so they win over the real kernel headers
kernel_test_incs = include_directories('Kernel/Shims', '../Kernel')
kernel_test_args = [
  '-DPRISM_USE_NAMESPACE=1',
  '-Wno-unused-parameter',
]
cxx = meson.get_compiler('cpp')
# The kernel builds with these off too (see the top-level meson.build)
foreach arg : ['-Wno-changes-meaning']
  if cxx.has_argument(arg)
    kernel_test_args += arg
  endif
endforeach

python = find_program('python3')

zlib_vectors = custom_target(
  'zlib_vectors',
  input: 'Kernel/ZLib/gen-vectors.py',
  output: 'Vectors.hpp',
  command: [python, '@INPUT@', '@OUTPUT@'],
)
zlib_test = executable(
  'ZLib',
  ['Kernel/ZLib/main.cpp', '../Kernel/Library/ZLib.cpp', zlib_vectors],
  cpp_args: kernel_test_args,
  include_directories: kernel_test_incs,
  link_with: prism_lib,
  dependencies: prism_dep,
)
test('ZLib', zlib_test, suite: 'Kernel', timeout: 120)

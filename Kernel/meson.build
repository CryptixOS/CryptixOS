#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'KernelStart.cpp',

  'Network/NetworkAdapter.cpp',
)

c_args = []
cxx_args = []

ld_args = [
  '-nostdlib',
  '-static',
  '-fuse-ld=lld',
  '-Wl,-pie',
  '-Wl,-z,max-page-size=0x1000',
  '-Wl,-T,' + meson.current_source_dir() + '/linker.' + arch + '.ld',
]

args = []
objs = []
if arch == 'x86_64'
  cxx_args += ['-DCTOS_ARCH=CTOS_ARCH_X86_64', '-DCTOS_TARGET_X86_64']
elif arch == 'aarch64'
  cxx_args += ['-DCTOS_ARCH=CTOS_ARCH_AARCH64', '-DCTOS_TARGET_AARCH64']
else
  error('Unknown architecture: ' + arch)
endif


subdir('API')
subdir('Arch')
subdir('Boot')
subdir('Debug')
subdir('Drivers')
subdir('Firmware')
subdir('Library')
subdir('Memory')
subdir('Scheduler')
subdir('System')
subdir('Time')
subdir('VFS')

git_tag = run_command('git', 'rev-parse', 'HEAD').stdout().strip()

version = meson.project_version().substring(1).split('.')
versionConf = configuration_data()
versionConf.set('kernel_name', 'cryptix')
versionConf.set('CTOS_VERSION_MAJOR', version[0])
versionConf.set('CTOS_VERSION_MINOR', version[1])
versionConf.set('CTOS_VERSION_PATCH', version[2])
versionConf.set('git_tag', git_tag)
versionConf.set('arch', arch)
versionConf.set('compiler', cxx.get_id())
versionConf.set('compiler_version', cxx.version())

kernel_version_string_template = '@0@.@1@.@2@-@3@'
versionConf.set('kernel_version_string', kernel_version_string_template.format(version[0], version[1], version[2], git_tag))

configure_file(
  input: 'Version.hpp.in', 
  output: 'Version.hpp',
  configuration: versionConf)

config = configuration_data()
config.set('KERNEL_VIRTUAL_RANGE_BASE', '0xffff\'0000\'0000\'0000zu')
config.set('KERNEL_VIRTUAL_RANGE_TOP', '0x6a9e\'7f5c\'8000\'0000zu')

config.set('USER_VIRTUAL_RANGE_BASE', '0x0000\'0000\'0010\'0000zu')
config.set('USER_VIRTUAL_RANGE_TOP', '0x0000\'7fff\'ffff\'ffffzu')

config.set('CTOS_ACPI_DISABLE', '0')

configure_file(
  input: 'Config.hpp.in',
  output: 'Config.hpp',
  configuration: config
)

kernel_elf = executable(
  'Cryptix.elf',
  [srcs, objs],
  dependencies: [deps, mod_deps],
  link_depends: [meson.current_source_dir() / 'linker.' + arch + '.ld'],
  include_directories: [include_directories, incs, include_directories('.')],
  c_args: [c_args, args],
  cpp_args: [cxx_args, args],
  cpp_pch: 'Library/PrecompiledHeader.hpp',
  link_args: [ld_args],
  install: false,
)

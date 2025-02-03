#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
project(
  'CryptixOS',
  ['c', 'cpp'],
  version: 'v0.0.1',
  license: 'GPL-3.0',
  default_options: [
    'c_std=c17',
    'cpp_std=c++23',
    'warning_level=2',
    'werror=true',
  ],
)

c = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
ld = cxx.get_linker_id()
arch = host_machine.cpu_family()
qemulog = meson.project_source_root() / 'qemu.log'

srcs = []
initrd_dir = meson.project_source_root() / 'sysroot'

include_directories = [
  include_directories('Kernel'),
]

# ==============================================================================
### Compiler arguments
warnings = [
  '-Wno-error=#warnings',
  '-Wno-builtin-macro-redefined',
  '-Wno-macro-redefined',
  '-Wno-nan-infinity-disabled',
  '-Wno-deprecated-builtins',
]

add_global_arguments(
  [
    warnings,
    '-ffreestanding',
    '-fno-omit-frame-pointer',
    '-fno-lto',
    '-fsanitize=undefined',
    #'-fsanitize=kernel-address',
    '-fstack-protector-all',
    '-mstack-protector-guard=global',
    '-fno-PIC',
    '-fPIE',
    '-O3',
    '-ffast-math',
    '-nostdlib',
    '-static',
  ],
  language: ['c', 'cpp'],
)

if not meson.get_compiler('cpp').has_argument('-std=gnu++23')
  error('C++23 support is needed. Please install clang 17 or higher.')
endif

add_global_arguments(
  [
    '-fno-rtti',
    '-fno-exceptions',
    '-fsized-deallocation',
    '-Wno-unused-parameter',
    '-Wno-non-virtual-dtor',
    '-Wno-frame-address',
    '-D__cplusplus=202302L',
    '-D__cpp_concepts=202207L',
    '-DFMT_STATIC_THOUSANDS_SEPARATOR=1',
    '-DFMT_USE_LONG_DOUBLE=0',
    '-DFMT_USE_DOUBLE=0',
    '-DFMT_USE_FLOAT=0',
    '-DMAGIC_ENUM_RANGE_MAX=0x8a00',
    '-MP',
  ],
  language: 'cpp',
)

# ==============================================================================
### Dependencies
cmake = import('cmake')
phmap_opts = cmake.subproject_options()
phmap_opts.add_cmake_defines(
  {
    'PHMAP_BUILD_EXAMPLES': false,
    'PHMAP_BUILD_TESTS': false,
    'PHMAP_INSTALL': false,
  },
)

limine = subproject('limine', default_options: 'arch=' + arch)
deps = [
  cmake.subproject('parallel_hashmap', options: phmap_opts).dependency('phmap'),

  dependency('limine'),
  dependency('flanterm'),
  #dependency('freestnd-cxx-hdrs'),
  dependency('libstdcxx_headers'),
  dependency(
    'compiler_rt_builtins',
    default_options: 'custom_includes=' + meson.project_source_root() / 'Kernel/Utility/KLibC',
  ),
  dependency('smart_ptr'),
  dependency('veque'),
  dependency('fmt'),
  dependency('string'),
  dependency('demangler'),
  dependency('magic_enum'),
  dependency('uacpi'),
]

inc_deps = []
foreach dep : deps
  inc_deps += dep.partial_dependency(includes: true)
endforeach

subdir('Kernel')

# ==============================================================================
### Qemu args
qemu_args = [
  '-cpu', 'max',
  '-net', 'none',
  '-serial', 'stdio',
  '-d', 'int,guest_errors',
  '-smp', '4',
  '-d', 'cpu_reset',
  '-m', '2G',
  '-rtc', 'base=localtime',
  '-no-shutdown', '-no-reboot',
  '-D', meson.project_source_root() / 'qemu.log',
  '-boot', 'order=d,menu=on,splash-time=100',
  '-audiodev', 'id=audio,driver=alsa',
  '-M', 'accel=kvm:hvf:whpx:haxm:tcg',
  '-smbios', 'type=1',
  '-device', 'nvme,id=nvme-ctrl1,serial=nvme',
]
qemu = 'qemu-system-' + arch

if arch == 'x86_64'
  qemu_args += [
    '-M', 'q35,smm=off',
    '-device', 'isa-debug-exit',
    '-debugcon', 'file:' + meson.project_source_root() / 'debug.log',
    '-enable-kvm',
    '-machine', 'pcspk-audiodev=audio',
    '-cpu', 'host',
  ]
  ovmf_id = 'X64'
  xorriso_args = [
    '-b', 'limine-bios-cd.bin',
    '-no-emul-boot',
    '-boot-load-size', '4',
    '-boot-info-table',
    '--efi-boot', 'limine-uefi-cd.bin',
    '-efi-boot-part',
    '--efi-boot-image',
    '--protective-msdos-label',
  ]

elif arch == 'aarch64'
  qemu_args += [
    '-M', 'virt',
    '-cpu', 'cortex-a72',
    '-device', 'ramfb',
    '-device', 'qemu-xhci',
    '-device', 'usb-kbd',
    '-device', 'usb-mouse'
  ]
  ovmf_id = 'AA64'
  xorriso_args = [
    '--efi-boot', 'limine-uefi-cd.bin',
    '-efi-boot-part',
    '--efi-boot-image',
    '--protective-msdos-label',
  ]
else
  error('Unknown architecture: ' + arch)
endif
# ==============================================================================

iso_dir = 'iso_root'
ksyms = custom_target(
  'ksyms',
  input: [kernel_elf],
  output: 'ksyms.sym',
  command: ['./generate_symbols.sh'],
  build_by_default: true,
  console: true,
  install: false,
  depends: [kernel_elf],
)

echfs_image = custom_target(
  'echfs_image',
  output: 'image.hdd',
  command: [
    './create_image.sh',
  ],
  build_by_default: true,
  console: true,
  install: false,
)

iso_files = [
  meson.project_source_root() / 'limine.conf',
  ksyms,
  limine.get_variable('limine_binaries'),
]

initrd = custom_target(
  'initrd',
  input: initrd_dir / 'usr' / 'sbin' / 'init',
  output: 'initrd.img.gz',
  command: ['tar', '-cvf', '@OUTPUT@', '-C', initrd_dir, '.'],
  build_by_default: true,
  console: true,
  install: false,
)
iso = custom_target(
  'iso',
  input: [kernel_elf, initrd, 'limine.conf'],
  output: 'Cryptix.iso',
  command: [
    'mkdir',
    '-p', iso_dir,
    '&&',
    'cp',
    '@INPUT0@',
    iso_dir / 'Cryptix.elf',
    '&&',
    'cp',
    '@INPUT1@',
    iso_dir / 'initrd.img.gz',
    '&&',
    'cp',
    iso_files,
    iso_dir,
    '&&',
    'xorriso',
    '-as', 'mkisofs',
    xorriso_args,
    iso_dir,
    '-o', '@OUTPUT@',
    '&&',
    meson.project_source_root() / 'subprojects/limine/limine',
    'bios-install',
    '@OUTPUT@',
    '&&',
    'rm',
    '-rf', iso_dir,
  ],
  build_by_default: true,
  console: true,
  install: false,
  depends: [kernel_elf, initrd, echfs_image],
)

run_bios = custom_target(
  'run_bios',
  input: iso,
  output: 'run_bios',
  command: [qemu, qemu_args, '-cdrom', '@INPUT@'],
  build_by_default: true,
  console: true,
  install: false,
  depends: [iso],
)

ovmf_binaries = subproject('ovmf_binaries')
ovmf = ovmf_binaries.get_variable(ovmf_id)

run_uefi = custom_target(
  input: [iso],
  output: 'run_uefi',
  command: [qemu, qemu_args, '-cdrom', '@INPUT@', '-drive', 'format=raw,file=' +  'image.hdd' + ',if=none,id=nvm1', '-device', 'nvme-ns,drive=nvm1,nsid=2,bus=nvme-ctrl1', '-bios', ovmf],
  console: true,
  install: false,
  depends: iso,
)

run_uefi_gdb = custom_target(
  'run_uefi_gdb',
  input: iso,
  output: 'run_uefi_gdb',
  command: [qemu, qemu_args, '-s', '-S', '-cdrom', '@INPUT@'],
  console: true,
  install: false,
  depends: iso,
)

build = custom_target(
  'build',
  output: 'build',
  command: 'true',
  install: false,
  depends: iso,
)

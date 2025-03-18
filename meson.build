#*
#* Created by v1tr10l7 on 16.11.2024.
#* Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
project('CryptixOS', ['c', 'cpp'],
  version: 'v0.0.1', license: 'GPL-3.0',
  default_options: [
    'optimization=3',
    'warning_level=2',
    'werror=true',
    'wrap_mode=forcefallback',
    'b_lundef=true',
    'b_pie=true',
    'c_std=c17',
    'cpp_std=c++23',
  ],
)

if not meson.get_compiler('cpp').has_argument('-std=gnu++23')
  error('C++23 support is needed. Please install clang 17 or higher.')
endif


c = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
ld = cxx.get_linker_id()
arch = host_machine.cpu_family()
qemulog = meson.project_source_root() / 'qemu.log'

srcs = []
initrd_dir = meson.project_source_root() / 'build-sysroot/system-root'

include_directories = [
  include_directories('Kernel'),
]

# ==============================================================================
### Compiler arguments
warnings = [
  #'-Wno-error=#warnings',
  '-Wno-builtin-macro-redefined',
  '-Wno-macro-redefined',
  '-Wno-nan-infinity-disabled',
  '-Wno-deprecated-builtins',
  '-Wno-unused-parameter',
  '-Wno-frame-address',
]

macros = [
    '-DFMT_HEADER_ONLY',
    '-DFMT_STATIC_THOUSANDS_SEPARATOR=1',
    '-DFMT_USE_LONG_DOUBLE=0',
    '-DFMT_USE_DOUBLE=0',
    '-DFMT_USE_FLOAT=0',
    '-DFMT_USE_FLOAT128=0',
    '-DFMT_USE_EXCEPTIONS=0',
    '-DLIMINE_API_REVISION=3',
    '-DMAGIC_ENUM_NO_STREAMS=1',
    '-DMAGIC_ENUM_RANGE_MAX=0x8a00',
    '-DPRISM_BUILD_DEBUG=1',
    '-DPRISM_LOG_ENABLE=1',
    '-DPRISM_TARGET_CRYPTIX=1',
    '-DUACPI_FORMATTED_LOGGING',
    '-DFMT_BUILTIN_TYPES=0'
    '-D_LIBCPP_HAS_NO_THREADS',
]

add_global_arguments(
  [
    warnings,
    macros,
    '-ffreestanding',
    '-fno-omit-frame-pointer',
    '-fno-lto',
    '-fsanitize=undefined',
    #'-fsanitize=kernel-address',
    '-fstack-protector-all',
    '-mstack-protector-guard=global',
    '-fno-PIC',
    '-fPIE',
    '-nostdlib',
    '-static',
  ],
  language: ['c', 'cpp'],
)

add_global_arguments(
  [
    '-fsized-deallocation',
    '-D__cplusplus=202302L',
    '-D__cpp_concepts=202207L',
    '-MP',
    '-fno-exceptions',
    '-fno-rtti',
    '-Wno-non-virtual-dtor',
    '-nostdinc++'
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
    'CMAKE_TRY_COMPILE_TARGET_TYPE': 'STATIC_LIBRARY',
    'CMAKE_CROSSCOMPILING': true,
  },
)

limine = subproject('limine', default_options: 'arch=' + arch)
deps = [

  dependency(
    'compiler_rt_builtins',
    default_options: 'custom_includes=' + 
      meson.project_source_root() / 'Kernel/Library/KLibC',
  ),
  dependency('demangler'),
  dependency('flanterm'),
  dependency('fmt10'),
  #dependency('freestnd-cxx-hdrs'),
  dependency('libstdcxx_headers'),
  dependency('limine'),
  dependency('magic_enum', default_options: [
    'test=false',
  ]),
  cmake.subproject('parallel_hashmap', options: phmap_opts).dependency('phmap'),
  #dependency('prism'),
  dependency('smart_ptr'),
  dependency('string'),
  dependency('uacpi'),
  dependency('veque'),
]

inc_deps = [ ]
foreach dep : deps
    inc_deps += dep.partial_dependency(includes : true)
endforeach

mod_deps = [ ]
ext_mods = [ ]
subdir('Modules')

no_ext_mods = ext_mods.length() == 0

incs = []
subdir('Prism/Source')
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
  #'-no-shutdown', '-no-reboot',
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
    '-M', 'virt,acpi=off', 
    #'-dtb', meson.project_source_root() / 'qemu.dtb',
    '-cpu', 'max',
    '-device', 'ramfb',
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


# ==============================================================================
### Targets
iso_dir = 'iso_root'
ksyms = custom_target(
  'ksyms',
  input: [kernel_elf],
  output: 'ksyms.sym',
  command: ['./Meta/generate_symbols.sh'],
  build_by_default: true,
  console: true,
  install: false,
  depends: [kernel_elf],
)

echfs_image = custom_target(
  'echfs_image',
  output: 'image.hdd',
  command: [
    './Meta/create_image.sh',
  ],
  build_by_default: true,
  console: true,
  install: false,
)

iso_files = [
  meson.project_source_root() / 'limine.conf',
  ksyms,
  limine.get_variable('limine_binaries'),
  meson.project_source_root() / 'qemu.dtb',
]

initrd = custom_target(
  'initrd',
  input: [initrd_dir / 'usr' / 'sbin' / 'init', ext_mods],
  output: 'initrd.img.gz',
  command: [
    'rm', '-rf', initrd_dir / 'usr/lib/modules', '&&',
    'mkdir', '-p', initrd_dir / 'usr/lib/modules', '&&',
    no_ext_mods ? [] : ['cp', '@INPUT@', initrd_dir / 'usr/lib/modules/', '&&'],
    'tar', '-cvf', '@OUTPUT@', '-C', initrd_dir, '.'],
  build_by_default: true,
  console: true,
  install: false,
  depends: ext_mods
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
  input: [iso, echfs_image],
  output: 'run_uefi',
  command: [qemu, qemu_args, '-cdrom', '@INPUT0@', '-drive', 'format=raw,file=@INPUT1@,if=none,id=nvm1', '-device', 'nvme-ns,drive=nvm1,nsid=2,bus=nvme-ctrl1', '-bios', ovmf],
  console: true,
  install: false,
  depends: [iso, echfs_image]
)

run_uefi_vnc = custom_target(
  input: [iso, echfs_image],
  output: 'run_uefi_vnc',
  command: [qemu, qemu_args, '-display', 'vnc=127.0.0.1:0', '-cdrom', '@INPUT0@', '-drive', 'format=raw,file=@INPUT1@,if=none,id=nvm1', '-device', 'nvme-ns,drive=nvm1,nsid=2,bus=nvme-ctrl1', '-bios', ovmf],
  console: true,
  install: false,
  depends: [iso, echfs_image]
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

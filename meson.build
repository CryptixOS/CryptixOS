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
  '-Wno-c23-extensions',
]

macros = [
    '-DFMT_HEADER_ONLY',
    '-DFMT_STATIC_THOUSANDS_SEPARATOR=1',
    '-DFMT_USE_LONG_DOUBLE=0',
    '-DFMT_USE_DOUBLE=0',
    '-DFMT_USE_FLOAT=0',
    '-DFMT_USE_FLOAT128=0',
    '-DFMT_EXCEPTIONS=0',
    '-DLIMINE_API_REVISION=3',
    '-DMAGIC_ENUM_NO_STREAMS=1',
    '-DMAGIC_ENUM_RANGE_MAX=0x8a00',
    '-DPRISM_BUILD_DEBUG=1',
    '-DPRISM_LOG_ENABLE=1',
    '-DPRISM_TARGET_CRYPTIX=1',
    '-DUACPI_FORMATTED_LOGGING',
    '-DFMT_BUILTIN_TYPES=0',
    '-D_LIBCPP_HAS_NO_THREADS',
]

add_global_arguments(
  [
    warnings,
    macros,
    '-ffreestanding',
    # keep stack pointer in the appropriate register
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
    # pass the size argument to delete operator
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
limine = subproject('limine', default_options: 'arch=' + arch)
deps = [

  dependency(
    'compiler_rt_builtins',
    default_options: 'custom_includes=' + 
      meson.project_source_root() / 'Kernel/Library/KLibC',
  ),
  dependency('demangler'),
  dependency('fmt10'),
  dependency('libstdcxx_headers'),
  dependency('limine'),
  dependency('magic_enum', default_options: [
    'test=false',
  ]),
  dependency('string'),
  dependency('uacpi'),
]

inc_deps = [ ]
foreach dep : deps
    inc_deps += dep.partial_dependency(includes : true)
endforeach

mod_deps = [ ]
ext_mods = [ ]
incs = [include_directories('Kernel'), include_directories('Kernel/Library/KLibC'), include_directories('Kernel/Library/KLibC++')]
subdir('Prism/Source')

subdir('Modules')

subdir('Kernel')
no_ext_mods = ext_mods.length() == 0

# ==============================================================================
### Qemu args
qemu_args = [
  # CPU
  # enable all host cpu features
  '-cpu', 'max',
  # use acceleration if available
  '-M', 'accel=kvm:hvf:whpx:haxm:tcg',
  # use 4 cores
  '-smp', '4',

  # setup NUMA nodes
  '-object', 'memory-backend-ram,id=ram0,size=1024M',
  '-object', 'memory-backend-ram,id=ram1,size=1024M',
  '-numa', 'node,cpus=0-1,memdev=ram0',
  '-numa', 'node,cpus=2-3,memdev=ram1',

  # use 2G of memory
  '-m', '2G',

  # reset the cpu
  '-d', 'cpu_reset',
  # do a register dump on exception
  '-d', 'int,guest_errors',

  # don't reboot nor shutdown on triple fault
  #'-no-shutdown', '-no-reboot',
  # output qemu logs to > qemu.log
  '-D', meson.project_source_root() / 'qemu.log',
  # enable boot menu
  '-boot', 'order=d,menu=on,splash-time=100',

  # set rtc to localtime
  '-rtc', 'base=localtime',
  # disable network
  '-net', 'none',
  # add smbios table of type 1
  '-smbios', 'type=1',

  # setup audio backend
  '-audiodev', 'id=audio,driver=alsa',
  # add rtl8139 nic card
  '-nic', 'user,model=rtl8139',
  # add erst table
  #'-object', 'memory-backend-file,id=erstnvram,mem-path=acpi-erst.backing,size=0x10000,share=on',
  #'-device', 'acpi-erst,memdev=erstnvram',
  #'-device', 'virtio-mmio-device,dev=hd',

  # image drives
  '-drive', 'format=raw,file=image.hdd,if=none,id=drive1', 
  # '-drive', 'format=raw,file=image2.hdd,if=none,id=drive2', 

  # nvme0 - nvme controller
  '-device', 'nvme,id=nvme0,serial=nvme,bus=pcie.0',
  # nvme0n1 - nvme namespace of controller nvme0
  '-device', 'nvme-ns,drive=drive1,nsid=2,bus=nvme0,id=nvme0n1',

  #'-usb', '-device', 'usb-ehci,id=ehci', '-device', 'usb-kbd,bus=usb-bus.0',  

  # pci-to-pci bridge
  '-device', 'pci-bridge,id=pcie2,bus=pcie.0,addr=1e,chassis_nr=1',
  # virtio drive
  # '-device', 'virtio-blk-pci,drive=drive2,id=virtio0,bus=pcie2,addr=0x6',


  #'-device', 'virtio-serial-pci,max_ports=2',
  #'-device', 'virtconsole,chardev=vcon,name=vcon0',
  #'-chardev', 'socket,id=vcon,ipv4=on,host=localhost,port=443,server,telnet=on,wait=off',
  #'-device', 'virtio-rng-pci',
  #'-device', 'virtio-keyboard',
  #'-device', 'virtio-tablet',
  '-vga', 'std',
  #'-device', 'virtio-serial,max_ports=2',
  #'-netdev', 'tap,id=mynet0,ifname=tap0,script=no,downscript=no',
  #'-device', 'rtl8139,netdev=mynet0',
  #'-nic', 'bridge,model=rtl8139',
  #'-netdev', 'user,id=net0', '-device', 'rtl8139,netdev=net0',
  #'-object', 'filter-dump,id=filter0,netdev=net0,file=packet.pcap',
]
qemu = 'qemu-system-' + arch

if arch == 'x86_64'
  qemu_args += [
    '-M', 'q35',
    '-device', 'isa-debug-exit',
    # output port 0xe9 to the stdio
    '-debugcon', 'stdio',
    # output serial to > debug.log
    '-serial', 'file:' + meson.project_source_root() / 'debug.log',
    # enable kvm acceleration
    '-enable-kvm',
    # enable pcspeaker
    '-machine', 'pcspk-audiodev=audio',
    # use host cpu
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
    '-device', 'usb-mouse',
    '-serial', 'stdio',
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

root_image = custom_target(
  'root_image',
  input: [initrd_dir / 'usr' / 'sbin' / 'init', ext_mods],
  output: 'image.hdd',
  command: [
    './Meta/create_image.py',
    '--input', meson.project_source_root() / 'Kernel',
    '--output', '@OUTPUT@',
    '--partition-table', 'mbr',
    '--size', '728MiB',
  ],
  build_by_default: true,
  console: true,
  install: false,
)

iso_files = [
  meson.project_source_root() / 'Meta' / 'limine.conf',
  meson.project_source_root() / 'Meta' / 'fonts' / 'font.bin',
  meson.project_source_root() / 'Meta' / 'images' / 'bg2.png',
  ksyms,
  limine.get_variable('limine_binaries'),
  meson.project_source_root() / 'Meta' / 'qemu.dtb',
]

initrd = custom_target(
  'initrd',
  input: [ext_mods],
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
initrd_zlib = custom_target(
  'initrd_zlib',
  input: [initrd],
  output: 'initrd.zlib',
  command: [
    meson.project_source_root() / 'Meta' / 'compress-initrd.sh', 
    '@INPUT@', '@OUTPUT@',
  ],
  build_by_default: true,
  console: true,
  install: false,
  depends: initrd,
)

iso = custom_target(
  'iso',
  input: [
    kernel_elf, initrd, 
    meson.project_source_root() / 'Meta' / 'limine.conf', 
    meson.project_source_root() / 'Meta' / 'images' / 'bg2.png', 
    meson.project_source_root() / 'Meta' / 'fonts' / 'font.bin'],
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
    '@INPUT2@',
    iso_dir / 'limine.conf',
    '&&',
    'cp',
    '@INPUT3@',
    iso_dir / 'bg2.png',
    '&&',
    'cp',
    '@INPUT4@',
    iso_dir / 'font.bin',
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
  depends: [kernel_elf, initrd, root_image],
)

acpi_dir = meson.project_source_root() / 'Meta' / 'firmware' / 'acpi-tables'
acpi_files = []
foreach f : run_command('find', acpi_dir, '-name', '*.dat').stdout().split()
  acpi_files += files(f)
endforeach

if arch == 'x86_64'
foreach table : acpi_files 
  #qemu_args += ['-acpitable', 'file='+table.full_path()]
endforeach
endif

run_bios = custom_target(
  'run_bios',
  input: [iso, root_image],
  output: 'run_bios',
  command: [qemu, qemu_args, '-cdrom', '@INPUT0@'],
  build_by_default: true,
  console: true,
  install: false,
  depends: [iso, root_image],
)

ovmf_binaries = subproject('ovmf_binaries')
ovmf = ovmf_binaries.get_variable(ovmf_id)

run_uefi = custom_target(
  input: [iso, root_image],
  output: 'run_uefi',
  command: [qemu, qemu_args, '-cdrom', '@INPUT0@', '-bios', ovmf],
  console: true,
  install: false,
  depends: [iso, root_image]
)

run_uefi_vnc = custom_target(
  input: [iso, root_image],
  output: 'run_uefi_vnc',
  command: [qemu, qemu_args, '-display', 'vnc=127.0.0.1:0', '-cdrom', '@INPUT0@', '-bios', ovmf],
  console: true,
  install: false,
  depends: [iso, root_image]
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
run_bochs = custom_target(
  'run_bochs',
  input: iso,
  output: 'run_bochs',
  command: ['bochs', '-q', '-f', meson.project_source_root() / '.bochsrc'],
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

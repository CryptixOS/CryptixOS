#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
"""
The steps after compiling the kernel, for the CMake build: the kernel symbol
table, the initrd, the disk image, the ISO and running it in QEMU. They do the
same as the custom targets in meson.build, which keeps its own copy for the
meson build. All paths are relative to the repository root.
"""

import os
import shutil

from helix import shell
from helix.log import info, fail

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
META = os.path.join(ROOT, 'Meta')
SYSROOT = os.path.join(ROOT, 'sysroot')
SUBPROJECTS = os.path.join(ROOT, 'subprojects')


def _qemu_args(arch: str) -> list[str]:
    args = [
        # CPU
        # enable all host cpu features
        '-cpu', 'max',
        # use 4 cores
        '-smp', '4',

        # setup NUMA nodes
        '-object', 'memory-backend-ram,id=ram0,size=2048M',
        '-object', 'memory-backend-ram,id=ram1,size=1024M',
        '-numa', 'node,cpus=0-1,memdev=ram0',
        '-numa', 'node,cpus=2-3,memdev=ram1',

        # use 3G of memory
        '-m', '3G',

        # reset the cpu
        '-d', 'cpu_reset',
        # do a register dump on exception
        '-d', 'int,guest_errors',

        # output qemu logs to > qemu.log
        '-D', 'qemu.log',
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
        '-object', 'filter-dump,id=hue,netdev=breh,file=e1000.pcap',
        '-netdev', 'user,id=breh,hostfwd=tcp:127.0.0.1:8888-10.0.2.15:8888,hostfwd=tcp:127.0.0.1:8823-10.0.2.15:23',
        '-device', 'e1000,netdev=breh',

        # image drives
        '-drive', 'format=raw,file=image.hdd,if=none,id=drive1',
        # nvme0 - nvme controller
        '-device', 'nvme,id=nvme0,serial=nvme,bus=pcie.0',
        # nvme0n1 - nvme namespace of controller nvme0
        '-device', 'nvme-ns,drive=drive1,nsid=2,bus=nvme0,id=nvme0n1',

        # pci-to-pci bridge
        '-device', 'pci-bridge,id=pcie2,bus=pcie.0,addr=1e,chassis_nr=1',
        '-vga', 'std',
    ]

    if arch == 'x86_64':
        args += [
            '-M', 'q35',
            '-device', 'isa-debug-exit',
            # output port 0xe9 to the stdio
            '-debugcon', 'stdio',
            # output serial to > debug.log
            '-serial', 'file:debug.log',
            # enable kvm acceleration
            '-enable-kvm',
            # enable pcspeaker
            '-machine', 'pcspk-audiodev=audio',
            # use host cpu
            '-cpu', 'host',
            # loopback i2c device
            '-device', 'i2c-echo',
        ]
    elif arch == 'aarch64':
        args += [
            # use virt board
            '-M', 'virt',
            '-cpu', 'cortex-a72',
            # define framebuffer device
            '-device', 'ramfb',
            # define usb xhci controller
            '-device', 'qemu-xhci',
            # define usb keyboard
            '-device', 'usb-kbd',
            # define usb mouse
            '-device', 'usb-mouse',
            # output serial to stdio
            '-serial', 'stdio',
        ]
    else:
        fail(f'unknown architecture: {arch}')
    return args


def _xorriso_args(arch: str) -> list[str]:
    if arch == 'x86_64':
        return [
            '-b', 'limine-bios-cd.bin',
            '-no-emul-boot',
            '-boot-load-size', '4',
            '-boot-info-table',
            '--efi-boot', 'limine-uefi-cd.bin',
            '-efi-boot-part',
            '--efi-boot-image',
            '--protective-msdos-label',
        ]
    return [
        '--efi-boot', 'limine-uefi-cd.bin',
        '-efi-boot-part',
        '--efi-boot-image',
        '--protective-msdos-label',
    ]


def _limine_binaries(arch: str) -> list[str]:
    limine = os.path.join(SUBPROJECTS, 'limine')
    names = ['limine-uefi-cd.bin']
    if arch == 'x86_64':
        names += ['limine-bios.sys', 'limine-bios-cd.bin', 'BOOTX64.EFI', 'BOOTIA32.EFI']
    elif arch == 'aarch64':
        names += ['BOOTAA64.EFI']
    return [os.path.join(limine, name) for name in names]


def _external_modules(build_dir: str) -> list[str]:
    modules_dir = os.path.join(build_dir, 'Modules')
    if not os.path.isdir(modules_dir):
        return []
    return sorted(os.path.join(modules_dir, f) for f in os.listdir(modules_dir) if f.endswith('.ko'))


def ksyms(build_dir: str):
    info('generating ksyms.sym...')
    result = shell.run(['llvm-nm', '-n', 'Kernel/Cryptix.elf'], cwd=build_dir)
    with open(os.path.join(build_dir, 'ksyms.sym'), 'wb') as f:
        f.write(result.stdout + b'\n')


def initrd(build_dir: str):
    """Copies the external modules into the sysroot and archives it."""
    modules_dir = os.path.join(SYSROOT, 'usr', 'lib', 'modules')
    shutil.rmtree(modules_dir, ignore_errors=True)
    os.makedirs(modules_dir)
    for module in _external_modules(build_dir):
        shutil.copy(module, modules_dir)

    info('creating initrd.img.gz...')
    output = os.path.join(build_dir, 'initrd.img.gz')
    shell.run(['tar', '-cf', output, '-C', SYSROOT + '/', '.'])
    shell.run([os.path.join(META, 'compress-initrd.sh'), output,
               os.path.join(build_dir, 'initrd.zlib')])


def root_image(build_dir: str):
    shell.run([
        os.path.join(META, 'create_image.py'),
        '--input', SYSROOT,
        '--output', os.path.join(build_dir, 'image.hdd'),
        '--partition-table', 'gpt',
        '--size', '2GiB',
    ], cwd=ROOT, stream=True)


def iso(build_dir: str, arch: str):
    iso_dir = os.path.join(build_dir, 'iso_root')
    os.makedirs(iso_dir, exist_ok=True)
    files = [
        (os.path.join(build_dir, 'Kernel', 'Cryptix.elf'), 'Cryptix.elf'),
        (os.path.join(build_dir, 'initrd.img.gz'), 'initrd.img.gz'),
        (os.path.join(META, 'limine.conf'), 'limine.conf'),
        (os.path.join(META, 'images', 'bg2.png'), 'bg2.png'),
        (os.path.join(META, 'fonts', 'font.bin'), 'font.bin'),
        (os.path.join(build_dir, 'ksyms.sym'), 'ksyms.sym'),
        (os.path.join(META, 'qemu.dtb'), 'qemu.dtb'),
    ]
    files += [(path, os.path.basename(path)) for path in _limine_binaries(arch)]
    for source, name in files:
        shutil.copy(source, os.path.join(iso_dir, name))

    output = os.path.join(build_dir, 'Cryptix.iso')
    shell.run(['xorriso', '-as', 'mkisofs', *_xorriso_args(arch), iso_dir, '-o', output])
    if arch == 'x86_64':
        shell.run([os.path.join(SUBPROJECTS, 'limine', 'limine'), 'bios-install', output])
    shutil.rmtree(iso_dir)


def build_all(build_dir: str, arch: str):
    """Everything the meson build produces by default, after compiling."""
    ksyms(build_dir)
    initrd(build_dir)
    root_image(build_dir)
    iso(build_dir, arch)


def run_qemu(build_dir: str, arch: str, firmware: str):
    qemu = f'qemu-system-{arch}'
    args = [qemu, *_qemu_args(arch), '-cdrom', 'Cryptix.iso']
    if firmware == 'uefi':
        ovmf_id = 'X64' if arch == 'x86_64' else 'AA64'
        args += ['-bios', os.path.join(SUBPROJECTS, 'ovmf_binaries', f'OVMF_{ovmf_id}.fd')]
    elif firmware == 'gdb':
        args += ['-s', '-S']
    args += ['-M', 'accel=kvm:hvf:whpx:haxm:tcg'] if firmware != 'gdb' else []
    shell.try_run(args, cwd=build_dir, stream=True)

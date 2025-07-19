#!/usr/bin/env python3

import os
import sys
import argparse

from helix.log import trace, info, error, fail
from helix import shell

def setup(build_dir: str, target: str, build_type: str, cxx: str = 'gcc'):
    trace(f'🔧 Setting the build directory => `{build_dir}`')
    info(f'target architecture: {target}')
    info(f'build-type: {build_type}')
    info(f'cxx: {cxx}')

    cross_file_path = f'CrossFiles/kernel-target-{cxx}-{target}.cross-file'
    info(f'cross-file: {cross_file_path}')

    if build_type == 'dist':
        build_type = 'release'
    elif build_type == 'release':
        build_type = 'debugoptimized'
    elif build_type != 'debug':
        fail(f'invalid build type \'{build_type}\'')

    shell.run(['rm', '-rf', build_dir])
    shell.run(['meson', 'setup', build_dir, f'--cross-file={cross_file_path}', '--force-fallback-for=fmt', f'--buildtype={build_type}'])
    shell.run(['mkdir', '-p', f'{build_dir}/iso_root'])
    info('✅ All selected builds are set up!')

def build(build_dir: str):
    info('Starting compilation...')
    try:
        shell.try_run(['meson', 'compile', '-C', build_dir, '-j5', 'build'], stream=True)
    except Exception:
        error('Compilation failed.')
        sys.exit(1)

    info('Compilation completed successfully.')

def run_qemu(build_dir: str, firmware_type: str):
    if firmware_type == '':
        firmware_type = 'uefi'

    shell.try_run(['meson', 'compile', '-C', build_dir, '-j5', f'run_{firmware_type}'], stream=True)

def prompt(message: str) -> str:
    result: str = ''
    while result not in ['yes', 'no', 'y', 'no']:
        result = input(f'{message} => [y/n]: ').lower()
    return result[0]

def run():
    build_dir = f'build_{args.build_type}_{args.target_arch}' if args.build_dir == '' else args.build_dir

    if not os.path.isdir(build_dir) and args.action != 'setup' and args.action != 'all':
        error('Build directory {build_dir} does not exist. Please run the Meson setup step before compiling.')
    if args.action == 'r':
        args.action = 'run'

    if args.action in ['setup', 's']:
        setup(build_dir, args.target_arch, args.build_type, args.compiler)
    elif args.action in ['build', 'b']:
        build(build_dir)
    elif args.action in ['rebuild', 'rb']:
        if prompt('this will regenerate your current build directory\ndo you want to proceed?') == 'n':
            sys.exit(0)
        setup(build_dir, args.target_arch, args.build_type, args.compiler)
        build(build_dir)

    elif args.action in ['run', 'run-bios', 'run-uefi', 'r']:
        firmware = 'uefi'
        if args.action != 'run':
            firmware = args.action.removesuffix('run-')
        
        run_qemu(build_dir, firmware)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--target-arch', choices=['x86_64', 'aarch64'], default='x86_64', help='target architecture')
    parser.add_argument('-c', '--compiler', choices=['clang', 'gcc'], default='gcc', help='compiler')
    parser.add_argument('-b', '--build-type', choices=['release', 'debug', 'dist'], default='release', help='build type')
    parser.add_argument('-C', '--build-dir', type=str, default='', help='build directory', required=False)
    parser.add_argument('action', type=str, choices=['setup', 'build', 'rebuild', 'run', 'run-bios', 'run-uefi', 's', 'b', 'rb', 'r'], help='')

    args = parser.parse_args()
    run()

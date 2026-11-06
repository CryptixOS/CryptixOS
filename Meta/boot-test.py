#!/usr/bin/env python3
#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
"""
Boots the ISO headless in QEMU, with the shell on the serial port, types the
commands of a test script into it and compares what they print with the
expected output. Fails on any mismatch, on a kernel panic and on a hang (no
answer within the timeout).

The ISO is remastered first: its limine.conf gets 'console=ttyS0' on the
kernel command line and a zero menu timeout, nothing else changes.

Test scripts (Tests/Boot/*.txt) are a list of commands, each followed by the
lines it must print:

    $ echo hello
    hello
    $ uname -s
    cryptix

Blank lines and lines starting with '#' are ignored. The output is compared
line by line after dropping '\\r' and trailing whitespace.
"""

import argparse
import os
import queue
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Printed by the kernel's panic path (Kernel/Debug/Panic.cpp)
PANIC_PATTERNS = [b'Kernel Panic!', b'System Halted!', b'CPU[0]: Halted']


def log(message: str):
    print(f'[boot-test]: {message}', flush=True)


def parse_script(path: str) -> list[tuple[str, list[str]]]:
    steps = []
    with open(path) as f:
        for number, line in enumerate(f, 1):
            line = line.rstrip('\n')
            if line.startswith('$ '):
                steps.append((line[2:], []))
            elif not line.strip() or line.startswith('#'):
                continue
            elif not steps:
                sys.exit(f'{path}:{number}: expected output before the first command')
            else:
                steps[-1][1].append(line.rstrip())
    return steps


def remaster_iso(iso: str, output: str):
    """Copies the ISO with the kernel booting on the serial console."""
    with tempfile.TemporaryDirectory() as tmp:
        tree = os.path.join(tmp, 'iso_root')
        subprocess.run(['xorriso', '-osirrox', 'on', '-indev', iso,
                        '-extract', '/', tree],
                       check=True, capture_output=True)
        subprocess.run(['chmod', '-R', 'u+w', tree], check=True)
        conf = os.path.join(tree, 'limine.conf')
        with open(conf) as f:
            text = f.read()

        text = re.sub(r'^timeout:.*$', 'timeout: 0', text, flags=re.M)
        text, count = re.subn(r'^(\$\{CMDLINE\}=.*)$', r'\1 console=ttyS0',
                              text, flags=re.M)
        if count != 1:
            sys.exit('boot-test: limine.conf has no ${CMDLINE}= line to extend')
        with open(conf, 'w') as f:
            f.write(text)

        if os.path.exists(output):
            os.remove(output)
        # The same flags as the iso target in meson.build (x86_64)
        subprocess.run(['xorriso', '-as', 'mkisofs',
                        '-b', 'limine-bios-cd.bin', '-no-emul-boot',
                        '-boot-load-size', '4', '-boot-info-table',
                        '--efi-boot', 'limine-uefi-cd.bin', '-efi-boot-part',
                        '--efi-boot-image', '--protective-msdos-label',
                        tree, '-o', output],
                       check=True, capture_output=True)

    limine = os.path.join(ROOT, 'subprojects', 'limine', 'limine')
    if os.path.exists(limine):
        subprocess.run([limine, 'bios-install', output],
                       check=True, capture_output=True)


class Machine:
    def __init__(self, args, iso: str, kernel_log: str):
        accel = 'kvm:tcg' if os.access('/dev/kvm', os.W_OK) else 'tcg'
        cmd = [
            'qemu-system-x86_64',
            '-M', f'q35,accel={accel}',
            '-cpu', 'max',
            '-smp', '2',
            '-m', args.memory,
            '-display', 'none',
            '-no-reboot',
            '-boot', 'order=d',
            '-cdrom', iso,
            '-device', 'isa-debug-exit',
            # COM1: the shell
            '-serial', 'stdio',
            # port 0xe9: the kernel log
            '-debugcon', f'file:{kernel_log}',
            '-net', 'none',
        ]
        if args.disk and os.path.exists(args.disk):
            # snapshot=on: the test never writes to the developer's image
            cmd += ['-drive', f'format=raw,file={args.disk},if=none,id=drive1,snapshot=on',
                    '-device', 'nvme,id=nvme0,serial=nvme',
                    '-device', 'nvme-ns,drive=drive1,nsid=2,bus=nvme0']
        if args.firmware:
            cmd += ['-bios', args.firmware]

        log(' '.join(cmd))
        self.kernel_log = kernel_log
        self.proc = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT)
        self.output = bytearray()
        self.chunks: queue.Queue[bytes] = queue.Queue()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self):
        while chunk := self.proc.stdout.read1(4096):
            self.chunks.put(chunk)
        self.chunks.put(b'')

    def send(self, text: str):
        self.proc.stdin.write(text.encode())
        self.proc.stdin.flush()

    def check_alive(self):
        if self.proc.poll() is not None:
            raise TestFailure(f'QEMU exited with status {self.proc.returncode}')
        try:
            with open(self.kernel_log, 'rb') as f:
                kernel_output = f.read()
        except FileNotFoundError:
            kernel_output = b''
        for pattern in PANIC_PATTERNS:
            if pattern in kernel_output or pattern in self.output:
                raise TestFailure('kernel panic')

    def wait_for(self, pattern: bytes, timeout: float, start: int = 0) -> int:
        """Waits until `pattern` shows up in the serial output after offset
        `start`, returns the offset just past it."""
        deadline = time.monotonic() + timeout
        while True:
            index = self.output.find(pattern, start)
            if index >= 0:
                return index + len(pattern)

            self.check_alive()
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TestFailure(f'hang: no {pattern!r} within {timeout:.0f}s')
            try:
                chunk = self.chunks.get(timeout=min(remaining, 0.5))
            except queue.Empty:
                continue
            if not chunk:
                self.check_alive()
                raise TestFailure('QEMU closed the serial port')
            self.output += chunk
            sys.stdout.buffer.write(chunk)
            sys.stdout.flush()

    def kill(self):
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()


class TestFailure(Exception):
    pass


def wait_for_shell(machine: Machine, timeout: float):
    """Pokes the serial console until the shell answers. Each poke has its
    own marker, so a late answer to an earlier poke is not mistaken for the
    shell being ready after the next one."""
    deadline = time.monotonic() + timeout
    attempt = 0
    while True:
        attempt += 1
        # The shell prints 'ready-N', its echo of the command shows the
        # unexpanded '$((N))', so only a running shell can produce the marker
        machine.send(f'echo boot-test-ready-$(({attempt}))\n')
        try:
            machine.wait_for(f'boot-test-ready-{attempt}\r\n'.encode(), 5)
            return
        except TestFailure as failure:
            if not str(failure).startswith('hang') or time.monotonic() > deadline:
                raise TestFailure(f'the shell never came up ({failure})')


def run_command(machine: Machine, index: int, command: str, timeout: float) -> list[str]:
    begin = f'boot-test-begin-{index}'
    end = f'boot-test-end-{index}'
    start = len(machine.output)
    # The markers are written as $((...)) so the echoed command line can't
    # match them, only the shell's output can
    machine.send(f'echo boot-test-begin-$(({index})); {command}; '
                 f'echo boot-test-end-$(({index}))\n')
    after_begin = machine.wait_for(f'{begin}\r\n'.encode(), timeout, start)
    end_index = machine.wait_for(f'{end}\r\n'.encode(), timeout, after_begin)
    body = machine.output[after_begin:end_index - len(end) - 2]
    body = body.decode(errors='replace').replace('\r', '')
    return [line.rstrip() for line in body.splitlines()]


def run_script(machine: Machine, path: str, timeout: float) -> int:
    failures = 0
    for index, (command, expected) in enumerate(parse_script(path), 1):
        actual = run_command(machine, index, command, timeout)
        if actual == expected:
            log(f'PASS  $ {command}')
            continue
        failures += 1
        log(f'FAIL  $ {command}')
        log(f'      expected: {expected}')
        log(f'      actual:   {actual}')
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--iso', default='build_release_x86_64/Cryptix.iso')
    parser.add_argument('--disk', help='raw disk image to attach (read-only, as a snapshot)')
    parser.add_argument('--firmware', help='UEFI firmware to boot with instead of BIOS')
    parser.add_argument('--memory', default='3G')
    parser.add_argument('--boot-timeout', type=float, default=300,
                        help='seconds to wait for the shell')
    parser.add_argument('--command-timeout', type=float, default=60,
                        help='seconds to wait for each command')
    parser.add_argument('--log-dir', default='boot-test-logs',
                        help='where the serial and kernel logs are kept')
    parser.add_argument('scripts', nargs='*',
                        default=[os.path.join(ROOT, 'Tests', 'Boot', 'smoke.txt')])
    args = parser.parse_args()

    for tool in ['qemu-system-x86_64', 'xorriso']:
        if not shutil.which(tool):
            sys.exit(f'boot-test: {tool} not found')

    os.makedirs(args.log_dir, exist_ok=True)
    iso = os.path.join(args.log_dir, 'boot-test.iso')
    kernel_log = os.path.join(args.log_dir, 'kernel.log')
    remaster_iso(args.iso, iso)
    if os.path.exists(kernel_log):
        os.remove(kernel_log)

    machine = Machine(args, iso, kernel_log)
    failures = 0
    try:
        wait_for_shell(machine, args.boot_timeout)
        log('shell is up')
        for script in args.scripts:
            log(f'running {script}')
            failures += run_script(machine, script, args.command_timeout)
    except TestFailure as failure:
        log(f'FAIL: {failure}')
        failures += 1
    finally:
        machine.kill()
        with open(os.path.join(args.log_dir, 'serial.log'), 'wb') as f:
            f.write(machine.output)
        log(f'logs are in {args.log_dir}/')

    if failures:
        log(f'{failures} failure(s)')
        sys.exit(1)
    log('all passed')


if __name__ == '__main__':
    main()

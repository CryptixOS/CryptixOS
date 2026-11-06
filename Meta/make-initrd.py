#!/usr/bin/env python3
#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
"""
Archives the sysroot into the initrd tarball and writes a depfile listing
every file in it, so the initrd is rebuilt exactly when the sysroot changes.
"""

import argparse
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from helix import depfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sysroot', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--depfile')
    args = parser.parse_args()

    sysroot = os.path.realpath(args.sysroot)
    # Written next to the output first, so an interrupted build never leaves
    # a truncated initrd that ninja considers up to date
    partial = args.output + '.partial'
    subprocess.run(['tar', '-cf', partial, '-C', sysroot + '/', '.'], check=True)
    os.replace(partial, args.output)

    if args.depfile:
        depfile.write(args.depfile, args.output, depfile.tree(sysroot))


if __name__ == '__main__':
    main()

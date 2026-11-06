#!/usr/bin/env python3
#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
"""
Makes <sysroot>/usr/lib/modules hold exactly the given kernel modules.
Only files whose contents differ are written and only modules that are no
longer built are removed, so the directory (which the initrd and disk image
depfiles track) changes only when the modules do.
"""

import argparse
import filecmp
import os
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sysroot', required=True)
    parser.add_argument('--stamp', required=True, help='file touched when done')
    parser.add_argument('modules', nargs='*')
    args = parser.parse_args()

    destination = os.path.join(os.path.realpath(args.sysroot), 'usr', 'lib', 'modules')
    os.makedirs(destination, exist_ok=True)

    wanted = {os.path.basename(module): module for module in args.modules}
    for name in os.listdir(destination):
        if name not in wanted:
            path = os.path.join(destination, name)
            if os.path.isdir(path) and not os.path.islink(path):
                shutil.rmtree(path)
            else:
                os.remove(path)

    for name, source in wanted.items():
        target = os.path.join(destination, name)
        if os.path.exists(target) and filecmp.cmp(source, target, shallow=False):
            continue
        shutil.copyfile(source, target)

    with open(args.stamp, 'w'):
        pass


if __name__ == '__main__':
    main()

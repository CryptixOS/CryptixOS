#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
"""
Ninja depfiles for build steps that read a whole directory tree (the initrd
and the disk image read the sysroot). Listing every file makes ninja rerun
the step when one changes; listing every directory makes it rerun when a
file is added, removed or renamed, since that updates the directory's mtime.
"""

import os


def tree(root: str) -> list[str]:
    """Every directory and regular file under `root`. Symlinks are left out:
    ninja stats through them and would treat a dangling one as a missing
    input, rebuilding forever. Adding or removing one still shows up in its
    directory's mtime."""
    root = os.path.realpath(root)
    paths = []
    for directory, dirs, files in os.walk(root):
        dirs.sort()
        paths.append(directory)
        for name in sorted(files):
            path = os.path.join(directory, name)
            if not os.path.islink(path):
                paths.append(path)
    return paths


def _escape(path: str) -> str:
    return path.replace('\\', '\\\\').replace(' ', '\\ ') \
        .replace('#', '\\#').replace('$', '$$')


def write(depfile: str, target: str, dependencies: list[str]):
    with open(depfile, 'w') as f:
        f.write(f'{_escape(target)}:')
        for dependency in dependencies:
            f.write(f' \\\n  {_escape(dependency)}')
        f.write('\n')

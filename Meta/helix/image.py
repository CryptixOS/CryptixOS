#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
"""
Builds a partitioned disk image without root: no loop devices and no
mounts. parted writes the partition table into the file, mke2fs and mkfs.fat
format the partitions in place at their byte offsets, mke2fs -d fills the
ext2 partition from a directory and mtools copies into the FAT one.
"""

import os
import shutil
import tempfile
from enum import Enum

from helix.log import trace, info, fail
from helix.shell import run

SECTOR_SIZE = 512


class PartitionTableType(Enum):
    MBR = "mbr"
    GPT = "gpt"


class Image:
    def __init__(self, path: str, size_bytes: int):
        trace(f'creating image `{path}` with size: `{size_bytes // 1024 // 1024}MiB`')
        if os.path.exists(path):
            os.remove(path)
        # Sparse: only the blocks the filesystems write take up space
        with open(path, 'wb') as f:
            f.truncate(size_bytes)
        self.image_path = path

    def close(self):
        pass

    def create_partition_table(self, table_type: PartitionTableType):
        trace(f'creating `{table_type}` partition table')
        label = 'msdos' if table_type == PartitionTableType.MBR else 'gpt'
        run(['parted', '-s', self.image_path, 'mklabel', label])

    def mkpart(self, type: str, start: str, end: str):
        trace(f'creating `{type}` partition, sectors => {start}-{end}')
        run(['parted', '-s', self.image_path, 'mkpart', type, 'fat32', start, end])

    def partition(self, index: int) -> tuple[int, int]:
        """The byte offset and size of partition `index` (1-based)."""
        output = run(['parted', '-s', '-m', self.image_path, 'unit', 'B', 'print']).stdout
        if isinstance(output, bytes):
            output = output.decode()
        for line in output.splitlines():
            fields = line.rstrip(';').split(':')
            if fields[0] == str(index):
                return int(fields[1].rstrip('B')), int(fields[3].rstrip('B'))
        fail(f'partition {index} not found in `{self.image_path}`')
        raise KeyError(index)

    def populate(self, index: int, fs: str, src_path: str):
        """Formats partition `index` as `fs` and copies `src_path` into it."""
        offset, size = self.partition(index)
        trace(f'formatting partition {index} (offset {offset}, {size} bytes) as {fs}')
        if fs == 'ext2':
            self._populate_ext2(offset, size, src_path)
        elif fs == 'fat32':
            self._populate_fat32(offset, size, src_path)
        else:
            fail(f'unsupported filesystem: {fs}')

    def _populate_ext2(self, offset: int, size: int, src_path: str):
        # Under fakeroot every file reads as owned by root, as it was when
        # the image was filled through a root mount; without it the files
        # keep the developer's uid
        prefix = ['fakeroot'] if shutil.which('fakeroot') else []
        run(prefix + ['mke2fs', '-q', '-F', '-t', 'ext2',
                      '-E', f'offset={offset}',
                      '-d', os.path.realpath(src_path),
                      self.image_path, f'{size // 1024}k'])

    def _populate_fat32(self, offset: int, size: int, src_path: str):
        if offset % SECTOR_SIZE:
            fail(f'partition offset {offset} is not sector aligned')
        run(['mkfs.fat', '-F', '32', '--offset', str(offset // SECTOR_SIZE),
             self.image_path, str(size // 1024)])

        # FAT has no symlinks; mcopy would follow them and copy whole trees
        # twice (bin -> usr/bin), so copy a symlink-free view of the tree.
        # Hard links keep it cheap when it's on the same filesystem.
        env = dict(os.environ, MTOOLS_SKIP_CHECK='1')
        with tempfile.TemporaryDirectory(dir=os.path.dirname(self.image_path)) as staging:
            source = os.path.realpath(src_path)
            for directory, dirs, files in os.walk(source):
                relative = os.path.relpath(directory, source)
                target_dir = os.path.join(staging, relative)
                os.makedirs(target_dir, exist_ok=True)
                for name in files:
                    path = os.path.join(directory, name)
                    if os.path.islink(path) or not os.path.isfile(path):
                        continue
                    target = os.path.join(target_dir, name)
                    try:
                        os.link(path, target)
                    except OSError:
                        shutil.copyfile(path, target)

            entries = [os.path.join(staging, name) for name in sorted(os.listdir(staging))]
            if entries:
                info(f'copying `{source}` into the FAT partition...')
                # -D o: overwrite on a name clash (FAT is case-insensitive)
                run(['mcopy', '-s', '-Q', '-D', 'o', '-i',
                     f'{self.image_path}@@{offset}', *entries, '::/'], env=env)

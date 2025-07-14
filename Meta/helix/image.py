import os 
from helix.log import trace, info
from helix.shell import run
from enum import Enum
import tempfile

class PartitionTableType(Enum):
    MBR = "mbr"
    GPT = "gpt"

class Image:
    def __init__(self, path: str, size_bytes: int):
        self.last_part_sector = 1
        size_mib = size_bytes // 1024 // 1024
        trace(f'creating image `{path}` with size: `{size_mib}`')
        run(['dd', 'if=/dev/zero', f'of={path}', 'bs=1M', f'count={size_mib}'])
        self.image_path = path
        
    def close(self):
        run(['losetup', '-d', self.loop])

    def create_partition_table(self, table_type: PartitionTableType):
        trace(f'creating `{table_type}` partition table')

        type = 'msdos' if table_type == PartitionTableType.MBR else 'gpt'
        run(['parted', '-s', self.image_path, 'mklabel', type])

    def mkpart(self, type: str, start: str, end: str):
        sector_size = 512
        trace(f'creating `{type}` partition, sectors => {start}-{end}')
        run(['parted', '-s', self.image_path, 'mkpart', type, 'fat32', f'{start}', f'{end}'])

    def setup_loop(self):
        self.loop = run(['losetup', '-fP', '--show', self.image_path]).stdout
        info(f'loopback device created at => `{self.loop}`')
    def mkfs(self, index: int, fs: str):
        device = f'/dev/sda{index}'
        trace(f'formatting {device} as {fs}')
        cmd = []
        if not hasattr(self, 'loop'):
            self.setup_loop()

        if fs == 'fat32':
            cmd.append('mkfs.vfat')
            cmd.append('-F32')
        else:
            cmd.append(f'mkfs.{fs}')
        part_path = f'{self.loop}p{index}'
        cmd.append(part_path)

        run(cmd)

    def copy_to_partition(self, index: int, src_path: str):
        device = f'{self.loop}p{index}'
        with tempfile.TemporaryDirectory() as tmpdirname:
            trace(f'mounting {device} at {os.path.abspath(tmpdirname)}')
            run(['mount', f'{self.loop}p{index}', os.path.abspath(tmpdirname)])
            trace(f'copying the files from `{src_path}` to mounted device at `{os.path.abspath(tmpdirname)}`')
            run(['cp', '-r', f'{src_path}', f'{os.path.abspath(tmpdirname)}/'])
            trace(f'unmounting {device} from {os.path.abspath(tmpdirname)}')
            run(['umount', f'{os.path.abspath(tmpdirname)}'])

    
        

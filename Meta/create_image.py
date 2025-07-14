#!/usr/bin/env python3

import os
import sys
import guestfs
import argparse
from enum import Enum

color_reset = '\x1b[0m'
red_color = '\x1b[31m'
green_color = '\x1b[32m' 
cyan_color = '\x1b[36m' 

log_levels = {
    "trace": green_color,
    "info": cyan_color,
    "error": red_color,
}

def log(level: str, message: str):
    color = log_levels[level]
    print(f'[{color}{level}{color_reset}]: {message}')

def trace(message: str): log('trace', message)
def info(message: str): log('info', message)
def error(message: str): log('error', message)

def fail(message: str):
    error(message)
    sys.exit(1)

def fail_cleanup(image: guestfs.GuestFS, message: str):
    image.close()
    fail(message)

def parse_size(size_str: str) -> int:
    size_str = size_str.lower()

    multiplier = 1024 if size_str.endswith('ib') else 1000
    if size_str.endswith('ib'):
        size_str = size_str.removesuffix('ib')
    elif size_str.endswith('b'):
        size_str = size_str.removesuffix('b')

    if size_str.endswith('g'):
        return int(size_str[:-1]) * (multiplier ** 3)
    elif size_str.endswith("m"):
        return int(size_str[:-1]) * (multiplier ** 2)
    elif size_str.endswith("k"):
        return int(size_str[:-1]) * multiplier 
    elif size_str.endswith("b"):
        return int(size_str[:-1])
    return int(size_str)

class PartitionTableType(Enum):
    MBR = "mbr"
    GPT = "gpt"

def create_image(path: str, size_bytes: int):
    trace(f'Creating image `{path}` of size {size_bytes / 1024 / 1024:.1f}MiB...')
    image = guestfs.GuestFS(python_return_dict=True)
    image.disk_create(path, "raw", size_bytes)
    image.add_drive_opts(path, format="raw", readonly=False)
    image.launch()
    return image

def create_partition_table(image: guestfs.GuestFS, type: PartitionTableType):
    trace(f"Creating partition table: {type.value}")
    image.part_init("/dev/sda", type.value)

prev_part_end = 1
def create_partition(image: guestfs.GuestFS, partition_type: str, size_bytes: int):
    global prev_part_end
    sector_size = 512
    sectors = size_bytes // sector_size
    start = prev_part_end + 1
    end = start + sectors - 1
    part_type = "p" if partition_type == "primary" else "l"
    trace(f"Creating {partition_type} partition from {start} to {end} sectors")
    image.part_add("/dev/sda", part_type, start, end)
    prev_part_end = end

def create_filesystem(image: guestfs.GuestFS, index: int, fs_type: str):
    device = f"/dev/sda{index}"
    trace(f"Formatting {device} as {fs_type}")
    image.mkfs(fs_type, device)

def process_directory(image: guestfs.GuestFS, base_path: str, root: str):
    for dirpath, _, filenames in os.walk(root):
        rel = os.path.relpath(dirpath, base_path)
        rel_dir = "/" if rel == "." else f"/{rel}"
        try:
            image.mkdir(rel_dir)
        except RuntimeError:
            pass  # already exists

        for file in filenames:
            src = os.path.join(dirpath, file)
            dst = os.path.join(rel_dir, file)
            if __debug__:
                trace(f"Uploading {src} -> {dst}")
            image.upload(src, dst)

def copy_to_partition(image: guestfs.GuestFS, partition_index: int, src_path: str):
    device = f"/dev/sda{partition_index}"
    fs_type = image.vfs_type(device)
    trace(f"Mounting {device} as {fs_type}")
    image.mount(device, "/")
    process_directory(image, src_path, src_path)
    image.umount_all()

def create_filesystems(image: guestfs.GuestFS, src_path: str):
    create_partition_table(image, PartitionTableType(args.partition_table.lower()))
    create_partition(image, 'primary', 256 * 1024 * 1024)
    create_partition(image, 'primary', 256 * 1024 * 1024)

    filesystems = ['vfat', 'ext2']
    for i, fs in enumerate(filesystems):
        create_filesystem(image, i + 1, fs)
        copy_to_partition(image, i + 1, src_path)

    trace("Syncing disk...")
    image.sync()

def run():
    if not os.path.isdir(args.input):
        fail(f"Input path is not a directory: {args.input}")

    image_path = os.path.abspath(args.output)
    image_size = parse_size(args.size)

    image = create_image(image_path, image_size)

    try:
        create_filesystems(image, args.input)
    except Exception as e:
        fail_cleanup(image, f"Error: {e}")
    finally:
        image.close()

    print("Done.")

def elevate():
    args = ['sudo', sys.executable] + sys.argv
    os.execlpe('sudo', *args, os.environ)

if __name__ == '__main__':
    if os.geteuid() != 0:
        info("Script not started as root. Running sudo...")
        elevate()

    parser = argparse.ArgumentParser(description="Create an MBR/GPT partitioned disk image.")
    parser.add_argument("-i", "--input", required=True, help="Directory to copy into both partitions")
    parser.add_argument("-o", "--output", required=True, help="Output image file path (e.g. image.hdd)")
    parser.add_argument("-s", "--size", required=True, help="Image size (e.g. 768MiB or 800000000)")
    parser.add_argument("-p", "--partition-table", choices=["mbr", "gpt"], default="mbr", help="Partition table type")

    args = parser.parse_args()
    run()


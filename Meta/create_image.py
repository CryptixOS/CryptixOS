#!/usr/bin/env python3

import os
import sys
import argparse

from helix.log import trace, info, error, fail
import helix.image
from helix import shell
from helix.image import Image, PartitionTableType
from enum import Enum

def fail_cleanup(image: Image, message: str):
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

def create_filesystems(image: Image, src_path: str):
    part_table_type = PartitionTableType(args.partition_table.lower())
    image.create_partition_table(part_table_type)
    image.mkpart('primary', '2048s', '1024MiB')
    image.mkpart('primary', '1024MiB', '100%')
    filesystems = ['fat32', 'ext2']
    for i, fs in enumerate(filesystems):
        image.mkfs(i + 1, fs)
        image.copy_to_partition(i + 1, src_path)

    trace("Syncing disk...")

def run():
    if not os.path.isdir(args.input):
        fail(f"Input path is not a directory: {args.input}")

    image_path = os.path.abspath(args.output)
    image_size = parse_size(args.size)
    image = Image(image_path, image_size)

    try:
        create_filesystems(image, args.input)
    except Exception as e:
        fail_cleanup(image, f"Error: {e}")
    finally:
        image.close()

    # shell.run(['chown', 'image_path', real_uid, real_gid])
    info("Done.")

def elevate():
    os.environ['RAISED_BY_SUDO_UID'] = str(os.getuid())
    os.environ['RAISED_BY_SUDO_GID'] = str(os.getgid())
    args = ['sudo', sys.executable] + sys.argv
    os.execlpe('sudo', *args, os.environ)

if __name__ == '__main__':
    uid = os.geteuid()

    if uid != 0:
        info("Script not started as root. Running sudo...")
        elevate()

    parser = argparse.ArgumentParser(description="Create an MBR/GPT partitioned disk image.")
    parser.add_argument("-i", "--input", required=True, help="Directory to copy into both partitions")
    parser.add_argument("-o", "--output", required=True, help="Output image file path (e.g. image.hdd)")
    parser.add_argument("-s", "--size", required=True, help="Image size (e.g. 768MiB or 800000000)")
    parser.add_argument("-p", "--partition-table", choices=["mbr", "gpt"], default="mbr", help="Partition table type")

    args = parser.parse_args()
    run()

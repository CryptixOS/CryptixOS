#!/usr/bin/env zsh

# Create an empty zeroed out 512MiB image file.
dd if=/dev/zero of=image.hdd bs=4M count=128
 
# Create a GPT partition table.
parted -s image.hdd mklabel msdos
 
# Create an ESP partition that spans the whole disk.
parted -s image.hdd mkpart primary 2048s 100%
echfs-utils -m -p0 image.hdd format 512
echfs-utils -m -p0 image.hdd mkdir dir1
echfs-utils -m -p0 image.hdd mkdir dir2
echfs-utils -m -p0 image.hdd mkdir dir3

python ../Meta/import_files.py


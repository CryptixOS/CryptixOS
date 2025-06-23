#!/usr/bin/env zsh

OUT_IMAGE=image.hdd

# Create an empty zeroed out 512MiB image file.
dd if=/dev/zero of=$OUT_IMAGE bs=4M count=128
 
# Create a MBR partition table.
parted -s $OUT_IMAGE mklabel msdos
 
# Create an ESP partition that spans the whole disk.
parted -s $OUT_IMAGE mkpart primary ext2 2048s 64MiB
parted -s $OUT_IMAGE mkpart primary fat32 64MiB 92MiB
parted -s $OUT_IMAGE mkpart primary fat32 92MiB 128MiB
# echfs-utils -m -p2 $OUT_IMAGE format 512

sudo losetup -fP --show $OUT_IMAGE
sudo mkfs.ext2 /dev/loop0p1
sudo mkfs.vfat -F32 /dev/loop0p2

sudo mount /dev/loop0p1
sudo cp -r ../Kernel ../BaseFiles /mnt
sudo umount /mnt

sudo mount /dev/loop0p2
sudo cp -r ../Kernel ../BaseFiles /mnt
sudo umount /mnt

# python ../Meta/import_files.py

sudo losetup -d /dev/loop0


# Create an empty zeroed out 64MiB image file.
dd if=/dev/zero bs=1M count=0 seek=64 of=image.hdd
 
# Create a GPT partition table.
parted -s image.hdd mklabel gpt
 
# Create an ESP partition that spans the whole disk.
parted -s image.hdd mkpart ESP fat32 2048s 100%
parted -s image.hdd set 1 esp on
 
# Install the Limine BIOS stages onto the image.
./subprojects/limine/limine bios-install image.hdd
 
# Mount the loopback device.
USED_LOOPBACK=$(sudo losetup -Pf --show image.hdd)
 
# Format the ESP partition as FAT32.
sudo mkfs.fat -F 32 ${USED_LOOPBACK}p1
 
# Mount the partition itself.
mkdir -p img_mount
sudo mount ${USED_LOOPBACK}p1 img_mount
 
# Copy the relevant files over.
sudo mkdir -p img_mount/EFI/BOOT
sudo cp -v build/Kernel/Cryptix.elf limine.conf subprojects/limine/limine-bios.sys img_mount/
sudo cp -v subprojects/limine/BOOTX64.EFI img_mount/EFI/BOOT/
 
# Sync system cache and unmount partition and loopback device.
sync
sudo umount img_mount
sudo losetup -d ${USED_LOOPBACK}

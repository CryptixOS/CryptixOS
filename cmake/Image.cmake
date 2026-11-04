#
# Boot image, ISO and QEMU targets (ported from the custom_target()s of the former meson.build).
#
#   cmake --build <dir> --target build         # kernel + initrd + root image + bootable ISO
#   cmake --build <dir> --target run_uefi      # ... and boot it in QEMU with OVMF
#
set(CRYPTIX_INITRD_DIR ${PROJECT_SOURCE_DIR}/sysroot)
set(CRYPTIX_ISO_DIR    ${CMAKE_BINARY_DIR}/iso_root)

# ------------------------------------------------------------------------------
# Limine's installer is a host tool
# ------------------------------------------------------------------------------
find_program(CRYPTIX_HOST_CC NAMES cc gcc clang NO_CMAKE_FIND_ROOT_PATH REQUIRED)
set(CRYPTIX_LIMINE_TOOL ${CMAKE_BINARY_DIR}/limine)
add_custom_command(
  OUTPUT  ${CRYPTIX_LIMINE_TOOL}
  COMMAND ${CRYPTIX_HOST_CC} ${limine_SOURCE_DIR}/limine.c -Wall -Wextra -std=c99 -O2
          -o ${CRYPTIX_LIMINE_TOOL}
  DEPENDS ${limine_SOURCE_DIR}/limine.c
  COMMENT "Building the limine installer"
  VERBATIM)

set(CRYPTIX_LIMINE_BINARIES ${limine_SOURCE_DIR}/limine-uefi-cd.bin)
if(CRYPTIX_ARCH STREQUAL "x86_64")
  list(APPEND CRYPTIX_LIMINE_BINARIES
    ${limine_SOURCE_DIR}/limine-bios.sys
    ${limine_SOURCE_DIR}/limine-bios-cd.bin
    ${limine_SOURCE_DIR}/BOOTX64.EFI
    ${limine_SOURCE_DIR}/BOOTIA32.EFI)
else()
  list(APPEND CRYPTIX_LIMINE_BINARIES ${limine_SOURCE_DIR}/BOOTAA64.EFI)
endif()

# ------------------------------------------------------------------------------
# Loadable kernel modules (*.ko) and the initial ramdisk
# ------------------------------------------------------------------------------
get_property(CRYPTIX_MODULE_FILES GLOBAL PROPERTY CRYPTIX_MODULE_FILES)
get_property(CRYPTIX_MODULE_TARGETS GLOBAL PROPERTY CRYPTIX_MODULE_TARGETS)

add_custom_command(
  OUTPUT  ${CMAKE_BINARY_DIR}/ksyms.sym
  COMMAND sh ${PROJECT_SOURCE_DIR}/Meta/generate_symbols.sh
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS Cryptix.elf
  COMMENT "Generating ksyms.sym"
  USES_TERMINAL VERBATIM)
add_custom_target(ksyms ALL DEPENDS ${CMAKE_BINARY_DIR}/ksyms.sym)

# Only regular files: BaseFiles contains symlinks (bin -> usr/bin, ...) that dangle in the
# source tree and cannot be build dependencies.
file(GLOB_RECURSE _base_candidates CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/BaseFiles/*)
set(CRYPTIX_BASE_FILES "")
foreach(_f IN LISTS _base_candidates)
  if(NOT IS_SYMLINK "${_f}" AND NOT IS_DIRECTORY "${_f}")
    list(APPEND CRYPTIX_BASE_FILES "${_f}")
  endif()
endforeach()

add_custom_command(
  OUTPUT  ${CMAKE_BINARY_DIR}/image.hdd
  COMMAND ${PROJECT_SOURCE_DIR}/Meta/create_image.py
          --input ${PROJECT_SOURCE_DIR}/sysroot
          --output ${CMAKE_BINARY_DIR}/image.hdd
          --partition-table gpt
          --size 2GiB
  DEPENDS ${CRYPTIX_INITRD_DIR}/usr/sbin/init ${CRYPTIX_MODULE_TARGETS}
  USES_TERMINAL VERBATIM)
add_custom_target(root_image ALL DEPENDS ${CMAKE_BINARY_DIR}/image.hdd)

set(_initrd_copy_modules "")
if(CRYPTIX_MODULE_FILES)
  set(_initrd_copy_modules COMMAND ${CMAKE_COMMAND} -E copy ${CRYPTIX_MODULE_FILES}
                                   ${CRYPTIX_INITRD_DIR}/usr/lib/modules/)
endif()
add_custom_command(
  OUTPUT  ${CMAKE_BINARY_DIR}/initrd.img.gz
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${CRYPTIX_INITRD_DIR}/usr/lib/modules
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CRYPTIX_INITRD_DIR}/usr/lib/modules
  ${_initrd_copy_modules}
  COMMAND tar -cvf ${CMAKE_BINARY_DIR}/initrd.img.gz -C ${CRYPTIX_INITRD_DIR} .
  DEPENDS ${CRYPTIX_INITRD_DIR}/usr/sbin/init ${CRYPTIX_MODULE_TARGETS} ${CRYPTIX_BASE_FILES}
  USES_TERMINAL VERBATIM)
add_custom_target(initrd ALL DEPENDS ${CMAKE_BINARY_DIR}/initrd.img.gz)

add_custom_command(
  OUTPUT  ${CMAKE_BINARY_DIR}/initrd.zlib
  COMMAND ${PROJECT_SOURCE_DIR}/Meta/compress-initrd.sh
          ${CMAKE_BINARY_DIR}/initrd.img.gz ${CMAKE_BINARY_DIR}/initrd.zlib
  DEPENDS ${CMAKE_BINARY_DIR}/initrd.img.gz
  USES_TERMINAL VERBATIM)
add_custom_target(initrd_zlib ALL DEPENDS ${CMAKE_BINARY_DIR}/initrd.zlib)

# ------------------------------------------------------------------------------
# ISO
# ------------------------------------------------------------------------------
if(CRYPTIX_ARCH STREQUAL "x86_64")
  set(CRYPTIX_XORRISO_ARGS
    -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table
    --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label)
  set(CRYPTIX_OVMF_FILE OVMF_X64.fd)
else()
  set(CRYPTIX_XORRISO_ARGS
    --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label)
  set(CRYPTIX_OVMF_FILE OVMF_AA64.fd)
endif()

set(CRYPTIX_ISO_FILES
  ${PROJECT_SOURCE_DIR}/Meta/limine.conf
  ${PROJECT_SOURCE_DIR}/Meta/fonts/font.bin
  ${PROJECT_SOURCE_DIR}/Meta/images/bg2.png
  ${CMAKE_BINARY_DIR}/ksyms.sym
  ${CRYPTIX_LIMINE_BINARIES}
  ${PROJECT_SOURCE_DIR}/Meta/qemu.dtb)

add_custom_command(
  OUTPUT  ${CMAKE_BINARY_DIR}/Cryptix.iso
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CRYPTIX_ISO_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Cryptix.elf> ${CRYPTIX_ISO_DIR}/Cryptix.elf
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/initrd.img.gz ${CRYPTIX_ISO_DIR}/initrd.img.gz
  COMMAND ${CMAKE_COMMAND} -E copy ${CRYPTIX_ISO_FILES} ${CRYPTIX_ISO_DIR}
  COMMAND xorriso -as mkisofs ${CRYPTIX_XORRISO_ARGS} ${CRYPTIX_ISO_DIR} -o ${CMAKE_BINARY_DIR}/Cryptix.iso
  COMMAND ${CRYPTIX_LIMINE_TOOL} bios-install ${CMAKE_BINARY_DIR}/Cryptix.iso
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${CRYPTIX_ISO_DIR}
  DEPENDS Cryptix.elf ${CMAKE_BINARY_DIR}/initrd.img.gz ${CMAKE_BINARY_DIR}/ksyms.sym
          ${CRYPTIX_LIMINE_TOOL} ${CRYPTIX_ISO_FILES}
  USES_TERMINAL VERBATIM)
add_custom_target(iso DEPENDS ${CMAKE_BINARY_DIR}/Cryptix.iso root_image)

add_custom_target(build DEPENDS iso)

# ------------------------------------------------------------------------------
# QEMU
# ------------------------------------------------------------------------------
set(CRYPTIX_QEMU qemu-system-${CRYPTIX_ARCH})
set(CRYPTIX_QEMU_ARGS
  # CPU
  -cpu max
  -smp 4
  # NUMA: two nodes
  -object memory-backend-ram,id=ram0,size=2048M
  -object memory-backend-ram,id=ram1,size=1024M
  -numa node,cpus=0-1,memdev=ram0
  -numa node,cpus=2-3,memdev=ram1
  -m 3G
  # debugging
  -d cpu_reset
  -d int,guest_errors
  -D qemu.log
  -boot order=d,menu=on,splash-time=100
  -rtc base=localtime
  -net none
  -smbios type=1
  # audio
  -audiodev id=audio,driver=alsa
  -object filter-dump,id=hue,netdev=breh,file=e1000.pcap
  -netdev user,id=breh,hostfwd=tcp:127.0.0.1:8888-10.0.2.15:8888,hostfwd=tcp:127.0.0.1:8823-10.0.2.15:23
  -device e1000,netdev=breh
  # drives
  -drive format=raw,file=image.hdd,if=none,id=drive1
  # nvme0 - nvme controller, nvme0n1 - nvme namespace of controller nvme0
  -device nvme,id=nvme0,serial=nvme,bus=pcie.0
  -device nvme-ns,drive=drive1,nsid=2,bus=nvme0,id=nvme0n1
  # pci-to-pci bridge
  -device pci-bridge,id=pcie2,bus=pcie.0,addr=1e,chassis_nr=1
  -vga std)

if(CRYPTIX_ARCH STREQUAL "x86_64")
  list(APPEND CRYPTIX_QEMU_ARGS
    -M q35
    -device isa-debug-exit
    -debugcon stdio                  # port 0xe9 -> stdio
    -serial file:debug.log
    -enable-kvm
    -machine pcspk-audiodev=audio
    -cpu host
    -device i2c-echo)
else()
  list(APPEND CRYPTIX_QEMU_ARGS
    -M virt
    -cpu cortex-a72
    -device ramfb
    -device qemu-xhci
    -device usb-kbd
    -device usb-mouse
    -serial stdio)
endif()

set(CRYPTIX_QEMU_ACCEL -M accel=kvm:hvf:whpx:haxm:tcg)

add_custom_target(run_bios
  COMMAND ${CRYPTIX_QEMU} ${CRYPTIX_QEMU_ARGS} -cdrom ${CMAKE_BINARY_DIR}/Cryptix.iso ${CRYPTIX_QEMU_ACCEL}
  DEPENDS iso
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  USES_TERMINAL VERBATIM)

set(CRYPTIX_OVMF ${ovmf_binaries_SOURCE_DIR}/${CRYPTIX_OVMF_FILE})

add_custom_target(run_uefi
  COMMAND ${CRYPTIX_QEMU} ${CRYPTIX_QEMU_ARGS} -cdrom ${CMAKE_BINARY_DIR}/Cryptix.iso
          -bios ${CRYPTIX_OVMF} ${CRYPTIX_QEMU_ACCEL}
  DEPENDS iso
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  USES_TERMINAL VERBATIM)

add_custom_target(run_uefi_vnc
  COMMAND ${CRYPTIX_QEMU} ${CRYPTIX_QEMU_ARGS} -display vnc=127.0.0.1:0
          -cdrom ${CMAKE_BINARY_DIR}/Cryptix.iso -bios ${CRYPTIX_OVMF}
  DEPENDS iso
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  USES_TERMINAL VERBATIM)

add_custom_target(run_uefi_gdb
  COMMAND ${CRYPTIX_QEMU} ${CRYPTIX_QEMU_ARGS} -s -S -cdrom ${CMAKE_BINARY_DIR}/Cryptix.iso
  DEPENDS iso
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  USES_TERMINAL VERBATIM)

add_custom_target(run_bochs
  COMMAND bochs -q -f ${PROJECT_SOURCE_DIR}/.bochsrc
  DEPENDS iso
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  USES_TERMINAL VERBATIM)

#!/usr/bin/env zsh
# usage: Meta/run.sh [build type] [architecture] [firmware: uefi | bios | uefi_vnc | uefi_gdb]

BUILD_TYPE=${1:-release}
TARGET_ARCH=${2:-x86_64}
FIRMWARE=${3:-uefi}
BUILD_DIR=build_${BUILD_TYPE}_$TARGET_ARCH

echo "running $BUILD_DIR"
cmake --build $BUILD_DIR --target run_$FIRMWARE

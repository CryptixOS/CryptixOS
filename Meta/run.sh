#!/usr/bin/env zsh

TARGET_ARCH=${2:-x86_64}
BUILD_DIR=build_${1:-release}_$TARGET_ARCH

echo "running $BUILD_DIR"
meson compile -C $BUILD_DIR run_${2:-uefi}

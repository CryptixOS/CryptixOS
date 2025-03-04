#!/usr/bin/env zsh

BUILD_DIR=build_${1:-release}

echo "running $BUILD_DIR"
meson compile -C $BUILD_DIR run_${2:-uefi}

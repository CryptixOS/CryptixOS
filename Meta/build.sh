#!/usr/bin/env zsh

BUILD_DIR=build_${1:-release}
echo $BUILD_DIR
meson compile -C $BUILD_DIR -j16 build
mkdir -p $BUILD_DIR/iso_root


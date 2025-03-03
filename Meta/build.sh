#!/usr/bin/env zsh

ninja -C build clang-format
meson compile -C build -j16 build
mkdir -p build/iso_root

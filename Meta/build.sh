#!/usr/bin/env zsh

meson compile -C ${1:-build_debug} -j16 build
mkdir -p build/iso_root

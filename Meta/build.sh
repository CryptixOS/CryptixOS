#!/usr/bin/env zsh

meson compile -C build -j16 build
mkdir -p build/iso_root

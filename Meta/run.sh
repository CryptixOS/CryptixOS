#!/usr/bin/env zsh

meson compile -C build run_${1:-uefi}

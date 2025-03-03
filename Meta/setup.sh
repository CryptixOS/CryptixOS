#/usr/bin/env zsh

rm -rf build
meson setup build --cross-file=CrossFiles/kernel-target-${1:-x86_64}.cross-file --force-fallback-for=fmt
mkdir -p build/iso_root


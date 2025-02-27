#/usr/bin/bash

meson setup --wipe build --cross-file=CrossFiles/kernel-target-${1:-x86_64}.cross-file --force-fallback-for=fmt &&
	mkdir -p build/iso_root


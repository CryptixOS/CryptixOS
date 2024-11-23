ARCH ?= x86_64

setup:
	meson setup --wipe build --cross-file=CrossFiles/kernel-target-$(ARCH).cross-file --force-fallback-for=fmt
	mkdir -p build/iso_root

run_uefi:
	ninja -C build run_uefi
run_gdb: 
	ninja -C build run_uefi_gdb
run_bios: 
	ninja -C build run_bios

build:
	ninja -C build

.PHONY: clean
clean:
	rm -r build


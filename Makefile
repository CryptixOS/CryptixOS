default_setup:
	meson setup build --cross-file=CrossFiles/kernel-target-aarch64.cross-file --force-fallback-for=fmt
	mkdir -p build/iso_root
setup_aarch64:
	meson setup --wipe build --cross-file=CrossFiles/kernel-target-aarch64.cross-file --force-fallback-for=fmt
	mkdir -p build/iso_root
setup_x86_64:
	meson setup --wipe build --cross-file=CrossFiles/kernel-target-x86_64.cross-file --force-fallback-for=fmt
	mkdir -p build/iso_root

run_uefi: default_setup
	ninja -C build run_uefi
run_gdb: default_setup
	ninja -C build run_uefi_gdb
run_bios:
	ninja -C build run_bios

build:
	ninja -C build

.PHONY: clean
clean:
	rm -r build


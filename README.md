### CryptixOS - Modern unix-like operating system being written in C++23

## Supported architectures

- x86-64
- aarch64

## Requirements

- nasm
- clang
- ld, lld
- meson
- ninja
- xorriso
- qemu

#

## How to build?

First setup meson build directory using one of the following
`make setup_x86_64`
`make setup_aarch64`

afterwards build and run the CryptixOS with uefi or bios **note:** bios is only supported for x86 architecture
`make run_uefi`
`make run_bios`


## References and Credits

***Meson build system*** - https://mesonbuild.com/



***Limine boot protocol*** - https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md



***OsDev wiki*** - https://wiki.osdev.org

### Third Party

***- Limine***, Modern and lightweight bootloader featuring the limine boot
protocol - https://github.com/limine-bootloader/limine.git



***- fmt***, An open-source formatting library providing a fast and safe alternative to C stdio and C++
iostreams - https://github.com/fmtlib/fmt.git




***- parallel-hashmap***, A set of excellent hashmap implementations - https://github.com/greg7mdp/parallel-hashmap



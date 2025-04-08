# CryptixOS 

<img src="./Meta/images/screenshot.png">

## Description
CryptixOS is a WIP 64 bit unix-like operating system,
it uses limine boot protocol to boot and mlibc as the c library implementation

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
- xbstrap

## How to build?

## System Root
The first thing you have to do in order to build CryptixOS is building the system root,
for that purpose you will need to use the `xbstrap` utility
here are the steps:
```
mkdir build-sysroot # create a directory in which we will build the sysroot
pushd build-sysroot # move into that directory
xbstrap init ..
xbstrap install base # build and install the base packages
```

## Actual building
Now you are able to build the actual kernel and generate an bootable .iso image
first choose the architecture you want to build the kernel for and generate the meson cache 
`./Meta/setup.sh <x86_64|aarch64>`
next you can just build the image
`./Meta/build.sh`

## Running
To run the CryptixOS you will use the ./run.sh script with either uefi or bios option(bios is only available for x86)
`./Meta/run.sh <run_uefi|run_bios>`

### References and Credits
* [Meson](https://mesonbuild.com/) - Meson is an open source build system meant to be both extremely fast, and, even more importantly, as user friendly as possible.
* [Limine boot protocol](https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md)
* [OsDev wiki](https://wiki.osdev.org)
* [mlibc wiki](https://github.com/managarm/mlibc.git) - mlibc is a fully featured C standard library designed with portability in mind.


### Third Party
* [Limine](https://github.com/limine-bootloader/limine.git) - Modern, advanced, portable, multiprotocol bootloader and boot manager, also used as the reference implementation for the Limine boot protocol.
* [compiler-rt-builtins](https://github.com/ilobilo/compiler-rt-builtins.git)
* [demangler](https://github.com/ilobilo/demangler.git) - C++, Microsoft C++, Rust and DLang name demangler
* [flanterm](https://github.com/mintsuki/flanterm.git) - Flanterm is a fast and reasonably complete terminal emulator with support for multiple output backends. Included is a fast framebuffer backend.
* [fmt](https://github.com/fmtlib/fmt.git) - fmt is an open-source formatting library providing a fast and safe alternative to C stdio and C++ iostreams.
* [libstdcxx-freestanding](https://github.com/ilobilo/libstdcxx-headers.git) - Headers from GCC's libstdc++ that can be used in a freestanding environment.
* [magic_enum](https://github.com/Neargye/magic_enum.git) - Header-only C++17 library provides static reflection for enums, work with any enum type without any macro or boilerplate code.
* [ovmf_binaries](https://github.com/ilobilo/ovmf-binaries.git) - Unofficial EDK2 nightly build
* [parallel_hashmap](https://github.com/greg7mdp/parallel-hashmap.git) - A set of excellent hash map implementations, as well as a btree alternative to std::map and std::set
* [smart_ptr](https://github.com/ilobilo/smart_ptr.git) - ISO C++ 2011 smart pointer implementation
* [string](https://github.com/ilobilo/string.git) - Single-header, almost fully complete C++20/23 std::string implementation with sso support.
* [uacpi](https://github.com/UltraOS/uACPI.git) - A portable and easy-to-integrate implementation of the Advanced Configuration and Power Interface (ACPI).
* [veque](https://github.com/Shmoopty/veque.git) - A very fast C++17 container combining the best features of std::vector and std::deque


## To Do

### Drivers
- [X] Kernel Module Loader
- [X] TTY
- [ ] PTY
- [X] PCI
- [X] PCIe
- [X] NVMe
- [ ] AHCI
- [X] Serial
- [X] PIT
- [X] Local APIC Timer
- [X] PIC
- [X] I/O APIC
- [X] I8042 Controller
- [X] PS/2 Keyboard
- [X] PC Speaker
- [X] RTC
- [X] HPET
- [ ] Device Tree
- [X] ACPI
- [X] Rtl8139

### File Systems
- [X] DevTmpFs
- [X] Ext2Fs
- [X] Fat32Fs
- [X] ProcFs
- [X] TmpFs
- [X] Initrd
- [ ] SysFs
- [ ] DevPtsFs
- [ ] Plan9Fs
- [ ] DevLoopFs
- [ ] Iso9660Fs
- [ ] EfiVarsFs

### Syscall
- [ ] Signals
- [ ] Thread API
- [X] VFS
- [X] Session Management
- [ ] Networking

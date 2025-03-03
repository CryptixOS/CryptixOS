FROM ubuntu:latest

RUN mkdir -p /cryptixos
WORKDIR /cryptixos

COPY . /cryptixos

RUN mkdir -p sysroot/usr/sbin/&&touch sysroot/usr/sbin/init
RUN apt update -y && apt install -y meson ninja-build clang lld llvm git cmake python3 python3-pip pipx qemu-system parted udev build-essentials
RUN pipx install xbstrap
RUN ./Meta/setup.sh x86_64
RUN ./Meta/build.sh



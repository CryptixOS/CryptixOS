# NixOS base image (we'll use nixos/nix as it’s a useful base for the Nix environment)
FROM nixos/nix:latest

# Install required packages via nix-env
RUN nix-env -iA nixpkgs.meson \
    nixpkgs.ninja \
    nixpkgs.clang \
    nixpkgs.lld \
    nixpkgs.llvmPackages_11.llvm \
    nixpkgs.git \
    nixpkgs.cmake \
    nixpkgs.python39Full \
    nixpkgs.buildPackages.gcc \
    nixpkgs.qemu \
    nixpkgs.parted \
    nixpkgs.udev \
    nixpkgs.bison \
    nixpkgs.flex \
    nixpkgs.gmp \
    nixpkgs.mpc \
    nixpkgs.mpfr \
    nixpkgs.texinfo \
    nixpkgs.isl \
    nixpkgs.gperf \
    nixpkgs.gettext \
    nixpkgs.autopoint \
    nixpkgs.readline \
    nixpkgs.groff \
    nixpkgs.zsh \
    nixpkgs.xorriso \
    nixpkgs.bochs \
    nixpkgs.qpdf \
    nixpkgs.fasm \
    && python3 -m ensurepip \
    && pip install --no-cache --upgrade pip \
    && pip install pipx

# Set up work directory and copy necessary files
WORKDIR /workspace
COPY . /workspace

# Default command (you can override it during container run)
CMD ["/bin/bash"]


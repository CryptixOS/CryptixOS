#!/usr/bin/env zsh

# Compresses the initrd tarball ($1) into a zlib stream ($2), the format
# Kernel/Library/ZLib decompresses (zlib header + deflate + adler32).
set -e
zlib-flate -compress < "$1" > "$2"


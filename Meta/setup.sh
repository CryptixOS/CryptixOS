#!/usr/bin/env zsh

# Function to set up a build directory
setup_build() {
  build_type=$1
  build_dir="build_$build_type"
  buildtype_flag=$2
  echo "Setting up $build_dir with buildtype=$buildtype_flag"
  meson setup "$build_dir" --cross-file="CrossFiles/kernel-target-${TARGET_ARCH}.cross-file" --force-fallback-for=fmt --buildtype="$buildtype_flag" --debug
}

# Remove existing build directories
rm -rf build_debug build_release build_dist


# Determine target architecture
TARGET_ARCH=${1:-x86_64}

# Determine which builds to set up
case $2 in
  debug)
    setup_build debug debug
    ;;
  release)
    setup_build release debugoptimized
    ;;
  dist)
    setup_build dist release
    ;;
  *)
    setup_build debug debug
    setup_build release debugoptimized
    setup_build dist release
    ;;
esac

# Create iso_root directory
mkdir -p build/iso_root

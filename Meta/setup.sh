#!/usr/bin/env zsh
set -e  # Exit immediately if a command fails

# Target architecture (default: x86_64)
TARGET_ARCH=${1:-x86_64}

# Define build configurations
declare -A BUILD_TYPES=(
  [debug]=debug
  [release]=debugoptimized
  [dist]=release
)

. Meta/log.sh

# If a specific build type is provided, use only that one; otherwise, set up all
BUILD_SELECTION=()
if [[ -n "$2" && -n ${BUILD_TYPES[$2]} ]]; then
  BUILD_SELECTION=($2)
else
  BUILD_SELECTION=(${(k)BUILD_TYPES})  # Get all keys (debug, release, dist)
fi

BUILD_DIR=""
# Function to set up a Meson build
setup_build() {
        BUILD_DIR="build_$1_$TARGET_ARCH"
  local build_dir="build_$1_$TARGET_ARCH"
  local build_type=$2
  log_info "🔧 Setting up $build_dir with buildtype=$build_type..."
  rm -rf "$build_dir"  # Remove only the relevant build directory
  meson setup "$build_dir" \
    --cross-file="CrossFiles/kernel-target-clang-${TARGET_ARCH}.cross-file" \
    --force-fallback-for=fmt \
    --buildtype="$build_type" \
}

# Iterate through selected build configurations and set them up
for build in $BUILD_SELECTION; do
  setup_build "$build" "${BUILD_TYPES[$build]}"
done

(cd subprojects/fmt-10.2.0 && patch -p1 < ../../fmt-10.2.0.patch)

# Ensure iso_root directory exists
mkdir -p $BUILD_DIR/iso_root
log_success "✅ All selected builds are set up!"


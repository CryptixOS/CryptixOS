#!/usr/bin/env zsh
set -e  # Exit immediately if a command fails

# Target architecture (default: x86_64)
TARGET_ARCH=${1:-x86_64}

# Define build configurations
BUILD_TYPES=(debug release dist)

. Meta/log.sh

# If a specific build type is provided, use only that one; otherwise, set up all
BUILD_SELECTION=()
if [[ -n "$2" && ${BUILD_TYPES[(Ie)$2]} -gt 0 ]]; then
  BUILD_SELECTION=($2)
else
  BUILD_SELECTION=($BUILD_TYPES)
fi

# Configure a CMake build (see CMakePresets.json)
setup_build() {
  local build_type=$1
  local build_dir="build_${build_type}_${TARGET_ARCH}"
  log_info "🔧 Setting up $build_dir with preset ${TARGET_ARCH}-${build_type}..."
  rm -rf "$build_dir"  # Remove only the relevant build directory
  cmake --preset "${TARGET_ARCH}-${build_type}"
}

# Iterate through selected build configurations and set them up
for build in $BUILD_SELECTION; do
  setup_build "$build"
done

log_success "✅ All selected builds are set up!"

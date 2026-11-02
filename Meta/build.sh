#!/usr/bin/env zsh
set -e  # Exit immediately if a command exits with a non-zero status

# Set build type and target architecture, with defaults
TARGET_ARCH=${1:-x86_64}
BUILD_TYPE=${2:-release}

. Meta/log.sh

# Define the build directory name
BUILD_DIR=build_${BUILD_TYPE}_$TARGET_ARCH
log_info "Build directory: $BUILD_DIR"

# Ensure the build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    log_error "Build directory '$BUILD_DIR' does not exist. Please run the CMake configure step (Meta/setup.sh) before compiling."
    exit 1
fi

# Compile the project using CMake
log_info "Starting compilation..."
if cmake --build "$BUILD_DIR" --target build -j5; then
    log_success "Compilation completed successfully."
else
    log_error "Compilation failed."
    exit 1
fi

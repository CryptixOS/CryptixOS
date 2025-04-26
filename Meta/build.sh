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
    log_error "Build directory '$BUILD_DIR' does not exist. Please run the Meson setup step before compiling."
    exit 1
fi

# Compile the project using Meson
log_info "Starting compilation..."
if meson compile -C "$BUILD_DIR" -j5 build; then
    log_success "Compilation completed successfully."
else
    log_error "Compilation failed."
    exit 1
fi

# Create the iso_root directory within the build directory
log_info "Creating iso_root directory..."
mkdir -p $BUILD_DIR/iso_root
log_success "iso_root directory created at '$BUILD_DIR/iso_root'."


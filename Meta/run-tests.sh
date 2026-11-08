#!/usr/bin/env bash
#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# Builds the host unit tests in Tests/ with CMake and runs them with CTest:
# the kernel code that runs on the host (GoogleTest) and Prism's own suite.
# Extra arguments go to ctest, e.g.
#   Meta/run-tests.sh -L Kernel          only the kernel tests
#   Meta/run-tests.sh -R ZLib -V         one test, verbose
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-$ROOT/build_tests}

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    cmake -S "$ROOT/Tests" -B "$BUILD_DIR" -G Ninja -DCTOS_TESTS_PRISM_SUITE=OFF
fi
cmake --build "$BUILD_DIR"
ctest --test-dir "$BUILD_DIR" --output-on-failure -j "$(nproc)" "$@"

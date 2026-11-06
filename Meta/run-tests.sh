#!/usr/bin/env bash
#*
#* Created by v1tr10l7 on 06.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
# Builds and runs the host unit tests in Tests/ (Prism's suite and the kernel
# code that runs on the host). Extra arguments go to `meson test`, e.g.
#   Meta/run-tests.sh --suite Kernel
#   Meta/run-tests.sh ZLib -v
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-$ROOT/build_tests}

if [[ ! -f "$BUILD_DIR/build.ninja" ]]; then
    meson setup "$BUILD_DIR" "$ROOT/Tests"
fi
meson compile -C "$BUILD_DIR"
meson test -C "$BUILD_DIR" --print-errorlogs "$@"

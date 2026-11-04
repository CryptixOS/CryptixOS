/*
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

// The kernel's stack trace support lives in the `Cryptix:Library.Stacktrace` module
// partition.  Prism, which is built from plain headers, still includes this path
// (and does not use anything from it), so keep an empty header around for it.

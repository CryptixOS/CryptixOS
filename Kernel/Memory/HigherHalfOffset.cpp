/*
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
// Prism is a header based library.  <Prism/Memory/Pointer.hpp> declares (and its inline
// code calls) `MM::HigherHalfOffset()`, a function that has to be provided by the kernel.
//
// That declaration belongs to the global module, whereas everything in `Cryptix` is
// attached to the named module (and mangled accordingly), so the definition cannot live in
// a module unit.  This ordinary translation unit forwards to the module's own accessor.
#include <Prism/Memory/Pointer.hpp>

import Cryptix;

usize MM::HigherHalfOffset() { return MM::GetHigherHalfOffset(); }

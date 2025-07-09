/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/ELF.hpp>
#include <VFS/VFS.hpp>

class ExecutableProgram
{
  public:
    ExecutableProgram() = default;

    ErrorOr<void> Load(PathView path, PageMap* pageMap,
                       AddressSpace& addressSpace, Pointer loadBase = 0);

    ELF::Image&   Image() { return m_Image; }

  private:
    ELF::Image m_Image;
};

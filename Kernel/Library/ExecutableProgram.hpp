/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/ELF.hpp>

class ExecutableProgram
{
  public:
    ExecutableProgram() = default;

    ErrorOr<void> Load(PathView path, PageMap* pageMap,
                       AddressSpace& addressSpace, Pointer loadBase = 0);
    Pointer PrepareStack(Pointer _stack, Pointer sp, Vector<StringView> argv,
                         Vector<StringView> envp);

    ELF::Image& Image() { return *m_Image.Raw(); }

    Pointer     EntryPoint() const { return m_EntryPoint; }
    Pointer     LoadBase() const { return m_LoadBase; }

  private:
    Ref<ELF::Image>          m_Image;
    Ref<ELF::Image>          m_Interpreter = nullptr;

    u64                      m_EntryPoint  = 0;
    u64                      m_LoadBase    = 0;
    // u64                      m_InterpreterBase = 0;

    ErrorOr<Ref<ELF::Image>> LoadImage(PathView path, PageMap* pageMap,
                                       AddressSpace& addressSpace,
                                       bool          interpreter = false);
};

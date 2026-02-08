/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/ELF/Image.hpp>
#include <Prism/Utility/Path.hpp>

class ExecutableProgram
{
  public:
    ExecutableProgram() = default;

    ErrorOr<void> Load(PathView path, PageMap* pageMap,
                       AddressSpace& addressSpace, Pointer loadBase = 0);
    ErrorOr<void> Load(::Ref<DirectoryEntry> entry, PageMap* pageMap,
                       AddressSpace& addressSpace, Pointer loadBase = 0);
    Pointer PrepareStack(Pointer _stack, Pointer sp, Vector<StringView> argv,
                         Vector<StringView> envp);

    ELF::Image& Image() { return *m_Image.Raw(); }

    Pointer     EntryPoint() const { return m_EntryPoint; }
    Pointer     LoadBase() const { return m_LoadBase; }
    Pointer     SignalTrampoline() { return m_SignalTrampoline; }

  private:
    Ref<ELF::Image>          m_Image;
    Ref<ELF::Image>          m_Interpreter      = nullptr;
    Path                     m_ExecutablePath   = ""_pv;

    u64                      m_EntryPoint       = 0;
    u64                      m_LoadBase         = 0;
    u64                      m_InterpreterBase  = 0;
    Pointer                  m_SignalTrampoline = nullptr;

    ErrorOr<Ref<ELF::Image>> LoadImage(PathView path, PageMap* pageMap,
                                       AddressSpace& addressSpace,
                                       bool          interpreter = false);

    ErrorOr<Ref<ELF::Image>> LoadImage(::Ref<DirectoryEntry> dentry,
                                       PageMap*              pageMap,
                                       AddressSpace&         addressSpace,
                                       bool interpreter = false);
};

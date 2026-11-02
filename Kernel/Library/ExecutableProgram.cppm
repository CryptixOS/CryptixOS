/*
 * Created by v1tr10l7 on 08.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Compiler.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>

export module Cryptix:Library.ExecutableProgram;

export import :Library.ELF.Image;

export class ExecutableProgram
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

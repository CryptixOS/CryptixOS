/*
 * Created by v1tr10l7 on 12.08.2025.
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

export module Cryptix:Library.ELF.Loader;

export import :Library.ELF.Image;
export import :Memory.PageTableEntry;

export class DirectoryEntry;
export namespace ELF
{
    class Loader
    {
      public:
        Loader();
        Loader(Ref<class Image> image)
            : m_Image(image)
        {
        }

        ErrorOr<void> LoadImage(PathView path);
        ErrorOr<void> LoadImage(Ref<DirectoryEntry> dentry);
        ErrorOr<void> LoadImage(Ref<INode> inode);
        ErrorOr<void> LoadImage(Ref<FileDescriptor> file);
        ErrorOr<void> LoadImage(u8* data, usize size);
        ErrorOr<void> LoadImage(Ref<Image> image);

        ErrorOr<void>
        AllocateMemory(PageMap& pageMap, AddressSpace& addressSpace,
                       PageAttributes flags    = PageAttributes::eRWX,
                       Pointer        loadBase = nullptr);
        ErrorOr<void> LoadSegments(PageMap& pageMap, AddressSpace& addressSpace,
                                   PageAttributes flags = PageAttributes::eRWX,
                                   Pointer        loadBase = nullptr);
        using SymbolLookup = Delegate<u64(StringView name)>;
        ErrorOr<void>    ResolveSymbols(SymbolLookup lookup);
        ErrorOr<void>    ApplyRelocations();

        Ref<class Image> Image() const { return m_Image; }
        inline Pointer   LoadBase() const { return m_LoadBase; }
        inline Pointer   VirtualBase() const
        {
            return m_LoadBase.Offset(m_MinVirt.Raw());
        }
        inline Pointer PhysicalBase() const { return m_Phys; }

        inline Pointer MinVirt() const { return m_MinVirt; }
        inline Pointer MaxVirt() const { return m_MaxVirt; }
        inline Pointer TrampolineVirt() const { return m_TrampolineVirt; }

        inline usize   AlignedSize() const { return m_AlignedSize; }

        inline Pointer EntryPoint() const { return m_EntryPoint; }
        Pointer        LookupSymbol(StringView name) const;

        using SymbolIterator
            = Delegate<IterationResult(StringView name, Pointer value)>;
        void           ForEachSymbol(SymbolIterator it);

        inline Pointer InitArray() const { return m_InitArray; }
        inline usize   InitArraySize() const { return m_InitArraySize; }

        inline Pointer FiniArray() const { return m_FiniArray; }
        inline usize   FiniArraySize() const { return m_FiniArraySize; }

      private:
        Ref<class Image>              m_Image;
        PageMap*                      m_PageMap        = nullptr;
        AddressSpace*                 m_AddressSpace   = nullptr;

        Pointer                       m_LoadBase       = nullptr;
        Pointer                       m_Phys           = nullptr;
        Pointer                       m_Bias           = 0;

        Pointer                       m_MinVirt        = 0;
        Pointer                       m_MaxVirt        = 0;
        Pointer                       m_TrampolineVirt = 0;

        usize                         m_Size           = 0;
        usize                         m_AlignedSize    = 0;
        usize                         m_VirtMisalign   = 0;

        RedBlackTree<StringView, u64> m_Symbols;
        Pointer                       m_EntryPoint      = nullptr;
        Path                          m_InterpreterPath = ""_pv;

        Pointer                       m_InitArray       = nullptr;
        usize                         m_InitArraySize   = 0;

        Pointer                       m_FiniArray       = nullptr;
        usize                         m_FiniArraySize   = 0;

        ErrorOr<void> ParseSegment(const ProgramHeader& segment);
        ErrorOr<void> ParseDynamic(const ProgramHeader& segment);
        ErrorOr<void> LoadSegment(const ProgramHeader& segment);
    };
}; // namespace ELF

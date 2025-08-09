/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Library/ELF_Definitions.hpp>

#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Core/Error.hpp>

#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>

#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>

class AddressSpace;
class PageMap;
class FileDescriptor;
class INode;

namespace ELF
{
    class Image : public RefCounted
    {
      public:
        ErrorOr<void> LoadFromMemory(u8* data, usize size);
        ErrorOr<void> Load(FileDescriptor* file, Pointer loadBase = 0);
        ErrorOr<void> Load(INode* inode, Pointer loadBase = 0);

        Pointer       Raw() const { return m_Image.Raw(); }
        inline const struct Header& Header() const { return m_Header; }
        inline ObjectType           Type() const { return m_Header.Type; }

        usize                       ProgramHeaderCount();
        struct ProgramHeader*       ProgramHeader(usize index);
        usize                       SectionHeaderCount();
        struct SectionHeader*       SectionHeader(usize index);

        using ProgramHeaderEnumerator = Delegate<bool(struct ProgramHeader*)>;
        void ForEachProgramHeader(ProgramHeaderEnumerator enumerator);

        using SectionHeaderEnumerator = Delegate<bool(struct SectionHeader*)>;
        void ForEachSectionHeader(SectionHeaderEnumerator enumerator);

        using SymbolEntryEnumerator
            = Delegate<bool(Symbol& symbol, StringView name)>;
        void ForEachSymbolEntry(SymbolEntryEnumerator enumerator);
        using SymbolEnumerator = Delegate<bool(StringView name, Pointer value)>;
        void ForEachSymbol(SymbolEnumerator enumerator);

        using SymbolLookup = Delegate<u64(StringView name)>;
        ErrorOr<void>  ApplyRelocations(SymbolLookup lookup);

        ErrorOr<void>  ResolveSymbols(SymbolLookup lookup);
        ErrorOr<void>  ResolveSymbols(struct SectionHeader& section,
                                      SymbolLookup          lookup);
        Pointer        LookupSymbol(StringView name);
        void           DumpSymbols();

        StringView     LookupString(usize index);

        inline Pointer EntryPoint() const
        {
            return m_AuxiliaryVector.EntryPoint;
        }
        inline Pointer ProgramHeaderAddress() const
        {
            return m_AuxiliaryVector.ProgramHeaderAddress;
        }
        inline usize ProgramHeaderEntrySize() const
        {
            return m_AuxiliaryVector.ProgramHeaderEntrySize;
        }

        inline PathView InterpreterPath() const { return m_InterpreterPath; }

      private:
        Buffer                        m_Image;
        Pointer                       m_LoadBase = nullptr;

        struct Header                 m_Header;
        AuxiliaryVector               m_AuxiliaryVector;

        struct SectionHeader*         m_SymbolSection = nullptr;
        struct SectionHeader*         m_StringSection = nullptr;
        struct SectionHeader*         m_GotSection    = nullptr;
        const char*                   m_StringTable   = nullptr;

        RedBlackTree<StringView, u64> m_Symbols;
        StringView                    m_InterpreterPath;

        ErrorOr<void>                 Parse();
        bool                          ParseSectionHeaders();
        bool                          LoadSymbols();

        template <typename T>
        void Read(T* buffer, isize offset, isize count = sizeof(T))
        {
            Memory::Copy(buffer, m_Image.Raw() + offset, count);
        }
    };
}; // namespace ELF

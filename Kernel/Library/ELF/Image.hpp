/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Library/ELF/Definitions.hpp>

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
        ErrorOr<void>  LoadFromMemory(u8* data, usize size);
        ErrorOr<void>  Load(FileDescriptor* file, Pointer loadBase = 0);
        ErrorOr<void>  Load(INode* inode, Pointer loadBase = 0);

        inline Pointer Raw() const { return m_Image.Raw(); }
        inline Pointer LoadBase() const { return m_LoadBase; }
        inline usize   LoadSize() const { return m_Size; }

        inline const struct Header& Header() const { return m_Header; }
        inline ObjectType           Type() const { return m_Header.Type; }

        inline bool                 IsRelocatable() const
        {
            return Type() == ObjectType::eRelocatable;
        }
        inline bool IsExecutable() const
        {
            return Type() == ObjectType::eExecutable;
        }
        inline bool IsShared() const { return Type() == ObjectType::eShared; }

        usize       ProgramHeaderCount();
        const struct ProgramHeader& ProgramHeader(usize index);
        usize                       SectionHeaderCount();
        const struct SectionHeader& SectionHeader(usize index);

        using ProgramHeaderIterator
            = Delegate<IterationResult(const struct ProgramHeader&)>;
        void ForEachProgramHeader(ProgramHeaderIterator it);

        using SectionHeaderIterator
            = Delegate<IterationResult(const struct SectionHeader&)>;
        void ForEachSectionHeader(SectionHeaderIterator& it);

        using RelocationEntryIterator = Delegate<IterationResult(
            const struct SectionHeader&, const struct RelocationEntry&)>;
        void ForEachRelocationEntry(RelocationEntryIterator it);

        using SymbolEntryIterator
            = Delegate<IterationResult(Symbol& symbol, StringView name)>;
        void ForEachSymbolEntry(SymbolEntryIterator it);
        using SymbolIterator
            = Delegate<IterationResult(StringView name, Pointer value)>;
        void        ForEachSymbol(SymbolIterator it);

        Pointer     LookupSymbol(StringView name) const;
        void        DumpSymbols();

        inline void LoadSymbols(RedBlackTree<StringView, u64>& symbols)
        {
            m_Symbols.Clear();
            for (auto& [symbol, value] : symbols) m_Symbols[symbol] = value;
        }
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

        inline struct SectionHeader* StringSection() const
        {
            return m_StringSection;
        }

        inline PathView InterpreterPath() const { return m_InterpreterPath; }

      private:
        Buffer                        m_Image;
        Pointer                       m_LoadBase = nullptr;
        usize                         m_Size     = 0;

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

/*
 * Created by v1tr10l7 on 12.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/ELF/Image.hpp>

namespace ELF
{
    class Loader
    {
      public:
        Loader() = default;
        Loader(Ref<class Image> image)
            : m_Image(image)
        {
        }

        ErrorOr<void> LoadSegments();
        using SymbolLookup = Delegate<u64(StringView name)>;
        ErrorOr<void>    ResolveSymbols(SymbolLookup lookup);
        ErrorOr<void>    ApplyRelocations();

        Ref<class Image> Image() const { return m_Image; }
        inline Pointer   LoadBase() const { return m_LoadBase; }

        inline Pointer   EntryPoint() const { return m_EntryPoint; }
        Pointer          LookupSymbol(StringView name) const;

        using SymbolIterator
            = Delegate<IterationResult(StringView name, Pointer value)>;
        void           ForEachSymbol(SymbolIterator it);

        inline Pointer InitArray() const { return m_InitArray; }
        inline usize   InitArraySize() const { return m_InitArraySize; }

        inline Pointer FiniArray() const { return m_FiniArray; }
        inline usize   FiniArraySize() const { return m_FiniArraySize; }

      private:
        Ref<class Image>              m_Image;

        Pointer                       m_LoadBase = nullptr;
        usize                         m_Size     = 0;

        RedBlackTree<StringView, u64> m_Symbols;
        Pointer                       m_EntryPoint    = nullptr;

        Pointer                       m_InitArray     = nullptr;
        usize                         m_InitArraySize = 0;

        Pointer                       m_FiniArray     = nullptr;
        usize                         m_FiniArraySize = 0;

        IterationResult               LoadSegment(const ProgramHeader& header);
    };
}; // namespace ELF

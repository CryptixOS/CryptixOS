/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Pointer.hpp>
#include <Prism/Types.hpp>

#include <compare>
#include <vector>

namespace Prism
{
    struct StackFrame
    {
        StackFrame* PreviousFrame      = nullptr;
        Pointer     InstructionPointer = nullptr;
    };
    using OnFrameCallback = void (*)(StackFrame* currentFrame);

    class Stacktrace
    {
      public:
        struct Symbol
        {
            char*                          Name;
            Pointer                        Address;

            const std::string              Demangle() const;

            constexpr std::strong_ordering operator<=>(const Symbol& rhs) const
            {
                return Address <=> rhs.Address;
            }
        };

        Stacktrace() = default;
        Stacktrace(Pointer frameAddress, usize skipFrames = 0,
                   usize maxDepth = 32);

        ErrorOr<void> LoadSymbols(std::vector<Symbol>&& symbols,
                                  PhysAddr lowestSymbolAddres   = 0x0000'0000,
                                  PhysAddr highestSymbolAddress = 0xffff'ffff)
        {
            m_Symbols = std::move(symbols);

            return {};
        }
        const Symbol*          GetSymbol(PhysAddr addr) const;

        constexpr StackFrame** begin() { return m_Frames.begin(); }
        constexpr StackFrame** end() { return m_Frames.end(); }

        static Stacktrace      GetCurrent();

      private:
        std::vector<StackFrame*> m_Frames;

        std::vector<Symbol>      m_Symbols;
        PhysAddr                 m_LowestSymbolAddress  = 0x0000'0000;
        PhysAddr                 m_HighestSymbolAddress = 0xffff'ffff;
    }; // namespace Stacktrace
};     // namespace Prism

/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Stacktrace.hpp"

#include "Common.hpp"
#include "Utility/BootInfo.hpp"

#include <demangler/Demangle.h>

namespace Stacktrace
{
    namespace
    {
        struct StackFrame
        {
            StackFrame* rbp;
            uintptr_t   rip;
        };

        struct Symbol
        {
            char*     name;
            uintptr_t address;
        };

        usize     symbolCount                = 0;
        Symbol*   symbols                    = nullptr;
        uintptr_t lowestKernelSymbolAddress  = 0xffffffff;
        uintptr_t highestKernelSymbolAddress = 0x00000000;

        u64       ParseHexDigit(char digit)
        {
            if (digit >= '0' && digit <= '9') return digit - '0';
            Assert(digit >= 'a' && digit <= 'f');

            return (digit - 'a') + 0xa;
        }

        Symbol* GetSymbol(uintptr_t address)
        {
            if (address < lowestKernelSymbolAddress
                || address > highestKernelSymbolAddress)
                return nullptr;
            Symbol* ret = nullptr;

            for (usize i = 0; i < symbolCount; ++i)
            {
                if (!symbols) break;
                if (address < symbols[i + 1].address)
                {
                    ret = &symbols[i];
                    break;
                }
            }

            return ret;
        }
    }; // namespace

    bool Initialize()
    {
        LogTrace("Stacktrace: Loading kernel symbols...");
        limine_file* file = BootInfo::FindModule("ksyms");
        if (!file || !file->address) return false;

        auto* current     = reinterpret_cast<char*>(file->address);
        char* startOfName = nullptr;
        for (usize i = 0; i < 8; ++i)
            symbolCount = (symbolCount << 4) | ParseHexDigit(*(current++));
        current++;

        symbols                      = new Symbol[symbolCount];
        usize     currentSymbolIndex = 0;

        uintptr_t address            = 0;
        while ((const u8*)current
               < reinterpret_cast<u8*>(file->address) + file->size)
        {
            for (usize i = 0; i < sizeof(void*) * 2; ++i)
                address = (address << 4) | ParseHexDigit(*(current++));
            current += 3;

            startOfName = current;
            while (*(++current))
                if (*current == '\n') break;
            auto& ksym   = symbols[currentSymbolIndex];
            ksym.address = address;
            ksym.name    = startOfName;

            *current     = '\0';
            if (ksym.address < lowestKernelSymbolAddress)
                lowestKernelSymbolAddress = ksym.address;
            if (ksym.address > highestKernelSymbolAddress)
                highestKernelSymbolAddress = ksym.address;

            ++current;
            ++currentSymbolIndex;
        }

        // TODO: Free pages
        LogInfo("Stacktrace: kernel symbols loaded");
        return true;
    }
    void Print(usize maxFrames)
    {
        auto stackFrame
            = reinterpret_cast<StackFrame*>(CTOS_GET_FRAME_ADDRESS(0));

        for (usize i = 0; stackFrame && i < maxFrames; i++)
        {
            u64 rip = stackFrame->rip;
            if (rip == 0) break;
            stackFrame         = stackFrame->rbp;
            Symbol*     symbol = GetSymbol(rip);

            std::string demangledName
                = symbol ? llvm::demangle(symbol->name) : "??";
            LogMessage("[\u001b[33mStacktrace\u001b[0m]: {}. {} <{:#x}>", i + 1,
                       demangledName, rip);
        }
    }
}; // namespace Stacktrace

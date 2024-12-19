/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Stacktrace.hpp"

#include "Common.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include "Boot/BootInfo.hpp"
#include "Utility/Math.hpp"

#include <demangler/Demangle.h>
#include <vector>

namespace Stacktrace
{
    namespace
    {
        struct StackFrame
        {
            StackFrame* rbp;
            uintptr_t   rip;
        };

        std::vector<Symbol> symbols;
        uintptr_t           lowestKernelSymbolAddress  = 0xffffffff;
        uintptr_t           highestKernelSymbolAddress = 0x00000000;

        u64                 ParseHexDigit(char digit)
        {
            if (digit >= '0' && digit <= '9') return digit - '0';
            Assert(digit >= 'a' && digit <= 'f');

            return (digit - 'a') + 0xa;
        }

        const Symbol* GetSymbol(uintptr_t address)
        {
            if (address < lowestKernelSymbolAddress
                || address > highestKernelSymbolAddress)
                return nullptr;
            const Symbol* ret = nullptr;

            for (const auto& symbol : symbols)
            {
                if ((&symbol + 1) == symbols.end()) break;
                if (address < (&symbol + 1)->address)
                {
                    ret = &symbol;
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

        auto*     current     = reinterpret_cast<char*>(file->address);
        char*     startOfName = nullptr;

        uintptr_t address     = 0;
        while ((const u8*)current
               < reinterpret_cast<u8*>(file->address) + file->size)
        {
            for (usize i = 0; i < sizeof(void*) * 2; ++i)
                address = (address << 4) | ParseHexDigit(*(current++));
            current += 3;

            startOfName = current;
            while (*(++current))
                if (*current == '\n') break;

            Symbol ksym;
            if (address < BootInfo::GetKernelVirtualAddress().Raw<>())
                address += BootInfo::GetKernelVirtualAddress().Raw<>();

            ksym.address = address;
            ksym.name    = startOfName;
            symbols.push_back(ksym);

            *current = '\0';
            if (ksym.address < lowestKernelSymbolAddress)
                lowestKernelSymbolAddress = ksym.address;
            if (ksym.address > highestKernelSymbolAddress)
                highestKernelSymbolAddress = ksym.address;

            ++current;
        }

        PMM::FreePages(FromHigherHalfAddress<uintptr_t>(file->address),
                       Math::AlignUp(file->size, PMM::PAGE_SIZE)
                           / PMM::PAGE_SIZE);
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
            stackFrame           = stackFrame->rbp;
            const Symbol* symbol = GetSymbol(rip);

            std::string   demangledName
                = symbol ? llvm::demangle(symbol->name) : "??";
            LogMessage("[\u001b[33mStacktrace\u001b[0m]: {}. {} <{:#x}>", i + 1,
                       demangledName, rip);
        }
    }
}; // namespace Stacktrace

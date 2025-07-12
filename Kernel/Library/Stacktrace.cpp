/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootInfo.hpp>

#include <Common.hpp>

#include <Library/Stacktrace.hpp>

#include <Memory/MemoryManager.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/String/String.hpp>
#include <Prism/Utility/Math.hpp>

#include <demangler/Demangle.h>

namespace Stacktrace
{
    namespace
    {
        struct StackFrame
        {
            StackFrame* Base;
            Pointer     InstructionPointer;
        };

        Vector<Symbol> s_Symbols;
        Pointer        s_LowestKernelSymbolAddress  = 0xffffffff;
        Pointer        s_HighestKernelSymbolAddress = 0x00000000;

        u64            ParseHexDigit(char digit)
        {
            if (digit >= '0' && digit <= '9') return digit - '0';
            Assert(digit >= 'a' && digit <= 'f');

            return (digit - 'a') + 0xa;
        }

        const Symbol* GetSymbol(Pointer address)
        {
            if (address < s_LowestKernelSymbolAddress
                || address > s_HighestKernelSymbolAddress)
                return nullptr;
            const Symbol* ret = nullptr;

            for (const auto& symbol : s_Symbols)
            {
                if ((&symbol + 1) == s_Symbols.end()) break;
                if (address < (&symbol + 1)->Address)
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
        LogTrace("Stacktrace: Loading kernel s_Symbols...");
        limine_file* file = BootInfo::FindModule("ksyms");
        if (!file || !file->address) return false;

        Pointer fileStart = file->address;
        Pointer fileEnd   = fileStart.Offset(file->size);
        char*   current   = fileStart.As<char>();

        Pointer address   = 0;
        while (current < fileEnd.As<char>())
        {
            for (usize i = 0; i < sizeof(void*) * 2; ++i)
                address = (address.Raw() << 4) | ParseHexDigit(*(current++));
            current += 3;

            StringView name = current;
            while (*(++current))
                if (*current == '\n') break;

            Symbol ksym;
            if (address < MemoryManager::KernelVirtualAddress().Raw<>())
                address += MemoryManager::KernelVirtualAddress().Raw<>();

            ksym.Address = address;
            ksym.Name    = name.Substr(0, name.FindFirstOf('\n'));
            s_Symbols.PushBack(ksym);

            *current = '\0';
            if (ksym.Address < s_LowestKernelSymbolAddress)
                s_LowestKernelSymbolAddress = ksym.Address;
            if (ksym.Address > s_HighestKernelSymbolAddress)
                s_HighestKernelSymbolAddress = ksym.Address;

            ++current;
        }

        u64 filePageCount = Math::DivRoundUp(file->size, PMM::PAGE_SIZE);
        PMM::FreePages(fileStart.FromHigherHalf(), filePageCount);
        LogInfo("Stacktrace: kernel symbols loaded");
        return true;
    }
    void Print(usize maxFrames)
    {
        auto stackFrame = reinterpret_cast<StackFrame*>(CtosGetFrameAddress(0));

        for (usize i = 0; stackFrame && i < maxFrames; i++)
        {
            auto rip = stackFrame->InstructionPointer;
            if (!rip.IsHigherHalf() || rip < s_LowestKernelSymbolAddress
                || rip > s_HighestKernelSymbolAddress)
                break;

            stackFrame           = stackFrame->Base;
            const Symbol* symbol = GetSymbol(rip);

            auto          demangledName
                = symbol ? llvm::demangle(symbol->Name.Raw()) : "??";
            LogMessage("[\u001b[33mStacktrace\u001b[0m]: {}. {} <{:#x}>\n",
                       i + 1, demangledName, rip.Raw());
        }
    }
}; // namespace Stacktrace

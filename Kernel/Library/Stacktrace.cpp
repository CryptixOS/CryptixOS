/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Boot/BootModuleInfo.hpp>
#include <Debug/Debug.hpp>
#include <Library/Stacktrace.hpp>

#include <Memory/MM.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/String/String.hpp>
#include <Prism/Utility/Math.hpp>

#include <System/System.hpp>

#include <demangler/Demangle.h>

using ConstructorFunction = void (*)();

extern "C" ConstructorFunction __init_array_start[];
extern "C" ConstructorFunction __init_array_end[];
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
    }; // namespace

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
    bool Initialize()
    {
        LogTrace("Stacktrace: Loading kernel s_Symbols...");
        const BootModuleInfo* module = System::FindBootModule("ksyms");
        if (!module || !module->LoadAddress || module->Size == 0) return false;

        Pointer fileStart  = module->LoadAddress;
        Pointer fileEnd    = fileStart.Offset(module->Size);
        char*   current    = fileStart.As<char>();

        auto    kernelVirt = MM::KernelVirtualAddress();
        Pointer address    = 0;
        while (current < fileEnd.As<char>())
        {
            for (usize i = 0; i < sizeof(void*) * 2; ++i)
                address = (address.Raw() << 4) | ParseHexDigit(*(current++));
            current += 3;

            StringView name = current;
            while (*(++current))
                if (*current == '\n') break;

            Symbol ksym;
            if (address < kernelVirt) address += kernelVirt;

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

        u64 filePageCount = Math::DivRoundUp(module->Size, PMM::PAGE_SIZE);
        PMM::FreePages(fileStart.FromHigherHalf(), filePageCount);
        LogInfo("Stacktrace: kernel symbols loaded");

#if CTOS_DUMP_INIT_ARRAY != 0
        LogTrace("System: Dumping init array =>");
        for (ConstructorFunction* entry = __init_array_start;
             entry < __init_array_end; entry++)
        {
            ConstructorFunction constructor = *entry;
            auto                sym         = GetSymbol(constructor);
            if (!sym) continue;

            LogTrace("\t{:#016p} => {}", Pointer(constructor).Raw(), sym->Name);
        }
#endif

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

            if (symbol->Name.StartsWith("interrupt_handler")) break;
        }
    }
}; // namespace Stacktrace

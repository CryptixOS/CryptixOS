/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Syscall.hpp>
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
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/PathView.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>
#include <Prism/Core/UUID.hpp>
#include <Prism/Network/IPv4Address.hpp>
#include <Debug/Debug.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/UnixTypes.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Core/Iterator.hpp>
#include <demangler/Demangle.h>

module Cryptix:Impl.Library.Stacktrace;

import :API.Syscall;
import :Arch.CPU;
import :Boot.BootModuleInfo;
import :Library.Stacktrace;
import :Memory.MM;
import :Memory.PMM;
import :Memory.VMM;
import :System.System;

using ConstructorFunction = void (*)();

extern "C" ConstructorFunction __init_array_start[];
extern "C" ConstructorFunction __init_array_end[];
namespace Stacktrace
{
    namespace
    {
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
            while (*(++current) && current < fileEnd.As<char>())
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

        LogInfo("Stacktrace: kernel symbols loaded");
        // u64 filePageCount = Math::DivRoundUp(module->Size, PMM::PAGE_SIZE);
        // PMM::FreePages(fileStart.FromHigherHalf(), filePageCount);

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
    void Print(StackFrame* stackFrame, usize maxFrames)
    {
        auto syscallID
            = CPU::Current()
                ? static_cast<Syscall::ID>(CPU::Current()->LastSyscallID)
                : static_cast<Syscall::ID>(0);
        LogDebug("Last failed syscall => {:#x}({})", ToUnderlying(syscallID),
                 ToString(syscallID));
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
    void Print(usize maxFrames)
    {
        return Print(reinterpret_cast<StackFrame*>(CtosGetFrameAddress(0)),
                     maxFrames);
    }
}; // namespace Stacktrace

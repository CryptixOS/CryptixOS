/*
 * Created by v1tr10l7 on 20.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Types.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <API/Posix/termios.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
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
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Firmware/EFI/Types.hpp>

module Cryptix:Impl.Arch.PowerManager;

import :Arch.PowerManager;
#if defined(CTOS_TARGET_X86_64)
import :Arch.x86_64.IO;
#endif
import :Drivers.TTY.VirtualConsole;
import :Firmware.ACPI.ACPI;
import :Firmware.EFI.SystemTable;
import :Memory.VMM;

#ifdef CTOS_TARGET_X86_64
#endif

namespace PowerManager
{
    void Reboot()
    {
        VirtualConsole* terminal = VirtualConsole::GetPrimary();

        terminal->Clear();
        auto print = [terminal](const char* string)
        {
            LogTrace("{}", string);
            terminal->PrintString(string);
        };

        print("PowerManager: Restarting system...\n");
        print("PowerManager: Trying to reboot via ACPI...");
        ACPI::Reboot();
        print("PowerManager: Rebooting via ACPI was unsuccessful");

        print("PowerManager: Trying to reboot via EFI Runtime Services...");
        EFI::SystemTable* systemTable = EFI::g_SystemTable;

        Assert(systemTable);
        CTOS_UNUSED auto runtimeServices
            = VMM::MapIoRegion<EFI::RuntimeServices>(
                Pointer(systemTable->RuntimeServices), true);
        // runtimeServices->ResetSystem(EFI::ResetType::eShutdown, 0, 0,
        // nullptr);

        // TODO(v1tr10l7): UEFI Service
        print(
            "PowerManager: Rebooting via EFI Runtime Services was "
            "unsuccessful");
        Arch::Reboot();
    }
    void PowerOff()
    {
#ifdef CTOS_TARGET_X86_64
        // Qemu shutdown
        IO::Out<u16>(0x604, 0x2000);
        IO::Out<u16>(0xb004, 0x2000);

        // Virtualbox shutdown
        IO::Out<u16>(0x4004, 0x3400);
#endif
    }
}; // namespace PowerManager

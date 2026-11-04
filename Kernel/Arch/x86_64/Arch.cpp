/*gg
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Arch/Arch.hpp>
#include <Compiler.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Prism/String/StringUtils.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <API/UnixTypes.hpp>
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
#include <Prism/Core/Singleton.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Containers/Array.hpp>

module Cryptix:Impl.Arch.x86_64.Arch;

import :Arch.Arch;
import :Arch.x86_64.CPU;
import :Arch.x86_64.Drivers.IoApic;
import :Arch.x86_64.Drivers.PCSpeaker;
import :Arch.x86_64.Drivers.PIC;
import :Arch.x86_64.Drivers.Time.HPET;
import :Arch.x86_64.Drivers.Time.PIT;
import :Arch.x86_64.Drivers.Time.RTC;
import :Time.Time;

namespace InterruptManager
{
    void SetController(Ref<InterruptController> ctrl);
};
namespace Arch
{
    KERNEL_INIT_SECTION
    void InstallExceptions()
    {
        GDT::Initialize();
        GDT::Load(0);

        IDT::Initialize();
        IDT::Load();
    }
    KERNEL_INIT_SECTION
    void Initialize()
    {
        auto pic = I8259A::Instance();
        if (!pic->Initialize())
            LogError("Arch: Failed to initialize I8259A controller");

        auto ioApic = CreateRef<IoApicController>();
        if (!ioApic->Initialize())
        {
            LogError("Arch: Failed to initialize io apic");
            InterruptManager::SetController(pic);
        }
        else InterruptManager::SetController(ioApic);

        if (!HPET::DetectAndSetup()) LogError("HPET: Not Available");
        Assert(Time::RegisterTimer(PIT::Instance()));
        CPU::InitializeBSP();
        CPU::StartAPs();

        PCSpeaker::ToneOn(1000);
        IO::Delay(1000);
        PCSpeaker::ToneOff();

        LogInfo("Date: {:02}/{:02}/{:04} {:02}:{:02}:{:02}", RTC::GetDay(),
                RTC::GetMonth(), RTC::GetCentury() * 100 + RTC::GetYear(),
                RTC::GetHour(), RTC::GetMinute(), RTC::GetSecond());
    }

    __attribute__((noreturn)) void Halt()
    {
        for (;;) Asm("hlt");

        Panic("Shouldn't Reach");
        AssertNotReached();
    }
    void Pause() { Asm("pause"); }

    void PowerOff() {}
    void Reboot()
    {
        CPU::DisableInterrupts();

        u8 status = 0;
        do {
            status = IO::In<byte>(0x64);
            if (status & Bit(0)) IO::In<byte>(0x60);
        } while (status & Bit(1));

        IO::Out<byte>(0x64, 0xfe);
        IO::Out<word>(0x604, 0x2000);

        // if it failed, just triple fault
        struct CTOS_PACKED
        {
            u16      Limit = 0;
            upointer Base  = 0;
        } invalidIDT;
        Asm("lidt %0" ::"m"(invalidIDT));

        for (;;) HaltAndCatchFire(nullptr, nullptr);
    }
    time_t GetEpoch() { return RTC::CurrentTime(); }
}; // namespace Arch

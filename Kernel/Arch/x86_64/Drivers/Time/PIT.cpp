/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Singleton.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/RefCounted.hpp>
#include <Library/Module.hpp>
#include <Compiler.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Utility/Path.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Core/Concepts.hpp>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Config.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>

module Cryptix:Impl.Arch.x86_64.Drivers.Time.PIT;

import :Arch.x86_64.Drivers.Time.PIT;
import :Arch.x86_64.IO;
import :Scheduler.Scheduler;
import :System.InterruptManager;
import :Time.Time;

PIT* PIT::s_Instance = nullptr;

PIT::PIT()
    : HardwareTimer(Name())
{
    LogTrace("PIT: Initializing...");
    auto success = SetFrequency(FREQUENCY / 10);
    if (!success) LogWarn("PIT: Failed to set frequency to {:#x}", FREQUENCY);

    LogInfo("PIT: Frequency set to {}Hz", FREQUENCY);

    m_Dispatcher = InterruptManager::AllocateHandler(IRQ_LINE, this, "cryptix");

    // TODO(v1tr10l7): IDT::SetIST(m_Dispatcher->IrqLine() + 0x20, 1);
    m_Dispatcher->SetHandler(Tick);

    LogInfo("PIT: Installed on interrupt handler at irq line #{:#x}",
            m_Dispatcher->IrqLine());
}

void          PIT::Initialize() { s_Instance = Instance(); }
bool          PIT::IsInitialized() { return s_Instance != nullptr; }

ErrorOr<void> PIT::Start(TimerMode mode, Timestep interval)
{
    if (mode == TimerMode::eOneShot) m_CurrentMode = Mode::eOneShot;
    else if (mode == TimerMode::ePeriodic) m_CurrentMode = Mode::eSquareWave;
    else return Error(ENOSYS);

    usize reloadValue = (interval.Milliseconds() * BASE_FREQUENCY) / 3000;
    SetReloadValue(reloadValue);
    IO::Out<byte>(COMMAND, CHANNEL0_DATA | SEND_WORD | m_CurrentMode);

    m_Dispatcher->Unmask();
    return {};
}
void PIT::Stop()
{
    m_Dispatcher->Mask();
    SetReloadValue(0);
}

u64 PIT::CurrentCount()
{
    IO::Out<byte>(COMMAND, SELECT_CHANNEL0);
    auto lo = IO::In<byte>(CHANNEL0_DATA);
    auto hi = IO::In<byte>(CHANNEL0_DATA) << 8;
    return static_cast<u16>(hi << 8) | lo;
}

ErrorOr<void> PIT::SetFrequency(usize frequency)
{
    u64 reloadValue = BASE_FREQUENCY / frequency;
    if (BASE_FREQUENCY % frequency > frequency / 2) reloadValue++;
    SetReloadValue(reloadValue);

    return {};
}
void PIT::SetReloadValue(u16 reloadValue)
{
    IO::Out<byte>(COMMAND, SEND_WORD | m_CurrentMode);
    IO::Out<byte>(CHANNEL0_DATA, static_cast<byte>(reloadValue));
    IO::Out<byte>(CHANNEL0_DATA, static_cast<byte>(reloadValue >> 8));
}

IrqResult PIT::Tick(Device* device, struct ExecutionContext* ctx)
{
    Instance()->m_Tick++;

    auto& callback = Instance()->m_OnTickCallback;
    if (callback) callback(ctx);
    Time::Tick((1'000 / FREQUENCY) * 1'000'000);

    return IrqResult::eHandled;
}

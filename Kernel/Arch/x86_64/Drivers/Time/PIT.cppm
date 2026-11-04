/*
 * Created by v1tr10l7 on 24.05.2024.
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
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
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

export module Cryptix:Arch.x86_64.Drivers.Time.PIT;

export import :System.InterruptHandler;
export import :Time.HardwareTimer;

export class PIT : public HardwareTimer, public Singleton<PIT>
{
  public:
    PIT();

    static void        Initialize();
    static bool        IsInitialized();

    virtual StringView Name() const PM_NOEXCEPT override { return "i8253"_sv; }
    StringView         ModelString() const override { return "i8253"_sv; }

    ErrorOr<void>      Start(TimerMode mode, Timestep interval) override;
    void               Stop() override;

    u64                CurrentCount();

    ErrorOr<void>      SetFrequency(usize frequency) override;
    void               SetReloadValue(u16 reloadValue);

    static constexpr usize BASE_FREQUENCY  = 1193182ull;
    static constexpr usize SEND_WORD       = 0x30;

    static constexpr usize CHANNEL0_DATA   = 0x40;
    static constexpr usize CHANNEL1_DATA   = 0x41;
    static constexpr usize CHANNEL2_DATA   = 0x42;
    static constexpr usize COMMAND         = 0x43;

    static constexpr usize SELECT_CHANNEL0 = 0x00;
    static constexpr usize SELECT_CHANNEL1 = 0x40;
    static constexpr usize SELECT_CHANNEL2 = 0x80;

    enum Mode
    {
        eCountDown  = 0x00,
        eOneShot    = 0x02,
        eRate       = 0x04,
        eSquareWave = 0x06,
    };

  private:
    static PIT*                            s_Instance;
    class ::Ref<class InterruptDispatcher> m_Dispatcher  = nullptr;

    // NOTE(v1tr10l7): When using PIC, we cannot choose irq number of PIT,
    // and it will have to be IRQ, only IoApic allows to redirect irqs
    usize                                  m_CurrentMode = Mode::eRate;
    Atomic<u64>                            m_Tick        = 0;

    static constexpr usize                 FREQUENCY     = 1000;
    static constexpr usize                 IRQ_LINE      = 0x00;

    static IrqResult Tick(Device* device, struct ExecutionContext* ctx);
}; // namespace PIT

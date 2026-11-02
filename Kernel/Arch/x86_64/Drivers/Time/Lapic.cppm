/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/Singleton.hpp>
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
#include <Prism/Core/Types.hpp>
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

export module Cryptix:Arch.x86_64.Drivers.Time.Lapic;

export import :System.InterruptHandler;
export import :Time.HardwareTimer;

export class Lapic : public HardwareTimer, public Singleton<Lapic>
{
  public:
    enum class Mode : u8
    {
        eOneshot     = 0,
        ePeriodic    = 1,
        eTscDeadline = 2,
    };

    Lapic();
    void          Initialize();

    static bool   IsInitialized() { return s_Initialized; }

    StringView    Name() const PM_NOEXCEPT override { return "lapic"_sv; }
    StringView    ModelString() const override { return "Local APIC"_sv; }

    virtual usize InterruptVector() const override;
    virtual bool  IsCPULocal() const override { return true; }

    void          SendIpi(u32 flags, u32 id);
    void          SendEOI();

    static void   PanicIpi();

    ErrorOr<void> Start(TimerMode mode, Timestep interval) override;
    void          Stop() override;

    ErrorOr<void> SetFrequency(usize frequency) override
    {
        (void)frequency;
        return {};
    }

  private:
    static AtomicBool                s_Initialized;

    u32                              m_ID          = 0;
    upointer                         m_BaseAddress = 0;
    bool                             m_X2Apic      = false;
    u64                              m_TicksPerMs  = 0;
    ::Ref<class InterruptDispatcher> m_Dispatcher  = nullptr;

    virtual ErrorOr<isize>           Read(const UserBuffer& out, usize count,
                                          isize offset = -1) override
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        return Error(ENOSYS);
    };
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        return Error(ENOSYS);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return Error(ENOSYS);
    }

    u32  Read(u32 reg);
    void Write(u32 reg, u64 value);

    void CalibrateTimer();
    void SetNmi(u8 vector, u8 currentCPUID, u8 cpuID, u16 flags, u8 lint);

    static IrqResult Tick(Device* device, ExecutionContext* context);
};

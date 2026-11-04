/*
 * Created by v1tr10l7 on 24.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

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
#include <API/UnixTypes.hpp>
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
#include <Prism/Memory/Scope.hpp>

module Cryptix:Impl.Drivers.Storage.NVMe.NVMeQueue;

import :Debug.Assertions;
import :Library.Logger;
import :Library.Stacktrace;
import :Drivers.Storage.NVMe.NVMeNameSpace;
import :Drivers.Storage.NVMe.NVMeQueue;
import :Memory.PMM;

namespace NVMe
{
    using Prism::Pointer;

    Queue::Queue(Pointer crAddress, u16 qid, u32 doorbellShift, u64 depth)
        : m_ID(qid)
        , m_Depth(depth)
    {
        auto submitDbOffset
            = PMM::PAGE_SIZE + (m_ID * 2) * (4 << doorbellShift);
        auto completeDbOffset
            = PMM::PAGE_SIZE + (m_ID * 2 + 1) * (4 << doorbellShift);

        m_Submit = new volatile Submission[depth];
        m_SubmitDoorbell
            = crAddress.Offset<Pointer>(submitDbOffset).As<volatile u32>();

        m_SubmitHead = 0;
        m_SubmitTail = 0;

        m_Complete   = new volatile Completion[depth];

        m_CompleteDoorbell
            = crAddress.Offset<Pointer>(completeDbOffset).As<volatile u32>();
        m_CompleteVec   = 0;
        m_CompleteHead  = 0;
        m_CompletePhase = 1;

        m_CmdId         = 0;
        m_PhysRegPgs    = nullptr;
    }
    Queue::Queue(Pointer crAddress, NameSpace& ns, u16 qid, u32 doorbellShift,
                 u64 depth)
        : Queue(crAddress, qid, doorbellShift, depth)
    {
        m_PhysRegPgs = new u64[ns.GetMaxPhysRPgs() * depth];
    }

    u16 Queue::AwaitSubmit(Submission* cmd)
    {
        u16 currentHead  = m_CompleteHead;
        u16 currentPhase = m_CompletePhase;
        cmd->CompleteID  = m_CmdId++;
        Submit(cmd);
        u16 status = 0;

        while (true)
        {
            status = m_Complete[m_CompleteHead].Status;
            if ((status & 0x01) == currentPhase) break;
        }

        status >>= 1;
        AssertFmt(!status, "NVMe: Command error: {:#x}", status);

        currentHead = (currentHead + 1) % m_Depth;
        if (currentHead == 0) currentPhase = !currentPhase;

        *(m_CompleteDoorbell) = currentHead;
        m_CompleteHead        = currentHead;
        m_CompletePhase       = currentPhase;
        return status;
    }

    void Queue::Submit(Submission* cmd)
    {
        u16       currentTail = m_SubmitTail;

        uintptr_t dest = reinterpret_cast<uintptr_t>(&m_Submit[currentTail]);

        Memory::Copy(dest, cmd, sizeof(Submission));
        currentTail         = (currentTail + 1) % m_Depth;

        *(m_SubmitDoorbell) = currentTail;
        m_SubmitTail        = currentTail;
    }
}; // namespace NVMe

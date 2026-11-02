/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Core/UUID.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Network/IPv4Address.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/UnixTypes.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
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
#include <Prism/Core/Iterator.hpp>

module Cryptix:Impl.VFS.Initrd.Initrd;

import :Boot.BootModuleInfo;
import :Library.Logger;
import :Memory.PMM;
import :Memory.VMM;
import :System.System;
import :VFS.Initrd.Initrd;
import :VFS.Initrd.Ustar;

namespace Initrd
{
    bool Initialize()
    {
        LogTrace("Initrd: Loading initial ramdisk...");
        auto initrd = System::FindBootModule("initrd");
        if (!initrd || !initrd->LoadAddress || initrd->Size == 0)
        {
            LogError("Initrd: Could not find initrd module!");
            return false;
        }

        auto address = Pointer(initrd->LoadAddress).ToHigherHalf();
        if (!Ustar::Validate(address))
        {
            LogError("Initrd: Unknown archive format!");
            return false;
        }

        Assert(Ustar::Load(address, initrd->Size));
        // usize pageCount
        //     = (Math::AlignUp(initrd->Size, 512) + 512) / PMM::PAGE_SIZE;
        // PMM::FreePages(address.FromHigherHalf<void*>(), pageCount);

        LogInfo("Initrd: Loaded successfully");
        return true;
    }
} // namespace Initrd

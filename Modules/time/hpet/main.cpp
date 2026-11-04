/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/String/String.hpp>
#include <Library/Module.hpp>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <compare>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Memory.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/UnixTypes.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
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

import Cryptix;

CTOS_MODULE_AUTHOR("v1tr10l7");
CTOS_MODULE_DESCRIPTION("high performance event timer driver");
CTOS_MODULE_LICENSE("GPL-3");
CTOS_MODULE_VERSION("0.2");

class HPET
{
  public:
    static ErrorOr<void> Probe(ACPI::DeviceHandle* handle, StringView)
    {
        return {};
    }
    static void Remove(ACPI::DeviceHandle* handle) {}
};

static auto         s_MatchIDs = ToArray({"PNP0103"_sv});
static ACPI::Driver s_Driver   = {
      .Name     = "hpet"_s,
      .MatchIDs = Span(s_MatchIDs.begin(), s_MatchIDs.Size()),
      .Hook     = {},
      .Probe    = HPET::Probe,
      .Remove   = HPET::Remove,
};
extern "C" CTOS_EXPORT bool ModuleInit() { return true; }
MODULE_INIT(hpet, ModuleInit);

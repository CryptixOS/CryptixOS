/*
 * Created by v1tr10l7 on 17.04.2025.
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
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>

module Cryptix:Impl.Memory.Region;

import :Memory.Region;
import :Memory.VMM;

namespace VMM
{
    enum PageAttributes Region::PageAttributes() const
    {
        enum PageAttributes flags = PageAttributes::eWriteBack;

        if (m_Access & Access::eRead) flags |= PageAttributes::eRead;
        if (m_Access & Access::eWrite) flags |= PageAttributes::eWrite;
        if (m_Access & Access::eExecute) flags |= PageAttributes::eExecutable;

        if (m_Access & Access::eUser) flags |= PageAttributes::eUser;
        return flags | PageAttributes::eRead | PageAttributes::eWrite
             | PageAttributes::eExecutable;
    }

    void Region::SetAttributes(enum PageAttributes flags)
    {
        enum Access access = Access::eNone;
        if (flags & PageAttributes::eRead) access |= Access::eRead;
        if (flags & PageAttributes::eWrite) access |= Access::eWrite;
        if (flags & PageAttributes::eExecutable) access |= Access::eExecute;
        if (flags & PageAttributes::eUser) access |= Access::eUser;

        m_Access = access;
    }
}; // namespace VMM

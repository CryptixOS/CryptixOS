/*
 * Created by v1tr10l7 on 30.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Prism/String/StringView.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/String/String.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>

module Cryptix:Impl.Library.KernelObject;

import :Library.KernelSet;
import :Library.Logger;

void KernelSet::SendEvent(KernelObjectEnvironment* event) const
{
    LogDebug("KernelObject-UEvent: {}", Name());
    for (const auto& [_, child] : m_Children)
    {
        LogDebug("KernelObject-UEvent: Sending event to child object: {}",
                 child->Name());
    }
}

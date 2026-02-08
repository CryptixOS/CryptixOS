/*
 * Created by v1tr10l7 on 30.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/KernelSet.hpp>
#include <Library/Logger.hpp>

void KernelSet::SendEvent(KernelObjectEnvironment* event) const
{
    LogDebug("KernelObject-UEvent: {}", Name());
    for (const auto& [_, child] : m_Children)
    {
        LogDebug("KernelObject-UEvent: Sending event to child object: {}",
                 child->Name());
    }
}

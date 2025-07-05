/*
 * Created by v1tr10l7 on 17.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

enum PageAttributes Region::PageAttributes() const
{
    return PageAttributes::eRWXU | PageAttributes::eWriteBack;
    enum PageAttributes flags = PageAttributes::eWriteBack;

    if (m_Access & Access::eRead) flags |= PageAttributes::eRead;
    if (m_Access & Access::eWrite) flags |= PageAttributes::eWrite;
    if (m_Access & Access::eExecute) flags |= PageAttributes::eExecutable;
    if (m_Access & Access::eUser) flags |= PageAttributes::eUser;

    return flags;
}

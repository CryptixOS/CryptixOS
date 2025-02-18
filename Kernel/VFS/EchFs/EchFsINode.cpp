/*
 * Created by v1tr10l7 on 01.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/EchFs/EchFs.hpp>
#include <VFS/EchFs/EchFsINode.hpp>

std::unordered_map<std::string_view, INode*>& EchFsINode::GetChildren()
{
    reinterpret_cast<EchFs*>(m_Filesystem)->Populate(this);
    return m_Children;
}

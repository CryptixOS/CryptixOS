/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Utility/PathView.hpp>

bool PathView::ValidateLength()
{

    usize pathLen = 0;
    while (m_Path[pathLen])
    {
        if (pathLen >= Limits::MAX_PATH_LENGTH) return false;
        ++pathLen;
    }

    return true;
}

std::string_view PathView::GetLastComponent() const
{
    auto forthSlash = m_Path.find_last_of('/');
    return m_Path.substr(forthSlash, m_Path.size() - forthSlash);
}

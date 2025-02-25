/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <Prism/PathView.hpp>

namespace Prism
{
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

    std::vector<std::string> PathView::SplitPath()
    {
        std::vector<std::string> segments;
        usize                    start     = m_Path[0] == '/' ? 1 : 0;
        usize                    end       = start;

        auto                     findSlash = [this](usize pos) -> usize
        {
            usize current = pos;
            while (m_Path[current] != '/' && current < m_Path.size()) current++;

            return current == m_Path.size() ? std::string::npos : current;
        };

        std::string path = m_Path.data();
        while ((end = findSlash(start)) != std::string::npos)
        {
            std::string segment = path.substr(start, end - start);
            if (start != end) segments.push_back(segment);

            start = end + 1;
        }

        // handle last segment
        if (start < path.length()) segments.push_back(path.substr(start));
        return segments;
    }

    std::string_view PathView::GetLastComponent() const
    {
        auto forthSlash = m_Path.find_last_of('/');
        return m_Path.substr(forthSlash, m_Path.size() - forthSlash);
    }
}; // namespace Prism

/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Prism/Path.hpp>

namespace Prism
{
    std::vector<std::string> Path::SplitPath()
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

        while ((end = findSlash(start)) != std::string::npos)
        {
            std::string segment = m_Path.substr(start, end - start);
            if (start != end) segments.push_back(segment);

            start = end + 1;
        }

        // handle last segment
        if (start < m_Path.length()) segments.push_back(m_Path.substr(start));
        return segments;
    }
}; // namespace Prism

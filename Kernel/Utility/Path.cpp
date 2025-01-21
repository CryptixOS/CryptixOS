/*
 * Created by v1tr10l7 on 21.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Utility/Path.hpp>

namespace Path
{
    bool IsAbsolute(std::string_view path)
    {
        return !path.empty() && path[0] == '/';
    }

    std::vector<std::string> SplitPath(std::string path)
    {
        std::vector<std::string> segments;
        usize                    start     = path[0] == '/' ? 1 : 0;
        usize                    end       = start;

        auto                     findSlash = [path](usize pos) -> usize
        {
            usize current = pos;
            while (path[current] != '/' && current < path.size()) current++;

            return current == path.size() ? std::string::npos : current;
        };

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

    bool ValidateLength(PathView path)
    {
        usize pathLen = 0;
        while (path[pathLen])
        {
            if (pathLen >= Limits::MAX_PATH_LENGTH) return false;
            ++pathLen;
        }

        return true;
    }
} // namespace Path

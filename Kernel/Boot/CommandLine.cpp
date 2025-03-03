/*
 * Created by v1tr10l7 on 01.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/CommandLine.hpp>

#include <unordered_map>

namespace CommandLine
{
    static std::string_view s_KernelCommandLine = "";
    static std::unordered_map<std::string, std::string> s_OptionMap;

    static void                                         ParseCommandLine()
    {
        usize keyStart     = 0;
        usize keyEnd       = 0;
        usize valueStart   = 0;
        usize valueEnd     = 0;

        bool  parsingValue = false;
        for (usize pos = 0; char c : s_KernelCommandLine)
        {
            if (c == '=')
            {
                parsingValue = true;
                keyEnd       = pos - 1;
                valueStart   = pos + 1;
            }
            if (c == '-' && keyEnd != 0)
            {

                if (parsingValue)
                {
                    valueEnd = pos - 1;
                    s_OptionMap[s_KernelCommandLine.substr(keyStart,
                                                           keyEnd - keyStart)]
                        = s_KernelCommandLine.substr(valueStart,
                                                     valueEnd - valueStart);
                }
                else
                    s_OptionMap[s_KernelCommandLine.substr(keyStart,
                                                           keyEnd - keyStart)]
                        = "";
                keyStart = pos;
            }

            ++pos;
        }

        for (auto option : s_OptionMap)
            LogInfo("CMD: {} = {}", option.first, option.second);
    }

    void Initialize()
    {
        s_KernelCommandLine = BootInfo::GetKernelCommandLine();

        ParseCommandLine();
        // TODO(v1tr10l7): Parse the options
    }
}; // namespace CommandLine

/*
 * Created by v1tr10l7 on 01.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/CommandLine.hpp>
#include <Library/Logger.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/String/String.hpp>

#include <cctype>

namespace CommandLine
{
    static StringView                         s_KernelCommandLine = "";
    static UnorderedMap<String, String> s_OptionMap;

    void                                      ParseArguments(StringView args)
    {
        UnorderedMap<String, String>& result = s_OptionMap;
        usize                               pos    = 0;

        while (pos < args.Size())
        {
            // Skip leading whitespace
            while (pos < args.Size()
                   && std::isspace(static_cast<unsigned char>(args[pos])))
                ++pos;

            // Ensure there's an argument to process
            if (pos >= args.Size()) break;

            // Check if the argument starts with '-'
            if (args[pos] == '-')
            {
                ++pos;
                usize start = pos;

                // Find the end of the argument
                while (pos < args.Size()
                       && !std::isspace(static_cast<unsigned char>(args[pos])))
                    ++pos;

                // Extract the argument substring
                auto  arg          = args.Substr(start, pos - start);

                // Find the '=' delimiter
                usize delimiterPos = arg.Find('=');
                if (delimiterPos != StringView::NPos)
                {
                    // Argument has a key=value format
                    String key(arg.Substr(0, delimiterPos));
                    auto   value = arg.Substr(delimiterPos + 1);
                    result[key]  = String(value);

                    continue;
                }

                // Argument is a flag without a value
                result[String(arg)] = String("");
                continue;
            }

            // Handle cases where arguments do not start with '-'
            ++pos;
        }

        LogTrace("CommandLine: Finished parsing the arguments");
    }

    void Initialize()
    {
        s_KernelCommandLine = BootInfo::KernelCommandLine();

        LogInfo("CommandLine: Parsing '{}'...", s_KernelCommandLine);
        ParseArguments(s_KernelCommandLine);

        LogInfo("CommandLine: Kernel Arguments:");
        for (auto& [arg, value] : s_OptionMap)
            LogInfo("CommandLine: {} -> {}", arg, value);
    }

    using namespace Prism::StringViewLiterals;
    Optional<bool> GetBoolean(StringView key)
    {
        auto it = s_OptionMap.Find(String(key));
        if (it != s_OptionMap.end())
            return it->Value == "true"_sv || it->Value == "1"_sv;

        return NullOpt;
    }
    StringView GetString(StringView key)
    {
        auto it = s_OptionMap.Find(String(key));
        if (it != s_OptionMap.end()) return it->Value;

        return "";
    }
}; // namespace CommandLine

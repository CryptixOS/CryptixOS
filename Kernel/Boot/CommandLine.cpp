/*
 * Created by v1tr10l7 on 01.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/CommandLine.hpp>

#include <cctype>
#include <unordered_map>

namespace CommandLine
{
    static std::string_view s_KernelCommandLine = "";
    static std::unordered_map<std::string, std::string> s_OptionMap;

    // Function to parse command-line arguments from a string_view
    void ParseArguments(std::string_view args)
    {
        std::unordered_map<std::string, std::string>& result = s_OptionMap;
        usize                                         pos    = 0;

        while (pos < args.size())
        {
            // Skip leading whitespace
            while (pos < args.size()
                   && std::isspace(static_cast<unsigned char>(args[pos])))
                ++pos;

            // Ensure there's an argument to process
            if (pos >= args.size()) break;

            // Check if the argument starts with '-'
            if (args[pos] == '-')
            {
                ++pos;
                usize start = pos;

                // Find the end of the argument
                while (pos < args.size()
                       && !std::isspace(static_cast<unsigned char>(args[pos])))
                    ++pos;

                // Extract the argument substring
                std::string_view arg          = args.substr(start, pos - start);

                // Find the '=' delimiter
                usize            delimiterPos = arg.find('=');
                if (delimiterPos != std::string_view::npos)
                {
                    // Argument has a key=value format
                    std::string key = std::string(arg.substr(0, delimiterPos));
                    std::string value
                        = std::string(arg.substr(delimiterPos + 1));
                    result[key] = value;
                }
                else
                    // Argument is a flag without a value
                    result[std::string(arg)] = "";
            }
            else
                // Handle cases where arguments do not start with '-'
                ++pos;
        }

        LogTrace("CommandLine: Finished parsing the arguments");
    }

    void Initialize()
    {
        s_KernelCommandLine = BootInfo::GetKernelCommandLine();

        LogInfo("CommandLine: Parsing '{}'...", s_KernelCommandLine);
        ParseArguments(s_KernelCommandLine);

        LogInfo("CommandLine: Kernel Arguments:");
        for (auto& [arg, value] : s_OptionMap)
            LogInfo("CommandLine: {} -> {}", arg, value);
    }

    std::optional<bool> GetBoolean(std::string_view key)
    {
        auto it = s_OptionMap.find(key);
        if (it != s_OptionMap.end())
            return it->second == "true" || it->second == "1";

        return std::nullopt;
    }
    std::string_view GetString(std::string_view key)
    {
        auto it = s_OptionMap.find(key);
        if (it != s_OptionMap.end()) return it->second;

        return "";
    }
}; // namespace CommandLine

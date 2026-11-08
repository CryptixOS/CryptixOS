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
#include <Prism/String/StringUtils.hpp>

namespace CommandLine
{
    using OptionMap = UnorderedMap<Prism::String, Prism::String>;

    static StringView s_KernelCommandLine = ""_sv;
    // Constructed by Initialize(), not as a global: the command line is
    // parsed before the global constructors run (MM::Initialize()), and
    // they would reset the map, losing every option.
    alignas(OptionMap) static u8 s_OptionMapStorage[sizeof(OptionMap)];
    static OptionMap* s_OptionMap = nullptr;

    void ParseArguments(StringView args)
    {
        OptionMap& result = *s_OptionMap;
        usize      pos    = 0;
        if (args.StartsWith("\"")) args.RemovePrefix(1);
        if (args.EndsWith("\"")) args.RemoveSuffix(1);

        while (pos < args.Size())
        {
            // Skip leading whitespace
            while (
                pos < args.Size()
                && StringUtils::IsSpace(static_cast<unsigned char>(args[pos])))
                ++pos;

            // Ensure there's an argument to process
            if (pos >= args.Size()) break;

            // Every whitespace separated word is an option, `key=value` or a
            // bare `flag`, with or without a leading '-'
            usize start = pos;
            while (pos < args.Size()
                   && !StringUtils::IsSpace(
                       static_cast<unsigned char>(args[pos])))
                ++pos;

            auto arg = args.Substr(start, pos - start);
            if (arg.StartsWith("-")) arg.RemovePrefix(1);
            if (arg.Empty()) continue;

            usize delimiterPos = arg.Find('=');
            if (delimiterPos != StringView::NPos)
            {
                Prism::String key(arg.Substr(0, delimiterPos));
                result[key] = Prism::String(arg.Substr(delimiterPos + 1));
                continue;
            }

            // A flag without a value
            result[Prism::String(arg)] = Prism::String();
        }

        LogTrace("CommandLine: Finished parsing the arguments");
    }

    void Initialize(StringView commandLine)
    {
        s_KernelCommandLine = commandLine;
        s_OptionMap         = new (s_OptionMapStorage) OptionMap();

        LogInfo("CommandLine: Parsing '{}'...", s_KernelCommandLine);
        ParseArguments(s_KernelCommandLine);

        LogInfo("CommandLine: Kernel Arguments:");
        for (auto& [arg, value] : *s_OptionMap)
            LogInfo("CommandLine: {} -> {}", arg, value);
    }

    StringView KernelCommandLine() { return s_KernelCommandLine; }

    bool       Contains(StringView key)
    {
        return s_OptionMap && s_OptionMap->Contains(Prism::String(key));
    }

    Optional<bool> Boolean(StringView key)
    {
        if (!s_OptionMap) return NullOpt;
        auto it = s_OptionMap->Find(Prism::String(key));
        if (it != s_OptionMap->end())
            return it->Value == "true"_sv || it->Value == "1"_sv;

        return NullOpt;
    }
    StringView String(StringView key)
    {
        if (!s_OptionMap) return ""_sv;
        auto it = s_OptionMap->Find(Prism::String(key));
        if (it != s_OptionMap->end()) return it->Value;

        return ""_sv;
    }

    Optional<bool> GetBoolean(StringView key) { return Boolean(key); }
    StringView     GetString(StringView key)
    {
        auto value = String(key);
        return value;
    }
}; // namespace CommandLine

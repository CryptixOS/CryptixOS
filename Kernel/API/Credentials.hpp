/*
 * Created by v1tr10l7 on 15.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Capability.hpp>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>

struct Credentials
{
    UserID     UserID;
    GroupID    GroupID;
    ::UserID   EffectiveUserID;
    ::GroupID  EffectiveGroupID;
    ::UserID   FilesystemUserID;
    ::GroupID  FilesystemGroupID;

    ::UserID   SetUserID;
    ::GroupID  SetGroupID;
    ProcessID  SessionID;
    ProcessID  ProcessGroupID;

    Capability EffectiveCapabilities{};

    CTOS_ALWAYS_INLINE constexpr Credentials()
        : UserID(0)
        , GroupID(0)
        , EffectiveUserID(0)
        , EffectiveGroupID(0)
        , FilesystemUserID(0)
        , FilesystemGroupID(0)
        , SetUserID(0)
        , SetGroupID(0)
        , SessionID(0)
        , ProcessGroupID(0)
        , EffectiveCapabilities(Capability::eNone)
    {
    }

    inline constexpr bool Capable(Capability capability) const
    {
        return EffectiveCapabilities & capability;
    }
};

inline constexpr Credentials s_RootCredentials{};

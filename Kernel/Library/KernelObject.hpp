/*
 * Created by v1tr10l7 on 30.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Compiler.hpp>
#include <Prism/String/StringView.hpp>

enum class ObjectType
{
    eAdd,
    eRemove,
    eChange,
    eMove,
    eOffline,
    eOnline,
    eCount,
};

class KernelObject
{
  public:
    inline constexpr StringView Name() const PM_NOEXCEPT { return m_Name; }

  private:
    StringView                m_Name   = ""_sv;
    CTOS_UNUSED KernelObject* m_Parent = nullptr;
};

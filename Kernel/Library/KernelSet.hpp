/*
 * Created by v1tr10l7 on 30.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Library/KernelObject.hpp>
#include <Library/Locking/Spinlock.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/String/String.hpp>

struct Attribute
{
    StringView Name;
    INodeMode  Mode;
};
struct KernelObjectEnvironment
{
    UnorderedMap<StringView, StringView> m_Environment;
    isize                                Index;
    String                               Value;
};
struct KernelAttribute
{
    struct Attribute Attribute;
};

class KernelSet : public KernelObject
{
  public:
    inline constexpr bool       Filter(KernelObject* object) const;
    void SendEvent(KernelObjectEnvironment* event) const;

  private:
    UnorderedMap<StringView, KernelObject*> m_Children;
    Spinlock                                m_Lock;
};

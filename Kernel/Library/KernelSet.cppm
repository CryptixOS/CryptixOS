/*
 * Created by v1tr10l7 on 30.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <Prism/String/StringView.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/String/String.hpp>

export module Cryptix:Library.KernelSet;

export import :Library.KernelObject;
export import :Library.Locking.Spinlock;

export struct Attribute
{
    StringView Name;
    INodeMode  Mode;
};
export struct KernelObjectEnvironment
{
    UnorderedMap<StringView, StringView> m_Environment;
    isize                                Index;
    String                               Value;
};
export struct KernelAttribute
{
    struct Attribute Attribute;
};

export class KernelSet : public KernelObject
{
  public:
    bool                        Filter(KernelObject* object) const;
    void SendEvent(KernelObjectEnvironment* event) const;

  private:
    UnorderedMap<StringView, KernelObject*> m_Children;
    Spinlock                                m_Lock;
};

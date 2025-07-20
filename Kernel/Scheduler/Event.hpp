/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Locking/Spinlock.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/Span.hpp>

#include <Prism/Core/Types.hpp>
#include <Prism/Utility/Optional.hpp>

constexpr usize MAX_LISTENERS = 32;
struct EventListener
{
    struct Thread* Thread;
    usize          Which;
};
struct Event
{
  public:
    bool Await(bool block = true)
    {
        Array evs = {this};
        return Await(Span(evs.Raw(), evs.Size()), block) != NullOpt;
    }
    void                   Trigger(bool drop = false) { Trigger(this, drop); }

    static Optional<usize> Await(Span<Event*> events, bool block = true);
    static void            Trigger(Event* event, bool drop = false);

    Spinlock               Lock;
    usize                  Pending;
    Deque<EventListener>   Listeners;
};

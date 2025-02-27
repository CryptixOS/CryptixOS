/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Spinlock.hpp>
#include <Prism/Types.hpp>

#include <deque>
#include <span>

constexpr usize MAX_LISTENERS = 32;
struct EventListener
{
    struct Thread* Thread;
    usize          Which;
};
struct Event
{
  public:
    static std::optional<usize> Await(std::span<Event*> events,
                                      bool              block = true);
    static void                 Trigger(Event* event, bool drop = false);

    Spinlock                    Lock;
    usize                       Pending;
    std::deque<EventListener>   Listeners;
};

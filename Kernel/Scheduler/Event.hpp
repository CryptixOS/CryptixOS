/*
 * Created by v1tr10l7 on 03.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Spinlock.hpp>
#include <Prism/Types.hpp>

constexpr usize MAX_LISTENERS = 32;
struct EventListener
{
    class Thread* Thread;
    usize         Which;
};
struct Event
{
  public:
    static isize  Await(Event** events, usize eventCount, bool block);
    static usize  Trigger(Event* event, bool drop);

    Spinlock      Lock;
    usize         Pending;
    usize         ListenersI;
    EventListener Listeners[MAX_LISTENERS];
};

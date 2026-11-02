/*
 * Created by v1tr10l7 on 14.09.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Time.hpp>
#include <Compiler.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Utility/Optional.hpp>

export module Cryptix:Time.Timer;

export import :Scheduler.Event;

export namespace Time
{
    struct Timer : public RefCounted
    {
        explicit Timer()       = default;

        Optional<usize>  Index = NullOpt;
        bool             Fired = false;
        Timestep         When{0};
        Timestep         ReloadValue{0};
        Event            Event;
        Delegate<void()> OnFired;

        void Arm(usize id, Timestep expiration, Timestep reloadValue = 0);
        template <typename F>
        inline void Arm(usize id, Timestep expiration, F fn,
                        Timestep reloadValue = 0)
        {
            OnFired.BindLambda(fn);
            Arm(id, expiration, reloadValue);
        }
        void Disarm();
    };
}; // namespace Time
export using Time::Timer;

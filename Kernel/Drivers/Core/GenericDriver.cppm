/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/String/String.hpp>
#include <Prism/Core/Error.hpp>

export module Cryptix:Drivers.Core.GenericDriver;

export struct GenericDriver
{
    String Name;

    using ProbeFn  = ErrorOr<void> (*)();
    using RemoveFn = void (*)();

    ProbeFn  Probe;
    RemoveFn Remove;
};

export extern bool RegisterGenericDriver(GenericDriver& driver);

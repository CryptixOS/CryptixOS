/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Ref.hpp>

export module Cryptix:Drivers.USB.USB;

export import :Drivers.USB.HostController;

export namespace USB 
{
    ErrorOr<void> Initialize();
    ErrorOr<void> RegisterController(HostController* controller);
};

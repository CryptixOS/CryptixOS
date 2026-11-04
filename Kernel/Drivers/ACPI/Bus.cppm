/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

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
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/String/String.hpp>

export module Cryptix:Drivers.ACPI.Bus;

export import :Drivers.ACPI.Driver;

namespace ACPI
{
    export class Device;
    export namespace Bus
    {
        KERNEL_INIT_SECTION void Initialize();

        ErrorOr<void>            RegisterDriver(Driver* driver);
        void                     UnregisterDriver(Driver* driver);

        ErrorOr<void>            DispatchDriver(Driver* driver);

        ErrorOr<void>            RegisterDevice(ACPI::Device* device);
        void                     UnregisterDevice(ACPI::Device* device);
    }; // namespace Bus
}; // namespace ACPI

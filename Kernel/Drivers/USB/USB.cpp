/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <API/Posix/termios.h>
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <uacpi/resources.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#if defined(CTOS_TARGET_X86_64)
#include <Prism/Core/Concepts.hpp>
#endif
#include <Prism/Core/TypeTraits.hpp>

module Cryptix:Impl.Drivers.USB.USB;

import :Drivers.PCI.PCI;
import :Drivers.PCI.HostController;
import :Drivers.PCI.Device;
import :Drivers.USB.USB;
import :Drivers.USB.UHCI.UHCiController;
import :Library.Logger;

namespace USB
{
    static HostController::List s_HostControllers{};

    ErrorOr<void>               Initialize()
    {
        LogTrace("USB: Initializing...");
        auto            pciController = PCI::GetHostController(0);

        PCI::Enumerator enumerator;
        enumerator.BindLambda(
            [pciController](const PCI::DeviceAddress& addr) -> bool
            {
                u8 classID = pciController->Read<u8>(
                    addr, ToUnderlying(PCI::RegisterOffset::eClassID));
                u8 subClassID = pciController->Read<u8>(
                    addr, ToUnderlying(PCI::RegisterOffset::eSubClassID));
                u8 progIf = pciController->Read<u8>(
                    addr, ToUnderlying(PCI::RegisterOffset::eProgIf));

                if (classID == 0x0c && subClassID == 0x03 && progIf == 0x00) 
                {
                    LogInfo("USB: Discovered UHCI controller on the PCI bus => {:#04x}:{:#04x}:{:#04x}:{:#04x}", addr.Domain, addr.Bus, addr.Slot, addr.Function);
                    if (!USB::RegisterController(new USB::UHCI::Controller(addr)))
                        LogError("USB: Failed to register USB UHCI controller");
                    else LogInfo("USB: Successfully registered new USB UHCI controller");

                    return true;
                }
                
                return false;
            });

        if (!pciController->EnumerateDevices(enumerator))
        {
            LogWarn("USB: Failed to detect any host controllers");
            return Error(ENODEV);
        }
        return {};
    }
    ErrorOr<void> RegisterController(HostController* controller)
    {
        LogTrace("USB: Registering the host controller...");
        auto status = controller->Initialize();

        if (!status)
        {
            LogError(
                "USB: Failed to initialize the host controller, aborting...");
            return Error(status.error());
        }

        s_HostControllers.PushBack(controller);
        LogInfo("USB: Initialized controller count => {}",
                s_HostControllers.Size());

        return {};
    }
}; // namespace USB

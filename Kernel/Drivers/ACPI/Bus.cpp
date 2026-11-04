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
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <compare>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/PathView.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Utility/Optional.hpp>
#include <Prism/String/StringUtils.hpp>
#include <uacpi/context.h>
#include <uacpi/event.h>
#include <uacpi/notify.h>
#include <uacpi/resources.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

module Cryptix:Impl.Drivers.ACPI.Bus;

import :Drivers.ACPI.Bus;
import :Drivers.ACPI.Resources;
import :Library.Locking.SpinlockProtected;

namespace ACPI
{
    struct DeviceHandle : public RefCounted
    {
        uacpi_namespace_node* Node      = nullptr;
        uacpi_resources*      Resources = nullptr;

        DeviceHandle(uacpi_namespace_node* node)
            : Node(node)
        {
        }

        inline bool MatchID(Span<StringView> ids, StringView id) const
        {
            for (const auto pnp : ids)
                if (pnp == id) return true;
            return false;
        }
        inline bool MatchID(Span<StringView> ids) const
        {
            bool                       result = false;

            uacpi_namespace_node_info* info;
            uacpi_pnp_id_list*         idList = UACPI_NULL;
            uacpi_status ret = uacpi_get_namespace_node_info(Node, &info);
            if (uacpi_unlikely_error(ret)) return false;

            if (info->flags & UACPI_NS_NODE_INFO_HAS_CID
                && !uacpi_unlikely_error(uacpi_eval_cid(Node, &idList)))
            {
                for (usize i = 0; i < idList->num_ids; ++i)
                {
                    if (MatchID(ids, idList->ids[i].value))
                    {
                        result = true;
                        break;
                    }
                }
                uacpi_free_pnp_id_list(idList);
            }

            uacpi_id_string* id = UACPI_NULL;
            if (!result && info->flags & UACPI_NS_NODE_INFO_HAS_HID
                && !uacpi_unlikely_error(uacpi_eval_hid(Node, &id)))
            {
                if (MatchID(ids, id->value)) result = true;
                uacpi_free_id_string(id);
            }

            uacpi_free_namespace_node_info(info);
            return result;
        }

        using HookType
            = IntrusiveRefListHook<DeviceHandle, ::Ref<DeviceHandle>>;
        friend class IntrusiveRefList<DeviceHandle, HookType>;
        friend struct IntrusiveRefListHook<DeviceHandle, ::Ref<DeviceHandle>>;

        using List = IntrusiveRefList<DeviceHandle, HookType>;
        HookType Hook;
    };

    namespace Bus
    {
        static Spinlock                        s_Lock;
        static DeviceHandle::List              s_DeviceHandles{};

        static SpinlockProtected<Driver::List> s_Drivers{};

        static uacpi_iteration_decision
        EnumerateDevice(void* ctx, uacpi_namespace_node* node, uacpi_u32)
        {
            uacpi_object_type nodeType;
            uacpi_namespace_node_type(node, &nodeType);

            const char* path
                = uacpi_namespace_node_generate_absolute_path(node);
            const char* nodeTypeString = uacpi_object_type_to_string(nodeType);

            LogDebug("ACPI: Discovered node `{}` with type: {}", path,
                     nodeTypeString);

            uacpi_namespace_node_info* info;
            uacpi_status ret = uacpi_get_namespace_node_info(node, &info);

            if (uacpi_unlikely_error(ret))
            {
                LogError("ACPI: Unable to retrieve node %s information: {}",
                         path, uacpi_status_to_string(ret));
                uacpi_free_absolute_path(path);
                return UACPI_ITERATION_DECISION_CONTINUE;
            }

            if (info->flags & UACPI_NS_NODE_INFO_HAS_ADR)
                ;
            else if (info->flags & UACPI_NS_NODE_INFO_HAS_HID)
            {
                uacpi_id_string* id = UACPI_NULL;
                auto error = uacpi_unlikely_error(uacpi_eval_hid(node, &id));
                if (error)
                    LogError(
                        "ACPI: Failed to acquire _HID object from device => {}",
                        path);
                else
                {
                    LogInfo("ACPI: _HID => {}", id->value);
                    uacpi_free_id_string(id);
                }
            }
            else if (info->flags & UACPI_NS_NODE_INFO_HAS_UID)
                ;
            else if (info->flags & UACPI_NS_NODE_INFO_HAS_CID)
            {
                uacpi_pnp_id_list* idList = UACPI_NULL;

                auto               error
                    = uacpi_unlikely_error(uacpi_eval_cid(node, &idList));
                if (error)
                    LogError(
                        "ACPI: Failed to acquire _CID object from device => {}",
                        path);
                for (usize i = 0; !error && i < idList->num_ids; ++i)
                    LogInfo("ACPI: _CID[{}] => {}", i, idList->ids[i].value);

                if (!error) uacpi_free_pnp_id_list(idList);
            }
            else if (info->flags & UACPI_NS_NODE_INFO_HAS_CLS)
                ;
            else if (info->flags & UACPI_NS_NODE_INFO_HAS_SXD)
                ;
            else if (info->flags & UACPI_NS_NODE_INFO_HAS_SXW)
                ;

            uacpi_free_absolute_path(path);

            auto handle = CreateRef<DeviceHandle>(node);
            uacpi_get_device_resources(node, "_CRS", &handle->Resources);

            ScopedLock guard(s_Lock);
            s_DeviceHandles.PushBack(handle);

            LogTrace("ACPI: Added the device node to the bus");

            uacpi_free_namespace_node_info(info);
            return UACPI_ITERATION_DECISION_CONTINUE;
        }
        void Initialize()
        {
            uacpi_namespace_for_each_child(
                uacpi_namespace_root(), EnumerateDevice, UACPI_NULL,
                UACPI_OBJECT_DEVICE_BIT, UACPI_MAX_DEPTH_ANY, UACPI_NULL);
        }

        ErrorOr<void> RegisterDriver(Driver* driver)
        {
            Driver* found = nullptr;
            s_Drivers.ForEachRead(
                [&found, driver](Driver* current)
                {
                    if (!found && current->Name == driver->Name)
                        found = current;
                });

            if (found) return Error(EEXIST);
            s_Drivers.With([driver](auto& list) { list.PushBack(driver); });

            DispatchDriver(driver);
            return {};
        }
        void UnregisterDriver(Driver* driver)
        {
            s_Drivers.With([driver](auto& list) { list.Erase(driver); });
        }

        ErrorOr<void> DispatchDriver(Driver* driver)
        {
            ScopedLock guard(s_Lock);

            usize      probedDevices = 0;
            for (auto device : s_DeviceHandles)
            {
                if (device->MatchID(driver->MatchIDs) && driver->Probe
                    && driver->Probe(device.Raw(), ""))
                    ++probedDevices;
            }

            if (!probedDevices) return Error(ENODEV);

            LogTrace("ACPI: Probed #{} devices for driver => {}", probedDevices,
                     driver->Name);
            return {};
        }

        ErrorOr<void> RegisterDevice(Device* device)
        {
            // TODO(v1tr10l7): Register the device
            return {};
        }
        void UnregisterDevice(Device* device)
        {
            // TODO(v1tr10l7): Unregister the device
            return;
        }

        ErrorOr<IrqResource> IrqResourceForHandle(DeviceHandle* handle)
        {
            auto resources = handle->Resources;

            if (!resources) return Error(ENOENT);
            for (usize i = 0; i < resources->length; i++)
            {
                auto& entry = resources->entries[i];
                if (entry.type == UACPI_RESOURCE_TYPE_IRQ)
                {
                    auto        irqRes = entry.irq;

                    IrqResource irqs;
                    irqs.Triggering     = irqRes.triggering;
                    irqs.Polarity       = irqRes.polarity;
                    irqs.Sharing        = irqRes.sharing;
                    irqs.WakeCapability = irqRes.wake_capability;
                    irqs.IRQs.Resize(irqRes.num_irqs);
                    Memory::Copy(irqs.IRQs.Raw(), irqRes.irqs, irqRes.num_irqs);

                    return irqs;
                }
            }

            return Error(ENOENT);
        }
        ErrorOr<IoResource> IoResourceForHandle(DeviceHandle* handle)
        {
            auto resources = handle->Resources;

            if (!resources) return Error(ENOENT);
            for (usize i = 0; i < resources->length; i++)
            {
                auto& entry = resources->entries[i];
                if (entry.type == UACPI_RESOURCE_TYPE_IO)
                {
                    auto       ioRes = entry.io;

                    IoResource io;
                    io.Least     = ioRes.minimum;
                    io.Highest   = ioRes.maximum;
                    io.Alignment = ioRes.alignment;
                    io.Length    = ioRes.length;

                    return io;
                }
            }

            return Error(ENOENT);
        }
    }; // namespace Bus
}; // namespace ACPI

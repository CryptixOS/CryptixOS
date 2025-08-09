/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/ACPI/Bus.hpp>
#include <Library/Locking/SpinlockProtected.hpp>

#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Memory/Ref.hpp>

#include <uacpi/context.h>
#include <uacpi/event.h>
#include <uacpi/notify.h>
#include <uacpi/resources.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>

namespace ACPI::Bus
{
    struct DeviceHandle : public RefCounted
    {
        uacpi_namespace_node* Node = nullptr;
        DeviceHandle(uacpi_namespace_node* node)
            : Node(node)
        {
        }

        using HookType
            = IntrusiveRefListHook<DeviceHandle, ::Ref<DeviceHandle>>;
        friend class IntrusiveRefList<DeviceHandle, HookType>;
        friend struct IntrusiveRefListHook<DeviceHandle, ::Ref<DeviceHandle>>;

        using List = IntrusiveRefList<DeviceHandle, HookType>;
        HookType Hook;
    };

    static Spinlock           s_Lock;
    static DeviceHandle::List s_DeviceHandles{};

    static uacpi_iteration_decision
    EnumerateDevice(void* ctx, uacpi_namespace_node* node, uacpi_u32)
    {
        uacpi_object_type nodeType;
        uacpi_namespace_node_type(node, &nodeType);

        const char* path = uacpi_namespace_node_generate_absolute_path(node);
        const char* nodeTypeString = uacpi_object_type_to_string(nodeType);

        LogDebug("ACPI: Discovered node `{}` with type: {}", path,
                 nodeTypeString);

        uacpi_namespace_node_info* info;
        uacpi_status ret = uacpi_get_namespace_node_info(node, &info);

        if (uacpi_unlikely_error(ret))
        {
            LogError("ACPI: Unable to retrieve node %s information: {}", path,
                     uacpi_status_to_string(ret));
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

            auto error = uacpi_unlikely_error(uacpi_eval_cid(node, &idList));
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
        ScopedLock guard(s_Lock);
        s_DeviceHandles.PushBack(CreateRef<DeviceHandle>(node));

        LogTrace("ACPI: Added the device node to the bus");
        return UACPI_ITERATION_DECISION_CONTINUE;
    }
    void Initialize()
    {
        uacpi_namespace_for_each_child(uacpi_namespace_root(), EnumerateDevice,
                                       UACPI_NULL, UACPI_OBJECT_DEVICE_BIT,
                                       UACPI_MAX_DEPTH_ANY, UACPI_NULL);
    }
}; // namespace ACPI::Bus

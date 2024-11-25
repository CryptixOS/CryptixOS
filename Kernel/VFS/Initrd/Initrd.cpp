/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Initrd.hpp"

#include "Memory/PMM.hpp"
#include "Memory/VMM.hpp"

#include "Utility/Logger.hpp"
#include "Utility/Math.hpp"
#include "VFS/Initrd/Ustar.hpp"

namespace Initrd
{
    bool Initialize()
    {
        auto initrd = BootInfo::FindModule("initrd");
        if (!initrd)
        {
            LogError("Could not find initrd module!");
            return false;
        }

        uintptr_t address = ToHigherHalfAddress<uintptr_t>(
            reinterpret_cast<uintptr_t>(initrd->address));

        if (Ustar::Validate(address))
        {
            LogTrace("Initrd: Trying to load USTAR archive...");
            Ustar::Load(address);
        }
        else
        {
            LogError("Initrd: Unknown archive format!");
            return false;
        }

        size_t pageCount
            = (Math::AlignUp(initrd->size, 512) + 512) / PMM::PAGE_SIZE;
        PhysicalMemoryManager::FreePages(FromHigherHalfAddress<void*>(address),
                                         pageCount);

        LogInfo("Initrd: Loaded");
        return true;
    }
} // namespace Initrd

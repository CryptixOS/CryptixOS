/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootModuleInfo.hpp>
#include <Library/Logger.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <System/System.hpp>

#include <VFS/Initrd/Initrd.hpp>
#include <VFS/Initrd/Ustar.hpp>

namespace Initrd
{
    bool Initialize()
    {
        LogTrace("Initrd: Loading initial ramdisk...");
        auto initrd = System::FindBootModule("initrd");
        if (!initrd || !initrd->LoadAddress || initrd->Size == 0)
        {
            LogError("Initrd: Could not find initrd module!");
            return false;
        }

        auto address = Pointer(initrd->LoadAddress).ToHigherHalf();

        if (Ustar::Validate(address)) Ustar::Load(address, initrd->Size);
        else
        {
            LogError("Initrd: Unknown archive format!");
            return false;
        }

        size_t pageCount
            = (Math::AlignUp(initrd->Size, 512) + 512) / PMM::PAGE_SIZE;
        PMM::FreePages(address.FromHigherHalf<void*>(),
                                         pageCount);

        LogInfo("Initrd: Loaded successfully");
        return true;
    }
} // namespace Initrd

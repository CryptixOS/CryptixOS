/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootModuleInfo.hpp>
#include <Library/Logger.hpp>
#include <Library/ZLib.hpp>

#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Math.hpp>
#include <System/System.hpp>

#include <VFS/Initrd/Initrd.hpp>
#include <VFS/Initrd/Ustar.hpp>

namespace Initrd
{
    namespace
    {
        // A zlib stream starts with CMF/FLG: deflate (CM == 8) with a window
        // of at most 32KiB (CINFO <= 7), and CMF * 256 + FLG divisible by 31.
        // A ustar archive starts with a file name instead, which never passes.
        bool IsZLibStream(Pointer address, usize size)
        {
            if (size < 2) return false;
            auto bytes = address.As<const u8>();
            u8   cmf   = bytes[0];
            u8   flg   = bytes[1];

            return (cmf & 0x0f) == 8 && (cmf >> 4) <= 7
                && ((u16(cmf) << 8) | flg) % 31 == 0;
        }
    } // namespace

    bool Initialize()
    {
        LogTrace("Initrd: Loading initial ramdisk...");
        auto initrd = System::FindBootModule("initrd");
        if (!initrd || !initrd->LoadAddress || initrd->Size == 0)
        {
            LogError("Initrd: Could not find initrd module!");
            return false;
        }

        auto  address = Pointer(initrd->LoadAddress).ToHigherHalf();
        usize size    = initrd->Size;

        // Owns the decompressed archive until it has been unpacked: Ustar
        // copies every file into the root filesystem, so the buffer is freed
        // when this returns.
        ZLib::Decompressor decompressor(address, size);
        if (IsZLibStream(address, size))
        {
            LogTrace("Initrd: Decompressing a {} KiB zlib stream...",
                     size / 1024);
            // zlib doesn't record the decompressed size: start from a guess
            // that fits a typical tarball, the buffer grows if it's too small
            if (!decompressor.Decompress(size * 4))
            {
                LogError("Initrd: Failed to decompress the initrd!");
                return false;
            }

            address = decompressor.DecompressedData();
            size    = decompressor.DecompressedSize();
            LogTrace("Initrd: Decompressed to {} KiB", size / 1024);
        }

        if (!Ustar::Validate(address))
        {
            LogError("Initrd: Unknown archive format!");
            return false;
        }

        Assert(Ustar::Load(address, size));
        // usize pageCount
        //     = (Math::AlignUp(initrd->Size, 512) + 512) / PMM::PAGE_SIZE;
        // PMM::FreePages(address.FromHigherHalf<void*>(), pageCount);

        LogInfo("Initrd: Loaded successfully");
        return true;
    }
} // namespace Initrd

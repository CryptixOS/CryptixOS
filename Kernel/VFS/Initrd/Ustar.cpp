/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <API/UnixTypes.hpp>
#include <Drivers/Device.hpp>
#include <Library/ZLib.hpp>

#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Math.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <VFS/Initrd/Ustar.hpp>

#include <cstring>

namespace Ustar
{
    bool Validate(Pointer address)
    {
        auto file = address.As<FileHeader>();
        return StringView(file->Signature, MAGIC_LENGTH - 1) == MAGIC;
    }

    ErrorOr<void> Load(Pointer address, usize size)
    {
        LogTrace("USTAR: Loading at '{:#x}'...", address);

        // ZLib::Decompressor decompressor(address, size);
        // Assert(decompressor.Decompress(size));
        //
        // address = decompressor.DecompressedData();
        // size = decompressor.DecompressedSize();

        auto current = address.As<FileHeader>();
        auto getNextFile
            = [](FileHeader* current, usize fileSize) -> FileHeader*
        {
            Pointer nextFile
                = Pointer(current).Offset(512 + Math::AlignUp(fileSize, 512));

            return nextFile.As<FileHeader>();
        };

        while (StringView(current->Signature, MAGIC_LENGTH - 1) == MAGIC)
        {
            PathView filename(current->FileName);
            PathView linkName(current->LinkName);

            auto     mode = StringUtils::ToNumber<mode_t>(current->Mode, 8);
            usize    size = StringUtils::ToNumber<usize>(current->FileSize, 8);

            if (filename == "./"_pv)
            {
                current = getNextFile(current, size);
                continue;
            }

            Ref<DirectoryEntry> dentry = nullptr;
            switch (current->Type)
            {
                case FILE_TYPE_NORMAL:
                case FILE_TYPE_NORMAL_:
                    dentry
                        = TryOrRet(VFS::CreateFile(filename, mode | S_IFREG));

                    if (!dentry)
                        LogError(
                            "USTAR: Failed to create regular file!, path: "
                            "'{}'",
                            filename);
                    else if (dentry->INode()->Write(
                                 reinterpret_cast<u8*>(
                                     reinterpret_cast<uintptr_t>(current)
                                     + 512),
                                 0, size)
                             != isize(size))
                        LogError(
                            "USTAR: Could not write to regular file! path: "
                            "'{}'",
                            filename);
                    break;
                case FILE_TYPE_HARD_LINK:
                    if (!VFS::Link(filename, linkName))
                        LogError(
                            "USTAR: Failed to create hardlink: '{}' -> '{}'",
                            filename, linkName);
                    break;
                case FILE_TYPE_SYMLINK:
                    if (!VFS::Symlink(filename, linkName))
                        LogError(
                            "USTAR: Failed to create Symlink: '{}' -> '{}'",
                            filename, linkName);
                    break;
                case FILE_TYPE_CHARACTER_DEVICE: CTOS_FALLTHROUGH;
                case FILE_TYPE_BLOCK_DEVICE:
                {
                    u32 deviceMajor
                        = StringUtils::ToNumber<u32>(current->DeviceMajor, 8);
                    u32 deviceMinor
                        = StringUtils::ToNumber<u32>(current->DeviceMinor, 8);

                    if (current->Type == FILE_TYPE_CHARACTER_DEVICE)
                        mode |= S_IFCHR;
                    else if (current->Type == FILE_TYPE_BLOCK_DEVICE)
                        mode |= S_IFBLK;

                    if (!VFS::CreateNode(filename, mode,
                                         MakeDevice(deviceMajor, deviceMinor)))
                        LogError(
                            "USTAR: Failed to create device node! path: "
                            "'{}', id: "
                            "{{ major: {}, minor: {} }}",
                            filename, deviceMajor, deviceMinor);
                    break;
                }
                case FILE_TYPE_DIRECTORY:
                    dentry = TryOrRet(
                        VFS::CreateDirectory(filename, mode | S_IFDIR));
                    if (!dentry)
                        LogError(
                            "USTAR: Failed to create a directory! path: '{}'",
                            filename);
                    break;
                case FILE_TYPE_FIFO: ToDo(); break;
                default: break;
            }

            current = getNextFile(current, size);
        }

        return {};
    }
} // namespace Ustar

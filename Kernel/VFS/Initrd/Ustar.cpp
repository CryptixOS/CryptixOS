/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/UnixTypes.hpp>
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
#include <Prism/Containers/Array.hpp>
#include <Prism/Memory/BitStream.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Library/Module.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.Initrd.Ustar;

import :Drivers.Core.Device;
import :Library.ZLib;
import :VFS.INode;
import :VFS.VFS;
import :VFS.Initrd.Ustar;

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
        usize nextThreshold = 10;

        // ZLib::Decompressor decompressor(address, size);
        // Assert(decompressor.Decompress(size));
        //
        // address = decompressor.DecompressedData();
        // size = decompressor.DecompressedSize();

        auto  current       = address.As<FileHeader>();
        auto  getNextFile
            = [](FileHeader* current, usize fileSize) -> FileHeader*
        {
            Pointer nextFile
                = Pointer(current).Offset(512 + Math::AlignUp(fileSize, 512));

            return nextFile.As<FileHeader>();
        };

        while (StringView(current->Signature, MAGIC_LENGTH - 1) == MAGIC)
        {
            usize offset     = Pointer(current) - address;
            usize percentage = (offset * 100) / size;

            if (percentage >= nextThreshold)
            {
                LogInfo("USTAR: Loading progress: {}%", (percentage / 10) * 10);

                nextThreshold = ((percentage / 10) + 1) * 10;
            }

            PathView filename(current->FileName);
            PathView linkName(current->LinkName);

            auto     mode = StringUtils::ToNumber<mode_t>(current->Mode, 8);
            usize    size = StringUtils::ToNumber<usize>(current->FileSize, 8);

            if (filename == "./"_pv)
            {
                current = getNextFile(current, size);
                continue;
            }

            // LogTrace("File: {}", filename);
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

        LogInfo("USTAR: Loading complete (100%)");
        return {};
    }
} // namespace Ustar

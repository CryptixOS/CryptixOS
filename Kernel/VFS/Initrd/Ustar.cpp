/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

#include <API/UnixTypes.hpp>
#include <Drivers/Device.hpp>
#include <Prism/Math.hpp>

#include <VFS/INode.hpp>
#include <VFS/Initrd/Ustar.hpp>

#include <cstring>

template <typename T>
inline static T parseOctNumber(const char* str, usize len)
    requires std::is_trivial_v<T>
{
    T value = 0;
    while (*str && len > 0)
    {
        value = value * 8 + (*str++ - '0');
        --len;
    }

    return value;
}

namespace Ustar
{
    bool Validate(uintptr_t address)
    {
        return strncmp(reinterpret_cast<FileHeader*>(address)->signature, MAGIC,
                       MAGIC_LENGTH - 1)
            == 0;
    }

    void Load(uintptr_t address)
    {
        LogTrace("USTAR: Loading at '{:#x}'...", address);

        auto current = reinterpret_cast<FileHeader*>(address);
        auto getNextFile
            = [](FileHeader* current, usize fileSize) -> FileHeader*
        {
            uintptr_t nextFile = reinterpret_cast<uintptr_t>(current) + 512
                               + Math::AlignUp(fileSize, 512);

            return reinterpret_cast<FileHeader*>(nextFile);
        };

        while (strncmp(current->signature, MAGIC, MAGIC_LENGTH - 1) == 0)
        {
            std::string_view filename(current->filename);
            std::string_view linkName(current->linkName);

            mode_t           mode
                = parseOctNumber<mode_t>(current->mode, sizeof(current->mode));
            usize size = parseOctNumber<usize>(current->fileSize,
                                               sizeof(current->fileSize));

            if (filename == "./")
            {
                current = getNextFile(current, size);
                continue;
            }

            INode* node = nullptr;
            switch (current->type)
            {
                case FILE_TYPE_NORMAL:
                case FILE_TYPE_NORMAL_:
                    node = VFS::CreateNode(nullptr, filename, mode | S_IFREG);

                    if (!node)
                        LogError(
                            "USTAR: Failed to create regular file!, path: "
                            "'{}'",
                            filename.data());
                    else if (node->Write(
                                 reinterpret_cast<u8*>(
                                     reinterpret_cast<uintptr_t>(current)
                                     + 512),
                                 0, size)
                             != isize(size))
                        LogError(
                            "USTAR: Could not write to regular file! path: "
                            "'{}'",
                            filename.data());
                    break;
                case FILE_TYPE_HARD_LINK:
                    LogError("USTAR: Loading hard links is not implemented.");
                    break;
                    LogError("USTAR: Failed to create hardlink: '{}' -> '{}'",
                             filename.data(), linkName.data());
                    break;
                case FILE_TYPE_SYMLINK:
                    node = VFS::Symlink(VFS::GetRootNode(), filename.data(),
                                        linkName.data());
                    if (!node)
                        LogError(
                            "USTAR: Failed to create Symlink: '{}' -> '{}'",
                            filename.data(), linkName.data());
                    break;
                case FILE_TYPE_CHARACTER_DEVICE:
                {
                    u32 deviceMajor = parseOctNumber<u32>(
                        current->deviceMajor, sizeof(current->deviceMajor));
                    u32 deviceMinor = parseOctNumber<u32>(
                        current->deviceMinor, sizeof(current->deviceMinor));

                    VFS::MkNod(VFS::GetRootNode(), filename, mode | S_IFCHR,
                               makeDevice(deviceMajor, deviceMinor));

                    if (!node)
                        LogError(
                            "USTAR: Failed to create character device! path: "
                            "'{}', id: "
                            "{{ major: {}, minor: {} }}",
                            filename, deviceMajor, deviceMinor);
                    break;
                }
                case FILE_TYPE_BLOCK_DEVICE: ToDo(); break;
                case FILE_TYPE_DIRECTORY:
                    node = VFS::CreateNode(nullptr, filename, mode | S_IFDIR);
                    if (!node)
                        LogError(
                            "USTAR: Failed to create a directory! path: '{}'",
                            filename.data());
                    break;
                case FILE_TYPE_FIFO: ToDo(); break;
                default: break;
            }

            current = getNextFile(current, size);
        }
    }
} // namespace Ustar

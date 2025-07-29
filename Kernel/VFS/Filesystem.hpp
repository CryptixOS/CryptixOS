/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/statfs.h>
#include <API/UnixTypes.hpp>

#include <Library/Locking/Spinlock.hpp>

#include <Prism/Core/Error.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Atomic.hpp>

#include <VFS/DirectoryEntry.hpp>

class INode;
class DirectoryEntry;
using INodeMode = mode_t;
using DeviceID  = dev_t;

/**
 * @class Filesystem
 * @brief Abstract base class representing a mounted filesystem.
 *
 * A filesystem handles mounting, inode creation, linking,
 * and other filesystem-level operations.
 * @ingroup Filesystem
 */
class Filesystem : public RefCounted
{
  public:
    /**
     * @brief Construct a new Filesystem instance.
     *
     * @param name Name of the filesystem.
     * @param flags Mount flags or other internal flags.
     */
    Filesystem(StringView name, u32 flags)
        : m_Name(name)
        , m_Flags(flags)
    {
    }
    /**
     * @brief Destructs the filesystem
     */
    virtual ~Filesystem() = default;

    /**
     * @brief Get the name of the filesystem.
     *
     * @return StringView Name of the filesystem.
     */
    inline StringView            Name() const { return m_Name; }
    /**
     * @brief Get the id of the backing device on which filesystem sits.
     *
     * @return DeviceID Backing device ID of the filesystem.
     */
    inline DeviceID              BackingDeviceID() const { return m_DeviceID; }
    /**
     * @brief Get the root DirectoryEntry of this filesystem.
     *
     * @return DirectoryEntry* Pointer to the root directory entry.
     * @ingroup Filesystem
     */
    inline ::Ref<DirectoryEntry> RootDirectoryEntry() { return m_RootEntry; }
    /**
     * @brief Get the next available inode index.
     *
     * @return ino_t Next inode index (auto-incremented).
     */
    inline ino_t                 NextINodeIndex() { return m_NextINodeIndex++; }
    /**
     * @brief Get the name of the backing device.
     *
     * @return StringView Device name.
     */
    virtual StringView           DeviceName() const { return m_Name; }
    /**
     * @brief Get a string representation of mount flags.
     *
     * @return StringView Flags as a string (e.g. "rw,noatime").
     */
    virtual StringView MountFlagsString() const { return "rw,noatime"; }

    /**
     * @brief Mount the filesystem.
     *
     * @param source Optional source device path.
     * @param data Optional filesystem-specific mount data.
     *
     * @return ErrorOr<DirectoryEntry*> Root entry of the mounted filesystem or
     * error.
     */
    virtual ErrorOr<::Ref<DirectoryEntry>> Mount(StringView  sourcePath,
                                                 const void* data = nullptr)
        = 0;

    /**
     * @brief Allocate a new inode.
     * @param name Name of the newly created inode
     * @param mode File mode (type and permissions)
     *
     * @return ErrorOr<INode*> Pointer to the new inode or error.
     */
    virtual ErrorOr<INode*> AllocateNode(StringView name = "",
                                         INodeMode  mode = 0)
    {
        return Error(ENOSYS);
    }
    /**
     * @brief Free an inode.
     * @param inode Pointer to the inode to be freed
     *
     * @return ErrorOr<void> Nothing or error.
     */
    virtual ErrorOr<void>          FreeINode(INode* inode) { return Error(ENOSYS); }
    /**
     * @brief Populate directory contents, if lazy-loading is used.
     * @param Directory entry to populate.
     *
     * @return true if successful or not needed, false on failure.
     */
    virtual bool          Populate(DirectoryEntry* entry) = 0;

    /**
     * @brief Return filesystem statistics (like block size, usage).
     *
     * @param stats Reference to struct to populate.
     *
     * @return ErrorOr<void> Success or error.
     */
    virtual ErrorOr<void> Stats(statfs& stats) { return Error(ENOSYS); }

    virtual bool          ShouldUpdateATime() { return true; }
    virtual bool          ShouldUpdateMTime() { return true; }
    virtual bool          ShouldUpdateCTime() { return true; }

  protected:
    constexpr static usize TMPFS_MAGIC = 0x01021994;

    // Synchronization lock for internal access
    Spinlock               m_Lock;

    String                 m_Name           = "NoFs";
    fsid_t                 m_ID             = {};

    dev_t                  m_DeviceID       = -1;
    usize                  m_BlockSize      = 512;
    usize                  m_BytesLimit     = 0;
    ///> Filesystem specific flags for internal use
    u32                    m_Flags          = 0;

    ///> Backing device
    INode*                 m_SourceDevice   = nullptr;
    ///> Root directory entry
    ::Ref<DirectoryEntry>  m_RootEntry      = nullptr;
    ///> Root inode
    INode*                 m_Root           = nullptr;

    ///> Filesystem specific data
    void*                  m_MountData      = nullptr;

    ///> Counter for generating inode ids
    Atomic<ino_t>          m_NextINodeIndex = 2;

    inline static fsid_t   NextFilesystemID()
    {
        static Atomic<fsid_t> id = 1000;
        return id++;
    }

  private:
};

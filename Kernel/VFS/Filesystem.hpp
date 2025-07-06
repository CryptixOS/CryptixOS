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

#include <Prism/Memory/Ref.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/Atomic.hpp>

#include <VFS/DirectoryEntry.hpp>

class INode;
class DirectoryEntry;
using INodeMode = mode_t;

/**
 * @class Filesystem
 * @brief Abstract base class representing a mounted filesystem.
 *
 * A filesystem handles mounting, inode creation, linking,
 * and other filesystem-level operations.
 * @ingroup Filesystem
 */
class Filesystem
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
    inline StringView          Name() const { return m_Name; }
    /**
     * @brief Get the root DirectoryEntry of this filesystem.
     *
     * @return DirectoryEntry* Pointer to the root directory entry.
     * @ingroup Filesystem
     */
    inline Ref<DirectoryEntry> RootDirectoryEntry() { return m_RootEntry; }
    /**
     * @brief Get the next available inode index.
     *
     * @return ino_t Next inode index (auto-incremented).
     */
    inline ino_t               NextINodeIndex() { return m_NextInodeIndex++; }
    /**
     * @brief Get the device ID of the backing device.
     *
     * @return dev_t Device ID.
     */
    inline dev_t               DeviceID() const { return m_DeviceID; }
    /**
     * @brief Get the name of the backing device.
     *
     * @return StringView Device name.
     */
    virtual StringView         DeviceName() const { return m_Name; }
    /**
     * @brief Get a string representation of mount flags.
     *
     * @return StringView Flags as a string (e.g. "rw,noatime").
     */
    virtual StringView         MountFlagsString() const { return "rw,noatime"; }

    /**
     * @brief Mount the filesystem.
     *
     * @param source Optional source device path.
     * @param data Optional filesystem-specific mount data.
     *
     * @return ErrorOr<DirectoryEntry*> Root entry of the mounted filesystem or
     * error.
     */
    virtual ErrorOr<Ref<DirectoryEntry>> Mount(StringView  sourcePath,
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
     * @brief Create a new inode (file or directory).
     *
     * @param parent Parent directory.
     * @param entry Directory entry under which to create node.
     * @param mode File mode (type and permissions).
     * @param uid Owner user ID.
     * @param gid Owner group ID.
     *
     * @return ErrorOr<INode*> Pointer to the new inode or error.
     */
    virtual ErrorOr<INode*> CreateNode(INode* parent, Ref<DirectoryEntry> entry,
                                       mode_t mode, uid_t uid = 0,
                                       gid_t gid = 0)
        = 0;
    /**
     * @brief Create a symbolic link.
     *
     * @param parent Parent directory.
     * @param entry Directory entry for the link.
     * @param target Path the link should point to.
     *
     * @return ErrorOr<INode*> Pointer to the symlink inode or error.
     */
    virtual ErrorOr<INode*> Symlink(INode* parent, Ref<DirectoryEntry> entry,
                                    StringView target)
        = 0;
    /**
     * @brief Create a hard link to an existing inode.
     *
     * @param parent Directory in which to place the link.
     * @param name Name of the new link.
     * @param oldNode The inode to link to.
     *
     * @return INode* Pointer to the newly linked inode.
     */
    virtual INode* Link(INode* parent, StringView name, INode* oldNode) = 0;
    /**
     * @brief Populate directory contents, if lazy-loading is used.
     * @param Directory entry to populate.
     *
     * @return true if successful or not needed, false on failure.
     */
    virtual bool   Populate(DirectoryEntry* entry)                      = 0;

    /**
     * @brief Create a special or device file node.
     *
     * @param parent Parent directory.
     * @param entry Directory entry.
     * @param mode Mode specifying node type.
     * @param dev Device ID for special node.
     *
     * @return ErrorOr<INode*> Pointer to new node or error.
     */
    virtual ErrorOr<INode*> MkNod(INode* parent, Ref<DirectoryEntry> entry,
                                  mode_t mode, dev_t dev = 0)
    {
        return Error(ENOSYS);
    }

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
    // Synchronization lock for internal access
    Spinlock            m_Lock;

    String              m_Name           = "NoFs";
    dev_t               m_DeviceID       = -1;
    usize               m_BlockSize      = 512;
    usize               m_BytesLimit     = 0;
    ///> Filesystem specific flags for internal use
    u32                 m_Flags          = 0;

    ///> Backing device
    INode*              m_SourceDevice   = nullptr;
    ///> Root directory entry
    Ref<DirectoryEntry> m_RootEntry      = nullptr;
    ///> Root inode
    INode*              m_Root           = nullptr;

    ///> Filesystem specific data
    void*               m_MountData      = nullptr;

    ///> Counter for generating inode ids
    Atomic<ino_t>       m_NextInodeIndex = 2;
};

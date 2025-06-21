/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/statfs.h>
#include <API/UnixTypes.hpp>

#include <Library/Spinlock.hpp>

#include <Prism/String/String.hpp>
#include <Prism/Utility/Atomic.hpp>

class INode;
class DirectoryEntry;
class Filesystem
{
  public:
    Filesystem(StringView name, u32 flags)
        : m_Name(name)
        , m_Flags(flags)
    {
    }
    virtual ~Filesystem() = default;

    inline StringView  GetName() const { return m_Name; }
    inline INode*      GetMountedOn() { return m_MountedOn; }
    inline INode*      GetRootNode() { return m_Root; }
    inline ino_t       GetNextINodeIndex() { return m_NextInodeIndex++; }
    inline dev_t       GetDeviceID() const { return m_DeviceID; }

    virtual StringView GetDeviceName() const { return m_Name; }
    virtual StringView GetMountFlagsString() const { return "rw,noatime"; }

    virtual INode*     Mount(INode* parent, INode* source, INode* target,
                             DirectoryEntry* entry, StringView name,
                             const void* data = nullptr)
        = 0;
    virtual INode* CreateNode(INode* parent, DirectoryEntry* entry, mode_t mode,
                              uid_t uid = 0, gid_t gid = 0)
        = 0;
    virtual INode* Symlink(INode* parent, DirectoryEntry* entry,
                           StringView target)
        = 0;

    virtual INode* Link(INode* parent, StringView name, INode* oldNode) = 0;
    virtual bool   Populate(INode* node)                                = 0;

    virtual INode* MkNod(INode* parent, DirectoryEntry* entry, mode_t mode,
                         dev_t dev)
    {
        return nullptr;
    }

    virtual ErrorOr<void> Stats(statfs& stats) { return Error(ENOSYS); }

    virtual bool          ShouldUpdateATime() { return true; }
    virtual bool          ShouldUpdateMTime() { return true; }
    virtual bool          ShouldUpdateCTime() { return true; }

  protected:
    Spinlock      m_Lock;

    String        m_Name           = "NoFs";
    dev_t         m_DeviceID       = -1;
    usize         m_BlockSize      = 512;
    usize         m_BytesLimit     = 0;
    u32           m_Flags          = 0;

    INode*        m_SourceDevice   = nullptr;
    INode*        m_Root           = nullptr;

    INode*        m_MountedOn      = nullptr;
    void*         m_MountData      = nullptr;

    Atomic<ino_t> m_NextInodeIndex = 2;
};

/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/UnixTypes.hpp>
#include <Library/Spinlock.hpp>

#include <Prism/String/String.hpp>

#include <atomic>

class INode;
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
                             StringView name, const void* data = nullptr)
        = 0;
    virtual INode* CreateNode(INode* parent, StringView name, mode_t mode) = 0;
    virtual INode* Symlink(INode* parent, StringView name, StringView target)
        = 0;

    virtual INode* Link(INode* parent, StringView name, INode* oldNode) = 0;
    virtual bool   Populate(INode* node)                                = 0;

    virtual INode* MkNod(INode* parent, StringView path, mode_t mode, dev_t dev)
    {
        return nullptr;
    }

    virtual bool ShouldUpdateATime() { return true; }
    virtual bool ShouldUpdateMTime() { return true; }
    virtual bool ShouldUpdateCTime() { return true; }

  protected:
    Spinlock           m_Lock;

    String             m_Name           = "NoFs";
    dev_t              m_DeviceID       = -1;
    usize              m_BlockSize      = 512;
    usize              m_BytesLimit     = 0;
    u32                m_Flags          = 0;

    INode*             m_SourceDevice   = nullptr;
    INode*             m_Root           = nullptr;

    INode*             m_MountedOn      = nullptr;
    void*              m_MountData      = nullptr;

    std::atomic<ino_t> m_NextInodeIndex = 2;
};

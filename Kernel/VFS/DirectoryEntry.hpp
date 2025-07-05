/*
 * Created by v1tr10l7 on 31.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Memory/Ref.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <VFS/INode.hpp>

class DirectoryEntry : public RefCounted
{
  public:
    explicit inline DirectoryEntry(StringView name)
        : m_Name(name)
    {
    }
    DirectoryEntry(::Ref<DirectoryEntry> parent, INode* inode);
    DirectoryEntry(::Ref<DirectoryEntry> parent, StringView name);
    DirectoryEntry(const DirectoryEntry& other)
        : m_INode(other.m_INode)
    {
    }

    virtual ~DirectoryEntry();

    inline INode*          INode() const { return m_INode; }
    inline StringView      Name() const { return m_Name; }
    inline ::Ref<DirectoryEntry> Parent()  { return m_Parent; }

    Path                   Path();
    const std::unordered_map<StringView, ::Ref<DirectoryEntry>>&
                    Children() const;

    void            SetParent(::Ref<DirectoryEntry> entry);
    void            SetMountGate(class INode* inode, ::Ref<DirectoryEntry> mountGate);
    void            Bind(class INode* inode);
    void            InsertChild(::Ref<class DirectoryEntry> entry);
    void            RemoveChild(::Ref<class DirectoryEntry> entry);

    ::Ref<DirectoryEntry> FollowMounts();
    ::Ref<DirectoryEntry> FollowSymlinks(usize cnt = 0);

    WeakRef<DirectoryEntry> GetEffectiveParent() ;
    ::Ref<DirectoryEntry> Lookup(const String& name);

    inline bool     IsCharDevice() const { return m_INode->IsCharDevice(); }
    inline bool     IsFifo() const { return m_INode->IsFifo(); }
    inline bool     IsDirectory() const { return m_INode->IsDirectory(); }
    inline bool     IsRegular() const { return m_INode->IsRegular(); }
    inline bool     IsSymlink() const { return m_INode->IsSymlink(); }
    inline bool     IsSocket() const { return m_INode->IsSocket(); }

    ::Ref<DirectoryEntry> m_MountGate = nullptr;

  private:
    friend class INode;

    Spinlock        m_Lock;
    class INode*    m_INode  = nullptr;

    String          m_Name   = "";

    ::Ref<DirectoryEntry> m_Parent = nullptr;
    std::unordered_map<StringView, ::Ref<class DirectoryEntry>> m_Children;
};

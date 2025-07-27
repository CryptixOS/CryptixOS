/*
 * Created by v1tr10l7 on 31.05.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/stat.h>
#include <Library/Locking/Spinlock.hpp>

#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Core/Error.hpp>

#include <Prism/Memory/Ref.hpp>
#include <Prism/Memory/WeakRef.hpp>

#include <Prism/String/String.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <Prism/Utility/Path.hpp>

class INode;

enum class DirectoryEntryFlags
{
    eNegative      = 0,
    eDirectory     = 2,
    eRegular       = 4,
    eSpecial       = 5,
    eSymlink       = 6,
    eEntryTypeMask = 7,

    eMountPoint    = Bit(5),
};

constexpr DirectoryEntryFlags& operator|=(DirectoryEntryFlags&      lhs,
                                          const DirectoryEntryFlags rhs)
{
    return lhs = static_cast<DirectoryEntryFlags>(ToUnderlying(lhs)
                                                  | ToUnderlying(rhs));
}
constexpr DirectoryEntryFlags& operator&=(DirectoryEntryFlags&      lhs,
                                          const DirectoryEntryFlags rhs)
{
    return lhs = static_cast<DirectoryEntryFlags>(ToUnderlying(lhs)
                                                  & ToUnderlying(rhs));
}
constexpr DirectoryEntryFlags operator~(const DirectoryEntryFlags flags)
{
    return static_cast<DirectoryEntryFlags>(~ToUnderlying(flags));
}
constexpr bool operator&(const DirectoryEntryFlags lhs,
                         const DirectoryEntryFlags rhs)
{
    return ToUnderlying(lhs) & ToUnderlying(rhs);
}

class DirectoryEntry : public RefCounted
{
  public:
    explicit inline DirectoryEntry(StringView name)
        : m_Name(name)
    {
    }
    DirectoryEntry(::WeakRef<DirectoryEntry> parent, StringView name);
    DirectoryEntry(const DirectoryEntry& other)
        : m_INode(other.m_INode)
    {
    }

    virtual ~DirectoryEntry();

    inline class INode*              INode() const { return m_INode; }
    inline StringView                Name() const { return m_Name; }
    inline ::WeakRef<DirectoryEntry> Parent() { return m_Parent; }

    Path                             Path();
    const UnorderedMap<StringView, ::Ref<DirectoryEntry>>& Children() const;

    void                      SetParent(::WeakRef<DirectoryEntry> entry);
    void                      SetMountGate(::Ref<DirectoryEntry> mountGate);
    void                      Bind(class INode* inode);
    void                      InsertChild(::Ref<class DirectoryEntry> entry);
    void                      RemoveChild(::Ref<class DirectoryEntry> entry);

    ::WeakRef<DirectoryEntry> FollowMounts();
    ::WeakRef<DirectoryEntry> FollowSymlinks(usize cnt = 0);

    WeakRef<DirectoryEntry>   GetEffectiveParent();

    using Iterator
        = UnorderedMap<StringView, ::Ref<class DirectoryEntry>>::Iterator<>;
    using ConstIterator
        = UnorderedMap<StringView, ::Ref<class DirectoryEntry>>::ConstIterator;
    Iterator      begin() { return m_Children.begin(); }
    ConstIterator begin() const { return m_Children.begin(); }
    Iterator      end() { return m_Children.end(); }
    ConstIterator end() const { return m_Children.end(); }

    using DirectoryIterator
        = Delegate<bool(StringView name, loff_t offset, usize ino, u64 type)>;
    ErrorOr<void> TraverseDirectories(::Ref<class DirectoryEntry> parent,
                                      DirectoryIterator           iterator);
    ::Ref<DirectoryEntry> Lookup(const String& name);
    ErrorOr<void>         PopulateDirectoryEntries();

    bool                  IsMountPoint() const;
    bool                  IsDirectory() const;
    bool                  IsRegular() const;
    bool                  IsSymlink() const;
    bool                  IsSpecial() const;

  private:
    friend class INode;

    Spinlock                  m_Lock;

    String                    m_Name      = "";
    DirectoryEntryFlags       m_Flags     = DirectoryEntryFlags::eNegative;
    class INode*              m_INode     = nullptr;
    bool                      m_Populated = false;
    usize                     m_DirOffset = 0;

    ::WeakRef<DirectoryEntry> m_Parent    = nullptr;
    ::Ref<DirectoryEntry>     m_MountGate = nullptr;
    UnorderedMap<StringView, ::Ref<class DirectoryEntry>> m_Children;
};

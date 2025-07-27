/*
 * Created by v1tr10l7 on 20.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/DirectoryEntry.hpp>
#include <VFS/INode.hpp>

enum class PathSegmentType
{
    eRegular,
    eDotDot,
    eDot,
    eMount,
};
enum class ResolutionState
{
    eUninitialized = 0,
    eInitialized   = 1,
    eFinished      = 2,
    eLast          = 3,
    eContinue      = 4,
    eTerminated    = 5,
};
enum class PathLookupFlags
{
    eRegular       = 1,
    eParent        = 2,
    eFollowLinks   = 4,
    eNegativeEntry = 8,
    eMountPoint    = 16,
};
constexpr PathLookupFlags operator|(PathLookupFlags lhs, PathLookupFlags rhs)
{
    return static_cast<PathLookupFlags>(ToUnderlying(lhs) | ToUnderlying(rhs));
}
constexpr bool operator&(PathLookupFlags& lhs, PathLookupFlags rhs)
{
    return (ToUnderlying(lhs) & ToUnderlying(rhs)) != 0;
}
constexpr PathLookupFlags operator|=(PathLookupFlags& lhs, PathLookupFlags rhs)
{
    return lhs
         = static_cast<PathLookupFlags>(ToUnderlying(lhs) | ToUnderlying(rhs)),
           lhs;
}

// TODO(v1tr10l7): Allow backtracing the path
class PathResolver
{
  public:
    inline PathResolver() = default;
    explicit PathResolver(Ref<DirectoryEntry> const root, PathView path);

    ErrorOr<void> Initialize(Ref<DirectoryEntry> const root, PathView path);
    ErrorOr<Ref<DirectoryEntry>>    Resolve(bool followLinks = true);
    ErrorOr<Ref<DirectoryEntry>>    Resolve(PathLookupFlags flags);

    ErrorOr<void>                   Step();
    ErrorOr<Ref<DirectoryEntry>>    FollowMounts(Ref<DirectoryEntry> dentry
                                                 = nullptr);
    ErrorOr<Ref<DirectoryEntry>>    FollowDown();
    ErrorOr<Ref<DirectoryEntry>>    FollowSymlinks();
    ErrorOr<Ref<DirectoryEntry>>    FollowSymlink();
    Ref<DirectoryEntry>             FollowDots();
    Error                           Terminate(ErrorCode code);

    inline Ref<DirectoryEntry>      RootDirectoryEntry() { return m_Root; }
    constexpr const Vector<String>& Segments() const { return m_Tokens; }
    constexpr isize                 Position() const { return m_Position; }
    constexpr StringView            CurrentSegment() const
    {
        return m_CurrentSegment.Name;
    }

    WeakRef<DirectoryEntry>    GetEffectiveParent(INode* node = nullptr);
    inline Ref<DirectoryEntry> ParentEntry() { return m_Parent; }
    inline Ref<DirectoryEntry> DirectoryEntry() { return m_DirectoryEntry; }

    inline INode*              ParentINode()
    {
        return m_Parent ? m_Parent->INode() : nullptr;
    }
    inline INode* INode() const
    {
        return m_DirectoryEntry ? m_DirectoryEntry->INode() : nullptr;
    }
    constexpr PathSegmentType SegmentType() const
    {
        return m_CurrentSegment.Type;
    }
    constexpr const PathView BaseName() const { return m_BaseName; }

  private:
    class Ref<::DirectoryEntry> m_Root  = nullptr;

    ResolutionState             m_State = ResolutionState::eUninitialized;
    PathView                    m_Path  = "/"_s;
    Vector<String>              m_Tokens;
    isize                       m_Position     = 0;
    isize                       m_SymlinkDepth = 0;
    PathLookupFlags             m_Flags        = PathLookupFlags::eRegular;

    struct Segment
    {
        String          Name   = ""_sv;
        bool            IsLast = false;
        PathSegmentType Type   = PathSegmentType::eRegular;
    } m_CurrentSegment;

    Segment                     GetNextSegment();

    class Ref<::DirectoryEntry> m_Parent         = nullptr;
    class Ref<::DirectoryEntry> m_DirectoryEntry = nullptr;

    Path                        m_BaseName       = ""_s;
};

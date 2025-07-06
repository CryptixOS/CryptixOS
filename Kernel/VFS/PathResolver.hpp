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

// TODO(v1tr10l7): Allow backtracing the path
class PathResolver
{
  public:
    inline PathResolver() = default;
    explicit PathResolver(Ref<DirectoryEntry> const root, PathView path);

    ErrorOr<void> Initialize(Ref<DirectoryEntry> const root, PathView path);
    ErrorOr<DirectoryEntry*>  Resolve(bool followLinks = true);

    ErrorOr<void>             Step();
    ErrorOr<Ref<DirectoryEntry>>  FollowMounts(Ref<DirectoryEntry> dentry = nullptr);
    ErrorOr<DirectoryEntry*>  FollowSymlinks();
    ErrorOr<DirectoryEntry*>  FollowSymlink();
    DirectoryEntry*           FollowDots();
    Error                     Terminate(ErrorCode code);

    constexpr Ref<DirectoryEntry> RootDirectoryEntry() { return m_Root; }
    constexpr const Vector<String>& Segments() const { return m_Tokens; }
    constexpr isize                 Position() const { return m_Position; }
    constexpr StringView            CurrentSegment() const
    {
        return m_CurrentSegment.Name;
    }

    DirectoryEntry*           GetEffectiveParent(INode* node = nullptr);
    constexpr Ref<DirectoryEntry> ParentEntry() { return m_Parent; }
    constexpr Ref<DirectoryEntry> DirectoryEntry() 
    {
        return m_DirectoryEntry;
    }

    constexpr INode* ParentINode() 
    {
        return m_Parent.Raw() ? m_Parent->INode() : nullptr;
    }
    constexpr INode* INode() const
    {
        return m_DirectoryEntry ? m_DirectoryEntry->INode() : nullptr;
    }
    constexpr PathSegmentType SegmentType() const
    {
        return m_CurrentSegment.Type;
    }
    constexpr const Path& BaseName() const { return m_BaseName; }

  private:
    class Ref<::DirectoryEntry> m_Root  = nullptr;

    ResolutionState       m_State = ResolutionState::eUninitialized;
    PathView              m_Path  = "/"_s;
    Vector<String>        m_Tokens;
    isize                 m_Position     = 0;
    isize                 m_SymlinkDepth = 0;

    struct Segment
    {
        String          Name   = ""_sv;
        bool            IsLast = false;
        PathSegmentType Type   = PathSegmentType::eRegular;
    } m_CurrentSegment;

    Segment               GetNextSegment();

    class Ref<::DirectoryEntry> m_Parent         = nullptr;
    class Ref<::DirectoryEntry> m_DirectoryEntry = nullptr;

    Path                  m_BaseName       = ""_s;
};

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

class PathWalker
{
  public:
    struct Result
    {
        class INode*          INode          = nullptr;
        class DirectoryEntry* DirectoryEntry = nullptr;
    };

    inline PathWalker() = default;
    explicit PathWalker(DirectoryEntry* const root, PathView path);

    ErrorOr<void> Initialize(DirectoryEntry* const root, PathView path);
    ErrorOr<DirectoryEntry*>  Resolve();

    ErrorOr<void>             Step();
    ErrorOr<DirectoryEntry*>  FollowMounts(DirectoryEntry* dentry = nullptr);
    ErrorOr<DirectoryEntry*>  FollowSymlinks();
    ErrorOr<DirectoryEntry*>  FollowSymlink();
    DirectoryEntry*           FollowDots();
    Error                     Terminate(ErrorCode code);

    constexpr DirectoryEntry* RootDirectoryEntry() const { return m_Root; }
    constexpr const Vector<String>& Segments() const { return m_Tokens; }
    constexpr isize                 Position() const { return m_Position; }
    constexpr StringView            CurrentSegment() const
    {
        return m_CurrentSegment.Name;
    }

    DirectoryEntry*  GetEffectiveParent(INode* node = nullptr);
    constexpr INode* ParentINode() const
    {
        return m_Parent ? m_Parent->INode() : nullptr;
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
    DirectoryEntry* m_Root  = nullptr;

    ResolutionState m_State = ResolutionState::eUninitialized;
    PathView        m_Path  = "/"_s;
    Vector<String>  m_Tokens;
    isize           m_Position     = 0;
    isize           m_SymlinkDepth = 0;

    struct Segment
    {
        String          Name   = ""_sv;
        bool            IsLast = false;
        PathSegmentType Type   = PathSegmentType::eRegular;
    } m_CurrentSegment;

    Segment               GetNextSegment();

    class DirectoryEntry* m_Parent         = nullptr;
    class DirectoryEntry* m_DirectoryEntry = nullptr;

    Path                  m_BaseName       = ""_s;
};

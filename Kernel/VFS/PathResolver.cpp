/*resolver.INode()
 * Created by v1tr10l7 on 20.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/PathResolver.hpp>
#include <VFS/VFS.hpp>

PathResolver::PathResolver(class Ref<::DirectoryEntry> const root,
                           PathView                          path)
{
    auto result = Initialize(root, path);
    CtosUnused(result);
}

ErrorOr<void> PathResolver::Initialize(Ref<class ::DirectoryEntry> const root,
                                       PathView                          path)
{
    if (path.Empty()) return Terminate(EINVAL);
    m_Root           = root ?: VFS::RootDirectoryEntry();

    m_Path           = path;
    m_Tokens         = m_Path.Split();
    m_Position       = 0;

    m_Parent         = path.Absolute() ? VFS::RootDirectoryEntry() : root;
    m_DirectoryEntry = m_Parent;
    m_BaseName       = "/"_sv;

    // if (!m_Root->IsDirectory()) return Terminate(ENOTDIR);

    if (m_Parent != VFS::RootDirectoryEntry())
    {
        auto result = FollowMounts(m_Parent);
        if (!result) return Error(result.Error());

        m_Parent = result.Value();
    }

    m_State = ResolutionState::eInitialized;
    return {};
}

ErrorOr<Ref<DirectoryEntry>> PathResolver::Resolve(bool followLinks)
{
    PathLookupFlags flags = PathLookupFlags::eRegular;
    if (followLinks) flags |= PathLookupFlags::eFollowLinks;

    return Resolve(flags);
}
ErrorOr<Ref<DirectoryEntry>> PathResolver::Resolve(PathLookupFlags flags)
{
    Assert(m_State == ResolutionState::eInitialized);

    auto view
        = (flags & PathLookupFlags::eParent) ? m_Path.ParentPath() : m_Path;
    m_Tokens = StringView(view).Split('/');

    if (m_Path == "/"_sv || m_Path.Empty())
    {
        m_State = ResolutionState::eFinished;

        if (flags & PathLookupFlags::eNegativeEntry
            && m_DirectoryEntry->Lookup(m_Path.BaseName()))
            return Error(EEXIST);
        return m_DirectoryEntry;
    }

    for (; m_Position < static_cast<isize>(m_Tokens.Size());)
    {
        auto segment     = GetNextSegment();
        m_CurrentSegment = segment;

        m_State          = ResolutionState::eTerminated;
        m_DirectoryEntry = FollowMounts().Value();

        if (segment.Type != PathSegmentType::eRegular)
        {
            FollowDots();
            if (m_State == ResolutionState::eFinished)
            {
                if (flags & PathLookupFlags::eNegativeEntry
                    && m_DirectoryEntry->Lookup(m_Path.BaseName()))
                    return Error(EEXIST);
                return m_DirectoryEntry;
            }
            continue;
        }

        auto didSucceed = Step();
        if (!didSucceed) return Error(didSucceed.Error());

        if (m_State == ResolutionState::eFinished) break;
    }

    if (m_State == ResolutionState::eTerminated)
    {
        m_DirectoryEntry = nullptr;
        if (errno == no_error) errno = ENOENT;
        return Error(errno);
    }

    if (flags & PathLookupFlags::eNegativeEntry && m_DirectoryEntry
        && m_DirectoryEntry->Lookup(m_Path.BaseName()))
        return Error(EEXIST);
    return m_DirectoryEntry;
}

ErrorOr<void> PathResolver::Step()
{
    m_Parent         = m_DirectoryEntry;
    m_DirectoryEntry = m_DirectoryEntry->Lookup(m_CurrentSegment.Name);
    m_BaseName       = Path(m_CurrentSegment.Name);
    if (!m_DirectoryEntry) return Terminate(ENOENT);

    if (m_CurrentSegment.IsLast)
    {
        if (m_Path[m_Path.Size() - 1] == '/'
            && !m_DirectoryEntry->IsDirectory())
            return Terminate(ENOENT);

        m_State = ResolutionState::eFinished;
        return {};
    }

    if (m_DirectoryEntry->IsSymlink())
    {
        auto result = FollowSymlinks();
        if (!result) return Terminate(ENOLINK);

        m_DirectoryEntry = result.Value();
    }

    if (!m_DirectoryEntry->IsDirectory()) return Terminate(ENOTDIR);
    return {};
}

ErrorOr<Ref<DirectoryEntry>>
PathResolver::FollowMounts(Ref<class DirectoryEntry> dentry)
{
    if (!dentry && m_DirectoryEntry) dentry = m_DirectoryEntry;
    else if (!dentry) return nullptr;

    return dentry->FollowMounts().Promote();
}

ErrorOr<Ref<DirectoryEntry>> PathResolver::FollowDown()
{
    auto segment     = GetNextSegment();
    m_CurrentSegment = segment;

    m_State          = ResolutionState::eTerminated;
    m_DirectoryEntry = FollowMounts().Value();

    if (segment.Type != PathSegmentType::eRegular)
    {
        FollowDots();
        if (m_State == ResolutionState::eFinished) return m_DirectoryEntry;

        return FollowDown();
    }

    auto didSucceed = Step();
    if (!didSucceed) return Error(didSucceed.Error());

    if (m_State == ResolutionState::eFinished) return m_DirectoryEntry;
    return Error(ENOENT);
}
ErrorOr<Ref<DirectoryEntry>> PathResolver::FollowSymlinks()
{
    auto current = m_DirectoryEntry->INode();
    while (current->IsSymlink())
    {
        auto result = FollowSymlink();
        if (!result) return Error(result.Error());

        m_DirectoryEntry = result.Value();
        current          = m_DirectoryEntry->INode();
    }

    if (!m_DirectoryEntry) return Error(ENOLINK);
    return m_DirectoryEntry;
}
ErrorOr<Ref<DirectoryEntry>> PathResolver::FollowSymlink()
{
    if (m_SymlinkDepth >= static_cast<isize>(SYMLOOP_MAX - 1))
        return Error(ELOOP);

    auto inode  = m_DirectoryEntry->INode();
    auto target = inode->GetTarget();
    if (target.Empty()) return m_DirectoryEntry;

    auto parent  = m_DirectoryEntry->Parent().Promote();

    auto pathRes = VFS::ResolvePath(parent, target, false);
    auto next    = pathRes ? pathRes.Value().Entry : nullptr;

    if (!next) return Error(ENOLINK);
    return next;
}
Ref<DirectoryEntry> PathResolver::FollowDots()
{
    if (m_CurrentSegment.Type == PathSegmentType::eDotDot)
        m_DirectoryEntry = m_DirectoryEntry->GetEffectiveParent().Promote();

    if (m_CurrentSegment.IsLast)
    {
        m_State          = ResolutionState::eFinished;
        m_Parent         = m_DirectoryEntry->GetEffectiveParent().Promote();
        m_DirectoryEntry = m_DirectoryEntry->FollowMounts().Promote();
        m_BaseName       = m_DirectoryEntry->Name();
    }
    else if (m_Position == static_cast<isize>(m_Tokens.Size() - 1))
        m_State = ResolutionState::eLast;

    return m_DirectoryEntry;
}
Error PathResolver::Terminate(ErrorCode code)
{
    m_State          = ResolutionState::eTerminated;
    m_DirectoryEntry = nullptr;
    errno            = code;

    return Error(code);
}

PathResolver::Segment PathResolver::GetNextSegment()
{
    Segment next;
    next.Name   = String(m_Tokens[m_Position].Raw());
    next.IsLast = m_Position == static_cast<isize>(m_Tokens.Size() - 1);
    next.Type   = PathSegmentType::eRegular;

    if (next.Name == ".") next.Type = PathSegmentType::eDot;
    else if (next.Name == "..") next.Type = PathSegmentType::eDotDot;

    ++m_Position;
    return next;
}

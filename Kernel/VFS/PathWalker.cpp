/*resolver.INode()
 * Created by v1tr10l7 on 20.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/PathWalker.hpp>

PathWalker::PathWalker(class Ref<::DirectoryEntry> const root, PathView path)
{
    auto result = Initialize(root, path);
    CtosUnused(result);
}

ErrorOr<void> PathWalker::Initialize(Ref<class ::DirectoryEntry> const root,
                                     PathView                    path)
{
    if (path.Empty()) return Terminate(EINVAL);
    m_Root           = root ?: VFS::GetRootDirectoryEntry();

    m_Path           = path;
    m_Tokens         = m_Path.SplitPath();
    m_Position       = 0;

    m_Parent         = path.Absolute() ? VFS::GetRootDirectoryEntry() : root;
    m_DirectoryEntry = m_Parent;
    m_BaseName       = "/"_sv;

    if (!root->IsDirectory()) return Terminate(ENOTDIR);

    if (m_Parent != VFS::GetRootDirectoryEntry())
    {
        auto result = FollowMounts(m_Parent);
        if (!result) return Error(result.error());

        m_Parent = result.value();
    }

    m_Tokens = StringView(path).Split('/');
    m_State  = ResolutionState::eInitialized;
    return {};
}

ErrorOr<DirectoryEntry*> PathWalker::Resolve(bool followLinks)
{
    Assert(m_State == ResolutionState::eInitialized);
    if (m_Path == "/"_sv || m_Path.Empty())
    {
        m_State = ResolutionState::eFinished;
        return m_DirectoryEntry.Raw();
    }

    for (; m_Position < static_cast<isize>(m_Tokens.Size());)
    {
        auto segment     = GetNextSegment();
        m_CurrentSegment = segment;

        m_State          = ResolutionState::eTerminated;
        m_DirectoryEntry = FollowMounts().value();

        if (segment.Type != PathSegmentType::eRegular)
        {
            FollowDots();
            if (m_State == ResolutionState::eFinished) return m_DirectoryEntry.Raw();

            continue;
        }

        auto didSucceed = Step();
        if (!didSucceed) return Error(didSucceed.error());

        if (m_State == ResolutionState::eFinished) break;
    }

    if (m_State == ResolutionState::eTerminated)
    {
        m_DirectoryEntry = nullptr;
        if (errno == no_error) errno = ENOENT;
        return Error(errno);
    }

    (void)followLinks;
    return m_DirectoryEntry.Raw();
}

ErrorOr<void> PathWalker::Step()
{
    m_Parent         = m_DirectoryEntry;
    m_DirectoryEntry = m_DirectoryEntry->Lookup(m_CurrentSegment.Name).Raw();
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

        m_DirectoryEntry = result.value();
    }

    if (!m_DirectoryEntry->IsDirectory()) return Terminate(ENOTDIR);
    return {};
}

ErrorOr<Ref<DirectoryEntry>> PathWalker::FollowMounts(Ref<class DirectoryEntry> dentry)
{
    if (!dentry && m_DirectoryEntry) dentry = m_DirectoryEntry;
    else if (!dentry) return nullptr;

    auto current = dentry;
    while (current->m_MountGate) current = current->m_MountGate;
    return current;
}
ErrorOr<DirectoryEntry*> PathWalker::FollowSymlinks()
{
    auto current = m_DirectoryEntry->INode();
    while (current->IsSymlink())
    {
        auto result = FollowSymlink();
        if (!result) return Error(result.error());

        m_DirectoryEntry = result.value();
        current          = m_DirectoryEntry->INode();
    }

    if (!m_DirectoryEntry) return Error(ENOLINK);
    return m_DirectoryEntry.Raw();
}
ErrorOr<DirectoryEntry*> PathWalker::FollowSymlink()
{
    if (m_SymlinkDepth >= static_cast<isize>(SYMLOOP_MAX - 1))
        return Error(ELOOP);

    auto inode  = m_DirectoryEntry->INode();
    auto target = inode->GetTarget();
    if (target.Empty()) return m_DirectoryEntry.Raw();

    auto parentDirectoryEntry = m_DirectoryEntry->Parent();

    auto pathRes = VFS::ResolvePath(parentDirectoryEntry.Raw(), target.Raw(), false);
    auto next    = pathRes ? pathRes.value().Entry : nullptr;

    if (!next) return Error(ENOLINK);
    return next.Raw();
}
DirectoryEntry* PathWalker::FollowDots()
{
    if (m_CurrentSegment.Type == PathSegmentType::eDotDot)
        m_DirectoryEntry = m_DirectoryEntry->GetEffectiveParent().Raw();

    if (m_CurrentSegment.IsLast)
    {
        m_State          = ResolutionState::eFinished;
        m_Parent         = m_DirectoryEntry->GetEffectiveParent().Raw();
        m_DirectoryEntry = m_DirectoryEntry->FollowMounts();
        m_BaseName       = m_DirectoryEntry->Name();
    }
    else if (m_Position == static_cast<isize>(m_Tokens.Size() - 1))
        m_State = ResolutionState::eLast;

    return m_DirectoryEntry.Raw();
}
Error PathWalker::Terminate(ErrorCode code)
{
    m_State          = ResolutionState::eTerminated;
    m_DirectoryEntry = nullptr;
    errno            = code;

    return Error(code);
}

PathWalker::Segment PathWalker::GetNextSegment()
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

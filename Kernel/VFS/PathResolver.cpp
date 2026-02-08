/*resolver.INode()
 * Created by v1tr10l7 on 20.06.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/MountPoint.hpp>
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
    if (path.Empty()) path = "/"_pv;
    m_Root           = root ?: VFS::RootDirectoryEntry();

    m_Path           = path;
    m_Tokens         = m_Path.Split();
    m_Position       = 0;

    m_Parent         = path.Absolute() ? VFS::RootDirectoryEntry() : root;
    m_DirectoryEntry = m_Parent;
    m_BaseName       = "/"_sv;

    if (m_Parent != VFS::RootDirectoryEntry())
        m_Parent = TryFollowMounts(m_Parent) ?: m_DirectoryEntry;

    m_State = ResolutionState::eInitialized;
    return {};
}

ErrorOr<Ref<DirectoryEntry>> PathResolver::Resolve(bool followLinks)
{
    PathLookupFlags flags
        = PathLookupFlags::eRegular | PathLookupFlags::eFollowMounts;
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
        if (segment.Type != PathSegmentType::eRegular)
        {
            m_DirectoryEntry = FollowDots(m_DirectoryEntry);
            continue;
        }

        auto dentry = TryOrRet(FollowDown(m_DirectoryEntry));
        if (m_State == ResolutionState::eFinished) break;
    }

    if (m_State == ResolutionState::eTerminated)
    {
        m_DirectoryEntry = nullptr;
        if (errno == no_error) errno = ENOENT;
        return Error(errno);
    }
    else if (m_State == ResolutionState::eFinished)
    {
        auto dentry = m_DirectoryEntry;
        if (dentry && dentry->IsMountPoint()
            && m_Flags & PathLookupFlags::eFollowMounts)
            dentry = TryFollowMounts(dentry);
        m_DirectoryEntry = dentry;
    }

    if (flags & PathLookupFlags::eNegativeEntry && m_DirectoryEntry
        && m_DirectoryEntry->Lookup(m_Path.BaseName()))
        return Error(EEXIST);

    m_DirectoryEntry = TryFollowMounts(m_DirectoryEntry);
    return m_DirectoryEntry;
}
ErrorOr<Ref<DirectoryEntry>>
PathResolver::LookupLastSegment(Ref<::DirectoryEntry> parent)
{
    if (ShouldFollowDots(parent)) parent = FollowDots(parent);
    StringView name             = m_CurrentSegment.Name;
    auto       child            = parent->Lookup(name);

    bool       shouldBeNegative = m_Flags & PathLookupFlags::eNegativeEntry;
    if (!child)
    {
        if (shouldBeNegative)
        {
            m_Parent         = parent;
            m_DirectoryEntry = child;
            return child;
        }
        else return Error(ENOENT);
    }
    else if (!shouldBeNegative) return Error(EEXIST);

    child = TryOrRet(TryFollowSymlinks(child));
    if (m_Flags & PathLookupFlags::eDirectory && !child->IsDirectory())
        return Error(ENOTDIR);

    m_Parent         = child->Parent().Promote();
    m_DirectoryEntry = child;
    return child;
}

ErrorOr<Ref<DirectoryEntry>>
PathResolver::FollowDown(Ref<::DirectoryEntry> dentry)
{
    String segmentName = m_CurrentSegment.Name;
    auto   child       = dentry->Lookup(segmentName);
    if (!child) return Terminate(ENOENT);
    child = TryFollowMounts(child);
    if (!child) return Terminate(ENOENT);

    if (ShouldFollowSymlink(child)) child = TryOrRet(TryFollowSymlinks(child));
    if (!m_CurrentSegment.IsLast && !child->IsDirectory())
        return Terminate(ENOTDIR);

    m_Parent         = m_DirectoryEntry;
    m_DirectoryEntry = child;

    if (m_CurrentSegment.IsLast)
    {
        if (m_Path[m_Path.Size() - 1] == '/' && !child->IsDirectory())
            return Terminate(ENOENT);

        m_State = ResolutionState::eFinished;
        return {};
    }

    return child;
}
ErrorOr<Ref<DirectoryEntry>>
PathResolver::FollowUp(Ref<::DirectoryEntry> dentry)
{
    auto parent = dentry->GetEffectiveParent().Promote();
    if (!parent) return dentry;

    return parent;
}
Ref<DirectoryEntry> PathResolver::FollowDots(Ref<::DirectoryEntry> dentry)
{
    if (m_CurrentSegment.Type == PathSegmentType::eDotDot)
    {
        dentry = TryOrRetVal(FollowUp(dentry), dentry);
        dentry = TryFollowMounts(dentry);
    }
    else if (m_CurrentSegment.Type != PathSegmentType::eDot) return dentry;

    if (m_CurrentSegment.IsLast)
    {
        m_State  = ResolutionState::eFinished;
        m_Parent = TryOrRetVal(FollowUp(dentry), nullptr);
        if (m_Flags & PathLookupFlags::eFollowMounts)
            dentry = TryFollowMounts(dentry);
        m_BaseName = dentry->Name();
    }
    else if (m_Position == static_cast<isize>(m_Tokens.Size() - 1))
        m_State = ResolutionState::eLast;

    return dentry;
}

Ref<DirectoryEntry>
PathResolver::TryFollowMounts(Ref<class DirectoryEntry> dentry)
{
    auto next = dentry;

    while (ShouldFollowMount(next))
    {
        dentry = next;
        next   = FollowMount(dentry);
    }

    return next ?: dentry;
}
Ref<DirectoryEntry> PathResolver::FollowMounts(Ref<class DirectoryEntry> dentry)
{
    while (ShouldFollowMount(dentry)) dentry = FollowMount(dentry);
    return dentry;
}
Ref<DirectoryEntry> PathResolver::FollowMount(Ref<class DirectoryEntry> dentry)
{
    if (!ShouldFollowMount(dentry)) return dentry;

    auto mountPoint = MountPoint::Lookup(dentry);
    return mountPoint ? mountPoint->GuestEntry() : nullptr;
}

ErrorOr<Ref<DirectoryEntry>>
PathResolver::TryFollowSymlinks(Ref<class DirectoryEntry> dentry)
{
    auto next = dentry;

    while (ShouldFollowSymlink(dentry))
    {
        dentry = next;
        next   = TryOrRet(FollowSymlink(dentry));
    }

    return next ?: dentry;
}
ErrorOr<Ref<DirectoryEntry>>
PathResolver::FollowSymlink(Ref<::DirectoryEntry> dentry)
{
    if (m_SymlinkDepth >= static_cast<isize>(SYMLOOP_MAX - 1))
        return Error(ELOOP);

    auto inode  = dentry->INode();
    auto target = TryOrRet(inode->ReadLink());
    if (target.Empty()) return dentry;

    auto parent  = dentry->Parent().Promote();

    auto pathRes = VFS::ResolvePath(parent, target, false);
    auto next    = pathRes ? pathRes.Value().Entry : nullptr;

    if (!next) return Error(ENOLINK);
    return next;
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
bool PathResolver::ShouldFollowMount(Ref<class DirectoryEntry> dentry)
{
    return dentry && dentry->IsMountPoint()
        && (!m_CurrentSegment.IsLast
            || m_Flags & PathLookupFlags::eFollowMounts);
}
bool PathResolver::ShouldFollowDots(Ref<class DirectoryEntry> dentry)
{
    return m_CurrentSegment.Type == PathSegmentType::eDotDot;
}
bool PathResolver::ShouldFollowSymlink(Ref<class DirectoryEntry> dentry)
{
    return dentry && dentry->IsSymlink()
        && (!m_CurrentSegment.IsLast
            || m_Flags & PathLookupFlags::eFollowLinks);
}

/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/UnorderedMap.hpp>
#include <Scheduler/Process.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/ProcFs/ProcFsINode.hpp>

class ProcFs : public Filesystem
{
  public:
    ProcFs(u32 flags)
        : Filesystem("ProcFs", flags)
    {
    }

    static Process*    GetProcess(pid_t pid);
    void               AddProcess(Process* process);
    void               RemoveProcess(pid_t pid);

    virtual StringView MountFlagsString() const override
    {
        return "rw,nosuid,nodev,noexec,relatime";
    }

    virtual ErrorOr<::Ref<DirectoryEntry>>
    Mount(StringView sourcePath, const void* data = nullptr) override;
    virtual ErrorOr<INode*> CreateNode(INode*                parent,
                                       ::Ref<DirectoryEntry> entry, mode_t mode,
                                       uid_t uid = 0, gid_t gid = 0) override;
    virtual ErrorOr<INode*> Symlink(INode* parent, ::Ref<DirectoryEntry> entry,
                                    StringView target) override;
    virtual INode*          Link(INode* parent, StringView name,
                                 INode* oldNode) override;
    virtual bool            Populate(DirectoryEntry* dentry) override;

  private:
    static UnorderedMap<pid_t, Process*>      s_Processes;

    UnorderedMap<StringView, ProcFsProperty*> m_Properties;

    void                                      AddChild(StringView name);
};

/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Scheduler/Process.hpp>

#include <VFS/Filesystem.hpp>
#include <VFS/ProcFs/ProcFsINode.hpp>

#include <unordered_map>

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

    virtual ErrorOr<INode*> Mount(INode* parent, INode* source, INode* target,
                                  DirectoryEntry* entry, StringView name,
                                  const void* data = nullptr) override;
    virtual ErrorOr<INode*> CreateNode(INode* parent, DirectoryEntry* entry,
                                       mode_t mode, uid_t uid = 0,
                                       gid_t gid = 0) override;
    virtual ErrorOr<INode*> Symlink(INode* parent, DirectoryEntry* entry,
                                    StringView target) override;
    virtual INode*          Link(INode* parent, StringView name,
                                 INode* oldNode) override;
    virtual bool            Populate(INode* node) override;

  private:
    static std::unordered_map<pid_t, Process*>      s_Processes;

    std::unordered_map<StringView, ProcFsProperty*> m_Properties;

    void                                            AddChild(StringView name);
};

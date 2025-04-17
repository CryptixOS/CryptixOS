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

    static Process*          GetProcess(pid_t pid);
    void                     AddProcess(Process* process);
    void                     RemoveProcess(pid_t pid);

    virtual std::string_view GetMountFlagsString() const override
    {
        return "rw,nosuid,nodev,noexec,relatime";
    }

    virtual INode* Mount(INode* parent, INode* source, INode* target,
                         std::string_view name,
                         const void*      data = nullptr) override;
    virtual INode* CreateNode(INode* parent, std::string_view name,
                              mode_t mode) override;
    virtual INode* Symlink(INode* parent, std::string_view name,
                           std::string_view target) override;
    virtual INode* Link(INode* parent, std::string_view name,
                        INode* oldNode) override;
    virtual bool   Populate(INode* node) override;

  private:
    static std::unordered_map<pid_t, Process*>            s_Processes;

    std::unordered_map<std::string_view, ProcFsProperty*> m_Properties;

    void AddChild(StringView name);
};

/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <vector>

using pid_t = i64;
using tid_t = i64;

enum class PrivilegeLevel
{
    ePrivileged,
    eUnprivileged,
};

struct Credentials
{
    uid_t uid;
    gid_t gid;
    uid_t euid;
    gid_t egid;
    uid_t suid;
    gid_t sgid;
    pid_t sid;
    gid_t pgid;
};

constexpr Credentials ROOT_CREDENTIALS = {
    .uid  = 0,
    .gid  = 0,
    .euid = 0,
    .egid = 0,
    .suid = 0,
    .sgid = 0,
    .sid  = 0,
    .pgid = 0,
};

struct Thread;

class Process
{
  public:
    Process() = default;
    Process(std::string_view name, PrivilegeLevel ring);
    Process(std::string_view name, pid_t pid);

    void InitializeStreams()
    {
        m_FileDescriptors.clear();
        INode* currentTTY
            = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), "/dev/tty0"));

        LogTrace("Process: Creating standard streams...");
        m_FileDescriptors.push_back(currentTTY->Open());
        m_FileDescriptors.push_back(currentTTY->Open());
        m_FileDescriptors.push_back(currentTTY->Open());
    }

    inline pid_t              GetPid() const { return m_Pid; }
    inline const Credentials& GetCredentials() const { return m_Credentials; }
    inline i32                CreateDescriptor(INode* node)
    {
        auto  descriptor = node->Open();

        usize fd         = m_FileDescriptors.size();
        m_FileDescriptors.push_back(descriptor);

        return fd;
    }
    inline FileDescriptor* GetFileHandle(i32 fd) const
    {
        if (fd >= static_cast<i32>(m_FileDescriptors.size())) return nullptr;

        return m_FileDescriptors[fd];
    }

    Process* Fork();
    i32      Exec(const char* path, char** argv, char** envp);
    i32      Exit(i32 code);

    friend struct Thread;
    pid_t                        m_Pid         = -1;
    std::string                  m_Name        = "?";
    PageMap*                     PageMap       = nullptr;
    std::atomic<tid_t>           m_NextTid     = 1;
    Credentials                  m_Credentials = {};
    PrivilegeLevel               m_Ring        = PrivilegeLevel::eUnprivileged;

    Process*                     m_Parent      = nullptr;
    std::vector<Process*>        m_Children;
    std::vector<Thread*>         m_Threads;

    std::vector<FileDescriptor*> m_FileDescriptors;
    std::vector<VMM::Region>     m_AddressSpace{};
    uintptr_t                    m_UserStackTop = 0x70000000000;
    usize                        m_Quantum      = 1000;
};

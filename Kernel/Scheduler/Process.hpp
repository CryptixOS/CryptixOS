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

#include <VFS/FileDescriptorTable.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <vector>

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
        m_FdTable.Clear();
        INode* currentTTY
            = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), "/dev/tty"));

        LogTrace("Process: Creating standard streams...");
        m_FdTable.Insert(currentTTY->Open(0, 0));
        m_FdTable.Insert(currentTTY->Open(0, 0));
        m_FdTable.Insert(currentTTY->Open(0, 0));
    }

    inline pid_t GetPid() const { return m_Pid; }
    inline pid_t GetParentPid() const
    {
        if (m_Parent) return m_Parent->m_Pid;

        // TODO(v1tr10l7): What should we return, if there is no parent??
        return 0;
    }
    inline const Credentials& GetCredentials() const { return m_Credentials; }
    i32        OpenAt(i32 dirFd, PathView path, i32 flags, mode_t mode);
    inline i32 CreateFileDescriptor(INode* node, i32 flags, mode_t mode)
    {
        auto descriptor = node->Open(flags, mode);

        return m_FdTable.Insert(descriptor);
    }
    i32         CloseFd(i32 fd);
    inline bool IsFdValid(i32 fd) const { return m_FdTable.IsValid(fd); }
    inline FileDescriptor* GetFileHandle(i32 fd) { return m_FdTable.GetFd(fd); }

    inline INode*          GetRootNode() const { return m_RootNode; }
    inline INode*          GetCWD() const { return m_CWD; }
    inline mode_t          GetUMask() const { return m_UMask; }

    inline const std::vector<Process*>& GetZombies() const { return m_Zombies; }

    Process*                            Fork();
    i32 Exec(const char* path, char** argv, char** envp);
    i32 Exit(i32 code);

    friend struct Thread;
    pid_t                    m_Pid         = -1;
    std::string              m_Name        = "?";
    PageMap*                 PageMap       = nullptr;
    std::atomic<tid_t>       m_NextTid     = 1;
    Credentials              m_Credentials = {};
    PrivilegeLevel           m_Ring        = PrivilegeLevel::eUnprivileged;

    INode*                   m_RootNode    = VFS::GetRootNode();
    INode*                   m_CWD         = VFS::GetRootNode();
    mode_t                   m_UMask       = 0;

    Process*                 m_Parent      = nullptr;
    std::vector<Process*>    m_Children;
    std::vector<Process*>    m_Zombies;
    std::vector<Thread*>     m_Threads;

    FileDescriptorTable      m_FdTable;
    std::vector<VMM::Region> m_AddressSpace{};
    uintptr_t                m_UserStackTop = 0x70000000000;
    usize                    m_Quantum      = 1000;
};

/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/Posix/signal.h>

#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <VFS/FileDescriptorTable.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

#include <expected>
#include <vector>

enum class PrivilegeLevel
{
    ePrivileged,
    eUnprivileged,
};

struct Credentials
{
    uid_t              uid;
    gid_t              gid;
    uid_t              euid;
    gid_t              egid;
    uid_t              suid;
    gid_t              sgid;
    pid_t              sid;
    gid_t              pgid;

    static Credentials s_Root;
};

struct Thread;

class Process
{
  public:
    Process() = default;
    Process(Process* parent, std::string_view name, const Credentials& creds);

    static Process* GetCurrent();
    static Process* CreateKernelProcess();
    static Process* CreateIdleProcess();

    bool            ValidateAddress(const Pointer address, i32 accessMode);

    inline pid_t    GetParentPid() const
    {
        if (m_Parent) return m_Parent->m_Pid;

        // TODO(v1tr10l7): What should we return, if there is no parent??
        return 0;
    }
    inline Process*           GetParent() const { return m_Parent; }
    inline pid_t              GetPid() const { return m_Pid; }
    inline std::string_view   GetName() const { return m_Name; }
    inline const Credentials& GetCredentials() const { return m_Credentials; }
    inline std::optional<i32> GetStatus() const { return m_Status; }

    inline pid_t              GetSid() const { return m_Credentials.sid; }
    inline pid_t              GetPGid() const { return m_Credentials.pgid; }

    pid_t                     SetSid();
    inline void               SetPGid(pid_t pgid) { m_Credentials.pgid = pgid; }

    inline const std::vector<Process*>& GetChildren() const
    {
        return m_Children;
    }
    inline const std::vector<Process*>& GetZombies() const { return m_Zombies; }
    inline const std::vector<Thread*>&  GetThreads() const { return m_Threads; }

    inline bool IsGroupLeader() const { return m_Pid == m_Credentials.pgid; }
    inline bool IsChild(Process* process) const
    {
        for (const auto& child : m_Children)
            if (process == child) return true;

        return false;
    }

    inline INode*   GetRootNode() const { return m_RootNode; }
    inline INode*   GetCWD() const { return m_CWD; }
    inline mode_t   GetUMask() const { return m_UMask; }

    inline sigset_t GetSignalMask() const { return m_SignalMask; }
    inline void     SetSignalMask(sigset_t mask) { m_SignalMask = mask; }

    void            SendSignal(i32 signal);

    ErrorOr<i32>    OpenAt(i32 dirFdNum, PathView path, i32 flags, mode_t mode);
    ErrorOr<i32>    DupFd(i32 oldFdNum, i32 newFdNum = -1, i32 flags = 0);
    i32             CloseFd(i32 fd);
    inline bool     IsFdValid(i32 fd) const { return m_FdTable.IsValid(fd); }
    inline FileDescriptor* GetFileHandle(i32 fd) { return m_FdTable.GetFd(fd); }

    ErrorOr<pid_t>         WaitPid(pid_t pid, i32* wstatus, i32 flags,
                                   struct rusage* rusage);

    Process*               Fork();
    i32                    Exec(const char* path, char** argv, char** envp);
    i32                    Exit(i32 code);

    friend struct Thread;
    Process*                 m_Parent      = nullptr;
    pid_t                    m_Pid         = -1;
    std::string              m_Name        = "?";
    PageMap*                 PageMap       = nullptr;
    Credentials              m_Credentials = {};
    PrivilegeLevel           m_Ring        = PrivilegeLevel::eUnprivileged;
    std::optional<i32>       m_Status;

    std::atomic<tid_t>       m_NextTid = m_Pid;
    std::vector<Process*>    m_Children;
    std::vector<Process*>    m_Zombies;
    std::vector<Thread*>     m_Threads;

    INode*                   m_RootNode = VFS::GetRootNode();
    INode*                   m_CWD      = VFS::GetRootNode();
    mode_t                   m_UMask    = 0;

    FileDescriptorTable      m_FdTable;
    sigset_t                 m_SignalMask = 0;
    std::vector<VMM::Region> m_AddressSpace{};
    uintptr_t                m_UserStackTop = 0x70000000000;
    usize                    m_Quantum      = 1000;
};

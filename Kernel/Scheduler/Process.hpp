/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/Posix/signal.h>

#include <Drivers/TTY.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <Library/ELF.hpp>
#include <Prism/String/String.hpp>

#include <VFS/FileDescriptorTable.hpp>

enum class PrivilegeLevel
{
    ePrivileged,
    eUnprivileged,
};

class INode;
struct Credentials
{
    uid_t              uid;
    gid_t              gid;
    uid_t              euid;
    gid_t              egid;
    uid_t              suid;
    gid_t              sgid;
    pid_t              sid;
    pid_t              pgid;

    static Credentials s_Root;
};

struct Thread;

class Process
{
  public:
    Process() = default;
    Process(Process* parent, StringView name, const Credentials& creds);

    static Process* GetCurrent();
    static Process* Current();
    static Process* CreateKernelProcess();
    static Process* CreateIdleProcess();

    Thread* CreateThread(Pointer rip, bool isUser = true, i64 runOn = -1);
    Thread* CreateThread(Pointer rip, Vector<StringView>& argv,
                         Vector<StringView>& envp, ELF::Image& program,
                         i64 runOn = -1);

    bool    ValidateAddress(const Pointer address, i32 accessMode, usize size);
    inline bool ValidateRead(const Pointer address, usize size)
    {
        return ValidateAddress(address, PROT_READ, size);
    }
    inline bool ValidateWrite(const Pointer address, usize size)
    {
        return ValidateAddress(address, PROT_WRITE, size);
    }

    template <typename T>
        requires(!IsPointerV<T>)
    inline bool ValidateRead(const T* address)
    {
        return ValidateRead(address, sizeof(T));
    }
    template <typename T>
        requires(!IsPointerV<T>)
    inline bool ValidateWrite(const T* address)
    {
        return ValidateWrite(address, sizeof(T));
    }

    inline pid_t ParentPid() const
    {
        if (m_Parent) return m_Parent->m_Pid;

        // TODO(v1tr10l7): What should we return, if there is no parent??
        return 0;
    }
    inline Process*           Parent() const { return m_Parent; }
    inline pid_t              Pid() const { return m_Pid; }
    inline StringView         Name() const { return m_Name; }
    inline const Credentials& Credentials() const { return m_Credentials; }
    inline Optional<i32>      Status() const { return m_Status; }

    inline Thread*            MainThread() { return m_MainThread; }
    inline AddressSpace&      AddressSpace() { return m_AddressSpace; }

    inline pid_t              Sid() const { return m_Credentials.sid; }
    inline pid_t              PGid() const { return m_Credentials.pgid; }

    pid_t                     SetSid();
    inline void               SetPGid(pid_t pgid) { m_Credentials.pgid = pgid; }

    inline TTY*               TTY() const { return m_TTY; }
    inline void               SetTTY(class TTY* tty) { m_TTY = tty; }

    inline const Vector<Process*>& Children() const { return m_Children; }
    inline const Vector<Process*>& Zombies() const { return m_Zombies; }
    inline const Vector<Thread*>&  Threads() const { return m_Threads; }

    inline bool IsSessionLeader() const { return m_Pid == m_Credentials.sid; }
    inline bool IsGroupLeader() const { return m_Pid == m_Credentials.pgid; }
    inline bool IsChild(Process* process) const
    {
        for (const auto& child : m_Children)
            if (process == child) return true;

        return false;
    }

    inline Ref<DirectoryEntry> RootNode() const { return m_RootDirectoryEntry; }
    inline StringView      CWD() const { return m_CWD; }
    inline void            SetCWD(StringView cwd) { m_CWD = cwd; }
    inline mode_t          Umask() const { return m_Umask; }
    mode_t                 Umask(mode_t mask);

    static void            SendGroupSignal(pid_t pgid, i32 signal);
    void                   SendSignal(i32 signal);

    ErrorOr<i32>   OpenAt(i32 dirFdNum, PathView path, i32 flags, mode_t mode);
    ErrorOr<isize> DupFd(isize oldFdNum, isize newFdNum, isize flags);
    i32            CloseFd(i32 fd);
    ErrorOr<isize> OpenPipe(i32* pipeFds);
    inline bool    IsFdValid(i32 fd) const { return m_FdTable.IsValid(fd); }
    inline FileDescriptor* GetFileHandle(i32 fd) { return m_FdTable.GetFd(fd); }

    ErrorOr<pid_t>         WaitPid(pid_t pid, i32* wstatus, i32 flags,
                                   struct rusage* rusage);

    ErrorOr<Process*>      Fork();
    ErrorOr<i32>           Exec(String path, char** argv, char** envp);
    i32                    Exit(i32 code);

    friend struct Thread;

    PageMap* PageMap = nullptr;

  private:
    Process*            m_Parent      = nullptr;
    pid_t               m_Pid         = -1;
    String              m_Name        = "?";

    struct Credentials  m_Credentials = {};
    class TTY*          m_TTY;
    PrivilegeLevel      m_Ring = PrivilegeLevel::eUnprivileged;
    Optional<i32>       m_Status;
    bool                m_Exited     = false;

    Thread*             m_MainThread = nullptr;
    Atomic<tid_t>       m_NextTid    = m_Pid;
    Vector<Process*>    m_Children;
    Vector<Process*>    m_Zombies;
    Vector<Thread*>     m_Threads;

    Ref<DirectoryEntry>     m_RootDirectoryEntry = nullptr;
    String              m_CWD                = "/";
    mode_t              m_Umask              = 0;

    FileDescriptorTable m_FdTable;
    class AddressSpace  m_AddressSpace;

    Pointer             m_UserStackTop = 0x70000000000u;
    usize               m_Quantum      = 1000;
    Spinlock            m_Lock;
    Event               m_Event;

    friend class Scheduler;
    friend struct Thread;
};

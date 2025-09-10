/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

#include <API/Credentials.hpp>
#include <API/Signals.hpp>
#include <Drivers/TTY.hpp>

#include <Memory/AddressSpace.hpp>
#include <Memory/Region.hpp>
#include <Memory/VMM.hpp>

#include <Library/ExecutableProgram.hpp>
#include <Library/Locking/SpinlockProtected.hpp>
#include <Prism/String/String.hpp>

#include <Scheduler/Thread.hpp>
#include <VFS/FileDescriptorTable.hpp>

enum class PrivilegeLevel
{
    ePrivileged,
    eUnprivileged,
};
enum class ProcessState
{
    eIdle    = 0,
    eRunning = 1,
    eDead    = 2,
};

class INode;
class Process
{
  public:
    Process() = default;
    Process(Process* parent, StringView name, const Credentials& creds);
    ~Process();

    static Process* GetCurrent();
    static Process* Current();
    static Process* CreateKernelProcess();
    static Process* CreateIdleProcess();

    using Iterator = Delegate<IterationResult(Process*)>;
    static void ForEach(Iterator it);
    static void ForEachInGroup(ProcessID pgid, Iterator it);

    Ref<Thread> CreateThread(Pointer rip, bool isUser = true, i64 runOn = -1);
    Ref<Thread> CreateThread(Vector<StringView>& argv, Vector<StringView>& envp,
                             ExecutableProgram& program, i64 runOn = -1);

    bool ValidateAddress(const Pointer address, i32 accessMode, usize size);
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

    inline ProcessID ParentID() const
    {
        if (m_Parent) return m_Parent->ID();

        // TODO(v1tr10l7): What should we return, if there is no parent??
        return 0;
    }
    inline Process*     Parent() const { return m_Parent; }
    inline ProcessID    ID() const { return m_ID; }
    inline StringView   Name() const { return m_Name; }
    inline ProcessState State() const { return m_State; }
    inline bool    IsDead() const { return m_State == ProcessState::eDead; }

    constexpr bool IsSuperUser() const
    {
        return m_Credentials.EffectiveUserID == 0;
    }
    inline const Credentials& Credentials() const { return m_Credentials; }
    inline Optional<i32>      Status() const { return m_Status; }

    inline Ref<Thread>        MainThread() { return m_MainThread; }
    inline AddressSpace&      AddressSpace() { return m_AddressSpace; }

    inline ProcessID          Sid() const { return m_Credentials.SessionID; }
    inline ProcessID PGid() const { return m_Credentials.ProcessGroupID; }

    inline void      SetUID(UserID uid) { m_Credentials.EffectiveUserID = uid; }
    inline void    SetGID(GroupID gid) { m_Credentials.EffectiveGroupID = gid; }

    ErrorOr<isize> SetReUID(UserID ruid, UserID euid);
    ErrorOr<isize> SetReGID(GroupID rgid, GroupID egid);
    ErrorOr<isize> SetResUID(UserID ruid, UserID euid, UserID suid);
    ErrorOr<isize> SetResGID(GroupID rgid, GroupID egid, GroupID sgid);

    ProcessID      SetSid();
    inline void SetPGid(ProcessID pgid) { m_Credentials.ProcessGroupID = pgid; }

    inline TTY* TTY() const { return m_TTY; }
    inline void SetTTY(class TTY* tty) { m_TTY = tty; }

    inline const Vector<Process*>&    Children() const { return m_Children; }
    inline const Vector<Process*>&    Zombies() const { return m_Zombies; }
    inline const Vector<Ref<Thread>>& Threads() const { return m_Threads; }

    inline bool                       IsSessionLeader() const
    {
        return m_ID == m_Credentials.SessionID;
    }
    inline bool IsGroupLeader() const
    {
        return m_ID == m_Credentials.ProcessGroupID;
    }
    inline bool IsChild(Process* process) const
    {
        for (const auto& child : m_Children)
            if (process == child) return true;

        return false;
    }

    inline Ref<DirectoryEntry> RootNode() const { return m_RootDirectoryEntry; }
    inline Ref<DirectoryEntry> CWD() const { return m_CWD; }
    inline void                SetCWD(Ref<DirectoryEntry> cwd) { m_CWD = cwd; }
    inline mode_t              Umask() const { return m_Umask; }
    mode_t                     Umask(mode_t mask);

    inline Timestep            Quantum() const { return m_Quantum; }
    const struct SignalAction& SignalAction(SignalID signal) const;
    void SetSignalAction(SignalID signal, const struct SignalAction& action);

    static void    SendGroupSignal(ProcessID pgid, i32 signal);
    void           SendSignal(i32 signal);

    ErrorOr<isize> OpenAt(i32 dirFdNum, PathView path, i32 flags, mode_t mode);
    ErrorOr<isize> DupFd(isize oldFdNum, isize newFdNum, isize flags);
    i32            CloseFd(i32 fd);
    ErrorOr<isize> InsertFd(Ref<FileDescriptor> fd);
    ErrorOr<isize> OpenPipe(i32* pipeFds);
    inline bool    IsFdValid(i32 fd) const { return m_FdTable.IsValid(fd); }
    ErrorOr<Ref<FileDescriptor>> GetFileDescriptor(isize fdNum);
    inline Ref<FileDescriptor>   GetFileHandle(i32 fd)
    {
        return m_FdTable.GetFd(fd);
    }

    ErrorOr<ProcessID> WaitPid(ProcessID pid, i32* wstatus, i32 flags,
                               struct rusage* rusage);

    ErrorOr<Process*>  Clone(usize cloneFlags);
    ErrorOr<i32>       Exec(String path, char** argv, char** envp);
    i32                Exit(i32 code);

    ErrorOr<void>      WaitForFutex(i32* vaddr, i32 expected);
    ErrorOr<void>      WakeFutex(i32* vaddr);

    friend struct Thread;
    using List       = IntrusiveList<Process>;

    PageMap* PageMap = nullptr;

  private:
    Process*            m_Parent      = nullptr;
    ProcessID           m_ID          = -1;
    String              m_Name        = "?";
    ProcessState        m_State       = ProcessState::eRunning;

    struct Credentials  m_Credentials = {};
    class TTY*          m_TTY         = nullptr;
    PrivilegeLevel      m_Ring        = PrivilegeLevel::eUnprivileged;
    Optional<i32>       m_Status;
    bool                m_Exited     = false;

    Ref<Thread>         m_MainThread = nullptr;
    Atomic<ThreadID>    m_NextTid    = m_ID;
    Vector<Process*>    m_Children;
    Vector<Process*>    m_Zombies;
    Vector<Ref<Thread>> m_Threads;

    Ref<DirectoryEntry> m_RootDirectoryEntry = nullptr;
    Ref<DirectoryEntry> m_CWD                = nullptr;
    mode_t              m_Umask              = 0;

    FileDescriptorTable m_FdTable;
    class AddressSpace  m_AddressSpace;

    Pointer             m_UserStackTop = 0x70000000000u;
    usize               m_Quantum      = 1'000;
    Spinlock            m_Lock;
    Event               m_Event;

    SpinlockProtected<
        Array<struct SignalAction, ToUnderlying(SignalID::eCount)>>
                                   m_SignalActions;
    SignalFlags                    m_SignalFlags          = SignalFlags::eNone;
    Pointer                        m_SignalTrampolineVirt = nullptr;

    UnorderedMap<upointer, Event*> m_FutexEvents{};

    void                           CopyFileDescriptors(Process* dest);
    void                           CopyMemory(Process* process);

    void                           SetupSignalTrampoline();

    friend class Scheduler;
    friend struct Thread;

    friend class IntrusiveList<Process>;
    friend struct IntrusiveListHook<Process>;

    IntrusiveListHook<Process> Hook;
};

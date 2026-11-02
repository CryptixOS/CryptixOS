/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/Posix/linux/capability.h>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <API/Posix/sys/resource.h>

export module Cryptix:Scheduler.Process;

export import :Debug.Assertions;
export import :Library.Logger;
export import :Library.Stacktrace;
export import :API.Credentials;
export import :API.Signals;
export import :Drivers.TTY.TTY;
export import :Memory.AddressSpace;
export import :Memory.Region;
export import :Memory.VMM;
export import :Library.ExecutableProgram;
export import :Library.Locking.SpinlockProtected;
export import :Scheduler.Thread;
export import :Time.Timer;
export import :VFS.FileDescriptorTable;
export import :VFS.FilesystemView;

export enum class PrivilegeLevel
{
    ePrivileged,
    eUnprivileged,
};
export enum class ProcessState
{
    eIdle    = 0,
    eRunning = 1,
    eDead    = 2,
    eZombie  = 3,
};

export class INode;
export class Process
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
    inline Process*      Parent() const { return m_Parent; }
    inline ProcessID     ID() const { return m_ID; }
    inline StringView    Name() const { return m_Name; }
    inline ProcessState  State() const { return m_State; }
    inline Optional<i32> Status() const { return m_Status; }
    inline bool        IsDead() const { return m_State == ProcessState::eDead; }

    inline Ref<Thread> MainThread() { return m_MainThread; }
    inline AddressSpace& AddressSpace() { return *m_AddressSpace; }

    constexpr bool       IsSuperUser() const
    {
        return m_Credentials.EffectiveUserID == 0;
    }
    inline const Credentials& Credentials() const { return m_Credentials; }
    inline constexpr bool     IsCapable(Capability capability) const
    {
        return m_Credentials.Capable(capability);
    }

    inline ::UserID UserID() const { return m_Credentials.UserID; }
    inline ::UserID EffectiveUserID() const
    {
        return m_Credentials.EffectiveUserID;
    }
    inline ::GroupID GroupID() const { return m_Credentials.GroupID; }
    inline ::GroupID EffectiveGroupID() const
    {
        return m_Credentials.EffectiveGroupID;
    }

    inline ProcessID Sid() const { return m_Credentials.SessionID; }
    inline ProcessID PGid() const { return m_Credentials.ProcessGroupID; }

    ErrorOr<void>    SetUserID(::UserID uid);
    ErrorOr<void>    SetGroupID(::GroupID gid);

    ErrorOr<isize>   SetReUID(::UserID ruid, ::UserID euid);
    ErrorOr<isize>   SetReGID(::GroupID rgid, ::GroupID egid);
    ErrorOr<isize>   SetResUID(::UserID ruid, ::UserID euid, ::UserID suid);
    ErrorOr<isize>   SetResGID(::GroupID rgid, ::GroupID egid, ::GroupID sgid);

    ProcessID        SetSid();
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

    inline Ref<DirectoryEntry> RootNode() const { return m_FsView->Root(); }
    inline Ref<DirectoryEntry> CWD() const
    {
        return m_FsView->WorkingDirectory();
    }
    inline void SetCWD(Ref<DirectoryEntry> cwd)
    {
        m_FsView->ChangeDirectory(cwd);
    }
    inline INodeMode Umask() const { return m_FsView->FileCreationMask(); }
    INodeMode        Umask(INodeMode mask);

    inline Timestep  Quantum() const { return m_Quantum; }
    inline Ref<struct Timer> Timer(usize index)
    {
        auto& timer = m_Timers[index];
        if (!timer) timer = CreateRef<struct Timer>();

        return timer;
    }

    const struct SignalAction& SignalAction(SignalID signal) const;
    void SetSignalAction(SignalID signal, const struct SignalAction& action);

    static void          SendGroupSignal(ProcessID pgid, i32 signal);
    void                 SendSignal(i32 signal);

    FileDescriptorTable& FdTable() { return *m_FdTable; }
    ErrorOr<isize>       OpenAt(i32 dirFdNum, PathView path, i32 flags,
                                INodeMode mode);
    isize                FirstFreeFdIndex();
    ErrorOr<isize>       DupFd(isize oldFdNum, isize newFdNum, isize flags);
    i32                  CloseFd(i32 fd);
    ErrorOr<isize>       InsertFd(Ref<::FileDescriptor> fd);
    ErrorOr<isize>       OpenPipe(i32* pipeFds, isize flags = 0);
    inline bool IsFdValid(i32 fd) const { return m_FdTable->IsValid(fd); }
    ErrorOr<Ref<::FileDescriptor>> GetFileDescriptor(isize fdNum);
    ErrorOr<Ref<::FileDescriptor>> FileDescriptor(isize fdNum);
    inline Ref<::FileDescriptor>   GetFileHandle(i32 fd)
    {
        return m_FdTable->GetFd(fd);
    }

    ErrorOr<ProcessID> WaitPid(ProcessID pid, i32* wstatus, i32 flags,
                               rusage* rusage);

    ErrorOr<Process*>  Clone(usize cloneFlags);
    ErrorOr<i32>       Exec(String path, char** argv, char** envp);
    i32                Exit(i32 code);

    ErrorOr<void>      WaitForFutex(i32* vaddr, i32 expected);
    ErrorOr<void>      WakeFutex(i32* vaddr);

    friend struct Thread;
    using List       = IntrusiveList<Process>;

    PageMap* PageMap = nullptr;

  private:
    Process*                         m_Parent      = nullptr;
    ProcessID                        m_ID          = -1;
    String                           m_Name        = "?";
    ProcessState                     m_State       = ProcessState::eRunning;

    struct Credentials               m_Credentials = {};
    class TTY*                       m_TTY         = nullptr;
    PrivilegeLevel                   m_Ring = PrivilegeLevel::eUnprivileged;
    Optional<i32>                    m_Status;
    bool                             m_Exited     = false;

    Ref<Thread>                      m_MainThread = nullptr;
    Atomic<ThreadID>                 m_NextTid    = m_ID;
    Vector<Process*>                 m_Children;
    Vector<Process*>                 m_Zombies;
    Vector<Ref<Thread>>              m_Threads;
    Array<Ref<struct Timer>, 3>      m_Timers;

    ::Ref<FilesystemView>            m_FsView       = nullptr;
    ::Ref<class FileDescriptorTable> m_FdTable      = nullptr;
    ::Ref<class AddressSpace>        m_AddressSpace = nullptr;

    Pointer                          m_UserStackTop = 0x70000000000u;
    usize                            m_Quantum      = 1'000;
    Spinlock                         m_Lock;
    Event                            m_Event;

    SpinlockProtected<
        Array<struct SignalAction, ToUnderlying(SignalID::eCount)>>
                                   m_SignalActions;
    SignalFlags                    m_SignalFlags          = SignalFlags::eNone;
    Pointer                        m_SignalTrampolineVirt = nullptr;

    UnorderedMap<upointer, Event*> m_FutexEvents{};

    void                           CopyFs(Process* dest);
    void                           CopyFileDescriptors(Process* dest);
    ErrorOr<void>                  CopyMemory(Process* process);

    void                           ReapChild(Process* child);

    friend class Scheduler;
    friend struct Thread;

    friend class IntrusiveList<Process>;
    friend struct IntrusiveListHook<Process>;

    IntrusiveListHook<Process> Hook;
};

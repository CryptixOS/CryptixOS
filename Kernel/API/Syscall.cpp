/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Syscall.hpp"

#include "API/MM.hpp"
#include "API/Process.hpp"
#include "API/VFS.hpp"
#include "Arch/CPU.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Thread.hpp"

#include <array>
#include <cerrno>
#include <magic_enum/magic_enum.hpp>

namespace Syscall
{
    void Initialize();

    struct Syscall
    {
        std::string                          name;
        std::function<uintptr_t(Arguments&)> handler;

        inline uintptr_t                     operator()(Arguments& args)
        {
            if (handler.operator bool()) return handler(args);

            return 0;
        }
        inline operator bool() { return handler.operator bool(); }
    };
    static std::array<Syscall, 512> syscalls;

    void                            RegisterHandler(usize                                     index,
                                                    std::function<uintptr_t(Arguments& args)> handler,
                                                    std::string                               name)
    {
        syscalls[index] = {name, handler};
    }

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004
    static uintptr_t SysArchPrCtl(Arguments& args)
    {
        auto      thread = CPU::GetCurrentThread();
        i32       op     = args.args[0];
        uintptr_t addr   = args.args[1];

        switch (op)
        {
            case ARCH_SET_GS:
                thread->gsBase = addr;
                CPU::SetKernelGSBase(thread->gsBase);
                break;
            case ARCH_SET_FS:
                thread->fsBase = addr;
                CPU::SetFSBase(thread->fsBase);
                break;
            case ARCH_GET_FS:
                *reinterpret_cast<uintptr_t*>(addr) = thread->fsBase;
                break;
            case ARCH_GET_GS:
                *reinterpret_cast<uintptr_t*>(addr) = thread->gsBase;
                break;

            default: thread->error = EINVAL; return -1;
        }

        return 0;
    }

    void InstallAll()
    {
        auto sysPanic = [](Arguments& args) -> uintptr_t
        {
            const char* errorMessage
                = reinterpret_cast<const char*>(args.args[0]);

            Panic("SYS_PANIC: {}", errorMessage);
        };

        Initialize();
        RegisterSyscall(ID::eRead, VFS::SysRead);
        RegisterSyscall(ID::eWrite, VFS::SysWrite);
        RegisterSyscall(ID::eOpen, VFS::SysOpen);
        RegisterSyscall(ID::eLSeek, VFS::SysLSeek);
        RegisterSyscall(ID::eMMap, MM::SysMMap);
        RegisterSyscall(ID::eIoCtl, VFS::SysIoCtl);
        RegisterSyscall(ID::eExit, Process::SysExit);
        RegisterSyscall(ID::eArchPrCtl, SysArchPrCtl);
        RegisterSyscall(ID::ePanic, sysPanic);
        // RegisterSyscall(ID::eGetTid, Process::SysGetTid);
    }
    void Handle(Arguments& args)
    {
        if (args.index >= 512 || !syscalls[args.index])
        {
            args.returnValue = -1;
            errno            = ENOSYS;
            LogError("Undefined syscall: {}", args.index);
            return;
        }

        args.returnValue = syscalls[args.index](args);
    }
} // namespace Syscall

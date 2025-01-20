/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/sys/mman.h>
#include <API/UnixTypes.hpp>
#include <API/VFS.hpp>

#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Utility/Path.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

namespace Syscall::VFS
{
    using Syscall::Arguments;
    using namespace ::VFS;

    std::expected<isize, std::errno_t> SysRead(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        void*    buffer  = reinterpret_cast<void*>(args.Args[1]);
        usize    bytes   = static_cast<usize>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        if (!buffer
            || !current->ValidateAddress(buffer, PROT_READ | PROT_WRITE))
            return std::errno_t(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        return fd->Read(buffer, bytes);
    }
    std::expected<isize, std::errno_t> SysWrite(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        void*    data    = reinterpret_cast<void*>(args.Args[1]);
        usize    bytes   = static_cast<usize>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        if (!data || current->ValidateAddress(data, PROT_READ | PROT_WRITE))
            return std::errno_t(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return std::errno_t(EBADF);

        return fd->Write(data, bytes);
    }

    std::expected<i32, std::errno_t> SysOpen(Arguments& args)
    {
        Process*  current = CPU::GetCurrentThread()->parent;

        uintptr_t path    = args.Args[0];
        i32       flags   = static_cast<i32>(args.Args[1]);
        mode_t    mode    = static_cast<mode_t>(args.Args[2]);

        if (!path || !current->ValidateAddress(path, PROT_READ))
            return std::errno_t(EFAULT);

        return current->OpenAt(AT_FDCWD, reinterpret_cast<const char*>(path),
                               flags, mode);
    }
    std::expected<i32, std::errno_t> SysClose(Arguments& args)
    {
        Process* current = CPU::GetCurrentThread()->parent;

        i32      fd      = static_cast<i32>(args.Args[0]);
        return current->CloseFd(fd);
    }
    std::expected<i32, std::errno_t> SysStat(Arguments& args)
    {
        args.Args[3] = args.Args[1];
        args.Args[2] = 0;
        args.Args[1] = args.Args[0];
        args.Args[0] = AT_FDCWD;

        return SysFStatAt(args);
    }
    std::expected<i32, std::errno_t> SysFStat(Arguments& args)
    {
        args.Args[3] = args.Args[1];
        args.Args[2] = AT_EMPTY_PATH;
        args.Args[1] = 0;

        return SysFStatAt(args);
    }
    std::expected<i32, std::errno_t> SysLStat(Arguments& args)
    {
        args.Args[3] = args.Args[1];
        args.Args[2] = AT_SYMLINK_NOFOLLOW;
        args.Args[1] = args.Args[0];
        args.Args[0] = AT_FDCWD;

        return SysFStatAt(args);
    }

    std::expected<off_t, std::errno_t> SysLSeek(Arguments& args)
    {
        i32      fd      = static_cast<i32>(args.Args[0]);
        off_t    offset  = static_cast<off_t>(args.Args[1]);
        i32      whence  = static_cast<i32>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        auto     file    = current->GetFileHandle(fd);
        if (!file) return_err(-1, ENOENT);

        return file->Seek(whence, offset);
    }

    std::expected<i32, std::errno_t> SysIoCtl(Arguments& args)
    {
        i32      fd      = static_cast<i32>(args.Args[0]);
        usize    request = static_cast<usize>(args.Args[1]);
        usize    arg     = static_cast<usize>(args.Args[2]);

        Process* current = CPU::GetCurrentThread()->parent;
        auto     file    = current->GetFileHandle(fd);
        if (!file) return_err(-1, EBADF);

        return file->GetNode()->IoCtl(request, arg);
    }
    std::expected<i32, std::errno_t> SysAccess(Arguments& args)
    {
        const char* path = reinterpret_cast<const char*>(args.Args[0]);
        i32         mode = static_cast<i32>(args.Args[1]);
        (void)mode;

        INode* node = std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), path));
        if (!node) return -1;

        return 0;
    }

    std::expected<i32, std::errno_t> SysDup(Arguments& args)
    {
        i32 oldFd = static_cast<i32>(args.Args[0]);

        LogTrace("Syscall::VFS::SysDup: {{ oldFd: {} }}", oldFd);
        Process* current = CPU::GetCurrentThread()->parent;

        auto     ret     = current->DupFd(oldFd, -1);
        if (!ret) return ret.error();
        auto desc = current->GetFileHandle(ret.value());
        if (!desc) Panic("Failed to duplicate fd");

        return ret;
    }
    std::expected<i32, std::errno_t> SysDup2(Syscall::Arguments& args)
    {
        i32 oldFd = static_cast<i32>(args.Args[0]);
        i32 newFd = static_cast<i32>(args.Args[1]);

        LogTrace("Syscall::VFS::SysDup2: {{ oldFd: {}, newFd: {} }}", oldFd,
                 newFd);
        Process* current = CPU::GetCurrentThread()->parent;

        auto     ret     = current->DupFd(oldFd, newFd);
        if (!ret) return ret.error();
        auto desc = current->GetFileHandle(newFd);
        if (!desc) Panic("Failed to duplicate fd, newFd: {}", ret.value());

        return ret.value();
    }
    std::expected<i32, std::errno_t> SysFcntl(Arguments& args)
    {
        i32             fdNum   = static_cast<i32>(args.Args[0]);
        i32             op      = static_cast<i32>(args.Args[1]);
        uintptr_t       arg     = reinterpret_cast<uintptr_t>(args.Args[2]);

        Process*        current = CPU::GetCurrentThread()->parent;
        FileDescriptor* fd      = current->GetFileHandle(fdNum);
        if (!fd) return_err(-1, EBADF);

        switch (op)
        {
            case F_DUPFD: return current->DupFd(fdNum, arg);
            case F_DUPFD_CLOEXEC: return current->DupFd(fdNum, arg, O_CLOEXEC);
            case F_GETFD: return fd->GetFlags();
            case F_SETFD:
                if (!fd) return_err(-1, EBADF);
                fd->SetFlags(arg);
                break;
            case F_GETFL: return fd->GetDescriptionFlags();
            case F_SETFL:
                if (arg & O_ACCMODE) return_err(-1, EINVAL);
                fd->SetDescriptionFlags(arg);
                break;

            default:
                LogError("Syscall::VFS::SysFcntl: Unknown opcode");
                return_err(-1, EINVAL);
        }

        return 0;
    }
    std::expected<i32, std::errno_t> SysGetCwd(Arguments& args)
    {
        class Process* current = CPU::GetCurrentThread()->parent;

        char*          buffer  = reinterpret_cast<char*>(args.Args[0]);
        usize          size    = args.Args[1];

        auto           cwd     = current->GetCWD()->GetPath();
        usize          count   = cwd.size();
        if (size < count) count = size;
        strncpy(buffer, cwd.data(), count);

        return 0;
    }
    std::expected<i32, std::errno_t> SysChDir(Arguments& args)
    {
        const char* path    = reinterpret_cast<const char*>(args.Args[0]);
        Process*    current = CPU::GetCurrentThread()->parent;

        INode* node = std::get<1>(VFS::ResolvePath(current->GetCWD(), path));
        if (!node) return_err(-1, ENOENT);
        if (!node->IsDirectory()) return_err(-1, ENOTDIR);

        current->m_CWD = node;
        return 0;
    }
    std::expected<i32, std::errno_t> SysFChDir(Arguments& args)
    {
        i32             fd             = static_cast<i32>(args.Args[0]);
        Process*        current        = CPU::GetCurrentThread()->parent;

        FileDescriptor* fileDescriptor = current->GetFileHandle(fd);
        if (!fileDescriptor) return_err(-1, EBADF);

        INode* node = fileDescriptor->GetNode();
        if (!node) return_err(-1, ENOENT);

        if (!node->IsDirectory()) return_err(-1, ENOTDIR);
        current->m_CWD = node;

        return 0;
    }

    [[clang::no_sanitize("alignment")]]
    std::expected<i32, std::errno_t> SysGetDents64(Arguments& args)
    {
        u32           fd        = static_cast<u32>(args.Args[0]);
        dirent* const outBuffer = reinterpret_cast<dirent* const>(args.Args[1]);
        u32           count     = static_cast<u32>(args.Args[2]);

        if (!outBuffer) return_err(-1, EFAULT);

        Process*        current        = CPU::GetCurrentThread()->parent;
        FileDescriptor* fileDescriptor = current->GetFileHandle(fd);
        if (!fileDescriptor) return_err(-1, EBADF);
        INode* node = fileDescriptor->GetNode();
        if (!node->IsDirectory()) return_err(-1, ENOTDIR);

        if (fileDescriptor->GetDirEntries().empty())
            fileDescriptor->GenerateDirEntries();

        if (fileDescriptor->GetDirEntries().empty()) return 0;

        usize length = 0;
        for (const auto& entry : fileDescriptor->GetDirEntries())
            length += entry->d_reclen;

        length = std::min(static_cast<usize>(count), length);

        if (fileDescriptor->GetDirEntries().front()->d_reclen > count)
            return_err(-1, EINVAL);

        bool end                       = fileDescriptor->DirentsInvalid;
        fileDescriptor->DirentsInvalid = false;

        usize i                        = 0;
        usize bytes                    = 0;
        while (i < length)
        {
            auto entry = fileDescriptor->GetDirEntries().pop_front_element();
            std::memcpy(reinterpret_cast<char*>(outBuffer) + i, entry,
                        entry->d_reclen);
            bytes = i;
            i += entry->d_reclen;
            free(entry);
        }

        if (fileDescriptor->GetDirEntries().empty())
            fileDescriptor->DirentsInvalid = true;

        return end ? 0 : bytes;
    }

    std::expected<i32, std::errno_t> SysOpenAt(Arguments& args)
    {
        Process* current = CPU::GetCurrentThread()->parent;

        i32      dirFd   = static_cast<i32>(args.Args[0]);
        PathView path    = reinterpret_cast<const char*>(args.Args[1]);
        i32      flags   = static_cast<i32>(args.Args[2]);
        mode_t   mode    = static_cast<mode_t>(args.Args[3]);

        return current->OpenAt(dirFd, path, flags, mode);
    }
    std::expected<i32, std::errno_t> SysFStatAt(Arguments& args)
    {
        Process*        current   = CPU::GetCurrentThread()->parent;

        i32             fd        = static_cast<i32>(args.Args[0]);
        const char*     path      = reinterpret_cast<const char*>(args.Args[1]);
        CTOS_UNUSED i32 flags     = static_cast<i32>(args.Args[2]);
        stat*           outBuffer = reinterpret_cast<stat*>(args.Args[3]);

        if (flags & ~(AT_EMPTY_PATH | AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW))
            return_err(-1, EINVAL);

        FileDescriptor* fileHandle     = current->GetFileHandle(fd);
        bool            followSymlinks = !(flags & AT_SYMLINK_NOFOLLOW);

        if (!outBuffer) return_err(-1, EFAULT);

        if (!path || *path == 0)
        {
            if (!(flags & AT_EMPTY_PATH)) return_err(-1, ENOENT);

            if (fd == AT_FDCWD) *outBuffer = current->GetCWD()->GetStats();
            else if (!fileHandle) return_err(-1, EBADF);
            else *outBuffer = fileHandle->GetNode()->GetStats();

            return 0;
        }

        INode* parent = Path::IsAbsolute(path) ? VFS::GetRootNode() : nullptr;
        if (fd == AT_FDCWD) parent = current->GetCWD();
        else if (fileHandle) parent = fileHandle->GetNode();

        if (!parent) return_err(-1, EBADF);
        INode* node
            = std::get<1>(VFS::ResolvePath(parent, path, followSymlinks));

        if (!node) return -1;
        *outBuffer = node->GetStats();

        return 0;
    }
}; // namespace Syscall::VFS

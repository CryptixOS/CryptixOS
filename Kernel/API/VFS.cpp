/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/sys/mman.h>
#include <API/UnixTypes.hpp>
#include <API/VFS.hpp>

#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Prism/Path.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

namespace API::VFS
{
    ErrorOr<isize> Read(i32 fdNum, u8* out, usize bytes)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateWrite(out, bytes)) return Error(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        CPU::UserMemoryProtectionGuard guard;
        return fd->Read(out, bytes);
    }
    ErrorOr<isize> Write(i32 fdNum, const u8* in, usize bytes)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(in, bytes)) return Error(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        CPU::UserMemoryProtectionGuard guard;
        return fd->Write(in, bytes);
    }
    ErrorOr<isize> Open(PathView path, i32 flags, mode_t mode)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(path.Raw(), Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        return current->OpenAt(AT_FDCWD, path, flags, mode);
    }
    ErrorOr<isize> Close(i32 fdNum)
    {
        Process* current = Process::GetCurrent();
        return current->CloseFd(fdNum);
    }
    ErrorOr<isize> Stat(const char* path, stat* out)
    {
        return API::VFS::FStatAt(AT_FDCWD, path, 0, out);
    }
    ErrorOr<isize> FStat(isize fdNum, stat* out)
    {
        return API::VFS::FStatAt(fdNum, "", AT_EMPTY_PATH, out);
    }
    ErrorOr<isize> LStat(const char* path, stat* out)
    {
        return API::VFS::FStatAt(AT_FDCWD, path, AT_SYMLINK_NOFOLLOW, out);
    }
    ErrorOr<i32> Access(const char* path, mode_t mode)
    {
        if (mode != (mode & S_IRWXO)) return Error(EINVAL);
        auto process = Process::GetCurrent();
        if (!process->ValidateRead(path, 4096)) return Error(EFAULT);

        auto inode
            = std::get<1>(::VFS::ResolvePath(::VFS::GetRootNode(), path));
        if (!inode) return Error(errno);

        return inode->CheckPermissions(mode);
    }

    ErrorOr<isize> Dup(isize oldFdNum)
    {
        Process* process = Process::GetCurrent();
        auto*    oldFd   = process->GetFileHandle(oldFdNum);
        if (!oldFd) return Error(EBADF);

        FileDescriptor* newFd = new FileDescriptor(oldFd);
        if (!newFd) return Error(ENOMEM);

        return process->m_FdTable.Insert(newFd);
    }
    ErrorOr<isize> Dup2(isize oldFdNum, isize newFdNum)
    {
        Process* process = Process::GetCurrent();
        auto*    oldFd   = process->GetFileHandle(oldFdNum);
        if (!oldFd) return Error(EBADF);

        auto* newFd = process->GetFileHandle(newFdNum);
        if (newFd == oldFd) return newFdNum;

        if (newFd) process->CloseFd(newFdNum);
        newFd = new FileDescriptor(oldFd);

        return process->m_FdTable.Insert(newFd, newFdNum);
    }

    ErrorOr<isize> Truncate(PathView path, off_t length)
    {
        Process* current = Process::GetCurrent();
        if (length < 0) return Error(EINVAL);

        if (!current->ValidateRead(path.Raw(), Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        ErrorOr<INode*> nodeOrError = ::VFS::ResolvePath(path);
        if (!nodeOrError) return nodeOrError.error();

        INode* node = nodeOrError.value();
        if (node->IsDirectory()) return Error(EISDIR);
        return node->Truncate(length);
    }
    ErrorOr<isize> FTruncate(i32 fdNum, off_t length)
    {
        if (length < 0) return Error(EINVAL);
        Process*        current = Process::GetCurrent();

        FileDescriptor* fd      = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);
        return fd->Truncate(length);
    }
    ErrorOr<isize> GetCwd(char* buffer, usize size)
    {
        Process* process = Process::GetCurrent();

        if (!buffer || size == 0) return Error(EINVAL);
        if (!process->ValidateWrite(buffer, size)) return Error(EFAULT);

        StringView cwd = process->GetCWD();
        if (size < cwd.Size()) return Error(ERANGE);

        CPU::UserMemoryProtectionGuard guard;
        cwd.Copy(buffer, cwd.Size());
        return 0;
    }

    ErrorOr<isize> RmDir(PathView path)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(path.Raw(), Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        StringView lastComponent = path.GetLastComponent().data();
        if (lastComponent == ".") return Error(EINVAL);
        if (lastComponent == "..") return Error(ENOTEMPTY);

        ErrorOr<INode*> nodeOrError = ::VFS::ResolvePath(path);
        if (!nodeOrError) return nodeOrError.error();

        INode* node = nodeOrError.value();
        if (node->GetParent()
            && !node->GetParent()->CanWrite(current->GetCredentials()))
            return Error(EACCES);
        if (!node->IsDirectory()) return Error(ENOTDIR);
        for (const auto& child : node->GetChildren())
        {
            std::string_view name = child.second->GetName();
            if (name != "." && name != "..") return Error(ENOTEMPTY);
        }

        ::VFS::RecursiveDelete(node);
        return 0;
    }
    ErrorOr<isize> ReadLink(PathView path, char* out, usize size)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(path.Raw(), size)) return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);
        if (!current->ValidateWrite(out, size)) return Error(EFAULT);

        if (static_cast<isize>(size) <= 0) return Error(EINVAL);
        ErrorOr<INode*> nodeOrError = ::VFS::ResolvePath(path);
        if (!nodeOrError) return nodeOrError.error();

        INode* node = nodeOrError.value();
        if (!node->IsSymlink()) return Error(EINVAL);

        StringView symlinkTarget = node->GetTarget().data();
        size                     = std::min(size, symlinkTarget.Size());

        return symlinkTarget.Copy(out, size);
    }
    ErrorOr<isize> ChMod(const char* path, mode_t mode)
    {
        return FChModAt(AT_FDCWD, path, mode);
    }

    ErrorOr<isize> Mount(const char* path, const char* target,
                         const char* filesystemType, usize flags,
                         const void* data)
    {
        Process* current = Process::GetCurrent();
        CtosUnused(current);

        bool success = ::VFS::Mount(::VFS::GetRootNode(), path, target,
                                    filesystemType, flags, data);

        if (!success) return Error(errno);
        return 0;
    }

    ErrorOr<isize> FChModAt(isize dirFdNum, PathView path, mode_t mode)
    {
        auto   process = Process::GetCurrent();

        INode* parent  = process->GetRootNode();
        if (!path.IsAbsolute())
        {
            if (dirFdNum == AT_FDCWD)
            {
                auto nodeOrError = ::VFS::ResolvePath(process->GetCWD());
                if (!nodeOrError) return Error(nodeOrError.error());
                parent = nodeOrError.value();
            }
            else
            {
                auto fd = process->GetFileHandle(dirFdNum);
                if (!fd) return Error(EBADF);

                parent = fd->GetNode();
            }
        }

        auto node = std::get<1>(::VFS::ResolvePath(parent, path));
        if (!node) return Error(ENOENT);

        auto ret = node->ChMod(mode);
        if (!ret) return Error(ret.error());
        return 0;
    }
    ErrorOr<isize> PSelect6(isize fdCount, fd_set* readFds, fd_set* writeFds,
                            fd_set* exceptFds, const timeval* timeout,
                            const sigset_t* sigmask)
    {
        auto                   current = Process::GetCurrent();

        [[maybe_unused]] isize i, j, max;
        return -1;
        usize set = 0;
        for (isize i = 0; i < FD_SETSIZE; i++)
        {
            j = i << 5;
            if (j >= fdCount) break;

            set = readFds->fds_bits[i] | writeFds->fds_bits[i]
                | exceptFds->fds_bits[i];
            for (; set; j++, set >>= 1)
            {
                if (j >= fdCount) goto end_check;
                if (!(set & Bit(0))) continue;

                auto fd = current->GetFileHandle(j);
                if (!fd) return Error(EBADF);
                max = j;
            }
        }
    end_check:
    }

    ErrorOr<isize> UTime(PathView path, const utimbuf* out)
    {
        ErrorOr<INode*> nodeOrError = ::VFS::ResolvePath(path);
        if (!nodeOrError) return nodeOrError.error();

        // FIXME(v1tr10l7): finish this
        Process* process = Process::GetCurrent();
        INode*   node    = nodeOrError.value();
        (void)process;
        (void)node;
        return Error(ENOSYS);
    }

    ErrorOr<isize> FStatAt(isize dirFdNum, const char* path, isize flags,
                           stat* out)
    {
        if (flags & ~(AT_EMPTY_PATH | AT_NO_AUTOMOUNT | AT_SYMLINK_NOFOLLOW)
            || !out)
            return Error(EINVAL);

        Process* current = Process::GetCurrent();
        if (path && !current->ValidateRead(path)) return Error(EFAULT);

        CPU::UserMemoryProtectionGuard guard;
        if (!PathView(path).ValidateLength()) return Error(ENAMETOOLONG);

        auto* fd             = current->GetFileHandle(dirFdNum);
        bool  followSymlinks = !(flags & AT_SYMLINK_NOFOLLOW);

        auto  cwd            = std::get<1>(
            ::VFS::ResolvePath(::VFS::GetRootNode(), current->GetCWD()));
        if (!path || !*path)
        {
            if (!(flags & AT_EMPTY_PATH)) return Error(ENOENT);

            if (dirFdNum == AT_FDCWD)
            {
                if (!cwd) return Error(ENOENT);

                *out = cwd->GetStats();
                return 0;
            }
            else if (!fd) return Error(EBADF);

            *out = fd->GetNode()->GetStats();
            return 0;
        }

        auto parent
            = PathView(path).IsAbsolute() ? ::VFS::GetRootNode() : nullptr;
        if (!parent)
        {
            if (dirFdNum == AT_FDCWD) parent = cwd;
            else if (fd)
            {
                parent = fd->GetNode();
                if (!parent->IsDirectory()) return Error(ENOTDIR);
            }
            else return Error(EBADF);

            parent = parent->Reduce(false, true);
        }

        auto node
            = std::get<1>(::VFS::ResolvePath(parent, path, followSymlinks));
        if (!node) return Error(errno);

        *out = node->GetStats();
        return 0;
    }
    ErrorOr<isize> Dup3(isize oldFdNum, isize newFdNum, isize flags)
    {
        if (oldFdNum == newFdNum) return Error(EINVAL);

        auto* process = Process::GetCurrent();
        auto* oldFd   = process->GetFileHandle(oldFdNum);
        if (!oldFd) return Error(EBADF);

        auto* newFd = process->GetFileHandle(newFdNum);
        if (newFd) process->CloseFd(newFdNum);

        newFd = new FileDescriptor(oldFd, flags);
        if (!newFd) return Error(ENOMEM);

        isize retFd = process->m_FdTable.Insert(newFd, newFdNum);
        if (retFd < 0)
        {
            delete newFd;
            return Error(EBADF);
        }

        return retFd;
    }
}; // namespace API::VFS

namespace Syscall::VFS
{
    using Syscall::Arguments;
    using namespace ::VFS;

    ErrorOr<off_t> SysLSeek(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        off_t    offset  = static_cast<off_t>(args.Args[1]);
        i32      whence  = static_cast<i32>(args.Args[2]);

        Process* current = Process::GetCurrent();
        auto     fd      = current->GetFileHandle(fdNum);
        if (!fd) return Error(ENOENT);

        return fd->Seek(whence, offset);
    }

    ErrorOr<i32> SysIoCtl(Arguments& args)
    {
        i32      fdNum   = static_cast<i32>(args.Args[0]);
        usize    request = static_cast<usize>(args.Args[1]);
        usize    arg     = static_cast<usize>(args.Args[2]);

        Process* current = Process::GetCurrent();
        auto     fd      = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        CPU::UserMemoryProtectionGuard guard;
        return fd->GetNode()->IoCtl(request, arg);
    }
    ErrorOr<i32> SysAccess(Arguments& args)
    {
        const char* path = reinterpret_cast<const char*>(args.Args[0]);
        i32         mode = static_cast<i32>(args.Args[1]);
        (void)mode;

        INode* node = CPU::AsUser(
            [path]() -> INode*
            {
                return std::get<1>(VFS::ResolvePath(VFS::GetRootNode(), path));
            });
        if (!node) return Error(EPERM);

        return 0;
    }

    ErrorOr<i32> SysFcntl(Arguments& args)
    {
        i32             fdNum   = static_cast<i32>(args.Args[0]);
        i32             op      = static_cast<i32>(args.Args[1]);
        uintptr_t       arg     = reinterpret_cast<uintptr_t>(args.Args[2]);

        Process*        current = Process::GetCurrent();
        FileDescriptor* fd      = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        bool cloExec = true;
        switch (op)
        {
            case F_DUPFD: cloExec = false;
            case F_DUPFD_CLOEXEC:
            {
                isize newFdNum = -1;
                for (newFdNum = arg; current->GetFileHandle(newFdNum);
                     newFdNum++);
                return API::VFS::Dup3(fdNum, newFdNum, cloExec ? O_CLOEXEC : 0);
            }
            case F_GETFD: return fd->GetFlags();
            case F_SETFD:
                if (!fd) return Error(EBADF);
                fd->SetFlags(arg);
                break;
            case F_GETFL: return fd->GetDescriptionFlags();
            case F_SETFL:
                if (arg & O_ACCMODE) return Error(EINVAL);
                fd->SetDescriptionFlags(arg);
                break;

            default:
                LogError("Syscall::VFS::SysFcntl: Unknown opcode");
                return Error(EINVAL);
        }

        return 0;
    }
    ErrorOr<i32> SysChDir(Arguments& args)
    {
        const char* path    = reinterpret_cast<const char*>(args.Args[0]);
        Process*    current = Process::GetCurrent();

        INode*      cwd     = std::get<1>(
            VFS::ResolvePath(current->GetRootNode(), current->GetCWD()));
        INode* node = nullptr;
        {
            CPU::UserMemoryProtectionGuard guard;
            node = std::get<1>(VFS::ResolvePath(cwd, path));
        }
        if (!node) return Error(ENOENT);
        if (!node->IsDirectory()) return Error(ENOTDIR);

        current->m_CWD
            = StringView{node->GetPath().data(), node->GetPath().size()};
        return 0;
    }
    ErrorOr<i32> SysFChDir(Arguments& args)
    {
        i32             fdNum   = static_cast<i32>(args.Args[0]);
        Process*        current = Process::GetCurrent();

        FileDescriptor* fd      = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        INode* node = fd->GetNode();
        if (!node) return Error(ENOENT);

        if (!node->IsDirectory()) return Error(ENOTDIR);
        current->m_CWD = StringView(node->GetPath().data());

        return 0;
    }
    ErrorOr<i32> SysMkDir(Syscall::Arguments& args)
    {
        PathView path    = args.Get<const char*>(0);
        mode_t   mode    = args.Get<mode_t>(1);

        // TODO(v1tr10l7): validate whether user has appropriate permissions
        Process* current = Process::GetCurrent();

        if (!current->ValidateRead(reinterpret_cast<uintptr_t>(path.Raw()),
                                   Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        INode* parent        = path.IsAbsolute()
                                 ? current->GetRootNode()
                                 : std::get<1>(VFS::ResolvePath(VFS::GetRootNode(),
                                                                current->GetCWD()));
        auto [_, node, name] = VFS::ResolvePath(parent, path);

        mode &= ~current->GetUmask() & 0777;
        if (!parent) return Error(ENOENT);
        if (!parent->IsDirectory()) return Error(ENOTDIR);

        if (node) return Error(EEXIST);

        node = VFS::CreateNode(parent, path, mode | S_IFDIR);
        if (!node) return Error(errno);

        return 0;
    }

    [[clang::no_sanitize("alignment")]] ErrorOr<i32>
    SysGetDents64(Arguments& args)
    {
        u32           fdNum     = static_cast<u32>(args.Args[0]);
        dirent* const outBuffer = reinterpret_cast<dirent* const>(args.Args[1]);
        u32           count     = static_cast<u32>(args.Args[2]);

        Process*      current   = Process::GetCurrent();
        if (!outBuffer
            || !current->ValidateRead(outBuffer, Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        CPU::UserMemoryProtectionGuard guard;
        return fd->GetDirEntries(outBuffer, count);
    }

    ErrorOr<i32> SysOpenAt(Arguments& args)
    {
        Process* current = Process::GetCurrent();

        i32      dirFd   = static_cast<i32>(args.Args[0]);
        PathView path;
        CPU::AsUser([&path, &args]()
                    { path = reinterpret_cast<const char*>(args.Args[1]); });
        i32    flags = static_cast<i32>(args.Args[2]);
        mode_t mode  = static_cast<mode_t>(args.Args[3]);

        return current->OpenAt(dirFd, path, flags, mode);
    }
    ErrorOr<i32> SysMkDirAt(Syscall::Arguments& args)
    {
        i32      dirFd   = args.Get<i32>(0);
        PathView path    = args.Get<const char*>(1);
        mode_t   mode    = args.Get<mode_t>(2);

        Process* process = Process::GetCurrent();
        if (!process->ValidateRead(reinterpret_cast<uintptr_t>(path.Raw()),
                                   Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        INode*          parent = nullptr;
        FileDescriptor* fd     = process->GetFileHandle(dirFd);

        if (!path.IsAbsolute())
        {
            if (dirFd != AT_FDCWD && !fd) return Error(EBADF);

            INode* node = fd->GetNode();
            if (!node) return Error(ENOENT);
            parent = node->GetParent();
        }

        if (!parent) return Error(ENOENT);
        if (!parent->IsDirectory()) return Error(ENOTDIR);
        (void)mode;

        mode &= ~process->GetUmask() & 0777;
        if (!parent) return Error(ENOENT);
        if (!parent->IsDirectory()) return Error(ENOTDIR);

        INode* node = std::get<1>(VFS::ResolvePath(parent, path));
        if (node) return Error(EEXIST);

        node = VFS::CreateNode(parent, path, mode | S_IFDIR);
        if (!node) return Error(errno);

        return 0;
    }
}; // namespace Syscall::VFS

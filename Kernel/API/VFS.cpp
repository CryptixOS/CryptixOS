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
#include <Time/Time.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

namespace API::VFS
{
    ErrorOr<::VFS::PathResolution> ResolveAtFd(i32 dirFdNum, PathView path,
                                               isize flags)
    {
        auto process = Process::GetCurrent();

        errno        = no_error;
        if (!path.Raw() || path.Empty())
        {
            if (!(flags & AT_EMPTY_PATH)) return Error(ENOENT);
            if (dirFdNum == AT_FDCWD)
            {
                auto res = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(),
                                              process->GetCWD());
                if (errno != no_error && errno != ENOENT) return Error(errno);
                return res;
            }

            auto* fd = process->GetFileHandle(dirFdNum);
            if (!fd) return Error(EBADF);
            ::VFS::PathResolution res;
            res.Entry    = fd->DirectoryEntry();
            res.Parent   = res.Entry->Parent();
            res.BaseName = res.Entry->Name();

            return res;
        }

        if (!path.ValidateLength()) return Error(ENAMETOOLONG);
        DirectoryEntry* base = ::VFS::GetRootDirectoryEntry();

        if (!path.Absolute())
        {
            if (dirFdNum == AT_FDCWD)
                base = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(),
                                          process->GetCWD())
                           .Entry;
            else
            {
                auto* fd = process->GetFileHandle(dirFdNum);
                if (!fd) return Error(EBADF);
                base = fd->DirectoryEntry();
                if (!base->IsDirectory()) return Error(ENOTDIR);
            }
        }

        const bool followSymlinks = !(flags & AT_SYMLINK_NOFOLLOW);
        auto       res = ::VFS::ResolvePath(base, path, followSymlinks);
        if (errno != no_error && errno != ENOENT) return Error(errno);

        return res;
    }

    ErrorOr<isize> Read(i32 fdNum, u8* out, usize bytes)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateWrite(out, bytes)) return Error(EFAULT);

        FileDescriptor* fd = current->GetFileHandle(fdNum);
        if (!fd || !fd->CanRead()) return Error(EBADF);

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

        auto entry
            = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(), path).Entry;
        if (!entry) return Error(errno);

        auto inode = entry->INode();
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

        auto pathRes = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(), path);
        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);

        if (entry->IsDirectory()) return Error(EISDIR);

        auto inode = entry->INode();
        return inode->Truncate(length);
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

    ErrorOr<isize> Rename(const char* oldPath, const char* newPath)
    {
        return RenameAt(AT_FDCWD, oldPath, AT_FDCWD, newPath);
    }
    ErrorOr<isize> MkDir(const char* pathname, mode_t mode)
    {
        return MkDirAt(AT_FDCWD, pathname, mode);
    }
    ErrorOr<isize> Link(const char* oldPath, const char* newPath)
    {
        return LinkAt(AT_FDCWD, oldPath, AT_FDCWD, newPath, 0);
    }
    ErrorOr<isize> Unlink(const char* path) { return Error(ENOSYS); }
    ErrorOr<isize> Symlink(const char* target, const char* linkPath)
    {
        return Error(ENOSYS);
    }
    ErrorOr<isize> RmDir(PathView path)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(path.Raw(), Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        StringView lastComponent = path.GetLastComponent();
        if (lastComponent == "."_sv) return Error(EINVAL);
        if (lastComponent == ".."_sv) return Error(ENOTEMPTY);

        auto pathRes
            = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(), path, true);
        if (!pathRes.Entry) return Error(ENOENT);

        auto dentry = pathRes.Entry;
        if (dentry->Parent()
            && !dentry->Parent()->INode()->CanWrite(current->GetCredentials()))
            return Error(EACCES);
        if (!dentry->IsDirectory()) return Error(ENOTDIR);
        for (const auto& [name, child] : dentry->Children())
            if (name != "."_sv && name != ".."_sv) return Error(ENOTEMPTY);

        // TODO(v1tr10l7):  ::VFS::RecursiveDelete(node);
        return 0;
    }
    ErrorOr<isize> ReadLink(PathView path, char* out, usize size)
    {
        return ReadLinkAt(AT_FDCWD, path.Raw(), out, size);
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

        bool success
            = ::VFS::Mount(nullptr, path, target, filesystemType, flags, data);

        if (!success) return Error(errno);
        return 0;
    }

    ErrorOr<isize> MkDirAt(isize dirFdNum, const char* pathname, mode_t mode)
    {
        auto process = Process::Current();
        if (!process->ValidateRead(pathname, 255)) return Error(EFAULT);
        const auto& creds = process->GetCredentials();

        auto        path  = CPU::AsUser(
            [&pathname]() -> Path
            {
                if (!pathname || *pathname == 0) return "";

                return pathname;
            });

        auto pathResOr = ResolveAtFd(dirFdNum, path, 0);
        if (!pathResOr) return Error(pathResOr.error());
        auto pathRes     = pathResOr.value();

        auto parentEntry = pathRes.Parent;
        auto parentINode = parentEntry->INode();

        if (!parentEntry->IsDirectory()) return Error(ENOTDIR);
        auto entry = pathRes.Entry;

        if (entry) return Error(EEXIST);
        auto success
            = parentINode->MkDir(pathRes.BaseName, mode, creds.uid, creds.gid);
        if (!success) Error(success.error());
        return 0;
    }
    ErrorOr<isize> ReadLinkAt(isize dirFdNum, const char* pathView,
                              char* outBuffer, usize bufferSize)
    {
        Path path
            = CPU::AsUser([&pathView]() -> Path { return Path(pathView); });

        auto pathResOr = ResolveAtFd(dirFdNum, path, AT_SYMLINK_NOFOLLOW);
        if (!pathResOr) return Error(pathResOr.error());
        auto pathRes = pathResOr.value();

        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);
        auto inode = entry->INode();

        auto userBufferSuccess
            = UserBuffer::ForUserBuffer(outBuffer, bufferSize);
        if (!userBufferSuccess) return Error(userBufferSuccess.error());
        auto userBuffer = userBufferSuccess.value();

        auto success    = inode->ReadLink(userBuffer);
        if (!success) return Error(success.error());

        return success.value();
    }
    ErrorOr<isize> FChModAt(isize dirFdNum, PathView pathView, mode_t mode)
    {
        Path path
            = CPU::AsUser([pathView]() -> Path
                          { return Path(pathView.Raw(), pathView.Size()); });
        auto pathResOr = ResolveAtFd(dirFdNum, path, mode);
        if (!pathResOr) return Error(pathResOr.error());
        auto pathRes = pathResOr.value();

        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);

        auto inode = entry->INode();
        auto ret   = inode->ChMod(mode);
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
        auto pathRes
            = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(), path, true);
        auto entry = pathRes.Entry;
        if (!entry) return Error(ENOENT);
        auto inode = entry->INode();
        if (!inode) return Error(ENOENT);

        Process* process = Process::GetCurrent();
        if (!inode->CanWrite(process->GetCredentials())) return Error(EPERM);
        auto     utime      = CPU::AsUser([&out]() -> utimbuf { return *out; });

        timespec accessTime = {utime.actime, 0};
        timespec modificationTime = {utime.modtime, 0};

        auto     result = inode->UpdateTimestamps(accessTime, modificationTime);
        if (!result) return Error(result.error());
        result = inode->FlushMetadata();
        if (!result) return Error(result.error());

        return {};
    }
    ErrorOr<isize> StatFs(PathView path, statfs* out)
    {
        auto pathRes = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(), path);
        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);

        auto inode = entry->INode();
        if (!entry) return Error(ENOENT);

        auto fs     = inode->GetFilesystem();
        auto result = fs->Stats(*out);

        return !result ? result.error() : 0;
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

        auto  cwdEntry = ::VFS::ResolvePath(::VFS::GetRootDirectoryEntry(),
                                            current->GetCWD())
                            .Entry;
        if (!cwdEntry) return Error(ENOENT);
        auto cwd = cwdEntry->INode();

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

            *out = fd->INode()->GetStats();
            return 0;
        }

        auto parentEntry = PathView(path).Absolute()
                             ? ::VFS::GetRootDirectoryEntry()
                             : nullptr;
        if (!parentEntry)
        {
            if (dirFdNum == AT_FDCWD) parentEntry = cwdEntry;
            else if (fd)
            {
                parentEntry = fd->DirectoryEntry();
                if (!parentEntry->IsDirectory()) return Error(ENOTDIR);
            }
            else return Error(EBADF);

            parentEntry = parentEntry->FollowMounts();
        }

        DirectoryEntry* entry
            = ::VFS::ResolvePath(parentEntry, path, followSymlinks).Entry;
        if (!entry) return Error(errno);

        auto inode = entry->INode();
        if (!inode) return Error(errno);

        *out = inode->GetStats();
        return 0;
    }
    ErrorOr<isize> UnlinkAt(isize dirFdNum, const char* path, isize flags)
    {
        return Error(ENOSYS);
    }
    ErrorOr<isize> RenameAt(isize oldDirFdNum, const char* oldPath,
                            isize newDirFdNum, const char* newPath)
    {
        return RenameAt2(oldDirFdNum, oldPath, newDirFdNum, newPath, 0);
    }
    ErrorOr<isize> LinkAt(isize oldDirFdNum, const char* oldPath,
                          isize newDirFdNum, const char* newPath, isize flags)
    {
        auto getPath = [](const char* path) -> Path
        { return CPU::AsUser([path]() -> Path { return path; }); };

        auto oldPathResolutionOr
            = ResolveAtFd(oldDirFdNum, getPath(oldPath), 0);
        if (!oldPathResolutionOr) return Error(oldPathResolutionOr.error());
        auto oldPathResolution = oldPathResolutionOr.value();

        auto newPathResolutionOr
            = ResolveAtFd(newDirFdNum, getPath(newPath), 0);
        if (!newPathResolutionOr) return Error(newPathResolutionOr.error());
        auto newPathResolution = newPathResolutionOr.value();

        if (!oldPathResolution.Entry) return Error(ENOENT);
        if (newPathResolution.Entry) return Error(EEXIST);

        auto inode   = oldPathResolution.Entry->INode();
        auto success = inode->Link(getPath(newPath));
        if (!success) return Error(success.error());

        return 0;
    }
    ErrorOr<isize> SymlinkAt(const char* target, isize newDirFdNum,
                             const char* linkPath)
    {
        return Error(ENOSYS);
    }

    ErrorOr<isize> UtimensAt(i64 dirFdNum, const char* pathname,
                             const timespec times[2], i64 flags)
    {
        auto process = Process::Current();
        if ((pathname && !process->ValidateRead(pathname))
            || (times && !process->ValidateRead(times, sizeof(timespec) * 2)))
            return Error(EFAULT);

        auto atime = times ? CPU::CopyFromUser(times[0]) : Time::GetReal();
        auto mtime = times ? CPU::CopyFromUser(times[1]) : Time::GetReal();
        auto ctime = Time::GetReal();
        if (atime.tv_nsec == UTIME_OMIT && mtime.tv_nsec == UTIME_OMIT)
            return 0;
        auto validateTimespec = [](const timespec& timespec) -> bool
        {
            auto nsec = timespec.tv_nsec;

            if (nsec == UTIME_NOW || nsec == UTIME_OMIT) return true;

            return nsec >= 0 && nsec <= 999999999;
        };

        if (times && !validateTimespec(atime) && !validateTimespec(mtime))
            return Error(EINVAL);
        if (atime.tv_nsec == UTIME_NOW) atime = Time::GetReal();
        else if (atime.tv_nsec == UTIME_OMIT) atime = {};

        if (mtime.tv_nsec == UTIME_NOW) mtime = Time::GetReal();
        else if (mtime.tv_nsec == UTIME_OMIT) mtime = {};

        auto   path      = CPU::AsUser([&]() -> PathView { return pathname; });
        auto   pathResOr = ResolveAtFd(dirFdNum, path, flags);

        INode* inode     = nullptr;
        if (!pathResOr)
        {
            if (dirFdNum < 0 && dirFdNum != AT_FDCWD)
                return Error(pathResOr.error());

            auto fd = process->GetFileHandle(dirFdNum);
            if (!fd) return Error(EBADF);

            inode = fd->INode();
        }
        else
        {
            auto dentry = pathResOr.value().Entry;
            if (!dentry) return Error(ENOENT);
            inode = dentry->INode();
        }

        if (!inode) return Error(ENOENT);
        if (!inode->CanWrite(process->GetCredentials())) return Error(EPERM);

        auto result = inode->UpdateTimestamps(atime, mtime, ctime);
        if (!result) return Error(result.error());
        result = inode->FlushMetadata();
        if (!result) return result.error();

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
    ErrorOr<isize> RenameAt2(isize oldDirFdNum, const char* oldPath,
                             isize newDirFdNum, const char* newPath,
                             usize flags)
    {
        auto getPath = [](const char* path) -> Path
        { return CPU::AsUser([path]() -> Path { return path; }); };

        auto oldPathResolutionOr
            = ResolveAtFd(oldDirFdNum, getPath(oldPath), 0);
        if (!oldPathResolutionOr) return Error(oldPathResolutionOr.error());
        auto oldPathResolution = oldPathResolutionOr.value();

        auto newPathResolutionOr
            = ResolveAtFd(newDirFdNum, getPath(newPath), 0);
        if (!newPathResolutionOr) return Error(newPathResolutionOr.error());
        auto newPathResolution = newPathResolutionOr.value();

        auto oldParent         = oldPathResolution.Parent->INode();
        if (!oldParent->IsDirectory()) return Error(ENOTDIR);

        if (!oldPathResolution.Entry) return Error(ENOENT);
        if (newPathResolution.Entry) return Error(EEXIST);

        auto newParent = newPathResolution.Parent;
        auto newName   = newPathResolution.BaseName.Size() > 0
                           ? newPathResolution.BaseName
                           : oldPathResolution.BaseName;
        auto success   = oldParent->Rename(newParent->INode(), newName);

        if (!success) return Error(success.error());
        return 0;
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
        return fd->INode()->IoCtl(request, arg);
    }
    ErrorOr<i32> SysAccess(Arguments& args)
    {
        const char* path = reinterpret_cast<const char*>(args.Args[0]);
        i32         mode = static_cast<i32>(args.Args[1]);
        (void)mode;

        DirectoryEntry* entry = CPU::AsUser(
            [path]() -> DirectoryEntry*
            {
                return VFS::ResolvePath(VFS::GetRootDirectoryEntry(), path)
                    .Entry;
            });
        if (!entry) return Error(EPERM);

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
        const char*     path    = reinterpret_cast<const char*>(args.Args[0]);
        Process*        current = Process::GetCurrent();

        DirectoryEntry* cwd
            = VFS::ResolvePath(current->GetRootNode(), current->GetCWD()).Entry;
        if (!cwd) return Error(ENOENT);

        DirectoryEntry* entry = nullptr;
        {
            CPU::UserMemoryProtectionGuard guard;
            entry = VFS::ResolvePath(cwd, path).Entry;
        }
        if (!entry) return Error(ENOENT);
        if (!entry->IsDirectory()) return Error(ENOTDIR);

        current->m_CWD = entry->Path().View();
        return 0;
    }
    ErrorOr<i32> SysFChDir(Arguments& args)
    {
        i32             fdNum   = static_cast<i32>(args.Args[0]);
        Process*        current = Process::GetCurrent();

        FileDescriptor* fd      = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        auto entry = fd->DirectoryEntry();
        if (!entry) return Error(ENOENT);

        if (!entry->IsDirectory()) return Error(ENOTDIR);
        current->m_CWD = entry->Path().View();

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
}; // namespace Syscall::VFS

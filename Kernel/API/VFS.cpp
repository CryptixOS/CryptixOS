/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/Limits.hpp>
#include <API/UnixTypes.hpp>
#include <API/VFS.hpp>

#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/sys/mman.h>
#include <API/Posix/sys/select.h>
#include <API/Posix/sys/statfs.h>
#include <API/Posix/time.h>
#include <API/Posix/utime.h>

#include <Arch/CPU.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Thread.hpp>

#include <Prism/Memory/Scope.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Utility/Path.hpp>
#include <Time/Time.hpp>

#include <VFS/FileDescriptor.hpp>
#include <VFS/Filesystem.hpp>
#include <VFS/INode.hpp>
#include <VFS/MountPoint.hpp>
#include <VFS/PathResolver.hpp>
#include <VFS/VFS.hpp>

namespace API::VFS
{
    using namespace ::VFS;
    ErrorOr<::VFS::PathResolution> ResolveAtFd(isize dirFdNum, PathView path,
                                               isize flags)
    {
        auto process = Process::GetCurrent();

        errno        = no_error;
        if (!path.Raw() || path.Empty())
        {
            if (!(flags & AT_EMPTY_PATH)) return Error(ENOENT);
            if (dirFdNum == AT_FDCWD)
            {
                auto cwd        = process->CWD();
                auto parent     = cwd->Parent();
                auto parentName = parent->Name();

                return ::VFS::PathResolution{
                    parent->Parent()->Lookup(parentName), cwd, cwd->Name()};
            }

            Ref<FileDescriptor> fd = process->GetFileHandle(dirFdNum);
            if (!fd) return Error(EBADF);
            ::VFS::PathResolution res;
            res.Entry    = fd->DirectoryEntry();
            res.Parent   = res.Entry->Parent().Promote();
            res.BaseName = res.Entry->Name();

            return res;
        }

        if (!path.ValidateLength()) return Error(ENAMETOOLONG);
        Ref<DirectoryEntry> base = ::VFS::RootDirectoryEntry();

        if (!path.Absolute())
        {
            if (dirFdNum == AT_FDCWD) base = process->CWD();
            else
            {
                Ref<FileDescriptor> fd = process->GetFileHandle(dirFdNum);
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

    ErrorOr<isize> Read(isize fdNum, u8* out, usize bytes)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateWrite(out, bytes)) return Error(EFAULT);

        Ref<FileDescriptor> fd = current->GetFileHandle(fdNum);
        if (!fd || !fd->CanRead()) return Error(EBADF);

        CPU::UserMemoryProtectionGuard guard;
        return fd->Read(out, bytes);
    }
    ErrorOr<isize> Write(isize fdNum, const u8* in, usize bytes)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(in, bytes)) return Error(EFAULT);

        Ref<FileDescriptor> fd = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        CPU::UserMemoryProtectionGuard guard;
        return fd->Write(in, bytes);
    }
    ErrorOr<isize> Open(PathView path, isize flags, mode_t mode)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(path.Raw(), Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        return current->OpenAt(AT_FDCWD, path, flags, mode);
    }
    ErrorOr<isize> Close(isize fdNum)
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

    ErrorOr<isize> LSeek(isize fdNum, off_t offset, isize whence)
    {
        auto process = Process::Current();
        auto fd      = process->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        return fd->Seek(whence, offset);
    }
    // TODO(v1tr10l7): FIBMAP, FIGETBSZ, FIONREAD
    ErrorOr<isize> IoCtl(isize fdNum, usize request, usize argument)
    {
        auto process = Process::Current();
        auto fd      = process->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        switch (request)
        {
            case FIOCLEX: fd->SetCloseOnExec(true); return 0;
            case FIONCLEX: fd->SetCloseOnExec(false); return 0;
            case FIONBIO:
            {
                isize nonblocking
                    = CPU::CopyFromUser(*reinterpret_cast<isize*>(argument));

                auto flags = fd->GetDescriptionFlags();
                if (nonblocking) flags |= O_NONBLOCK;
                else flags &= ~O_NONBLOCK;

                fd->SetDescriptionFlags(flags);
                return 0;
            }
            case FIOASYNC:
            {
                isize fasync
                    = CPU::CopyFromUser(*reinterpret_cast<isize*>(argument));

                auto flags = fd->GetDescriptionFlags();
                if (fasync) flags |= FASYNC;
                else flags &= ~FASYNC;

                fd->SetDescriptionFlags(flags);
                // TODO(v1tr10l7): fasync
                return 0;
            }
            case FIOQSIZE:
            {
                if (!fd->IsDirectory() && !fd->IsRegular() && !fd->IsSymlink())
                    return Error(ENOTTY);
                // TODO(v1tr10l7): Add helper function for retrieving size in
                // bytes to inode class
                usize size = fd->INode()->Stats().st_size;

                CPU::CopyToUser(reinterpret_cast<usize*>(argument), size);
                return 0;
            }

            default: break;
        }

        // FIXME(v1tr10l7): shouldn't be here
        CPU::UserMemoryProtectionGuard guard;
        return fd->INode()->IoCtl(request, argument);
    }

    ErrorOr<isize> PRead(isize fdNum, void* out, usize count, off_t offset)
    {
        auto process = Process::Current();
        auto fd      = process->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        if (!process->ValidateAddress(out, PROT_READ, count))
            return Error(EFAULT);

        auto maybeBuffer = UserBuffer::ForUserBuffer(out, count);
        RetOnError(maybeBuffer);
        auto outBuffer = maybeBuffer.Value();

        return fd->Read(outBuffer, count, offset);
    }
    ErrorOr<isize> PWrite(isize fdNum, const void* in, usize count,
                          off_t offset)
    {
        auto process = Process::Current();
        auto fd      = process->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        if (!process->ValidateAddress(in, PROT_WRITE, count))
            return Error(EFAULT);

        auto maybeBuffer = UserBuffer::ForUserBuffer(in, count);
        RetOnError(maybeBuffer);
        auto inBuffer = maybeBuffer.Value();

        return fd->Write(inBuffer, count, offset);
    }

    ErrorOr<isize> Access(const char* filename, mode_t mode)
    {
        auto process = Process::Current();
        if (!process->ValidateRead(filename, Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        Path path = CPU::CopyStringFromUser(filename);

        if (!path.ValidateLength()) return Error(ENAMETOOLONG);
        if (mode != (mode & S_IRWXO)) return Error(EINVAL);

        auto pathRes = TryOrRet(VFS::ResolvePath(nullptr, path));

        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);

        auto inode = entry->INode();
        if (!inode) return Error(ENOENT);

        auto status = inode->CheckPermissions(mode);
        if (!status && (mode & S_IWOTH) /* && inode->ReadOnly()*/
            && (inode->IsDirectory() || inode->IsRegular()))
            return Error(EROFS);

        return status;
    }

    ErrorOr<isize> Dup(isize oldFdNum)
    {
        Process* process = Process::Current();

        return process->DupFd(oldFdNum, -1, 0);
    }
    ErrorOr<isize> Dup2(isize oldFdNum, isize newFdNum)
    {
        Process* process = Process::Current();

        return process->DupFd(oldFdNum, newFdNum, 0);
    }
    ErrorOr<isize> FCntl(isize fdNum, isize op, pointer arg)
    {
        Process*            current = Process::GetCurrent();
        Ref<FileDescriptor> fd = TryOrRet(current->GetFileDescriptor(fdNum));

        bool                cloExec = true;
        switch (op)
        {
            case F_DUPFD: cloExec = false; CTOS_FALLTHROUGH;
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

            default: return Error(EINVAL);
        }

        return 0;
    }

    ErrorOr<isize> Truncate(PathView path, off_t length)
    {
        Process* current = Process::Current();
        if (length < 0) return Error(EINVAL);

        if (!current->ValidateRead(path.Raw(), Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        auto maybePathRes
            = ::VFS::ResolvePath(::VFS::RootDirectoryEntry(), path);
        RetOnError(maybePathRes);
        auto pathRes = maybePathRes.Value();

        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);

        if (entry->IsDirectory()) return Error(EISDIR);
        else if (!entry->IsRegular()) return Error(EINVAL);

        auto inode  = entry->INode();
        auto status = inode->CheckPermissions(W_OK);
        RetOnError(status);

        return inode->Truncate(length);
    }
    ErrorOr<isize> FTruncate(isize fdNum, off_t length)
    {
        if (length < 0) return Error(EINVAL);
        Process*            current = Process::GetCurrent();

        Ref<FileDescriptor> fd      = current->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);
        return fd->Truncate(length);
    }
    ErrorOr<isize> GetCwd(char* buffer, usize size)
    {
        Process* process = Process::GetCurrent();

        if (!buffer || size == 0) return Error(EINVAL);
        if (!process->ValidateWrite(buffer, size)) return Error(EFAULT);

        auto cwd     = process->CWD();
        auto cwdPath = cwd->Path();

        if (size < cwdPath.Size()) return Error(ERANGE);

        CPU::UserMemoryProtectionGuard guard;
        cwdPath.StrView().Copy(buffer, cwdPath.Size());
        return 0;
    }
    ErrorOr<isize> ChDir(const char* filename)
    {
        auto process = Process::Current();
        if (!process->ValidateRead(filename)) return Error(EFAULT);
        Path path = CPU::AsUser([filename]() -> Path { return filename; });

        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        auto cwd = process->CWD();
        if (!cwd) return Error(ENOENT);

        auto maybePathRes = ::VFS::ResolvePath(cwd, path);
        RetOnError(maybePathRes);
        auto pathRes      = maybePathRes.Value();

        auto newDirectory = pathRes.Entry;
        if (!newDirectory) return Error(ENOENT);
        if (!newDirectory->IsDirectory()) return Error(ENOTDIR);

        // auto inode = newDirectory->INode();
        // if (!inode->CheckPermissions(S_IEXEC)) return Error(EACCES);
        process->SetCWD(newDirectory);

        return 0;
    }
    ErrorOr<isize> FChDir(isize fdNum)
    {
        auto process = Process::Current();
        auto fd      = process->GetFileHandle(fdNum);
        if (!fd) return Error(EBADF);

        auto dentry = fd->DirectoryEntry();
        if (!dentry) return Error(ENOENT);
        auto inode = dentry->INode();
        if (!inode) return Error(ENOENT);

        if (!inode->IsDirectory()) return Error(ENOTDIR);
        // if (!inode->CheckPermissions(S_IEXEC)) return Error(EACCES);
        process->SetCWD(dentry);

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
    ErrorOr<isize> RmDir(const char* pathname)
    {
        auto process = Process::Current();
        if (!process->ValidateRead(pathname, 255)) return Error(EFAULT);

        Path path = pathname ? CPU::CopyStringFromUser(pathname) : ""_p;

        auto cwd  = process->CWD();
        Assert(cwd);

        auto entryName = path.BaseName().Empty() ? path.ParentPath().BaseName()
                                                 : path.BaseName();
        LogDebug("VFS::RmDir: Removing {} from {}, full path: {}", entryName,
                 path.BaseName().Empty() ? path.ParentPath().ParentPath()
                                         : path.ParentPath(),
                 path);
        auto parentEntry = TryOrRet(
            VFS::ResolveParent(path.StartsWith("/") ? nullptr : cwd, path));
        auto parentINode = parentEntry->INode();

        auto entry       = parentEntry->Lookup(entryName);
        if (!entry) return Error(ENOENT);
        if (!parentINode->IsDirectory() || !entry->IsDirectory())
            return Error(ENOTDIR);

        // FIXME(v1tr10l7): error handling, posix compliance
        LogDebug("VFS::RmDir: Calling parentINode->RmDir(entry);");
        RetOnError(parentINode->RmDir(entry));
        LogDebug("VFS::RmDir: Successfully removed directory");

        return 0;
    }
    ErrorOr<isize> Creat(const char* pathname, mode_t mode)
    {
        Process* current = Process::GetCurrent();
        if (!current->ValidateRead(pathname, Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);

        auto path = CPU::CopyStringFromUser(pathname);
        if (!path.ValidateLength()) return Error(ENAMETOOLONG);

        return current->OpenAt(AT_FDCWD, path, O_CREAT | O_WRONLY | O_TRUNC,
                               mode);
    }
    ErrorOr<isize> Link(const char* oldPath, const char* newPath)
    {
        return LinkAt(AT_FDCWD, oldPath, AT_FDCWD, newPath, 0);
    }
    ErrorOr<isize> Unlink(const char* path) { return Error(ENOSYS); }
    ErrorOr<isize> Symlink(const char* target, const char* linkPath)
    {
        return SymlinkAt(target, AT_FDCWD, linkPath);
    }
    ErrorOr<isize> ReadLink(PathView path, char* out, usize size)
    {
        return ReadLinkAt(AT_FDCWD, path.Raw(), out, size);
    }
    ErrorOr<isize> ChMod(const char* path, mode_t mode)
    {
        return FChModAt(AT_FDCWD, path, mode, 0);
    }
    ErrorOr<isize> FChMod(isize fdNum, mode_t mode)
    {
        return FChModAt(fdNum, ".", mode, 0);
    }

    ErrorOr<isize> Mount(const char* pathname, const char* targetPath,
                         const char* filesystemType, usize flags,
                         const void* data)
    {
        Process* current = Process::GetCurrent();
        if (!current->IsSuperUser()) return ErrorCode(EPERM);

        if (pathname
            && !current->ValidateRead(pathname, Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!current->ValidateRead(targetPath, Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);
        if (!current->ValidateRead(filesystemType, 255)) return Error(EFAULT);
        if (data && !current->ValidateRead(data, PMM::PAGE_SIZE))
            return Error(EFAULT);

        Path source = pathname ? CPU::CopyStringFromUser(pathname) : ""_p;
        Path target = targetPath ? CPU::CopyStringFromUser(targetPath) : ""_p;
        if (target.Empty()) return Error(ENOENT);

        if (!filesystemType) return Error(EINVAL);
        String fsType = CPU::CopyStringFromUser(filesystemType).StrView();
        if (fsType.Empty()) return ErrorCode(ENODEV);

        Scope<u8[]> options = new u8[CPU::CopyStringFromUser(
                                         reinterpret_cast<const char*>(data))
                                         .Size()];

        LogDebug("VFS: Mounting `{}`({}) at `{}`", source, fsType, target);
        auto mountPoint = TryOrRet(::VFS::Mount(nullptr, source, target, fsType,
                                                flags, options.Raw()));

        return 0;
    }

    CTOS_NO_SANITIZE("alignment")
    ErrorOr<isize> GetDEnts64(isize fdNum, dirent* const outBuffer, usize count)
    {
        auto current = Process::Current();
        if (!outBuffer
            || !current->ValidateRead(outBuffer, Limits::MAX_PATH_LENGTH))
            return Error(EFAULT);

        Ref<FileDescriptor> fd = TryOrRet(current->GetFileDescriptor(fdNum));
        LogDebug("SysGetDents64: {{ Path: {} }}", fd->DirectoryEntry()->Path());

        CPU::UserMemoryProtectionGuard guard;
        return fd->GetDirEntries(outBuffer, count);
    }
    ErrorOr<isize> OpenAt(isize dirFdNum, const char* pathname, isize flags,
                          mode_t mode)
    {
        Process* current = Process::GetCurrent();
        Path     path    = CPU::CopyStringFromUser(pathname);

        return current->OpenAt(dirFdNum, path, flags, mode);
    }
    ErrorOr<isize> MkDirAt(isize dirFdNum, const char* pathname, mode_t mode)
    {
        auto process = Process::Current();
        if (!process->ValidateRead(pathname, 255)) return Error(EFAULT);

        auto path = CPU::AsUser(
            [&pathname]() -> Path
            {
                if (!pathname || *pathname == 0) return "";

                return pathname;
            });

        auto pathRes   = TryOrRet(ResolveAtFd(dirFdNum, path, 0));
        auto directory = pathRes.Parent;
        auto baseName  = pathRes.BaseName;

        return ::VFS::CreateDirectory(directory, baseName, mode);
    }
    ErrorOr<isize> MkNodAt(isize dirFdNum, const char* pathname, mode_t mode,
                           dev_t dev)
    {
        auto process = Process::Current();
        if (!process->ValidateRead(pathname, 255)) return Error(EFAULT);

        auto getPath = [](const char* path) -> Path
        { return CPU::AsUser([path]() -> Path { return path; }); };

        auto path      = getPath(pathname);
        auto pathRes   = TryOrRet(ResolveAtFd(dirFdNum, path, 0));
        auto directory = pathRes.Parent;
        auto baseName  = pathRes.BaseName;

        return ::VFS::CreateNode(directory, baseName, mode, dev);
    }
    ErrorOr<isize> ReadLinkAt(isize dirFdNum, const char* pathView,
                              char* outBuffer, usize bufferSize)
    {
        Path path
            = CPU::AsUser([&pathView]() -> Path { return Path(pathView); });

        auto pathResOr = ResolveAtFd(dirFdNum, path, AT_SYMLINK_NOFOLLOW);
        if (!pathResOr) return Error(pathResOr.Error());
        auto pathRes = pathResOr.Value();

        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);
        auto inode     = entry->INode();

        auto linkValue = TryOrRet(inode->ReadLink());
        CPU::AsUser(
            [&]()
            {
                Memory::Copy(outBuffer, linkValue.Raw(),
                             Min(linkValue.Size(), bufferSize));
            });

        return 0;
    }
    ErrorOr<isize> FChModAt(isize dirFdNum, const char* pathView, mode_t mode,
                            isize flags)
    {
        Path path    = CPU::CopyStringFromUser(pathView);
        auto pathRes = TryOrRet(ResolveAtFd(dirFdNum, path, mode));

        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);

        auto process = Process::Current();

        auto inode   = entry->INode();
        if (!process->IsSuperUser()
            && process->Credentials().UserID != inode->UserID())
            return Error(EPERM);
        // TODO(v1tr10l7): Validate group id

        RetOnError(inode->ChangeMode(mode));
        // TODO(v1tr10l7): don't flush metadata immediately
        RetOnError(inode->FlushMetadata());
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
        auto maybePathRes
            = ::VFS::ResolvePath(::VFS::RootDirectoryEntry(), path, true);
        RetOnError(maybePathRes);
        auto pathRes = maybePathRes.Value();

        auto entry   = pathRes.Entry;
        if (!entry) return Error(ENOENT);
        auto inode = entry->INode();
        if (!inode) return Error(ENOENT);

        Process* process = Process::GetCurrent();
        if (!inode->CanWrite(process->Credentials())) return Error(EPERM);
        auto     utime      = CPU::AsUser([&out]() -> utimbuf { return *out; });

        timespec accessTime = {utime.actime, 0};
        timespec modificationTime = {utime.modtime, 0};

        auto     result = inode->UpdateTimestamps(accessTime, modificationTime);
        if (!result) return Error(result.Error());
        result = inode->FlushMetadata();
        if (!result) return Error(result.Error());

        return {};
    }
    ErrorOr<isize> StatFs(PathView path, statfs* out)
    {
        PathResolver resolver(nullptr,
                              CPU::AsUser([path]() -> Path { return path; }));
        auto entry = TryOrRet(resolver.Resolve(PathLookupFlags::eFollowLinks));

        auto inode = entry->INode();
        if (!inode || !inode->Filesystem()) return Error(ENOENT);

        auto   fs = inode->Filesystem();

        statfs stats;
        Memory::Fill(&stats, 0, sizeof(stats));

        RetOnError(fs->Stats(stats));
        CPU::CopyToUser(out, stats);
        return 0;
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

        Ref<FileDescriptor> fd             = current->GetFileHandle(dirFdNum);
        bool                followSymlinks = !(flags & AT_SYMLINK_NOFOLLOW);

        auto                cwdEntry       = current->CWD();
        if (!cwdEntry) return Error(ENOENT);
        auto cwd = cwdEntry->INode();

        if (!path || !*path)
        {
            if (!(flags & AT_EMPTY_PATH)) return Error(ENOENT);

            if (dirFdNum == AT_FDCWD)
            {
                if (!cwd) return Error(ENOENT);

                *out = cwd->Stats();
                return 0;
            }
            else if (!fd) return Error(EBADF);

            *out = fd->INode()->Stats();
            return 0;
        }

        WeakRef parentEntry
            = PathView(path).Absolute() ? ::VFS::RootDirectoryEntry() : nullptr;
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

        Ref<DirectoryEntry> entry
            = ::VFS::ResolvePath(parentEntry.Promote(), path, followSymlinks)
                  .Value()
                  .Entry;
        if (!entry) return Error(errno);

        auto inode = entry->INode();
        if (!inode) return Error(errno);

        *out = inode->Stats();
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

        auto oldPathName = getPath(oldPath);
        auto newPathName = getPath(newPath);
        LogTrace("VFS::LinkAt Entry => linking `{}` to `{}`...", oldPathName,
                 newPathName);

        auto oldPathRes  = TryOrRet(ResolveAtFd(oldDirFdNum, oldPath, 0));
        auto newPathRes  = TryOrRet(ResolveAtFd(newDirFdNum, newPath, 0));

        auto oldParent   = oldPathRes.Parent;
        auto newParent   = newPathRes.Parent;

        auto oldBaseName = oldPathRes.BaseName;
        auto newBaseName = newPathRes.BaseName;

        auto err
            = ::VFS::Link(oldParent, oldBaseName, newParent, newBaseName, 0);
        if (!err)
            LogError("VFS: Failed to link `{}` to `{}`, err => {}", oldPathName,
                     newPathName, StringUtils::ToString(err.Error()));

        return err;
    }
    ErrorOr<isize> SymlinkAt(const char* targetPath, isize newDirFdNum,
                             const char* linkPath)
    {
        auto getPath = [](const char* path) -> Path
        { return CPU::AsUser([path]() -> Path { return path; }); };

        auto target    = getPath(targetPath);
        auto link      = getPath(linkPath);

        auto pathRes   = TryOrRet(ResolveAtFd(newDirFdNum, link, 0));
        auto directory = pathRes.Parent;
        auto baseName  = pathRes.BaseName;

        return ::VFS::Symlink(directory, baseName, target);
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
                return Error(pathResOr.Error());

            auto fd = process->GetFileHandle(dirFdNum);
            if (!fd) return Error(EBADF);

            inode = fd->INode();
        }
        else
        {
            auto dentry = pathResOr.Value().Entry;
            if (!dentry) return Error(ENOENT);
            inode = dentry->INode();
        }

        if (!inode) return Error(ENOENT);
        if (!inode->CanWrite(process->Credentials())) return Error(EPERM);

        auto result = inode->UpdateTimestamps(atime, mtime, ctime);
        if (!result) return Error(result.Error());
        result = inode->FlushMetadata();
        if (!result) return result.Error();

        return 0;
    }
    ErrorOr<isize> Dup3(isize oldFdNum, isize newFdNum, isize flags)
    {
        if (oldFdNum == newFdNum) return Error(EINVAL);

        auto* process = Process::GetCurrent();
        return process->DupFd(oldFdNum, newFdNum, flags);
    }
    ErrorOr<isize> RenameAt2(isize oldDirFdNum, const char* oldPath,
                             isize newDirFdNum, const char* newPath,
                             usize flags)
    {
        auto getPath = [](const char* path) -> Path
        { return CPU::AsUser([path]() -> Path { return path; }); };

        auto oldPathResolutionOr
            = ResolveAtFd(oldDirFdNum, getPath(oldPath), 0);
        if (!oldPathResolutionOr) return Error(oldPathResolutionOr.Error());
        auto oldPathResolution = oldPathResolutionOr.Value();

        auto newPathResolutionOr
            = ResolveAtFd(newDirFdNum, getPath(newPath), 0);
        if (!newPathResolutionOr) return Error(newPathResolutionOr.Error());
        auto newPathResolution = newPathResolutionOr.Value();

        auto oldParent         = oldPathResolution.Parent->INode();
        if (!oldParent->IsDirectory()) return Error(ENOTDIR);

        if (!oldPathResolution.Entry) return Error(ENOENT);
        if (newPathResolution.Entry) return Error(EEXIST);

        auto newParent = newPathResolution.Parent;
        auto newName   = newPathResolution.BaseName.Size() > 0
                           ? newPathResolution.BaseName
                           : oldPathResolution.BaseName;
        auto success   = oldParent->Rename(newParent->INode(), newName);

        if (!success) return Error(success.Error());
        return 0;
    }
}; // namespace API::VFS

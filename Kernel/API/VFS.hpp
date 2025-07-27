/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

#include <API/Posix/signal.h>
#include <Prism/Utility/PathView.hpp>

struct dirent;
struct fd_set;
struct utimbuf;
struct statfs;
namespace API::VFS
{
    ErrorOr<isize> Read(isize fdNum, u8* out, usize bytes);
    ErrorOr<isize> Write(isize fdNum, const u8* in, usize bytes);
    ErrorOr<isize> Open(PathView path, isize flags, mode_t mode);
    ErrorOr<isize> Close(isize fdNum);

    ErrorOr<isize> Stat(const char* path, stat* out);
    ErrorOr<isize> FStat(isize fdNum, stat* out);
    ErrorOr<isize> LStat(const char* path, stat* out);

    ErrorOr<isize> LSeek(isize fdNum, off_t offset, isize whence);
    ErrorOr<isize> IoCtl(isize fdNum, usize request, usize argument);

    ErrorOr<isize> PRead(isize fdNum, void* out, usize count, off_t offset);
    ErrorOr<isize> PWrite(isize fdNum, const void* in, usize count,
                          off_t offset);
    ErrorOr<isize> Access(const char* filename, mode_t mode);

    ErrorOr<isize> Dup(isize oldFdNum);
    ErrorOr<isize> Dup2(isize oldFdNum, isize newFdNum);
    ErrorOr<isize> FCntl(isize fdNum, isize op, pointer arg);

    ErrorOr<isize> Truncate(PathView path, off_t length);
    ErrorOr<isize> FTruncate(isize fdNum, off_t length);
    ErrorOr<isize> GetCwd(char* buffer, usize size);
    ErrorOr<isize> ChDir(const char* filename);
    ErrorOr<isize> FChDir(isize fdNum);

    ErrorOr<isize> Rename(const char* oldPath, const char* newPath);
    ErrorOr<isize> MkDir(const char* pathname, mode_t mode);
    ErrorOr<isize> RmDir(const char* pathname);
    ErrorOr<isize> Creat(const char* pathname, mode_t mode);
    ErrorOr<isize> Link(const char* oldPath, const char* newPath);
    ErrorOr<isize> Unlink(const char* path);
    ErrorOr<isize> Symlink(const char* target, const char* linkPath);
    ErrorOr<isize> ReadLink(PathView path, char* out, usize size);
    ErrorOr<isize> ChMod(const char* path, mode_t mode);
    ErrorOr<isize> FChMod(isize fdNum, mode_t mode);

    ErrorOr<isize> Mount(const char* path, const char* target,
                         const char* filesystemType, usize flags,
                         const void* data);

    CTOS_NO_SANITIZE("alignment")
    ErrorOr<isize> GetDEnts64(isize dirFdNum, dirent* const outBuffer,
                              usize count);
    ErrorOr<isize> OpenAt(isize dirFdNum, const char* path, isize flags,
                          mode_t mode);
    ErrorOr<isize> MkDirAt(isize dirFdNum, const char* path, mode_t mode);
    ErrorOr<isize> MkNodAt(isize dirFdNum, const char* path, mode_t mode,
                           dev_t dev);
    ErrorOr<isize> ReadLinkAt(isize dirFdNum, const char* path, char* out,
                              usize bufferSize);
    ErrorOr<isize> FChModAt(isize dirFdNum, const char* path, mode_t mode,
                            isize flags);
    ErrorOr<isize> PSelect6(isize fdCount, fd_set* readFds, fd_set* writeFds,
                            fd_set* exceptFds, const timeval* timeout,
                            const sigset_t* sigmask);
    ErrorOr<isize> UTime(PathView path, const utimbuf* out);
    ErrorOr<isize> StatFs(PathView path, statfs* out);
    ErrorOr<isize> FStatAt(isize dirFd, const char* path, isize flags,
                           stat* out);
    ErrorOr<isize> UnlinkAt(isize dirFdNum, const char* path, isize flags);
    ErrorOr<isize> RenameAt(isize oldDirFdNum, const char* oldPath,
                            isize newDirFdNum, const char* newPath);
    ErrorOr<isize> LinkAt(isize oldDirFdNum, const char* oldPath,
                          isize newDirFdNum, const char* newPath, isize flags);
    ErrorOr<isize> SymlinkAt(const char* target, isize newDirFdNum,
                             const char* linkPath);
    ErrorOr<isize> UtimensAt(i64 dirFdNum, const char* path,
                             const timespec times[2], i64 flags);
    ErrorOr<isize> Dup3(isize oldFdNum, isize newFdNum, isize flags);
    ErrorOr<isize> RenameAt2(isize oldDirFdNum, const char* oldPath,
                             isize newDirFdNum, const char* newPath,
                             usize flags);
} // namespace API::VFS

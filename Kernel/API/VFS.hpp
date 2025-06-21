/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/signal.h>
#include <API/Posix/sys/select.h>
#include <API/Posix/sys/statfs.h>
#include <API/Posix/utime.h>
#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

#include <Prism/PathView.hpp>

namespace API::VFS
{
    ErrorOr<isize> Read(i32 fdNum, u8* out, usize bytes);
    ErrorOr<isize> Write(i32 fdNum, const u8* in, usize bytes);
    ErrorOr<isize> Open(PathView path, i32 flags, mode_t mode);
    ErrorOr<isize> Close(i32 fdNum);

    ErrorOr<isize> Stat(const char* path, stat* out);
    ErrorOr<isize> FStat(isize fdNum, stat* out);
    ErrorOr<isize> LStat(const char* path, stat* out);

    ErrorOr<isize> Dup(isize oldFdNum);
    ErrorOr<isize> Dup2(isize oldFdNum, isize newFdNum);

    ErrorOr<isize> Truncate(PathView path, off_t length);
    ErrorOr<isize> FTruncate(i32 fdNum, off_t length);
    ErrorOr<isize> GetCwd(char* buffer, usize size);

    ErrorOr<isize> Rename(const char* oldPath, const char* newPath);
    ErrorOr<isize> MkDir(const char* pathname, mode_t mode);
    ErrorOr<isize> RmDir(PathView path);
    ErrorOr<isize> Link(const char* oldPath, const char* newPath);
    ErrorOr<isize> Unlink(const char* path);
    ErrorOr<isize> Symlink(const char* target, const char* linkPath);
    ErrorOr<isize> ReadLink(PathView path, char* out, usize size);
    ErrorOr<isize> ChMod(const char* path, mode_t mode);

    ErrorOr<isize> Mount(const char* path, const char* target,
                         const char* filesystemType, usize flags,
                         const void* data);

    ErrorOr<isize> MkDirAt(isize dirFdNum, const char* path, mode_t mode);
    ErrorOr<isize> ReadLinkAt(isize dirFdNum, const char* path, char* out,
                              usize bufferSize);
    ErrorOr<isize> FChModAt(isize dirFdNum, PathView path, mode_t mode);
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
    ErrorOr<isize> Dup3(isize oldFdNum, isize newFdNum, isize flags);
    ErrorOr<isize> RenameAt2(isize oldDirFdNum, const char* oldPath,
                             isize newDirFdNum, const char* newPath,
                             usize flags);
} // namespace API::VFS
namespace Syscall::VFS
{
    ErrorOr<off_t> SysLSeek(Syscall::Arguments& args);
    ErrorOr<i32>   SysIoCtl(Syscall::Arguments& args);

    ErrorOr<i32>   SysAccess(Syscall::Arguments& args);

    ErrorOr<i32>   SysFcntl(Syscall::Arguments& args);
    ErrorOr<i32>   SysChDir(Syscall::Arguments& args);
    ErrorOr<i32>   SysFChDir(Syscall::Arguments& args);
    ErrorOr<i32>   SysMkDir(Syscall::Arguments& args);

    [[clang::no_sanitize("alignment")]] ErrorOr<i32>
                 SysGetDents64(Syscall::Arguments& args);
    ErrorOr<i32> SysOpenAt(Syscall::Arguments& args);
    ErrorOr<i32> SysMkDirAt(Syscall::Arguments& args);
    ErrorOr<i32> SysFStatAt(Syscall::Arguments& args);
} // namespace Syscall::VFS

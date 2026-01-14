/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Syscall.hpp>
#include <API/UnixTypes.hpp>

#include <API/Posix/bits/socket.h>
#include <API/Posix/poll.h>
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
    ErrorOr<isize> Open(const char* pathname, isize flags, INodeMode mode);
    ErrorOr<isize> Close(isize fdNum);

    ErrorOr<isize> Stat(const char* path, stat* out);
    ErrorOr<isize> FStat(isize fdNum, stat* out);
    ErrorOr<isize> LStat(const char* path, stat* out);

    ErrorOr<isize> LSeek(isize fdNum, off_t offset, isize whence);
    ErrorOr<isize> IoCtl(isize fdNum, usize request, usize argument);

    ErrorOr<isize> PRead(isize fdNum, void* out, usize count, off_t offset);
    ErrorOr<isize> PWrite(isize fdNum, const void* in, usize count,
                          off_t offset);
    ErrorOr<isize> ReadV(isize fdNum, const struct iovec* vec, usize vlen);
    ErrorOr<isize> WriteV(isize fdNum, const struct iovec* vec, usize vlen);
    ErrorOr<isize> Access(const char* filename, INodeMode mode);
    ErrorOr<isize> Pipe(isize pipefd[2], isize flags);

    ErrorOr<isize> Dup(isize oldFdNum);
    ErrorOr<isize> Dup2(isize oldFdNum, isize newFdNum);

    ErrorOr<isize> Socket(isize domain, isize type, isize protocol);
    ErrorOr<isize> Connect(isize sockFdNum, const struct sockaddr* addr,
                           socklen_t addrlen);
    ErrorOr<isize> Accept(isize sockFdNum, struct sockaddr* addr,
                          socklen_t* addrlen);
    ErrorOr<isize> SendTo(isize sockFdNum, const u8* data, usize size,
                          isize flags, const sockaddr* destAddr,
                          socklen_t addrlen);
    ErrorOr<isize> ReceiveFrom(isize sockFdNum, u8* data, usize size,
                               isize flags, sockaddr* destAddr,
                               socklen_t* addrlen);
    ErrorOr<isize> Bind(isize sockFdNum, const struct sockaddr* addr,
                        socklen_t addrlen);
    ErrorOr<isize> Listen(isize sockFdNum, isize backlog);
    ErrorOr<isize> GetSockName(isize sockFdNum, struct sockaddr* addr,
                               socklen_t* addrlen);
    ErrorOr<isize> GetPeerName(isize sockFdNum, struct sockaddr* addr,
                               socklen_t* addrlen);
    ErrorOr<isize> SocketPair(isize domain, isize type, isize protocol,
                              isize* sv);
    ErrorOr<isize> SetSockOpt(isize sockFdNum, isize level, isize option,
                              const u8* value, const usize valueSize);
    ErrorOr<isize> GetSockOpt(isize sockFdNum, isize level, isize option,
                              u8* value, socklen_t* valueSize);
    ErrorOr<isize> FCntl(isize fdNum, isize op, pointer arg);

    ErrorOr<isize> Truncate(PathView path, off_t length);
    ErrorOr<isize> FTruncate(isize fdNum, off_t length);
    ErrorOr<isize> GetCwd(char* buffer, usize size);
    ErrorOr<isize> ChDir(const char* filename);
    ErrorOr<isize> FChDir(isize fdNum);

    ErrorOr<isize> Rename(const char* oldPath, const char* newPath);
    ErrorOr<isize> MkDir(const char* pathname, INodeMode mode);
    ErrorOr<isize> RmDir(const char* pathname);
    ErrorOr<isize> Creat(const char* pathname, INodeMode mode);
    ErrorOr<isize> Link(const char* oldPath, const char* newPath);
    ErrorOr<isize> Unlink(const char* path);
    ErrorOr<isize> Symlink(const char* target, const char* linkPath);
    ErrorOr<isize> ReadLink(PathView path, char* out, usize size);
    ErrorOr<isize> ChMod(const char* path, INodeMode mode);
    ErrorOr<isize> FChMod(isize fdNum, INodeMode mode);

    ErrorOr<isize> SyncFilesystems();
    ErrorOr<isize> Mount(const char* path, const char* target,
                         const char* filesystemType, usize flags,
                         const void* data);

    ErrorOr<isize> SetXAttr(const char* path, const char* name, const u8* value,
                            usize size, isize flags);
    ErrorOr<isize> LSetXAttr(const char* path, const char* name,
                             const u8* value, usize size, isize flags);
    ErrorOr<isize> FSetXAttr(isize fdNum, const char* name, const u8* value,
                             usize size, isize flags);
    ErrorOr<isize> GetXAttr(const char* path, const char* name, const u8* value,
                            usize size);
    ErrorOr<isize> LGetXAttr(const char* path, const char* name,
                             const u8* value, usize size);
    ErrorOr<isize> FGetXAttr(isize fdNum, const char* name, const u8* value,
                             usize size);
    ErrorOr<isize> ListXAttr(const char* path, char* list, usize size);
    ErrorOr<isize> LListXAttr(const char* path, char* list, usize size);
    ErrorOr<isize> FListXAttr(isize fdNum, char* list, usize size);

    ErrorOr<isize> RemoveXAttr(const char* path, const char* name);
    ErrorOr<isize> LRemoveXAttr(const char* path, const char* name);
    ErrorOr<isize> FRemoveXAttr(isize fdNum, const char* name);

    CTOS_NO_SANITIZE("alignment")
    ErrorOr<isize> GetDEnts64(isize dirFdNum, dirent* const outBuffer,
                              usize count);
    ErrorOr<isize> OpenAt(isize dirFdNum, const char* path, isize flags,
                          INodeMode mode);
    ErrorOr<isize> MkDirAt(isize dirFdNum, const char* path, INodeMode mode);
    ErrorOr<isize> MkNodAt(isize dirFdNum, const char* path, INodeMode mode,
                           dev_t dev);
    ErrorOr<isize> ReadLinkAt(isize dirFdNum, const char* path, char* out,
                              usize bufferSize);
    ErrorOr<isize> FChModAt(isize dirFdNum, const char* path, INodeMode mode,
                            isize flags);
    ErrorOr<isize> PSelect6(isize fdCount, fd_set* readFds, fd_set* writeFds,
                            fd_set* exceptFds, const timeval* timeout,
                            const sigset_t* sigmask);
    ErrorOr<isize> PPoll(pollfd* fds, nfds_t nfds, int timeout);
    ErrorOr<isize> UTime(PathView path, const utimbuf* out);
    ErrorOr<isize> StatFs(const char* pathname, statfs* out);
    ErrorOr<isize> PivotRoot(const char* newRoot, const char* putOld);
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
    ErrorOr<isize> Pipe2(i32* pipeFds, isize flags);

    ErrorOr<isize> SyncFs(isize fdNum);
    ErrorOr<isize> RenameAt2(isize oldDirFdNum, const char* oldPath,
                             isize newDirFdNum, const char* newPath,
                             usize flags);

    ErrorOr<isize> SetXAttrAt(isize dirFdNum, const char* path, usize flags,
                              const char* name, const struct xattr_args* uargs,
                              usize size);
    ErrorOr<isize> GetXAttrAt(isize dirFdNum, const char* path, usize flags,
                              const char* name, const struct xattr_args* uargs,
                              usize size);
    ErrorOr<isize> ListXAttrAt(isize dirFdNum, const char* path, usize flags,
                               char* list, usize size);
    ErrorOr<isize> RemoveXAttrAt(isize dirFdNum, const char* path, usize flags,
                                 const char* name);
    ErrorOr<isize> OpenTreeAttr(isize dirFdNum, const char* filename,
                                usize flags, struct mount_attr* uattr,
                                usize size);
} // namespace API::VFS

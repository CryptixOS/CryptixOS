/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Limits.hpp>
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
#include <Compiler.hpp>
#include <API/Posix/termios.h>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
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
#include <API/UnixTypes.hpp>
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
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Utility/PathView.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Core/Singleton.hpp>
#include <API/Posix/linux/capability.h>
#include <Prism/Containers/CircularQueue.hpp>
#include <Config.hpp>
#include <API/Posix/sys/resource.h>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <API/Posix/sys/statfs.h>
#include <VFS/EchFs/EchFsStructures.hpp>
#include <VFS/TmpFs/TmpFsINode.hpp>

module Cryptix:Impl.VFS.VFS;

import :Arch.CPU;
import :Drivers.Core.BlockDevice;
import :Drivers.Core.DeviceManager;
import :Library.Locking.Spinlock;
import :Library.Locking.SpinlockProtected;
import :Scheduler.Scheduler;
import :Time.Time;
import :VFS.DevPtsFs.DevPtsFs;
import :VFS.DevTmpFs.DevTmpFs;
import :VFS.INode;
import :VFS.EchFs.EchFs;
import :VFS.EchFs.EchFsINode;
import :VFS.Ext2Fs.Ext2Fs;
import :VFS.Fat32Fs.Fat32Fs;
import :VFS.Initrd.Initrd;
import :VFS.MountPoint;
import :VFS.PathResolver;
import :VFS.ProcFs.ProcFs;
import :VFS.TmpFs.TmpFs;
import :VFS.VFS;

namespace VFS
{
    static SpinlockProtected<FilesystemDriver::List> s_FilesystemDrivers;

    static bool                                      s_Initialized    = false;
    static Ref<DirectoryEntry>                       s_RootEntry      = nullptr;
    static Ref<MountPoint>                           s_RootMountPoint = nullptr;

    template <typename T>
    static void registerFilesystem(StringView name)
    {
        auto fs            = CreateRef<FilesystemDriver>();

        fs->Owner          = nullptr;
        fs->FilesystemName = name;
        fs->FlagsMask      = 0;

        fs->Instantiate = []() -> ErrorOr<Ref<Filesystem>> { return new T(0); };
        fs->Destroy     = [](Ref<Filesystem>) -> ErrorOr<void> { return {}; };

        VFS::RegisterFilesystem(fs);
    }

    static void registerFilesystems()
    {
        registerFilesystem<TmpFs>("tmpfs");
        registerFilesystem<DevTmpFs>("devfs");
        registerFilesystem<DevPtsFs>("devptsfs");
        registerFilesystem<ProcFs>("proc");
        registerFilesystem<Fat32Fs>("vfat");
        registerFilesystem<Ext2Fs>("ext2");
        registerFilesystem<EchFs>("echfs");
    }

    static void filesystemSyncDaemon()
    {
        for (;;)
        {
            LogDebug("Sync");
            Time::NanoSleep(15'000'000'000);
            Sync();
        }
    }

    void Initialize()
    {
        registerFilesystems();
        Assert(MountRoot("tmpfs"));
        Initrd::Initialize();

        CreateDirectory("/dev", 0755);
        Assert(Mount(nullptr, "", "/dev", "devfs"));

        CreateDirectory("/proc", 0755);
        Assert(Mount(nullptr, "", "/proc", "proc"));

        if (CreateDirectory("/dev/pts", 0755)
            && !Mount(nullptr, "", "/dev/pts", "devptsfs"))
            LogError("VFS: Failed to mount devptsfs");

        CreateDirectory("/run", 0755);
        Mount(nullptr, "", "/run", "tmpfs");

        auto colonel = Scheduler::KernelProcess();
        auto syncd   = colonel->CreateThread(filesystemSyncDaemon, 0);
        Scheduler::EnqueueThread(syncd.Raw());

        using DeviceManager::BlockDeviceIterator;
        using DeviceManager::CharDeviceIterator;
        CharDeviceIterator  cdevIterator;
        BlockDeviceIterator blkdevIterator;
        auto                iterator = [](auto device) -> bool
        {
            auto path = "/dev/"_s + device->Name();
            if (!CreateNode(path, S_IFCHR | 0666, device->ID()))
                LogError("VFS: Failed to create device node for: `{}`", path);

            return true;
        };

        cdevIterator.BindLambda(iterator);
        blkdevIterator.BindLambda(iterator);

        DeviceManager::IterateCharDevices(cdevIterator);
        DeviceManager::IterateBlockDevices(blkdevIterator);
        s_Initialized = true;
    }
    bool                           IsInitialized() { return s_Initialized; }

    ErrorOr<Ref<FilesystemDriver>> FindFilesystemDriver(StringView name)
    {
        Ref<FilesystemDriver> found = nullptr;
        s_FilesystemDrivers.ForEachRead(
            [&](auto fs)
            {
                if (fs->FilesystemName == name) found = fs;
            });

        if (!found) return Error(ENODEV);
        return found;
    }

    ErrorOr<void> RegisterFilesystem(Ref<FilesystemDriver> driver)
    {
        Assert(driver);

        StringView fsName = driver->FilesystemName;
        LogTrace("VFS: Trying to register a filesystem driver: '{}'", fsName);
        if (driver->Hook.IsLinked())
        {
            LogError("VFS: The driver: '{}' is already linked!", fsName);
            return Error(EBUSY);
        }

        auto found = FindFilesystemDriver(driver->FilesystemName);
        if (found)
        {
            LogError("VFS: Driver with name: '{}', already exists!", fsName);
            return Error(EEXIST);
        }

        s_FilesystemDrivers.With([&](auto& list) { list.PushBack(driver); });

        LogTrace("VFS: Successfully registered '{}' driver", fsName);
        return {};
    }
    ErrorOr<void> UnregisterFilesystem(Ref<FilesystemDriver> driver)
    {
        Assert(driver);
        Assert(driver->Hook.IsLinked());

        auto fsName = driver->FilesystemName;
        LogTrace("VFS: Trying to unregister filesystem driver for '{}'",
                 fsName);

        auto fs = TryOrRetFmt(FindFilesystemDriver(driver->FilesystemName),
                              Error(result.Error()),
                              "VFS: Failed to find the driver '{}'", fsName);

        fs->Hook.Unlink(fs);
        return {};
    }

    static ErrorOr<Ref<Filesystem>> InstantiateFilesystem(StringView name)
    {
        Ref<FilesystemDriver> fsDriver = TryOrRet(FindFilesystemDriver(name));

        LogTrace(
            "VFS: Successfully retrieved '{}' file system driver, needed "
            "for mounting",
            name);
        if (!fsDriver->Instantiate)
        {
            LogError("VFS: The '{}' driver's Instantiate function is nullptr",
                     name);
            return Error(ENODEV);
        }

        LogTrace("VFS: Instantiating '{}' instance...", name);
        Ref<Filesystem> fs = TryOrRet(fsDriver->Instantiate());

        if (!fs)
        {
            LogError("VFS: Failed to create filesystem: '{}'", name);
            return Error(ENODEV);
        }

        return fs;
    }

    Ref<DirectoryEntry> RootDirectoryEntry()
    {
        auto rootMount = s_RootMountPoint;

        return rootMount ? rootMount->GuestEntry() : s_RootEntry;
    }
    Ref<MountPoint> RootMount() { return s_RootMountPoint; }

    void            RecursiveDelete(INode* node)
    {
        if (!node) return;

        // TODO(v1tr10l7): Recursive deletion of inodes
        //  if (node->IsDirectory())
        //      for (auto [name, child] : node->Children())
        //          RecursiveDelete(child);

        delete node;
    }

    bool Access(PathView path, INodeMode mode)
    {
        auto pathRes
            = TryOrRetVal(ResolvePath(RootDirectoryEntry(), path), false);
        auto dentry = pathRes.Entry;

        Assert(dentry);
        auto inode = dentry->INode();
        return inode && inode->CheckPermissions(mode);
    }
    ErrorOr<Ref<DirectoryEntry>> OpenDirectoryEntry(Ref<DirectoryEntry> parent,
                                                    PathView path, isize flags,
                                                    INodeMode mode)
    {
        Process* current        = Process::GetCurrent();
        bool     followSymlinks = !(flags & O_NOFOLLOW);
        auto     acc            = flags & O_ACCMODE;
        mode &= ~(S_IRWXU | S_IRWXG | S_IRWXO | S_ISVTX | S_ISUID | S_ISGID);

        // if (flags & ~VALID_OPEN_FLAGS || flags & (O_CREAT | O_DIRECTORY))
        //     return Error(EINVAL);

        bool didExist = true;

        if (flags & O_TMPFILE)
        {
            if (!(flags & O_DIRECTORY)) return Error(EINVAL);
            if (acc != O_WRONLY && acc != O_RDWR) return Error(EINVAL);
            return Error(ENOSYS);
        }
        if (flags & O_PATH)
        {
            if (flags & ~(O_DIRECTORY | O_NOFOLLOW | O_PATH | O_CLOEXEC))
                return Error(EINVAL);
        }

        PathResolver resolver(parent, path);
        auto         lookupFlags = PathLookupFlags::eFollowMounts;
        if (followSymlinks) lookupFlags |= PathLookupFlags::eFollowLinks;

        auto status    = resolver.Resolve(lookupFlags);
        Ref  dentry    = status ? *status : nullptr;
        Ref  directory = resolver.ParentEntry();

        LogDebug("VFS::Open: O_EXCL => {}, O_CREAT => {}", flags & O_EXCL,
                 flags & O_CREAT);
        if (!dentry)
        {
            LogTrace("VFS::Open: Parent resolving...\nParentPath => {}",
                     path.ParentPath());
            auto parentPathRes = TryOrRet(
                VFS::ResolvePath(RootDirectoryEntry(), path.ParentPath()));
            directory = parentPathRes.Entry;
        }

        if (!dentry)
        {
            LogTrace("VFS::Open: Creating entry...");

            didExist = false;
            if (errno != ENOENT || (!(flags & O_CREAT) && !(flags & O_EXCL)))
            {
                LogError("VFS: Open failed to find entry: `{}`\nerrno => {}",
                         path, StringUtils::ToString(ErrorCode(errno)));
                return Error(ENOENT);
            }

            if (!directory || !directory->INode()) return Error(ENOENT);
            if (!directory->INode()->ValidatePermissions(current->Credentials(),
                                                         5))
                return Error(EACCES);

            if (path.Contains("sock"))
                LogTrace("VFS: Creating socket: {}, parent => `{}`", path,
                         directory->Path());
            dentry = TryOrRet(VFS::CreateFile(directory, path.BaseName(),
                                              mode & ~current->Umask()));

            if (!dentry) return Error(ENOENT);
        }
        else if (flags & O_EXCL && dentry->INode()) return Error(EEXIST);

        if (dentry->IsMountPoint())
        {
            auto mountPoint = MountPoint::Lookup(dentry);
            if (mountPoint) dentry = mountPoint->GuestEntry();
        }
        dentry = dentry->FollowSymlinks().Promote();

        if (dentry->IsSymlink()) return Error(ELOOP);
        if ((flags & O_DIRECTORY && !dentry->IsDirectory()))
            return Error(ENOTDIR);
        if ((dentry->IsDirectory() && (acc & O_WRONLY || acc & O_RDWR)))
            return Error(EISDIR);

        // TODO(v1tr10l7): check acc modes and truncate
        if (flags & O_TRUNC && dentry->IsRegular() && didExist)
            ;
        return dentry;
    }
    ErrorOr<Ref<FileDescriptor>> Open(Ref<DirectoryEntry> parent, PathView path,
                                      isize flags, INodeMode mode)
    {
        auto dentry = TryOrRet(OpenDirectoryEntry(parent, path, flags, mode));

        auto acc    = flags & O_ACCMODE;
        FileAccessMode accMode = FileAccessMode::eNone;
        switch (acc)
        {
            case O_RDONLY: accMode |= FileAccessMode::eRead; break;
            case O_WRONLY: accMode |= FileAccessMode::eWrite; break;
            case O_RDWR:
                accMode |= FileAccessMode::eRead | FileAccessMode::eWrite;
                break;

            default: return Error(EINVAL);
        }
        if (flags & O_PATH) accMode = FileAccessMode::eNone;

        auto inode = dentry->INode();
        if (inode && inode->IsCharDevice())
        {
            DeviceID id     = inode->DeviceID();
            auto     device = DeviceManager::LookupCharDevice(id);
            if (device)
            {
                File* deviceFile  = reinterpret_cast<File*>(device);
                deviceFile        = TryOrRet(deviceFile->Open(deviceFile));

                DeviceMajor major = GetDeviceMajor(id);
                DeviceMinor minor = GetDeviceMinor(id);

                LogTrace("VFS: Opening device with id: {}.{}", major, minor);
                return CreateRef<FileDescriptor>(dentry, deviceFile, flags,
                                                 accMode);
            }
        }

        return CreateRef<FileDescriptor>(dentry, flags, accMode);
    }

    ErrorOr<PathResolution> ResolvePath(Ref<DirectoryEntry> parent,
                                        PathView path, bool followLinks)
    {
        if (!parent || path.Absolute()) parent = RootDirectoryEntry();
        PathResolution res = {nullptr, nullptr, ""_sv};

        PathResolver   resolver(parent, path);

        auto           flags = PathLookupFlags::eFollowMounts;
        if (followLinks) flags |= PathLookupFlags::eFollowLinks;
        auto resolutionResult = resolver.Resolve(flags);
        CtosUnused(resolutionResult);

        auto parentEntry = resolver.ParentEntry();
        auto entry       = resolver.DirectoryEntry();
        if (followLinks && entry) entry = entry->FollowSymlinks().Promote();

        res.Parent   = parentEntry;
        res.Entry    = entry;
        res.BaseName = resolver.BaseName();

        return res;
    }
    ErrorOr<Ref<DirectoryEntry>> ResolveParent(Ref<DirectoryEntry> parent,
                                               PathView            path)
    {
        PathResolver resolver(RootDirectoryEntry(), path);
        auto         directory = TryOrRet(resolver.Resolve(
            PathLookupFlags::eParent | PathLookupFlags::eFollowMounts
            | PathLookupFlags::eFollowLinks));
        if (directory->IsMountPoint())
        {
            auto mountPoint = MountPoint::Lookup(directory);
            if (mountPoint) directory = mountPoint->GuestEntry();
        }

        return directory;
    }

    ErrorOr<Ref<MountPoint>> MountRoot(StringView filesystemName)
    {
        if (s_RootMountPoint)
        {
            LogError("VFS: Root already mounted!");
            return Error(EEXIST);
        }

        auto root = CreateRef<DirectoryEntry>("/");
        if (!root) return Error(ENOMEM);

        s_RootEntry = root;
        s_RootMountPoint
            = TryOrRet(Mount(nullptr, ""_pv, "/"_pv, filesystemName));
        return s_RootMountPoint;
    }

    // TODO: flags
    ErrorOr<Ref<MountPoint>> Mount(Ref<DirectoryEntry> parent,
                                   PathView sourcePath, PathView target,
                                   StringView fsName, i32 flags,
                                   const void* data)
    {
        Ref<Filesystem> fs = TryOrRet(InstantiateFilesystem(fsName));
        if (target.Empty()) return Error(EINVAL);

        PathResolver targetResolver(parent, target);
        auto         targetEntry = TryOrRet(targetResolver.Resolve(
            PathLookupFlags::eRegular | PathLookupFlags::eFollowMounts
            | PathLookupFlags::eFollowLinks));

        bool         isRoot      = targetEntry == RootDirectoryEntry();
        if (!isRoot && !targetEntry->IsDirectory())
        {
            LogError("VFS: '{}' target is not a directory", target);
            return Error(ENOTDIR);
        }

        if (targetEntry->IsMountPoint() || MountPoint::Lookup(targetEntry))
            return Error(EBUSY);
        auto fsRoot = TryOrRetFmt(
            fs->Mount(sourcePath, data), Error(result.Error()),
            "VFS: Failed to mount filesystem '{}' on '/'", fsName);

        if (!fsRoot)
        {
            LogError("VFS: Failed to mount '{}' fs", fsName);
            return Error(ENODEV);
        }

        fsRoot->SetParent(fsRoot);
        targetEntry->SetMountGate(fsRoot);

        if (sourcePath.Empty())
            LogTrace("VFS: Mounted Filesystem '{}' on '{}'", fsName, target);
        else
            LogTrace("VFS: Mounted  '{}' on '{}' with Filesystem '{}'",
                     sourcePath, target, fsName);

        Ref<MountPoint> mountPoint = CreateRef<MountPoint>(targetEntry, fs);
        MountPoint::Attach(mountPoint);
        return mountPoint;
    }

    bool Unmount(Ref<DirectoryEntry> parent, PathView path, i32 flags)
    {
        // TODO: Unmount
        ToDo();
        return false;
    }
    ErrorOr<void> PivotRoot(PathView newRoot, PathView putOld)
    {
        auto newRootRes = TryOrRet(ResolvePath(RootDirectoryEntry(), newRoot));
        auto newRootEntry = newRootRes.Entry;

        auto putOldRes    = TryOrRet(ResolvePath(RootDirectoryEntry(), putOld));
        auto putOldEntry  = putOldRes.Entry;

        auto rootMount    = RootMount();
        auto newRootMount = MountPoint::Lookup(newRootEntry);
        if (!newRootMount) return Error(ENOENT);

        auto oldRootFs   = rootMount->Filesystem();
        auto oldRoot     = rootMount->ExchangeGuest(newRootMount->GuestEntry());

        auto putOldMount = CreateRef<MountPoint>(oldRoot, oldRootFs);
        MountPoint::Attach(putOldMount);

        return {};
    }

    ErrorOr<void> Sync()
    {
        MountPoint::Iterate(
            [](Ref<MountPoint> mount) -> IterationResult
            {
                auto fs = mount->Filesystem();
                fs->Sync();

                return IterationResult::eContinue;
            });

        return {};
    }

    ErrorOr<Ref<DirectoryEntry>> CreateNode(Ref<DirectoryEntry> directory,
                                            StringView name, INodeMode mode,
                                            dev_t dev, PathView target)
    {
        Assert(directory);
        if (!directory->IsDirectory()) return Error(ENOTDIR);
        if (directory->Lookup(name)) return Error(EEXIST);

        Ref  entry          = CreateRef<DirectoryEntry>(nullptr, name);
        auto directoryINode = directory->INode();
        if (!directoryINode || !directoryINode->Filesystem())
            return Error(ENODEV);

        switch (mode & S_IFMT)
        {
            case S_IFREG:
                TryOrRet(directoryINode->CreateFile(entry, mode));
                break;
            case S_IFDIR:
                TryOrRet(directoryINode->CreateDirectory(entry, mode));
                break;
            case S_IFLNK:
                TryOrRet(directoryINode->Symlink(entry, target));
                break;

            default:
                TryOrRet(directoryINode->CreateNode(entry, mode, dev));
                break;
        }

        directory->InsertChild(entry);
        entry->SetParent(directory);
        return entry;
    }
    ErrorOr<Ref<DirectoryEntry>> CreateNode(PathView path, INodeMode mode,
                                            dev_t dev)
    {
        return CreateNode(TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
                          path.BaseName(), mode, dev);
    }

    ErrorOr<Ref<DirectoryEntry>> CreateFile(Ref<DirectoryEntry> directory,
                                            StringView name, INodeMode mode)
    {
        mode &= ~S_IFMT;
        mode |= S_IFREG;

        return CreateNode(directory, name, mode, 0);
    }
    ErrorOr<Ref<DirectoryEntry>> CreateFile(PathView path, INodeMode mode)
    {
        return CreateFile(TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
                          path.BaseName(), mode);
    }

    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(Ref<DirectoryEntry> directory,
                                                 StringView          name,
                                                 INodeMode           mode)
    {
        mode &= ~S_IFMT;
        mode |= S_IFDIR;

        return CreateNode(directory, name, mode, 0);
    }
    ErrorOr<Ref<DirectoryEntry>> CreateDirectory(PathView path, INodeMode mode)
    {
        return CreateDirectory(
            TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
            path.BaseName(), mode);
    }

    ErrorOr<Ref<DirectoryEntry>> Symlink(Ref<DirectoryEntry> directory,
                                         StringView name, PathView targetPath)
    {
        INodeMode mode = 0777 | S_IFLNK;

        return CreateNode(directory, name, mode, 0, targetPath);
    }
    ErrorOr<Ref<DirectoryEntry>> Symlink(PathView path, PathView targetPath)
    {
        return Symlink(TryOrRet(ResolveParent(RootDirectoryEntry(), path)),
                       path.BaseName(), targetPath);
    }

    ErrorOr<Ref<DirectoryEntry>> Link(Ref<DirectoryEntry> oldParent,
                                      StringView          oldName,
                                      Ref<DirectoryEntry> newParent,
                                      StringView newName, i32 flags)
    {
        Assert(oldParent);
        Assert(newParent);

        auto oldEntry = oldParent->Lookup(oldName);
        if (!oldEntry) return Error(ENOENT);
        if (oldEntry->IsDirectory()) return Error(EPERM);

        auto newEntry = newParent->Lookup(oldName);
        if (newEntry) return Error(EEXIST);

        newEntry            = CreateRef<DirectoryEntry>(nullptr, newName);

        auto newParentINode = newParent->INode();
        TryOrRet(newParentINode->Link(oldEntry, newEntry));

        newParent->InsertChild(newEntry);
        newEntry->SetParent(newParent);

        LogInfo("Link result => oldEntry: `{}`, newEntry: `{}`",
                oldEntry->Path(), newEntry->Path());
        LogInfo("old parent name: `{}`, new parent name: `{}`",
                oldParent->Name(), newParent->Name());
        LogInfo("old parent path: `{}`, new parent path: `{}`",
                oldParent->Path(), newParent->Path());
        LogInfo("old name: `{}`, new name: `{}`", oldName, newName);
        return newEntry;
    }
    ErrorOr<Ref<DirectoryEntry>> Link(PathView oldPath, PathView newPath,
                                      i32 flags)
    {
        auto oldParent = TryOrRet(ResolveParent(RootDirectoryEntry(), oldPath));
        auto newParent = TryOrRet(ResolveParent(RootDirectoryEntry(), newPath));

        return Link(oldParent, oldPath.BaseName(), newParent,
                    newPath.BaseName(), flags);
    }

    bool Unlink(Ref<DirectoryEntry> parent, PathView path, i32 flags)
    {
        if (!parent)
            parent = path.StartsWith("/") ? RootDirectoryEntry()
                                          : Process::Current()->CWD();

        ToDo();
        return false;
    }

    ErrorOr<isize> SetExtendedAttribute(PathView path, StringView name,
                                        const u8* value, usize size,
                                        isize flags, bool followLinks)
    {
        auto pathRes = TryOrRet(
            ResolvePath(VFS::RootDirectoryEntry(), path, followLinks));
        auto dentry = pathRes.Entry;
        if (!dentry) return Error(ENOENT);

        auto inode = dentry->INode();
        if (!inode) return Error(ENOENT);
        return inode->SetExtendedAttribute(dentry, name, value, size, flags);
    }
    ErrorOr<isize> GetExtendedAttribute(PathView path, StringView name,
                                        u8* value, usize size, bool followLinks)
    {
        auto pathRes = TryOrRet(
            ResolvePath(VFS::RootDirectoryEntry(), path, followLinks));
        auto dentry = pathRes.Entry;
        if (!dentry) return Error(ENOENT);

        auto inode = dentry->INode();
        if (!inode) return Error(ENOENT);
        return inode->GetExtendedAttribute(dentry, name, value, size);
    }
    ErrorOr<isize> ListExtendedAttributes(PathView path, char* list, usize size,
                                          bool followLinks)
    {
        auto pathRes = TryOrRet(
            ResolvePath(VFS::RootDirectoryEntry(), path, followLinks));
        auto dentry = pathRes.Entry;
        if (!dentry) return Error(ENOENT);

        auto inode = dentry->INode();
        if (!inode) return Error(ENOENT);
        return inode->ListExtendedAttributes(dentry, list, size);
    }
    ErrorOr<isize> RemoveExtendedAttribute(PathView path, StringView name,
                                           bool followLinks)
    {
        auto pathRes = TryOrRet(
            ResolvePath(VFS::RootDirectoryEntry(), path, followLinks));
        auto dentry = pathRes.Entry;
        if (!dentry) return Error(ENOENT);

        // TODO(v1tr10l7): Check permissions
        auto inode = dentry->INode();
        if (!inode) return Error(ENOENT);

        return inode->RemoveExtendedAttribute(dentry, name);
    }
} // namespace VFS

/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/sys/resource.h>
#include <API/Posix/sys/utsname.h>
#include <API/Syscall.hpp>
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
#include <Prism/Containers/Tuple.hpp>
#include <Prism/Utility/PathView.hpp>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_format.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/UnixTypes.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/String/StringUtils.hpp>
#include <Prism/Core/Singleton.hpp>
#include <API/Posix/linux/capability.h>
#include <API/Posix/signal.h>
#include <Prism/Core/TypeTraits.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Config.hpp>
#include <Prism/Containers/RedBlackTree.hpp>
#include <Prism/Containers/IntrusiveList.hpp>
#include <Library/Module.hpp>
#include <uacpi/resources.h>
#include <uacpi/utilities.h>
#include <Prism/Core/Iterator.hpp>
#include <API/Posix/sys/statfs.h>

module Cryptix:Impl.VFS.ProcFs.ProcFs;

import :API.System;
import :Boot.CommandLine;
import :Drivers.Core.BlockDevice;
import :Drivers.Core.DeviceManager;
import :Scheduler.Scheduler;
import :System.System;
import :Time.Time;
import :VFS.MountPoint;
import :VFS.VFS;
import :VFS.ProcFs.ProcFs;
import :VFS.ProcFs.ProcFsINode;

struct ProcFsCmdLineProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();

        StringView kernelPath = System::KernelExecutablePath();
        StringView cmdline    = CommandLine::KernelCommandLine();

        Buffer.Resize(kernelPath.Size() + cmdline.Size() + 10);
        Write("{} {}\n", kernelPath, cmdline);
    }
};
struct ProcFsFilesystemsProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();
        Buffer.Resize(PMM::PAGE_SIZE);

        // for (auto& [physical, fs] : VFS::Filesystems())
        //     Write("{} {}\n", physical ? "     " : "nodev", fs);
    }
};
struct ProcFsModulesProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();
        Buffer.Resize(PMM::PAGE_SIZE);

        ModuleIterator iterator;
        iterator.BindLambda(
            [this](auto module) -> IterationResult
            {
                Write("{}\n", module->Name);
                return IterationResult::eContinue;
            });
    }
};
struct ProcFsMountsProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();
        Buffer.Resize(PMM::PAGE_SIZE);

        auto mountPoint = MountPoint::Head();
        while (mountPoint)
        {
            auto dentry = mountPoint->HostEntry();
            auto fs     = mountPoint->Filesystem();

            Write("{} {} {} {}\n", fs->DeviceName(), dentry->Path(), fs->Name(),
                  fs->MountFlagsString());
            mountPoint = mountPoint->NextMountPoint();
        }
    }
};
struct ProcFsPartitionsProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();
        Buffer.Resize(PMM::PAGE_SIZE);

        Write("major\tminor\t#blocks\tname\n");
        DeviceManager::BlockDeviceIterator iterator;
        iterator.BindLambda(
            [this](auto device) -> bool
            {
                u16         major      = device->ID();
                u16         minor      = device->ID();
                const stat& stats      = device->Stats();
                u64         blockCount = stats.st_blocks;
                StringView  name       = device->Name();

                Write("{}\t{}\t{}\t{}\n", major, minor, blockCount, name);
                return true;
            });

        DeviceManager::IterateBlockDevices(iterator);
    }
};
struct ProcFsUptimeProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();
        Buffer.Resize(PMM::PAGE_SIZE);

        auto uptime = Time::GetReal();
        Write("{} {}\n", uptime.tv_sec, uptime.tv_nsec);
    }
};
struct ProcFsVersionProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();
        Buffer.Resize(PMM::PAGE_SIZE);

        utsname uname;
        auto    ret = API::System::Uname(&uname);
        if (!ret) return;

        Write("{} {} {}\n", uname.sysname, uname.release, uname.version);
    }
};

struct ProcFsMemoryRegionsProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.Clear();
        Buffer.Resize(PMM::PAGE_SIZE);

        auto process = Process::GetCurrent();

        Write("======================================================\n");
        Write("Base\t\tLength\t\tPhys\t\tAccess\n");
        for (const auto& [base, region] : process->AddressSpace())
            Write("{:#x}\t\t{:#x}\t\t{:#x}\t\t{:#b}\n", base, region->Size(),
                  region->PhysicalBase().Raw(), ToString(region->Access()));
        Write("======================================================\n");
    }
};

struct ProcFsStatusProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        StringView pidString = m_Parent->Name();
        pid_t      pid       = StringUtils::ToNumber<pid_t>(pidString, 10);
        auto       process   = Scheduler::GetProcess(pid);
        if (!process) return;

        Write("Name: {}\n", process->Name());
    }
};

static constexpr ProcFsProperty* CreateProcFsProperty(StringView name)
{
    if (name == "cmdline"_sv) return new ProcFsCmdLineProperty();
    else if (name == "filesystems"_sv) return new ProcFsFilesystemsProperty();
    else if (name == "modules"_sv) return new ProcFsModulesProperty();
    else if (name == "mounts"_sv) return new ProcFsMountsProperty();
    else if (name == "partitions"_sv) return new ProcFsPartitionsProperty();
    else if (name == "uptime"_sv) return new ProcFsUptimeProperty();
    else if (name == "version"_sv) return new ProcFsVersionProperty();
    else if (name == "vm_regions"_sv) return new ProcFsMemoryRegionsProperty;

    return nullptr;
}

static ::Ref<ProcFsINode> CreateProcFsNode(::Ref<INode> parent, StringView name,
                                           Filesystem* filesystem)
{
    ProcFsProperty* property = CreateProcFsProperty(name);
    auto node = new ProcFsINode(name, filesystem, 0755 | S_IFREG, property);
    property->m_Parent = node;

    return node;
}

ProcFs::ProcFs(u32 flags)
    : Filesystem("ProcFs", flags)
{
}

ErrorOr<::Ref<DirectoryEntry>> ProcFs::Mount(StringView  sourcePath,
                                             const void* data)
{
    ScopedLock guard(m_Lock);
    if (m_Root) return Error(EEXIST);

    m_RootEntry          = CreateRef<DirectoryEntry>(nullptr, "/");
    // m_Root               = TryOrRet(AllocateNode("/", 0644 | S_IFDIR));
    m_Root               = new ProcFsRootINode("/", this);

    auto inode           = m_Root.As<ProcFsINode>();
    inode->m_Metadata.ID = 2;

    m_RootEntry->Bind(m_Root);
    m_RootEntry->SetParent(m_RootEntry);

    AddChild("cmdline");
    AddChild("filesystems");
    AddChild("modules");
    AddChild("mounts");
    AddChild("partitions");
    AddChild("uptime");
    AddChild("version");
    AddChild("vm_regions");

    return m_RootEntry;
}

ErrorOr<::Ref<INode>> ProcFs::AllocateNode(StringView name, INodeMode mode)
{
    auto inode = new ProcFsINode(name, this, NextINodeIndex(), mode);
    if (!inode) return Error(ENOMEM);

    return inode;
}
ErrorOr<::Ref<INode>> ProcFs::CreateNode(::Ref<INode>          parent,
                                         ::Ref<DirectoryEntry> entry,
                                         INodeMode mode, uid_t uid, gid_t gid)
{
    auto inode = CreateRef<ProcFsINode>(entry->Name(), this, mode, nullptr);
    entry->Bind(inode);

    return inode;
}
bool          ProcFs::Populate(DirectoryEntry* dentry) { return true; }
ErrorOr<void> ProcFs::Stats(statfs& stats)
{
    constexpr usize PROC_SUPER_MAGIC = 0x9fa0;

    stats.f_type                     = PROC_SUPER_MAGIC;
    stats.f_bsize                    = PMM::PAGE_SIZE;
    stats.f_bfree                    = 0;
    stats.f_bavail                   = 0;
    stats.f_ffree                    = 0;
    stats.f_namelen                  = 255;

    return {};
}

void ProcFs::AddChild(StringView name)
{
    auto entry = CreateRef<DirectoryEntry>(nullptr, name);
    auto inode = CreateProcFsNode(m_Root, name, this);
    entry->Bind(inode);

    if (inode)
    {
        m_Root->InsertChild(inode, name);
        m_RootEntry->InsertChild(entry);
    }

    entry->SetParent(m_RootEntry);
}

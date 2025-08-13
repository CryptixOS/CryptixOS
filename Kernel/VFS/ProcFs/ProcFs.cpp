/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/System.hpp>
#include <Boot/CommandLine.hpp>

#include <Drivers/Core/BlockDevice.hpp>
#include <Drivers/Core/DeviceManager.hpp>
#include <Prism/String/StringUtils.hpp>

#include <System/System.hpp>
#include <Time/Time.hpp>

#include <VFS/MountPoint.hpp>
#include <VFS/VFS.hpp>

#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/ProcFs/ProcFsINode.hpp>

UnorderedMap<pid_t, Process*> ProcFs::s_Processes;

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
        auto       process   = ProcFs::GetProcess(pid);
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

static ProcFsINode* CreateProcFsNode(INode* parent, StringView name,
                                     Filesystem* filesystem)
{
    ProcFsProperty* property = CreateProcFsProperty(name);
    auto node = new ProcFsINode(name, filesystem, 0755 | S_IFREG, property);
    property->m_Parent = node;

    return node;
}

Process* ProcFs::GetProcess(pid_t pid)
{
    auto it = s_Processes.Find(pid);
    if (it == s_Processes.end()) return nullptr;

    return it->Value;
}
void ProcFs::AddProcess(Process* process)
{
    ScopedLock guard(m_Lock);
    Assert(!s_Processes.Contains(process->Pid()));
    s_Processes[process->Pid()] = process;
    auto name                   = StringUtils::ToString(process->Pid());
    auto entry                  = CreateRef<DirectoryEntry>(nullptr, name);

    auto inode                  = reinterpret_cast<ProcFsINode*>(
        Try(AllocateNode(entry->Name(), S_IFDIR | 0755)));
    inode->m_Parent = m_Root;

    m_Lock.Release();
    m_Root->InsertChild(inode, entry->Name());
    m_Lock.Acquire();

    entry->Bind(inode);
    m_RootEntry->InsertChild(entry);
    entry->SetParent(m_RootEntry);
}
void ProcFs::RemoveProcess(pid_t pid)
{
    ScopedLock guard(m_Lock);
    s_Processes.Erase(pid);
}
ErrorOr<::Ref<DirectoryEntry>> ProcFs::Mount(StringView  sourcePath,
                                             const void* data)
{
    ScopedLock guard(m_Lock);
    if (m_Root) VFS::RecursiveDelete(m_Root);

    m_RootEntry          = CreateRef<DirectoryEntry>(nullptr, "/");
    m_Root               = TryOrRet(AllocateNode("/", 0644 | S_IFDIR));

    auto inode           = reinterpret_cast<ProcFsINode*>(m_Root);
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

ErrorOr<INode*> ProcFs::AllocateNode(StringView name, INodeMode mode)
{
    auto inode = new ProcFsINode(name, this, NextINodeIndex(), mode);
    if (!inode) return Error(ENOMEM);

    return inode;
}
ErrorOr<INode*> ProcFs::CreateNode(INode* parent, ::Ref<DirectoryEntry> entry,
                                   mode_t mode, uid_t uid, gid_t gid)
{
    auto inode = new ProcFsINode(entry->Name(), this, mode, nullptr);
    entry->Bind(inode);

    return inode;
}
bool          ProcFs::Populate(DirectoryEntry* dentry) { return true; }
ErrorOr<void> ProcFs::Stats(statfs& stats)
{
    constexpr usize PROC_SUPER_MAGIC = 0x9fa0;

    stats.f_type                     = PROC_SUPER_MAGIC;
    stats.f_bsize                    = PMM::PAGE_SIZE / sizeof(long);
    stats.f_bfree                    = 0;
    stats.f_bavail                   = 0;
    stats.f_ffree                    = 0;
    stats.f_namelen                  = 255;

    return {};
}

void ProcFs::AddChild(StringView name)
{
    auto entry = new DirectoryEntry(nullptr, name);
    auto inode = CreateProcFsNode(m_Root, name, this);
    entry->Bind(inode);

    if (inode)
    {
        m_Root->InsertChild(inode, name);
        m_RootEntry->InsertChild(entry);
    }

    entry->SetParent(m_RootEntry);
    m_RootEntry->InsertChild(entry);
}

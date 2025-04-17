/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/System.hpp>

#include <Drivers/DeviceManager.hpp>
#include <Library/Module.hpp>
#include <Prism/String/StringUtils.hpp>

#include <VFS/ProcFs/ProcFs.hpp>
#include <VFS/ProcFs/ProcFsINode.hpp>

std::unordered_map<pid_t, Process*> ProcFs::s_Processes;

struct ProcFsCmdLineProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.clear();

        StringView kernelPath = BootInfo::GetExecutableFile()->path;
        StringView cmdline    = BootInfo::GetKernelCommandLine();

        Buffer.resize(kernelPath.Size() + cmdline.Size() + 10);
        Write("{} {}\n", kernelPath, cmdline);
    }
};
struct ProcFsFilesystemsProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.clear();
        Buffer.resize(PMM::PAGE_SIZE);

        for (auto& [physical, fs] : VFS::GetFilesystems())
            Write("{} {}\n", physical ? "     " : "nodev", fs);
    }
};
struct ProcFsModulesProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.clear();
        Buffer.resize(PMM::PAGE_SIZE);

        for (auto& [name, module] : GetModules()) Write("{}\n", name);
    }
};
struct ProcFsMountsProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.clear();
        Buffer.resize(PMM::PAGE_SIZE);

        for (auto it = VFS::GetMountPoints().begin();
             it != VFS::GetMountPoints().end();)
        {
            auto&            mountPoint = *it;
            std::string_view mountPath  = mountPoint.first;
            Filesystem*      fs         = mountPoint.second;

            Write("{} {} {} {}\n", fs->GetDeviceName(), mountPath,
                  fs->GetName(), fs->GetMountFlagsString());
            ++it;
        }
    }
};
struct ProcFsPartitionsProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.clear();
        Buffer.resize(PMM::PAGE_SIZE);

        Write("major\tminor\t#blocks\tname\n");
        for (auto& partition : DeviceManager::GetBlockDevices())
        {
            u16         major      = partition->GetID();
            u16         minor      = partition->GetID();
            const stat& stats      = partition->GetStats();
            u64         blockCount = stats.st_blocks;
            StringView  name       = partition->GetName().data();

            Write("{}\t{}\t{}\t{}\n", major, minor, blockCount, name);
        }
    }
};
struct ProcFsVersionProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        Buffer.clear();
        Buffer.resize(PMM::PAGE_SIZE);

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
        Buffer.clear();
        Buffer.resize(PMM::PAGE_SIZE);

        auto process = Process::GetCurrent();

        Write("======================================================\n");
        Write("Base\t\tLength\t\tPhys\t\tProt\n");
        for (const auto& [base, region] : process->GetAddressSpace())
            Write("{:#x}\t\t{:#x}\t\t{:#x}\t\t{:#b}\n", base, region->GetSize(),
                  region->GetPhysicalBase().Raw(), region->GetProt());
        Write("======================================================\n");
    }
};
struct ProcFsStatusProperty : public ProcFsProperty
{
    virtual void GenerateRecord() override
    {
        auto  pidString = m_Parent->GetName();
        pid_t pid
            = StringUtils::ToNumber<pid_t>(pidString.data(), pidString.size());
        auto process = ProcFs::GetProcess(pid);
        if (!process) return;

        Write("Name: {}\n", process->GetName());
    }
};

static constexpr ProcFsProperty* CreateProcFsProperty(StringView name)
{
    if (name == "cmdline") return new ProcFsCmdLineProperty();
    else if (name == "filesystems") return new ProcFsFilesystemsProperty();
    else if (name == "modules") return new ProcFsModulesProperty();
    else if (name == "mounts") return new ProcFsMountsProperty();
    else if (name == "partitions") return new ProcFsPartitionsProperty();
    else if (name == "version") return new ProcFsVersionProperty();
    else if (name == "vm_regions") return new ProcFsMemoryRegionsProperty;

    return nullptr;
}

static ProcFsINode* CreateProcFsNode(INode* parent, StringView name,
                                     Filesystem* filesystem)
{
    ProcFsProperty* property = CreateProcFsProperty(name);
    auto            node
        = new ProcFsINode(parent, name, filesystem, 0755 | S_IFREG, property);
    property->m_Parent = node;

    return node;
}

Process* ProcFs::GetProcess(pid_t pid)
{
    auto it = s_Processes.find(pid);
    if (it == s_Processes.end()) return nullptr;

    return it->second;
}
void ProcFs::AddProcess(Process* process)
{
    ScopedLock guard(m_Lock);
    Assert(!s_Processes.contains(process->GetPid()));
    s_Processes[process->GetPid()] = process;
    auto  nodeName                 = std::to_string(process->GetPid());
    auto* processNode
        = new ProcFsINode(m_Root, nodeName.data(), this, 0755 | S_IFDIR);

    auto statusProperty = new ProcFsStatusProperty();
    auto statusNode     = new ProcFsINode(processNode, "status", this,
                                          0755 | S_IFREG, statusProperty);
    processNode->InsertChild(statusNode, statusNode->GetName());

    m_Root->InsertChild(processNode, processNode->GetName());
}
void ProcFs::RemoveProcess(pid_t pid)
{
    ScopedLock guard(m_Lock);
    s_Processes.erase(pid);
}
INode* ProcFs::Mount(INode* parent, INode* source, INode* target,
                     std::string_view name, const void* data)
{
    ScopedLock guard(m_Lock);
    m_MountData
        = data ? reinterpret_cast<void*>(strdup(static_cast<const char*>(data)))
               : nullptr;

    if (m_Root) VFS::RecursiveDelete(m_Root);
    m_Root = new ProcFsINode(parent, name.data(), this, 0755 | S_IFDIR);
    if (m_Root) m_MountedOn = target;

    AddChild("cmdline");
    AddChild("filesystems");
    AddChild("modules");
    AddChild("mounts");
    AddChild("partitions");
    AddChild("version");
    AddChild("vm_regions");

    m_MountedOn = target;

    return m_Root;
}
INode* ProcFs::CreateNode(INode* parent, std::string_view name, mode_t mode)
{
    return nullptr;
}
INode* ProcFs::Symlink(INode* parent, std::string_view name,
                       std::string_view target)
{
    return nullptr;
}
INode* ProcFs::Link(INode* parent, std::string_view name, INode* oldNode)
{
    return nullptr;
}
bool ProcFs::Populate(INode* node) { return true; }

void ProcFs::AddChild(StringView name)
{
    m_Root->InsertChild(CreateProcFsNode(m_Root, name, this), name.Raw());
}

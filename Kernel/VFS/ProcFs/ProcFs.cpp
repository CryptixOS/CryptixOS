/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <VFS/ProcFs/ProcFs.hpp>

std::unordered_map<pid_t, Process*> ProcFs::s_Processes;

void                                ProcFs::AddProcess(Process* process)
{
    ScopedLock guard(m_Lock);
    Assert(!s_Processes.contains(process->GetPid()));
    s_Processes[process->GetPid()] = process;
    auto  nodeName                 = std::to_string(process->GetPid());
    auto* processNode = new ProcFsINode(m_Root, nodeName, this, 0755 | S_IFDIR);

    m_Root->InsertChild(processNode, nodeName);
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
    m_Root = new ProcFsINode(parent, name, this, 0755 | S_IFDIR);
    if (m_Root) m_MountedOn = target;

    ProcFsProperty* mounts = new ProcFsProperty(
        [](std::string& buffer)
        {
            for (const auto& mountPoint : VFS::GetMountPoints())
            {
                std::string_view mountPath = mountPoint.first;
                Filesystem*      fs        = mountPoint.second;

                buffer += std::format("{} {} {} {}\n", fs->GetDeviceName(),
                                      mountPath, fs->GetName(),
                                      fs->GetMountFlagsString());
            }
        });
    ProcFsProperty* cmdline = new ProcFsProperty(
        [](std::string& buffer)
        {
            std::string_view kernelPath = BootInfo::GetExecutableFile()->path;
            std::string_view cmdline    = BootInfo::GetKernelCommandLine();

            buffer.reserve(kernelPath.size() + cmdline.size() + 10);

            buffer += BootInfo::GetExecutableFile()->path;
            buffer += " ";
            buffer += BootInfo::GetKernelCommandLine();
            buffer += "\n";
        });
    ProcFsProperty* filesystems = new ProcFsProperty(
        [](std::string& buffer)
        {
            for (auto& [physical, fs] : VFS::GetFilesystems())
            {
                buffer += physical ? "     " : "nodev";
                buffer += " ";
                buffer += fs;
                buffer += "\n";
            }
        });
    m_Root->InsertChild(new ProcFsINode(m_Root, "mounts", this, 0755, mounts),
                        "mounts");
    m_Root->InsertChild(new ProcFsINode(m_Root, "cmdline", this, 0755, cmdline),
                        "cmdline");
    m_Root->InsertChild(
        new ProcFsINode(m_Root, "filesystems", this, 0755, filesystems),
        "filesystems");
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

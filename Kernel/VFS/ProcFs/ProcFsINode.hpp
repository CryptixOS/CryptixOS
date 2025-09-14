/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Utility/Delegate.hpp>

#include <Prism/Memory/Buffer.hpp>
#include <Prism/String/StringView.hpp>

#include <VFS/INode.hpp>

class ProcFsINode;
struct ProcFsProperty
{
    using ProcFsGenPropertyFunc = Delegate<void(String&)>;
    Delegate<void(ProcFsProperty&)> GenProp;
    ProcFsINode*                    m_Parent = nullptr;
    String                          Buffer;
    usize                           Offset = 0;

    Spinlock                        Lock;

    ProcFsProperty()
    {
        GenProp.BindLambda([](ProcFsProperty&) {});
    }
    virtual ~ProcFsProperty() = default;

    template <typename... Args>
    void Write(fmt::format_string<Args...> format, Args&&... args)
    {
        if (Offset < Buffer.Size())
        {
            auto result = fmt::format_to_n(Buffer.Raw() + Offset,
                                           Buffer.Size() - Offset, format,
                                           Forward<Args>(args)...);
            if (Offset + result.size < Buffer.Size())
            {
                Offset += result.size;
                return;
            }
        }

        Offset = Buffer.Size();
    }
    isize Read(u8* outBuffer, off_t offset, usize count);

    template <typename F>
    explicit ProcFsProperty(F f)
    {
        GenProp.BindLambda(f);
    }

    virtual void GenerateRecord()
    {
        Buffer.Clear();
        GenProp(*this);
    }
    operator String&()
    {
        if (Buffer.Empty()) GenerateRecord();
        Buffer.ShrinkToFit();
        return Buffer;
    }
};

class ProcFsINode : public INode
{
  public:
    ProcFsINode(StringView name, class Filesystem* fs, INodeID id,
                INodeMode mode, ProcFsProperty* property = nullptr);
    ProcFsINode(StringView name, class Filesystem* fs, INodeMode mode,
                ProcFsProperty* property = nullptr);
    virtual ~ProcFsINode() override
    {
        if (m_Property) delete m_Property;
    }

    virtual ErrorOr<void>
    TraverseDirectories(::Ref<class DirectoryEntry> parent,
                        DirectoryIterator           iterator) override;
    virtual ErrorOr<::Ref<DirectoryEntry>>
                  Lookup(::Ref<DirectoryEntry> dentry) override;

    virtual void  InsertChild(::Ref<INode> node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    friend class ProcFs;

    virtual bool Populate() { return false; }

  protected:
    ProcFsProperty*                    m_Property = nullptr;
    UnorderedMap<String, ::Ref<INode>> m_Children;
    bool                               m_Populated = false;
};
class Process;
class ProcFsRootINode : public ProcFsINode
{
  public:
    ProcFsRootINode(StringView name, class Filesystem* fs);

    virtual bool Populate() override;
};
class ProcFsSelfLinkINode : public ProcFsINode
{
  public:
    ProcFsSelfLinkINode(StringView name, class Filesystem* fs);
    virtual ErrorOr<Path> ReadLink() override;
};
class ProcFsProcessINode : public ProcFsINode
{
  public:
    ProcFsProcessINode(StringView name, class Filesystem* fs, INodeID id,
                       INodeMode mode, ProcessID pid);

    virtual bool Populate() override;

  private:
    ProcessID m_ProcessID = -1;
};
class ProcFsFdINode : public ProcFsINode
{
  public:
    ProcFsFdINode(StringView name, class Filesystem* fs, ProcessID pid);

    virtual bool Populate() override;

  private:
    ProcessID m_ProcessID = -1;
};

class ProcFsSymlinkINode : public ProcFsINode
{
  public:
    ProcFsSymlinkINode(StringView name, class Filesystem* fs, INodeMode mode,
                       PathView target);

    virtual ErrorOr<Path> ReadLink() override;

  private:
    Path m_Target = ""_sv;
};

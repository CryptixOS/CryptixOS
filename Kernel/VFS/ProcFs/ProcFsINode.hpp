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
        if (Offset >= Buffer.Size()) Offset = 0;
        auto result
            = fmt::format_to_n(Buffer.Raw() + Offset, Buffer.Size() - Offset,
                               format, Forward<Args>(args)...);
        Offset += result.size;
    }
    isize Read(u8* outBuffer, off_t offset, usize count);

    template <typename F>
    ProcFsProperty(F f)
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
    ProcFsINode(StringView name, class Filesystem* fs, mode_t mode,
                ProcFsProperty* property = nullptr);
    virtual ~ProcFsINode() override
    {
        if (m_Property) delete m_Property;
    }

    virtual const stat Stats() override;
    virtual ErrorOr<void>
    TraverseDirectories(class DirectoryEntry* parent,
                        DirectoryIterator     iterator) override;

    virtual const UnorderedMap<StringView, INode*>& Children() const
    {
        return m_Children;
    }
    virtual void  InsertChild(INode* node, StringView name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

  private:
    ProcFsProperty*                        m_Property = nullptr;
    UnorderedMap<StringView, INode*> m_Children;
};

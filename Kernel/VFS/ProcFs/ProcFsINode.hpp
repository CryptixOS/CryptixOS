/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Vector.hpp>
#include <Prism/Delegate.hpp>

#include <Prism/Memory/Buffer.hpp>
#include <Prism/StringView.hpp>

#include <VFS/INode.hpp>

struct ProcFsProperty
{
    using ProcFsGenPropertyFunc = Delegate<void(std::string&)>;
    Delegate<void(ProcFsProperty&)> GenProp;
    std::string                     Buffer;
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
        if (Offset >= Buffer.size()) Offset = 0;
        auto result
            = fmt::format_to_n(Buffer.data() + Offset, Buffer.size() - Offset,
                               format, std::forward<Args>(args)...);
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
        Buffer.clear();
        GenProp(*this);
    }
    operator std::string&()
    {
        if (Buffer.empty()) GenerateRecord();
        Buffer.shrink_to_fit();
        return Buffer;
    }
};

class ProcFsINode : public INode
{
  public:
    ProcFsINode(INode* parent, StringView name, Filesystem* fs, mode_t mode,
                ProcFsProperty* property = nullptr);
    virtual ~ProcFsINode() override
    {
        if (m_Property) delete m_Property;
    }

    virtual const stat& GetStats() override;

    virtual void  InsertChild(INode* node, std::string_view name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    virtual ErrorOr<void>  ChMod(mode_t mode) override { return Error(ENOSYS); }

  private:
    ProcFsProperty* m_Property = nullptr;
};

/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Delegate.hpp>

#include <VFS/INode.hpp>

struct ProcFsProperty
{
    using ProcFsGenPropertyFunc = Delegate<void(std::string&)>;
    Delegate<void(std::string&)> GenProp;
    std::string                  String;

    ProcFsProperty()
    {
        GenProp.BindLambda([](std::string&) {});
    }
    template <typename F>
    ProcFsProperty(F f)
    {
        GenProp.BindLambda(f);
    }

    inline void GenerateProperty()
    {
        String.clear();
        GenProp(String);
    }
    operator std::string&()
    {
        if (String.empty()) GenerateProperty();
        String.shrink_to_fit();
        return String;
    }
};

class ProcFsINode : public INode
{
  public:
    ProcFsINode(INode* parent, std::string_view name, Filesystem* fs,
                mode_t mode, ProcFsProperty* property = nullptr);
    virtual ~ProcFsINode() override
    {
        if (m_Property) delete m_Property;
    }

    virtual const stat& GetStats() override;

    virtual void  InsertChild(INode* node, std::string_view name) override;
    virtual isize Read(void* buffer, off_t offset, usize bytes) override;
    virtual isize Write(const void* buffer, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Truncate(usize size) override;

    virtual ErrorOr<isize> ChMod(mode_t mode) override { return -1; }

  private:
    ProcFsProperty* m_Property = nullptr;
};

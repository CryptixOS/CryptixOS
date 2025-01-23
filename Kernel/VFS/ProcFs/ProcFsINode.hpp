/*
 * Created by v1tr10l7 on 23.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <VFS/INode.hpp>

struct ProcFsProperty
{
    using ProcFsGenPropertyFunc            = std::string (*)();
    ProcFsGenPropertyFunc GenerateProperty = []() -> std::string { return ""; };
    std::string           String;

    ProcFsProperty() = default;
    ProcFsProperty(ProcFsGenPropertyFunc genProp)
        : GenerateProperty(genProp)
    {
    }

    operator std::string&()
    {
        if (String.empty()) String = GenerateProperty();
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

  private:
    ProcFsProperty* m_Property = nullptr;
};

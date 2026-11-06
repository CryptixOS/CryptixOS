/*
 * Created by v1tr10l7 on 04.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/TTY/PTY.hpp>
#include <Drivers/TTY/TTY.hpp>

class MasterPTY final : public CharacterDevice
{
  public:
    explicit MasterPTY(::Ref<PTYPair> pair);

    virtual StringView     Name() const noexcept override { return m_Name; }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;
    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        return Read(out.Raw(), offset, count);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return Write(in.Raw(), offset, count);
    }
    virtual ErrorOr<isize> IoCtl(usize request, upointer arg) override;

    virtual void           Close(bool writer) override;

  private:
    ::Ref<PTYPair> m_Pair;
    String         m_Name;
};

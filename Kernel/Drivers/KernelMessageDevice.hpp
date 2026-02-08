/*
 * Created by v1tr10l7 on 25.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>
#include <Prism/Containers/RingBuffer.hpp>

class KernelMessageDevice final : public CharacterDevice
{
  public:
    static isize Write(StringView str);

    KernelMessageDevice();
    ~KernelMessageDevice() override;

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override;
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override;

  private:
    static KernelMessageDevice* s_Instance;

    RingBuffer                  m_Buffer;
};

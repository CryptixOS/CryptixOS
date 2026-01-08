/*
 * Created by v1tr10l7 on 08.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/Core/CharacterDevice.hpp>

class PTYMultiplexer : public CharacterDevice
{
  public:
    PTYMultiplexer();
    virtual ~PTYMultiplexer();

    static ErrorOr<void>   Initialize();

    virtual ErrorOr<File*> Open(File* file) override;

  private:
    static PTYMultiplexer* s_PTMX;
};

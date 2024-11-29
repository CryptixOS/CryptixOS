/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "API/Posix/Termios.hpp"
#include "Drivers/Device.hpp"

class TTY : public Device
{
  public:
    TTY(Terminal* terminal, usize minor);

    virtual std::string_view GetName() const noexcept override { return "tty"; }

    virtual isize Read(void* dest, off_t offset, usize bytes) override;
    virtual isize Write(const void* src, off_t offset, usize bytes) override;

    virtual i32   IoCtl(usize request, uintptr_t argp) override;

    static void   Initialize();

  private:
    Terminal* terminal = nullptr;
    termios   termios;
    pid_t     controlSid = -1;
    gid_t     pgid       = -1;
};

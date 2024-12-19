/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "API/Posix/Termios.hpp"
#include "Drivers/Device.hpp"

#include "Scheduler/Spinlock.hpp"

#include <deque>

class TTY : public Device
{
  public:
    TTY(Terminal* terminal, usize minor);

    inline static TTY*       GetCurrent() { return s_CurrentTTY; }

    void                     PutChar(char c);

    virtual std::string_view GetName() const noexcept override { return "tty"; }

    virtual isize Read(void* dest, off_t offset, usize bytes) override;
    virtual isize Write(const void* src, off_t offset, usize bytes) override;

    virtual i32   IoCtl(usize request, uintptr_t argp) override;

    static void   Initialize();

  private:
    static TTY*      s_CurrentTTY;

    Spinlock         spinlock;

    Terminal*        terminal = nullptr;
    termios          termios;
    pid_t            controlSid = -1;
    gid_t            pgid       = -1;
    std::deque<char> charBuffer;
};

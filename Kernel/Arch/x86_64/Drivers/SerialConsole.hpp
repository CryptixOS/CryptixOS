/*
 * Created by v1tr10l7 on 06.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Drivers/TTY/TTY.hpp>
#include <System/InterruptHandler.hpp>

// /dev/ttyS0: a terminal on COM1. Received bytes come in on IRQ 4 and go to
// the line discipline, output goes straight to the UART. Boot with
// 'console=ttyS0' to make it the console init runs the shell on, which is
// how the CI boot test types commands and reads their output.
class SerialConsole final : public TTY
{
  public:
    static SerialConsole* Create();

  protected:
    virtual ErrorOr<void>  TransmitChar(u64 c) override;
    virtual ErrorOr<isize> Transmit(StringView data) override;

  private:
    SerialConsole();

    ::Ref<InterruptDispatcher> m_Dispatcher = nullptr;

    void                     Put(char c);
    static IrqResult HandleInterrupt(::Device* device, ExecutionContext* ctx);
};

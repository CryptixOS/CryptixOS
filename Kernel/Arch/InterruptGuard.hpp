/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Arch/CPU.hpp>

class InterruptGuard final
{
  public:
    InterruptGuard(bool ints)
        : m_SavedInts(CPU::GetInterruptFlag())
    {
        CPU::SetInterruptFlag(ints);
    }
    ~InterruptGuard() { CPU::SetInterruptFlag(m_SavedInts); }

  private:
    bool m_SavedInts;
};

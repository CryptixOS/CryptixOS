/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Debug/Panic.hpp>

namespace PMM
{
    extern bool IsInitialized();
}

#define ExpectInterrupts(expected) Assert(CPU::GetInterruptFlag() == expected)
#define AssertPMM_Ready()          Assert(PMM::IsInitialized())

#include <Prism/Debug/Assertions.hpp>

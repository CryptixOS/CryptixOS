/*
 * Created by v1tr10l7 on 14.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/ELF/Definitions.hpp>

#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Math.hpp>

class StackBuilder
{
  public:
    explicit StackBuilder(Pointer stackTop)
        : m_StackCurrent(stackTop)
        , m_StackTop(stackTop)
    {
    }

    inline Pointer Write(const Pointer source, usize bytes)
    {
        m_StackCurrent = Pointer(m_StackCurrent).Offset<upointer*>(-bytes);
        Memory::Copy(m_StackCurrent, source, bytes);

        return m_StackCurrent;
    }
    inline Pointer Write(upointer value)
    {
        *(--m_StackCurrent) = value;

        return m_StackCurrent;
    }
    inline Pointer Write(ELF::AuxiliaryValueType aux, upointer value)
    {
        Write(value);
        Write(ToUnderlying(aux));

        return m_StackCurrent;
    }
    inline Pointer Align(usize alignment)
    {
        Pointer aligned = Math::AlignDown(upointer(m_StackCurrent), alignment);
        return m_StackCurrent = aligned, aligned;
    }

    inline Pointer Current() const { return m_StackCurrent; }
    inline Pointer Top() const { return m_StackTop; }

  private:
    upointer* m_StackCurrent = nullptr;
    Pointer   m_StackTop     = nullptr;
};

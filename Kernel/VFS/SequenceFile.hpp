/*
 * Created by v1tr10l7 on 28.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>

#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Pointer.hpp>

class SequenceFile
{
  public:
    /*
    virtual Pointer Start(isize* pos)           = 0;
    virtual Pointer Next(Pointer v, isize* pos) = 0;
    virtual void    Stop(Pointer v)             = 0;
    virtual bool    Show(Pointer v)             = 0;
*/

    SequenceFile()
    {
        m_Buffer.Resize(4096);
        m_Offset = 0;
    }

  private:
    Vector<char> m_Buffer;
    usize        m_Offset = 0;
};

/*
 * Created by v1tr10l7 on 10.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Memory/Pointer.hpp>

enum class PageAttributes : isize
{
    eRead              = Bit(0),
    eWrite             = Bit(1),
    eExecutable        = Bit(2),
    eUser              = Bit(3),
    eGlobal            = Bit(4),
    eLPage             = Bit(5),
    eLLPage            = Bit(6),

    eRW                = eRead | eWrite,
    eRWX               = eRW | eExecutable,
    eRWXU              = eRWX | eUser,

    eUncacheableStrong = Bit(8),
    eWriteCombining    = Bit(7) | Bit(7),
    eWriteThrough      = Bit(9),
    eWriteProtected    = Bit(9) | Bit(7),
    eWriteBack         = Bit(9) | Bit(8),
    eUncacheable       = Bit(7) | Bit(8) | Bit(9),
};

inline bool operator!(const PageAttributes& value)
{
    return !static_cast<usize>(value);
}
inline PageAttributes operator~(const PageAttributes& value)
{
    return static_cast<PageAttributes>(~static_cast<int>(value));
}

inline PageAttributes operator|(const PageAttributes& left,
                                const PageAttributes& right)
{
    return static_cast<PageAttributes>(static_cast<int>(left)
                                       | static_cast<int>(right));
}
inline isize operator&(const PageAttributes& left, const PageAttributes& right)
{
    return static_cast<isize>(static_cast<isize>(left)
                              & static_cast<isize>(right));
}
inline PageAttributes operator^(const PageAttributes& left,
                                const PageAttributes& right)
{
    return static_cast<PageAttributes>(static_cast<int>(left)
                                       ^ static_cast<int>(right));
}
inline PageAttributes& operator|=(PageAttributes&       left,
                                  const PageAttributes& right)
{
    return left = left | right;
}
inline PageAttributes& operator&=(PageAttributes&       left,
                                  const PageAttributes& right)
{
    return left = static_cast<PageAttributes>(left & right);
}
inline PageAttributes& operator^=(PageAttributes&       left,
                                  const PageAttributes& right)
{
    return left = left ^ right;
}

namespace Arch::VMM
{
    extern uintptr_t GetAddressMask();
}; // namespace Arch::VMM

struct PageTable;
class PageTableEntry final
{
  public:
    PageTableEntry() = default;

    inline Pointer Address() const
    {
        return m_Address & Arch::VMM::GetAddressMask();
    }
    inline bool GetFlag(u64 flag) const { return m_Address & flag; }
    inline u64  Flags() const
    {
        return m_Address & ~Arch::VMM::GetAddressMask();
    }

    inline void SetAddress(Pointer address)
    {
        m_Address &= ~Arch::VMM::GetAddressMask();
        m_Address |= address;
    }
    inline void SetFlags(u64 flags, bool enabled)
    {
        if (enabled) m_Address |= flags;
        else m_Address &= ~flags;
    }

    inline void Clear() { m_Address = 0; }

    bool        IsValid();
    bool        IsLarge();

  private:
    Pointer m_Address;
};

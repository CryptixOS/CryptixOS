/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

enum class DeliveryMode
{
    eFixed       = 0b000,
    eLowPriority = 0b001,
    eSmi         = 0b010,
    eNmi         = 0b100,
    eInit        = 0b101,
    eExtInt      = 0b111,
};

enum class DestinationMode
{
    ePhysical = 0,
    eLogical  = 1,
};

enum class IoApicRedirectionFlags
{
    eActiveLow      = BIT(0),
    eLevelTriggered = BIT(1),
    eMasked         = BIT(2),
};

inline IoApicRedirectionFlags operator~(const IoApicRedirectionFlags& value)
{
    return static_cast<IoApicRedirectionFlags>(~static_cast<int>(value));
}
inline IoApicRedirectionFlags operator&(const IoApicRedirectionFlags& left,
                                        const IoApicRedirectionFlags& right)
{
    return static_cast<IoApicRedirectionFlags>(static_cast<int>(left)
                                               & static_cast<int>(right));
}
inline IoApicRedirectionFlags operator|(const IoApicRedirectionFlags& left,
                                        const IoApicRedirectionFlags& right)
{
    return static_cast<IoApicRedirectionFlags>(static_cast<i32>(left)
                                               | static_cast<i32>(right));
}
inline IoApicRedirectionFlags& operator|=(IoApicRedirectionFlags&       left,
                                          const IoApicRedirectionFlags& right)
{
    return left = left | right;
}

struct IoApicRedirectionEntry
{
    union
    {
        u8                     Vector;
        DeliveryMode           DeliveryMode    : 3;
        DestinationMode        DestinationMode : 1;
        u8                     DeliveryStatus  : 1;
        IoApicRedirectionFlags Flags           : 4;
        u64                    Reserved        : 39;
        u64                    Destination     : 8;
    };
    union
    {
        u32 Low;
        u32 High;
    };
};

class IoApic
{
  public:
    IoApic() = default;
    IoApic(Pointer baseAddress, u32 gsiBase);

    void        MaskAll() {}

    static void SetIRQRedirect(u32 lapicID, u8 vector, u8 irq, bool status);
    static void SetGSIRedirect(u32 lapicID, u8 vector, u8 gsi, u16 flags,
                               bool status);

    static void Initialize();

  private:
    Pointer    m_BaseAddress           = 0;
    u32        m_GsiBase               = 0;
    usize      m_RedirectionEntryCount = 0;

    inline u32 Read(u32 reg) const
    {
        *m_BaseAddress.ToHigherHalf<volatile u32*>() = reg;

        return *m_BaseAddress.ToHigherHalf<Pointer>().Offset<volatile u32*>(16);
    }
    inline void Write(u32 reg, u32 value) const
    {
        *m_BaseAddress.ToHigherHalf<volatile u32*>() = reg;

        *m_BaseAddress.ToHigherHalf<Pointer>().Offset<volatile u32*>(16)
            = value;
    }

    inline usize   GetGsiCount() const { return m_RedirectionEntryCount; }

    static IoApic& GetIoApicForGsi(u32 gsi);
};

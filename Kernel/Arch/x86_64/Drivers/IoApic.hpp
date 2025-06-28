/*
 * Created by v1tr10l7 on 18.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>
#include <Prism/Containers/Vector.hpp>

#include <magic_enum/magic_enum.hpp>

enum class DeliveryMode
{
    eFixed       = 0b000,
    eLowPriority = 0b001,
    eSmi         = 0b010,
    eNmi         = 0b100,
    eInit        = 0b101,
    eExternal    = 0b111,
};

enum class DestinationMode
{
    ePhysical = 0,
    eLogical  = 1,
};

enum class IoApicRedirectionFlags
{
    eActiveLow      = Bit(0),
    eLevelTriggered = Bit(1),
    eMasked         = Bit(16),
};

inline u64& operator|=(u64& lhs, const IoApicRedirectionFlags rhs)
{
    lhs |= ToUnderlying(rhs);
    return lhs;
}

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

enum class PinPolarity
{
    eActiveHigh = 0,
    eActiveLow  = 1,
};
enum class InterruptTriggerMode
{
    eEdge  = 0,
    eLevel = 1,
};

union [[gnu::packed]] IoApicRedirectionEntry
{
    struct
    {
        u8                     Vector;
        DeliveryMode           DeliveryMode    : 3;
        DestinationMode        DestinationMode : 1;
        u8                     DeliveryStatus  : 1;
        PinPolarity            PinPolarity     : 1;
        u8                     RemoteIRR       : 1;
        InterruptTriggerMode   TriggerMode     : 1;
        bool                   Masked          : 1;
        IoApicRedirectionFlags Flags           : 4;
        u64                    Reserved        : 39;
        u64                    Destination     : 8;
    };
    struct
    {
        u32 Low;
        u32 High;
    };
};

enum class IoApicRegister
{
    eID                   = 0x00,
    // Bits 16-23 contain redirection entry count
    eVersion              = 0x01,
    eArbitrationID        = 0x02,
    eRedirectionTableLow  = 0x10,
    eRedirectionTableHigh = 0x11,
};

inline IoApicRegister operator+(u32 lhs, const IoApicRegister& rhs)
{
    lhs += ToUnderlying(rhs);

    return static_cast<IoApicRegister>(lhs);
}

class IoApic final
{
  public:
    IoApic() = default;
    IoApic(Pointer baseAddress, u32 gsiBase);

    inline static bool IsAnyEnabled()
    {
        bool enabled = false;
        for (const auto& ioapic : GetIoApics()) enabled |= ioapic.IsEnabled();

        return enabled;
    }
    inline bool IsEnabled() const { return m_Enabled; }

    inline void Enable() { m_Enabled = true; }
    void        Disable()
    {
        MaskAllEntries();
        m_Enabled = false;
    }

    void MaskAllEntries() const
    {
        for (usize i = m_GsiBase; i < m_GsiBase + m_RedirectionEntryCount; i++)
            MaskGsi(i);
    }
    void MaskGsi(u32 gsi) const;

    void SetRedirectionEntry(u32 gsi, IoApicRedirectionEntry& entry)
    {
        SetRedirectionEntry(gsi,
                            (static_cast<u64>(entry.High) << 32) | entry.Low);
    }
    void                   SetRedirectionEntry(u32 gsi, u64 entry);

    static Vector<IoApic>& GetIoApics();

    static void SetIrqRedirect(u32 lapicID, u8 vector, u8 irq, bool status);
    static void SetGsiRedirect(u32 lapicID, u8 vector, u8 gsi, u16 flags,
                               bool status);

    static void Initialize();

  private:
    bool          m_Enabled               = false;
    Pointer       m_BaseAddressPhys       = 0;
    Pointer       m_BaseAddressVirt       = 0;
    u32           m_GsiBase               = 0;

    volatile u32* m_RegisterSelect        = 0;
    volatile u32* m_RegisterWindow        = 0;

    usize         m_RedirectionEntryCount = 0;

    inline u32    Read(IoApicRegister reg) const
    {
        Assert(m_RegisterSelect);
        Assert(m_RegisterWindow);
        *m_RegisterSelect = ToUnderlying(reg);

        return *m_RegisterWindow;
    }
    inline void Write(IoApicRegister reg, u32 value) const
    {
        Assert(m_RegisterSelect);
        Assert(m_RegisterWindow);

        AssertFmt(reg != IoApicRegister::eVersion
                      && reg != IoApicRegister::eArbitrationID,
                  "Cannot write to read only register: '{}'",
                  magic_enum::enum_name(reg));
        *m_RegisterSelect = ToUnderlying(reg);

        *m_RegisterWindow = value;
    }

    inline usize   GetGsiCount() const { return m_RedirectionEntryCount; }

    static IoApic& GetIoApicForGsi(u32 gsi);
};

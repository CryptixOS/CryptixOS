/*
 * Created by v1tr10l7 on 09.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

enum class PageFaultReason
{
    eNotPresent             = Bit(0),
    eWrite                  = Bit(1),
    eUser                   = Bit(2),
    eReservedWrite          = Bit(3),
    eInstructionFetch       = Bit(4),
    eProtectionKey          = Bit(5),
    eShadowStack            = Bit(6),
    eSoftwareGuardExtension = Bit(7),
};

inline constexpr bool operator&(const PageFaultReason lhs,
                                const PageFaultReason rhs)
{
    return std::to_underlying(lhs) & std::to_underlying(rhs);
}
inline constexpr PageFaultReason& operator|=(PageFaultReason&      lhs,
                                             const PageFaultReason rhs)
{
    auto result = std::to_underlying(lhs) | std::to_underlying(rhs);
    lhs         = static_cast<PageFaultReason>(result);

    return lhs;
}

class PageFaultInfo
{
  public:
    inline constexpr PageFaultInfo(Pointer virt, PageFaultReason reason,
                                   struct CPUContext* context)
        : m_VirtualAddress(virt)
        , m_Reason(reason)
        , m_CPUContext(context)
    {
    }

    inline constexpr Pointer VirtualAddress() const { return m_VirtualAddress; }
    inline constexpr PageFaultReason Reason() const { return m_Reason; }
    inline constexpr CPUContext*     CPUContext() const { return m_CPUContext; }

  private:
    Pointer            m_VirtualAddress = nullptr;
    PageFaultReason    m_Reason;
    struct CPUContext* m_CPUContext = nullptr;
};

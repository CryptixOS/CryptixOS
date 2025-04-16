/*
 * Created by v1tr10l7 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Network/MacAddress.hpp>

#include <Prism/Core/Types.hpp>

enum class HardwareType : u16
{
    eEthernet = 0x0001,
};
enum class ProtocolType : u16
{
    eArp = 0x0800,
};
enum class ArpOpCode : u16
{
    eRequest = 0x0001,
    eReply   = 0x0002,
};

struct [[gnu::packed]] EthernetHeader
{
    u8  DestinationHardwareAddress[6];
    u8  SourceHardwareAddress[6];
    u16 EthernetType;
};
struct [[gnu::packed]] ArpPacket
{
    HardwareType HardwareType;
    ProtocolType ProtocolType;
    u8           HardwareAddressLength;
    u8           ProtocolAddressLength;
    ArpOpCode    OpCode;
    u8           SourceHardwareAddress[6];
    u32          SourceIPv4;
    u8           DestinationHardwareAddress[6];
    u32          DestIPv4;
};

class NetworkAdapter
{
  public:
    NetworkAdapter()          = default;
    virtual ~NetworkAdapter() = default;

    MacAddress&  GetMacAddress() { return m_MacAddress; }

    virtual bool SendPacket(const u8* data, usize length) = 0;

    static bool  RegisterNIC(NetworkAdapter* nic);

  protected:
    MacAddress m_MacAddress;
};

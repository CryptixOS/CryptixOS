/*
 * Created by v1tr10l7 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Core/Types.hpp>
#include <API/Posix/termios.h>

export module Cryptix:Network.NetworkAdapter;

export import :Network.MacAddress;

export enum class HardwareType : u16
{
    eEthernet = 0x0001,
};
export enum class ProtocolType : u16
{
    eArp = 0x0800,
};
export enum class ArpOpCode : u16
{
    eRequest = 0x0001,
    eReply   = 0x0002,
};

export struct [[gnu::packed]] EthernetHeader
{
    u8  DestinationHardwareAddress[6];
    u8  SourceHardwareAddress[6];
    u16 EthernetType;
};
export struct [[gnu::packed]] ArpPacket
{
    enum HardwareType HardwareType;
    enum ProtocolType ProtocolType;
    u8                HardwareAddressLength;
    u8                ProtocolAddressLength;
    ArpOpCode         OpCode;
    u8                SourceHardwareAddress[6];
    u32               SourceIPv4;
    u8                DestinationHardwareAddress[6];
    u32               DestIPv4;
};

export class NetworkAdapter
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

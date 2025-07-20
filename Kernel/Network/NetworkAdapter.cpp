/*
 * Created by v1tr10l7 on 21.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Network/NetworkAdapter.hpp>

uint32_t htonl(uint32_t hostlong)
{
    return ((hostlong & 0x000000FF) << 24) | // Move byte 0 to byte 3
           ((hostlong & 0x0000FF00) << 8) |  // Move byte 1 to byte 2
           ((hostlong & 0x00FF0000) >> 8) |  // Move byte 2 to byte 1
           ((hostlong & 0xFF000000) >> 24);  // Move byte 3 to byte 0
}
bool NetworkAdapter::RegisterNIC(NetworkAdapter* nic)
{
    return true;
    u8              packet[42]     = {0};

    EthernetHeader* ethernetHeader = reinterpret_cast<EthernetHeader*>(packet);
    Memory::Fill(ethernetHeader->DestinationHardwareAddress, 0xff, 6);
    Memory::Copy(ethernetHeader->SourceHardwareAddress,
                nic->GetMacAddress().Raw(), 6);
    ethernetHeader->EthernetType = __builtin_bswap16(0x0806);

    ArpPacket* arpHeader
        = reinterpret_cast<ArpPacket*>(packet + sizeof(EthernetHeader));
    arpHeader->HardwareType = static_cast<HardwareType>(
        __builtin_bswap16(ToUnderlying(HardwareType::eEthernet)));
    arpHeader->ProtocolType = static_cast<ProtocolType>(
        __builtin_bswap16(ToUnderlying(ProtocolType::eArp)));
    arpHeader->HardwareAddressLength = 6;
    arpHeader->ProtocolAddressLength = 4;
    arpHeader->OpCode                = static_cast<ArpOpCode>(
        __builtin_bswap16(ToUnderlying(ArpOpCode::eRequest)));

    Memory::Copy(arpHeader->SourceHardwareAddress, nic->GetMacAddress().Raw(),
                6);

    Memory::Fill(arpHeader->DestinationHardwareAddress, 0, 6);

    arpHeader->SourceIPv4 = __builtin_bswap32(0xC0A8007A);
    arpHeader->DestIPv4   = __builtin_bswap32(0xC0A8007C);
    nic->SendPacket(reinterpret_cast<const u8*>(arpHeader), 42);
    return true;
}

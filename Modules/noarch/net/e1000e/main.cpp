/*
 * Created by v1tr10l7 on 18.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <Library/Logger.hpp>
#include <Library/Module.hpp>

#include <Memory/MMIO.hpp>
#include <Network/NetworkAdapter.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/String/String.hpp>
#include <Prism/Utility/PathView.hpp>

#include <VFS/INode.hpp>
#include <VFS/VFS.hpp>

using namespace Prism;

namespace Intel
{
    constexpr usize VENDOR_ID               = 0x8086;
    constexpr usize E1000_QEMU_DEVICE_ID    = 0x100e;
    constexpr usize E1000_I217_DEVICE_ID    = 0x153a;
    constexpr usize E1000_82577LM_DEVICE_ID = 0x10ea;
}; // namespace Intel

namespace E1000e
{
    static Array s_IdTable = ToArray({
        PCI::DeviceID{Intel::VENDOR_ID, Intel::E1000_QEMU_DEVICE_ID,
                      PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{Intel::VENDOR_ID, Intel::E1000_I217_DEVICE_ID,
                      PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID, 2, 0},
        PCI::DeviceID{Intel::VENDOR_ID, Intel::E1000_82577LM_DEVICE_ID,
                      PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID, 2, 0},
    });

    enum class Register
    {
        eControl                 = 0x0000,
        eStatus                  = 0x0008,
        eEeProm                  = 0x0014,
        eControlExt              = 0x0018,
        eInterruptMask           = 0x00d0,

        eRControl                = 0x0100,
        eRxDescLow               = 0x2800,
        eRxDescHigh              = 0x2804,
        eRxDescSize              = 0x2808,
        eRxDescHead              = 0x2810,
        eRxDescTail              = 0x2818,

        eTControl                = 0x0400,
        eTxDescLow               = 0x3800,
        eTxDescHigh              = 0x3804,
        eTxDescSize              = 0x3808,
        eTxDescHead              = 0x3810,
        eTxDescTail              = 0x3818,

        eRxDelayTimer            = 0x2820,
        eRxDescControl           = 0x2828,
        eRxIntAbsoluteDelayTimer = 0x282c,
        eRxSmallPacketInterrupt  = 0x2c00,

        eTransmitInterPacketGap  = 0x0410,
    };

    struct [[gnu::packed]] RxDescriptor
    {
        volatile u64 Address;
        volatile u16 Length;
        volatile u16 Checksum;
        volatile u8  Status;
        volatile u8  Errors;
        volatile u16 Special;
    };
    struct [[gnu::packed]] TxDescriptor
    {
        volatile u64 Address;
        volatile u16 Length;
        volatile u16 Cso;
        volatile u8  Command;
        volatile u8  Status;
        volatile u8  Css;
        volatile u16 Special;
    };
    constexpr usize RX_DESCRIPTOR_COUNT = 32;
    constexpr usize TX_DESCRIPTOR_COUNT = 8;
#define RCTL_EN            (1 << 1)  // Receiver Enable
#define RCTL_SBP           (1 << 2)  // Store Bad Packets
#define RCTL_UPE           (1 << 3)  // Unicast Promiscuous Enabled
#define RCTL_MPE           (1 << 4)  // Multicast Promiscuous Enabled
#define RCTL_LPE           (1 << 5)  // Long Packet Reception Enable
#define RCTL_LBM_NONE      (0 << 6)  // No Loopback
#define RCTL_LBM_PHY       (3 << 6)  // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF    (0 << 8)  // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER (1 << 8)  // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH  (2 << 8)  // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36         (0 << 12) // Multicast Offset - bits 47:36
#define RCTL_MO_35         (1 << 12) // Multicast Offset - bits 46:35
#define RCTL_MO_34         (2 << 12) // Multicast Offset - bits 45:34
#define RCTL_MO_32         (3 << 12) // Multicast Offset - bits 43:32
#define RCTL_BAM           (1 << 15) // Broadcast Accept Mode
#define RCTL_VFE           (1 << 18) // VLAN Filter Enable
#define RCTL_CFIEN         (1 << 19) // Canonical Form Indicator Enable
#define RCTL_CFI           (1 << 20) // Canonical Form Indicator Bit Value
#define RCTL_DPF           (1 << 22) // Discard Pause Frames
#define RCTL_PMCF          (1 << 23) // Pass MAC Control Frames
#define RCTL_SECRC         (1 << 26) // Strip Ethernet CRC

// Buffer Sizes
#define RCTL_BSIZE_256     (3 << 16)
#define RCTL_BSIZE_512     (2 << 16)
#define RCTL_BSIZE_1024    (1 << 16)
#define RCTL_BSIZE_2048    (0 << 16)
#define RCTL_BSIZE_4096    ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192    ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384   ((1 << 16) | (1 << 25))

    // Transmit Command

#define CMD_EOP            (1 << 0) // End of Packet
#define CMD_IFCS           (1 << 1) // Insert FCS
#define CMD_IC             (1 << 2) // Insert Checksum
#define CMD_RS             (1 << 3) // Report Status
#define CMD_RPS            (1 << 4) // Report Packet Sent
#define CMD_VLE            (1 << 6) // VLAN Packet Enable
#define CMD_IDE            (1 << 7) // Interrupt Delay Enable

    // TCTL Register

#define TCTL_EN            (1 << 1)  // Transmit Enable
#define TCTL_PSP           (1 << 3)  // Pad Short Packets
#define TCTL_CT_SHIFT      4         // Collision Threshold
#define TCTL_COLD_SHIFT    12        // Collision Distance
#define TCTL_SWXOFF        (1 << 22) // Software XOFF Transmission
#define TCTL_RTLC          (1 << 24) // Re-transmit on Late Collision

#define TSTA_DD            (1 << 0) // Descriptor Done
#define TSTA_EC            (1 << 1) // Excess Collisions
#define TSTA_LC            (1 << 2) // Late Collision
#define LSTA_TU            (1 << 3) // Transmit Underrun

    class Adapter : public NetworkAdapter, PCI::Device
    {
      public:
        Adapter(PCI::DeviceAddress address)
            : PCI::Device(address)
        {
            m_Bar0 = GetBar(0);
            EnableBusMastering();
        }
        bool Start()
        {
            DetectEEProm();
            if (!ReadMacAddress()) return false;
            // TODO(v1tr10l7): StartLink();

            for (u32 i = 0; i < 0x80; i++)
                Write<u32>((Register)(0x5200 + i * 4), 0);
            EnableInterrupt();
            InitializeRx();
            InitializeTx();
            return true;
        }

        virtual bool SendPacket(const u8* data, usize length) override
        {
            return false;
        }

      private:
        PCI::Bar    m_Bar0;
        bool        m_EEPromExists = false;
        CTOS_UNUSED Array<RxDescriptor*, RX_DESCRIPTOR_COUNT> m_RxDescriptors;
        CTOS_UNUSED Array<TxDescriptor*, TX_DESCRIPTOR_COUNT> m_TxDescriptors;
        CTOS_UNUSED u16                                       m_RxCurrent;
        CTOS_UNUSED u16                                       m_TxCurrent;

        inline void                                           InitializeRx()
        {
            for (auto& rx : m_RxDescriptors)
            {
                rx          = new RxDescriptor;
                rx->Address = Pointer(new u8[8192 * 16]);
                rx->Status  = 0;
            }

            auto rx = Pointer(m_RxDescriptors.Raw()).FromHigherHalf();
            Write<u32>(Register::eTxDescLow, rx >> 32);
            Write<u32>(Register::eTxDescLow, rx & u32(~0));

            Write<u32>(Register::eRxDescLow, rx);
            Write<u32>(Register::eRxDescHigh, 0);

            Write<u32>(Register::eRxDescSize,
                       m_RxDescriptors.Size() * sizeof(RxDescriptor));

            Write<u32>(Register::eRxDescHead, 0);
            Write<u32>(Register::eRxDescTail, m_RxDescriptors.Size() - 1);

            m_RxCurrent = 0;
            // TODO(v1tr10l7): enable rx
            Write<u32>(Register::eRControl, RCTL_EN | RCTL_SBP | RCTL_UPE
                                                | RCTL_MPE | RCTL_LBM_NONE
                                                | RTCL_RDMTS_HALF | RCTL_BAM
                                                | RCTL_SECRC | RCTL_BSIZE_8192);
            Write<u32>(Register::eRControl, 0);
        }
        inline void InitializeTx()
        {
            u8*           tx = nullptr;
            TxDescriptor* descriptors;
            tx          = new u8[TX_DESCRIPTOR_COUNT + 16];
            descriptors = (TxDescriptor*)tx;

            for (u32 i = 0; i < TX_DESCRIPTOR_COUNT; i++)
            {
                m_TxDescriptors[i] = (TxDescriptor*)((u8*)descriptors + i * 16);
                m_TxDescriptors[i]->Address = 0;
                m_TxDescriptors[i]->Command = 0;
                m_TxDescriptors[i]->Status  = TSTA_DD;
            }

            Write<u32>(Register::eTxDescHigh, u64(tx) >> 32);
            Write<u32>(Register::eTxDescLow, u64(tx) & 0xFFFFFFFF);

            Write<u32>(Register::eTxDescSize, TX_DESCRIPTOR_COUNT * 16);

            Write<u32>(Register::eTxDescHead, 0);
            Write<u32>(Register::eTxDescTail, 0);

            m_TxCurrent = 0;
            Write<u32>(Register::eTControl,
                       TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT)
                           | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);
            Write<u32>(Register::eTControl, 0b0110000000000111111000011111010);
            Write<u32>(Register::eTransmitInterPacketGap, 0x0060200A);
        }
        void EnableInterrupt()
        {
            Write<u32>(Register::eInterruptMask, 0x1f6dc);
            Write<u32>(Register::eInterruptMask, 0xff & ~4);
            Read<u32>((Register)0xc0);
        }

        inline bool DetectEEProm()
        {
            Write<u32>(Register::eEeProm, 0x01);
            for (usize i = 0; i < 1000 && !m_EEPromExists; i++)
                m_EEPromExists = Read<u32>(Register::eEeProm) & 0x10;

            return m_EEPromExists;
        }
        inline bool ReadMacAddress()
        {
            if (m_EEPromExists)
            {
                u16 word        = ReadEEProm(0x00);
                m_MacAddress[0] = word & 0xff;
                m_MacAddress[1] = word >> 8;

                word            = ReadEEProm(0x01);
                m_MacAddress[2] = word & 0xff;
                m_MacAddress[3] = word >> 8;

                word            = ReadEEProm(0x02);
                m_MacAddress[4] = word & 0xff;
                m_MacAddress[5] = word >> 8;
                return true;
            }

            u8*  baseMac8  = m_Bar0.Address.Offset<u8*>(0x5400);
            u32* baseMac32 = m_Bar0.Address.Offset<u32*>(0x5400);
            if (!baseMac32[0]) return false;

            for (u8 i = 0; i < 6; i++) m_MacAddress[i] = baseMac8[i];
            return true;
        }

        inline u32 ReadEEProm(u8 offset)
        {
            if (m_EEPromExists)
            {
                Write<u32>(Register::eEeProm, (offset << 8u) | Bit(0));
                for (;;)
                {
                    u32 value = Read<u32>(Register::eEeProm);
                    if (!(value & Bit(4))) return (value >> 16) | 0xffff;
                }
            }

            Write<u32>(Register::eEeProm, (offset << 2u) | Bit(0));
            for (;;)
            {
                u32 value = Read<u32>(Register::eEeProm);
                if (!(value & Bit(1))) return (value >> 16) | 0xffff;
            }
        }

        template <UnsignedIntegral T>
        inline T Read(Register reg)
        {
            const usize registerOffset  = ToUnderlying(reg);
            const auto  registerAddress = m_Bar0.Address.Offset(registerOffset);

            return MMIO::Read<T>(registerAddress);
        }
        template <UnsignedIntegral T>
        inline void Write(Register reg, const T value)
        {
            const usize registerOffset  = ToUnderlying(reg);
            const auto  registerAddress = m_Bar0.Address.Offset(registerOffset);

            return MMIO::Write<T>(registerAddress, value);
        }
    };
} // namespace E1000e

static ErrorOr<void> ProbeDevice(PCI::DeviceAddress&  address,
                                 const PCI::DeviceID& id)
{
    LogTrace("E1000e: Detected pci nic device");
    auto nic = new E1000e::Adapter(address);

    if (!NetworkAdapter::RegisterNIC(nic))
    {
        LogError("E1000e: Failed to register nic device");
        return Error(ENODEV);
    }
    return {};
}
static void        RemoveDevice(PCI::Device& device) {}

static PCI::Driver s_Driver = {
    .Name = "e1000e",
    .MatchIDs
    = Span<PCI::DeviceID>(E1000e::s_IdTable.begin(), E1000e::s_IdTable.Size()),
    .Probe  = ProbeDevice,
    .Remove = RemoveDevice,
};

extern "C" bool ModuleInit() { return PCI::RegisterDriver(s_Driver); }

MODULE_INIT(e1000e, ModuleInit);

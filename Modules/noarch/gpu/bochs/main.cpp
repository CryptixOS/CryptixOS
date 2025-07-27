/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#ifdef CTOS_TARGET_X86_64
    #include <Arch/x86_64/IO.hpp>
#endif

#include <Drivers/Device.hpp>
#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/PCI.hpp>

#include <Library/Module.hpp>
#include <Prism/Containers/Array.hpp>

class BochsAdapter : public PCI::Device
{
  public:
    explicit BochsAdapter(PCI::DeviceAddress address)
        : Device(address)
    {
    }

    enum class VbeRegister
    {
        eID            = 0,
        eResolutionX   = 1,
        eResolutionY   = 2,
        eBitsPerPixel  = 3,
        eEnable        = 4,
        eBank          = 5,
        eVirtualWidth  = 6,
        eVirtualHeight = 7,
        eOffsetX       = 8,
        eOffsetY       = 9,
    };

    static ErrorOr<void> Probe(PCI::DeviceAddress&  address,
                               const PCI::DeviceID& id)
    {
        u16 vbeID = Read(VbeRegister::eID);
        if (vbeID < 0xb0c0 || vbeID > 0xb0c5) return Error(ENODEV);

        // BochsAdapter* bxvga = new BochsAdapter(address);
        return {};
    }

  private:
    constexpr static u16 VBE_DISPI_IOPORT_INDEX = 0x01ce;
    constexpr static u16 VBE_DISPI_IOPORT_DATA  = 0x01cf;

    inline static u16    Read(VbeRegister index)
    {
        IO::Out<word>(VBE_DISPI_IOPORT_INDEX, std::to_underlying(index));
        return IO::In<word>(VBE_DISPI_IOPORT_DATA);
    }
    inline static void Write(VbeRegister index, u16 value)
    {
        IO::Out<word>(VBE_DISPI_IOPORT_INDEX, std::to_underlying(index));
        IO::Out<word>(VBE_DISPI_IOPORT_DATA, value);
    }
};

static Array       s_BochsIdTable = ToArray({
    PCI::DeviceID{0x1234, 0x1111, PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID,
                  PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID},
    PCI::DeviceID{0x4321, 0x1111, PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID,
                  PCI::DeviceID::ANY_ID, PCI::DeviceID::ANY_ID},
});
static PCI::Driver s_BochsDriver  = {
     .Name     = "bxvga",
     .MatchIDs = std::span(s_BochsIdTable.begin(), s_BochsIdTable.end()),
     .Probe    = BochsAdapter::Probe,
     .Remove   = nullptr,
};

static bool ModuleInit() { return PCI::RegisterDriver(s_BochsDriver); }
MODULE_INIT(bxvga, ModuleInit);

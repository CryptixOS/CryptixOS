/*
 * Created by v1tr10l7 on 20.04.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <API/DeviceIDs.hpp>

#include <Drivers/ACPI/Bus.hpp>
#include <Drivers/ACPI/Driver.hpp>
#include <Drivers/Core/DeviceManager.hpp>
#include <Firmware/ACPI/ACPI.hpp>
#include <Library/Module.hpp>

CTOS_MODULE_AUTHOR("v1tr10l7");
CTOS_MODULE_DESCRIPTION("Generic 8250/16x50 serial driver");
CTOS_MODULE_LICENSE("GPL-3");
CTOS_MODULE_VERSION("0.2");

// This list is based on linux drivers/tty/serial/8250_pnp.c. Only the IDs are
// kept: Linux's per-entry flags don't fit in a StringView (a {"ID", flags}
// initializer picked the (pointer, length) constructor, which made every ID
// empty), and its wildcard entries ("PNPCXXX", "WACFXXX", ...) can't match,
// because the ACPI bus compares IDs exactly.
static auto     s_MatchIDs  = ToArray<StringView>({
    /* Archtek America Corp. */
     /* Archtek SmartLink Modem 3334BT Plug & Play */
     "AAC000F",
     /* Anchor Datacomm BV */
     /* SXPro 144 External Data Fax Modem Plug & Play */
     "ADC0001",
     /* SXPro 288 External Data Fax Modem Plug & Play */
     "ADC0002",
     /* PROLiNK 1456VH ISA PnP K56flex Fax Modem */
     "AEI0250",
     /* Actiontec ISA PNP 56K X2 Fax Modem */
     "AEI1240",
     /* Rockwell 56K ACF II Fax+Data+Voice Modem */
     "AKY1021",
     /* AZT3005 PnP SOUND DEVICE */
     "AZT4001",
     /* Best Data Products Inc. Smart One 336F PnP Modem */
     "BDP3336",
     /*  Boca Research */
     /* Boca Complete Ofc Communicator 14.4 Data-FAX */
     "BRI0A49",
     /* Boca Research 33,600 ACF Modem */
     "BRI1400",
     /* Boca 33.6 Kbps Internal FD34FSVD */
     "BRI3400",
     /* Boca 33.6 Kbps Internal FD34FSVD */
     "BRI0A49",
     /* Best Data Products Inc. Smart One 336F PnP Modem */
     "BDP3336",
     /* Computer Peripherals Inc */
     /* EuroViVa CommCenter-33.6 SP PnP */
     "CPI4050",
     /* Creative Labs */
     /* Creative Labs Phone Blaster 28.8 DSVD PnP Voice */
     "CTL3001",
     /* Creative Labs Modem Blaster 28.8 DSVD PnP Voice */
     "CTL3011",
     /* Davicom ISA 33.6K Modem */
     "DAV0336",
     /* Creative */
     /* Creative Modem Blaster Flash56 DI5601-1 */
     "DMB1032",
     /* Creative Modem Blaster V.90 DI5660 */
     "DMB2001",
     /* E-Tech */
     /* E-Tech CyberBULLET PC56RVP */
     "ETT0002",
     /* FUJITSU */
     /* Fujitsu 33600 PnP-I2 R Plug & Play */
     "FUJ0202",
     /* Fujitsu FMV-FX431 Plug & Play */
     "FUJ0205",
     /* Fujitsu 33600 PnP-I4 R Plug & Play */
     "FUJ0206",
     /* Fujitsu Fax Voice 33600 PNP-I5 R Plug & Play */
     "FUJ0209",
     /* Archtek America Corp. */
     /* Archtek SmartLink Modem 3334BT Plug & Play */
     "GVC000F",
     /* Archtek SmartLink Modem 3334BRV 33.6K Data Fax Voice */
     "GVC0303",
     /* Hayes */
     /* Hayes Optima 288 V.34-V.FC + FAX + Voice Plug & Play */
     "HAY0001",
     /* Hayes Optima 336 V.34 + FAX + Voice PnP */
     "HAY000C",
     /* Hayes Optima 336B V.34 + FAX + Voice PnP */
     "HAY000D",
     /* Hayes Accura 56K Ext Fax Modem PnP */
     "HAY5670",
     /* Hayes Accura 56K Ext Fax Modem PnP */
     "HAY5674",
     /* Hayes Accura 56K Fax Modem PnP */
     "HAY5675",
     /* Hayes 288, V.34 + FAX */
     "HAYF000",
     /* Hayes Optima 288 V.34 + FAX + Voice, Plug & Play */
     "HAYF001",
     /* IBM */
     /* IBM Thinkpad 701 Internal Modem Voice */
     "IBM0033",
     /* Intermec */
     /* Intermec CV60 touchscreen port */
     "PNP4972",
     /* Intertex */
     /* Intertex 28k8 33k6 Voice EXT PnP */
     "IXDC801",
     /* Intertex 33k6 56k Voice EXT PnP */
     "IXDC901",
     /* Intertex 28k8 33k6 Voice SP EXT PnP */
     "IXDD801",
     /* Intertex 33k6 56k Voice SP EXT PnP */
     "IXDD901",
     /* Intertex 28k8 33k6 Voice SP INT PnP */
     "IXDF401",
     /* Intertex 28k8 33k6 Voice SP EXT PnP */
     "IXDF801",
     /* Intertex 33k6 56k Voice SP EXT PnP */
     "IXDF901",
     /* Kortex International */
     /* KORTEX 28800 Externe PnP */
     "KOR4522",
     /* KXPro 33.6 Vocal ASVD PnP */
     "KORF661",
     /* Lasat */
     /* LASAT Internet 33600 PnP */
     "LAS4040",
     /* Lasat Safire 560 PnP */
     "LAS4540",
     /* Lasat Safire 336  PnP */
     "LAS5440",
     /* Microcom, Inc. */
     /* Microcom TravelPorte FAST V.34 Plug & Play */
     "MNP0281",
     /* Microcom DeskPorte V.34 FAST or FAST+ Plug & Play */
     "MNP0336",
     /* Microcom DeskPorte FAST EP 28.8 Plug & Play */
     "MNP0339",
     /* Microcom DeskPorte 28.8P Plug & Play */
     "MNP0342",
     /* Microcom DeskPorte FAST ES 28.8 Plug & Play */
     "MNP0500",
     /* Microcom DeskPorte FAST ES 28.8 Plug & Play */
     "MNP0501",
     /* Microcom DeskPorte 28.8S Internal Plug & Play */
     "MNP0502",
     /* Motorola */
     /* Motorola BitSURFR Plug & Play */
     "MOT1105",
     /* Motorola TA210 Plug & Play */
     "MOT1111",
     /* Motorola HMTA 200 (ISDN) Plug & Play */
     "MOT1114",
     /* Motorola BitSURFR Plug & Play */
     "MOT1115",
     /* Motorola Lifestyle 28.8 Internal */
     "MOT1190",
     /* Motorola V.3400 Plug & Play */
     "MOT1501",
     /* Motorola Lifestyle 28.8 V.34 Plug & Play */
     "MOT1502",
     /* Motorola Power 28.8 V.34 Plug & Play */
     "MOT1505",
     /* Motorola ModemSURFR External 28.8 Plug & Play */
     "MOT1509",
     /* Motorola Premier 33.6 Desktop Plug & Play */
     "MOT150A",
     /* Motorola VoiceSURFR 56K External PnP */
     "MOT150F",
     /* Motorola ModemSURFR 56K External PnP */
     "MOT1510",
     /* Motorola ModemSURFR 56K Internal PnP */
     "MOT1550",
     /* Motorola ModemSURFR Internal 28.8 Plug & Play */
     "MOT1560",
     /* Motorola Premier 33.6 Internal Plug & Play */
     "MOT1580",
     /* Motorola OnlineSURFR 28.8 Internal Plug & Play */
     "MOT15B0",
     /* Motorola VoiceSURFR 56K Internal PnP */
     "MOT15F0",
     /* Com 1 */
     /*  Deskline K56 Phone System PnP */
     "MVX00A1",
     /* PC Rider K56 Phone System PnP */
     "MVX00F2",
     /* NEC 98NOTE SPEAKER PHONE FAX MODEM(33600bps) */
     "nEC8241",
     /* Pace 56 Voice Internal Plug & Play Modem */
     "PMC2430",
     /* Generic */
     /* Generic standard PC COM port	 */
     "PNP0500",
     /* Generic 16550A-compatible COM port */
     "PNP0501",
     /* Compaq 14400 Modem */
     "PNPC000",
     /* Compaq 2400/9600 Modem */
     "PNPC001",
     /* Dial-Up Networking Serial Cable between 2 PCs */
     "PNPC031",
     /* Dial-Up Networking Parallel Cable between 2 PCs */
     "PNPC032",
     /* Standard 9600 bps Modem */
     "PNPC100",
     /* Standard 14400 bps Modem */
     "PNPC101",
     /*  Standard 28800 bps Modem*/
     "PNPC102",
     /*  Standard Modem*/
     "PNPC103",
     /*  Standard 9600 bps Modem*/
     "PNPC104",
     /*  Standard 14400 bps Modem*/
     "PNPC105",
     /*  Standard 28800 bps Modem*/
     "PNPC106",
     /*  Standard Modem */
     "PNPC107",
     /* Standard 9600 bps Modem */
     "PNPC108",
     /* Standard 14400 bps Modem */
     "PNPC109",
     /* Standard 28800 bps Modem */
     "PNPC10A",
     /* Standard Modem */
     "PNPC10B",
     /* Standard 9600 bps Modem */
     "PNPC10C",
     /* Standard 14400 bps Modem */
     "PNPC10D",
     /* Standard 28800 bps Modem */
     "PNPC10E",
     /* Standard Modem */
     "PNPC10F",
     /* Standard PCMCIA Card Modem */
     "PNP2000",
     /* Rockwell */
     /* Modular Technology */
     /* Rockwell 33.6 DPF Internal PnP */
     /* Modular Technology 33.6 Internal PnP */
     "ROK0030",
     /* Kortex International */
     /* KORTEX 14400 Externe PnP */
     "ROK0100",
     /* Rockwell 28.8 */
     "ROK4120",
     /* Viking Components, Inc */
     /* Viking 28.8 INTERNAL Fax+Data+Voice PnP */
     "ROK4920",
     /* Rockwell */
     /* British Telecom */
     /* Modular Technology */
     /* Rockwell 33.6 DPF External PnP */
     /* BT Prologue 33.6 External PnP */
     /* Modular Technology 33.6 External PnP */
     "RSS00A0",
     /* Viking 56K FAX INT */
     "RSS0262",
     /* K56 par,VV,Voice,Speakphone,AudioSpan,PnP */
     "RSS0250",
     /* SupraExpress 28.8 Data/Fax PnP modem */
     "SUP1310",
     /* SupraExpress 336i PnP Voice Modem */
     "SUP1381",
     /* SupraExpress 33.6 Data/Fax PnP modem */
     "SUP1421",
     /* SupraExpress 33.6 Data/Fax PnP modem */
     "SUP1590",
     /* SupraExpress 336i Sp ASVD */
     "SUP1620",
     /* SupraExpress 33.6 Data/Fax PnP modem */
     "SUP1760",
     /* SupraExpress 56i Sp Intl */
     "SUP2171",
     /* Phoebe Micro */
     /* Phoebe Micro 33.6 Data Fax 1433VQH Plug & Play */
     "TEX0011",
     /* Archtek America Corp. */
     /* Archtek SmartLink Modem 3334BT Plug & Play */
     "UAC000F",
     /* 3Com Corp. */
     /* Gateway Telepath IIvi 33.6 */
     "USR0000",
     /* U.S. Robotics Sporster 33.6K Fax INT PnP */
     "USR0002",
     /*  Sportster Vi 14.4 PnP FAX Voicemail */
     "USR0004",
     /* U.S. Robotics 33.6K Voice INT PnP */
     "USR0006",
     /* U.S. Robotics 33.6K Voice EXT PnP */
     "USR0007",
     /* U.S. Robotics Courier V.Everything INT PnP */
     "USR0009",
     /* U.S. Robotics 33.6K Voice INT PnP */
     "USR2002",
     /* U.S. Robotics 56K Voice INT PnP */
     "USR2070",
     /* U.S. Robotics 56K Voice EXT PnP */
     "USR2080",
     /* U.S. Robotics 56K FAX INT */
     "USR3031",
     /* U.S. Robotics 56K FAX INT */
     "USR3050",
     /* U.S. Robotics 56K Voice INT PnP */
     "USR3070",
     /* U.S. Robotics 56K Voice EXT PnP */
     "USR3080",
     /* U.S. Robotics 56K Voice INT PnP */
     "USR3090",
     /* U.S. Robotics 56K Message  */
     "USR9100",
     /* U.S. Robotics 56K FAX EXT PnP*/
     "USR9160",
     /* U.S. Robotics 56K FAX INT PnP*/
     "USR9170",
     /* U.S. Robotics 56K Voice EXT PnP*/
     "USR9180",
     /* U.S. Robotics 56K Voice INT PnP*/
     "USR9190",
     /* Compaq touchscreen */
     "FPI2002",
     /* Fujitsu Stylistic touchscreens */
     "FUJ02B2",
     "FUJ02B3",
     /* Fujitsu Stylistic LT touchscreens */
     "FUJ02B4",
     /* Passive Fujitsu Stylistic touchscreens */
     "FUJ02B6",
     "FUJ02B7",
     "FUJ02B8",
     "FUJ02B9",
     "FUJ02BC",
     /* Fujitsu Wacom Tablet PC device */
     "FUJ02E5",
     /* Fujitsu P-series tablet PC device */
     "FUJ02E6",
     /* Fujitsu Wacom 2FGT Tablet PC device */
     "FUJ02E7",
     /* Fujitsu Wacom 1FGT Tablet PC device */
     "FUJ02E9",
     /*
      * LG C1 EXPRESS DUAL (C1-PB11A3) touch screen (actually a FUJ02E6 in
      * disguise)
      */
     "LTS0001",
     /* Rockwell's (PORALiNK) 33600 INT PNP */
     "WCI0003",
});

using namespace Prism;

class SerialDevice
{
  public:
    static ErrorOr<void> Probe(ACPI::DeviceHandle* handle, StringView)
    {
        LogModuleTrace("SerialPnp", "Probing 8250/16550 serial device...");
        return Error(ENOSYS);
    }
    static void Remove(ACPI::DeviceHandle*) {}
};

static ACPI::Driver s_Driver = {
    .Name     = "serial"_s,
    .MatchIDs = Span(s_MatchIDs.begin(), s_MatchIDs.Size()),
    .Hook     = {},
    .Probe    = SerialDevice::Probe,
    .Remove   = SerialDevice::Remove,
};
extern "C" CTOS_EXPORT bool ModuleInit()
{
    LogTrace("SerialPnp: Initializing the module");
    return ACPI::Bus::RegisterDriver(&s_Driver).operator bool();
}
MODULE_INIT(SerialPnp, ModuleInit);

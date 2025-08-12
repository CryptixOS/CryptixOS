/*
 * Created by v1tr10l7 on 10.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/ACPI/Driver.hpp>
#include <Library/Module.hpp>

CTOS_MODULE_AUTHOR("v1tr10l7");
CTOS_MODULE_DESCRIPTION("high performance event timer driver");
CTOS_MODULE_LICENSE("GPL-3");
CTOS_MODULE_VERSION("0.2");

class HPET
{
  public:
    static ErrorOr<void> Probe(ACPI::DeviceHandle* handle, StringView)
    {
        return {};
    }
    static void Remove(ACPI::DeviceHandle* handle) {}
};

static auto         s_MatchIDs = ToArray({"PNP0103"_sv});
static ACPI::Driver s_Driver   = {
      .Name     = "hpet"_s,
      .MatchIDs = Span(s_MatchIDs.begin(), s_MatchIDs.Size()),
      .Hook     = {},
      .Probe    = HPET::Probe,
      .Remove   = HPET::Remove,
};
extern "C" CTOS_EXPORT bool ModuleInit() { return true; }
MODULE_INIT(hpet, ModuleInit);

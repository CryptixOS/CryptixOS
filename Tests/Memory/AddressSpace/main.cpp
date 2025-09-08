/*
 * Created by v1tr10l7 on 12.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Memory/AddressSpace.hpp>
#include <Memory/Region.hpp>

void Test_AllocateBasicRegion()
{
    AddressSpace space;
    auto         region = space.AllocateRegion(0x1000); // 4KiB

    assert(region != nullptr);
    assert(region->Size() == 0x1000);
    assert(space.Contains(region->VirtualBase()));
}

void Test_AllocateAlignedRegion()
{
    AddressSpace space;
    usize        align  = 0x2000;

    auto         region = space.AllocateRegion(0x1000, align);
    assert(region != nullptr);
    assert(region->VirtualBase().Raw() % align == 0);
}

void Test_AllocateMultipleNonOverlapping()
{
    AddressSpace space;

    auto         r1 = space.AllocateRegion(0x1000);
    auto         r2 = space.AllocateRegion(0x2000);
    auto         r3 = space.AllocateRegion(0x3000);

    assert(r1 != nullptr && r2 != nullptr && r3 != nullptr);
    assert(r1->VirtualBase() != r2->VirtualBase());
    assert(r2->VirtualBase() != r3->VirtualBase());
}

void Test_AllocateFixedSuccess()
{
    AddressSpace space;
    Pointer      addr{0x50000000};

    auto         region = space.AllocateFixed(addr, 0x1000);
    assert(region != nullptr);
    assert(region->VirtualBase() == addr);
}

void Test_AllocateFixedConflict()
{
    AddressSpace space;
    Pointer      addr{0x60000000};

    auto         r1 = space.AllocateFixed(addr, 0x2000);
    auto r2 = space.AllocateFixed(addr.Offset(0x1000), 0x2000); // Overlaps

    assert(r1 != nullptr);
    assert(r2 == nullptr); // Should fail
}

void Test_EraseRegion()
{
    AddressSpace space;
    auto         region = space.AllocateRegion(0x1000);
    Pointer      addr   = region->VirtualBase();

    assert(space.Contains(addr));

    space.Erase(addr);
    assert(!space.Contains(addr));
}

void Test_FindValid()
{
    AddressSpace space;
    auto         region = space.AllocateRegion(0x4000);
    auto         base   = region->VirtualBase();

    assert(space.Find(base.Offset(0x1000)) == region);
    assert(space.Find(base.Offset(0x3FFF)) == region);
}

void Test_FindInvalid()
{
    AddressSpace space;
    assert(space.Find(Pointer(0xDEADBEEF)) == nullptr);
}

void RunAddressSpaceTests()
{
    Test_AllocateBasicRegion();
    Test_AllocateAlignedRegion();
    Test_AllocateMultipleNonOverlapping();
    Test_AllocateFixedSuccess();
    Test_AllocateFixedConflict();
    Test_EraseRegion();
    Test_FindValid();
    Test_FindInvalid();

    LogInfo("All AddressSpace tests passed.");
}

/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Memory/VMM.hpp"

struct TTBR
{
    PageTableEntry entries[512]{};
} __attribute__((packed));

struct PageTable
{
    TTBR* ttbr0;
    TTBR* ttbr1;
};

[[maybe_unused]] constexpr usize PA_RANGE_BITS32 = 0xb0000;
[[maybe_unused]] constexpr usize PA_RANGE_BITS36 = 0xb0001;
[[maybe_unused]] constexpr usize PA_RANGE_BITS40 = 0xb0010;
[[maybe_unused]] constexpr usize PA_RANGE_BITS42 = 0xb0011;
[[maybe_unused]] constexpr usize PA_RANGE_BITS44 = 0xb0100;
[[maybe_unused]] constexpr usize PA_RANGE_BITS48 = 0xb0101;
[[maybe_unused]] constexpr usize PA_RANGE_BITS52 = 0xb0110;

union MMFR0
{
    struct
    {
        u64 physRange : 4;
        u64 asidBits  : 4;
        u64 biGend    : 4;
        u64 snsMemory : 4;
        u64 bigEndEl0 : 4;
        u64 tGran16   : 4;
        u64 tGran64   : 4;
        u64 tGran4    : 4;
        u8  tGran16_2 : 4;
        u8  tGran64_2 : 4;
        u8  tGran4_2  : 4;
        u8  exS       : 4;
        u8  reserved;
        u8  fgt : 4;
        u8  ecv : 4;
    } __attribute__((packed));
    u64 value;
};
static_assert(sizeof(MMFR0) == 8);

union MMFR2
{
    struct
    {
        u64 cnp       : 4;
        u64 uao       : 4;
        u64 lsm       : 4;
        u64 iesb      : 4;
        u64 virtRange : 4; // 0b0000: 48bit va, 0b0001: 52bit va with 64kb
        u64 ccIdx     : 4;
        u64 nv        : 4;
        u64 st        : 4;
        u64 at        : 4;
        u64 ids       : 4;
        u64 fwb       : 4;
        u64 reserved  : 4;
        u64 ttl       : 4;
        u64 bbm       : 4;
        u64 evt       : 4;
        u64 e0pd      : 4;
    } __attribute__((packed));
    u64 value;
};
static_assert(sizeof(MMFR2) == 8);

static constexpr size_t PAGE_SIZE_4KIB  = 4_kib;
static constexpr size_t PAGE_SIZE_16KIB = 16_kib;
static constexpr size_t PAGE_SIZE_64KIB = 64_kib;

namespace Arch::VMM
{
    constexpr usize                  VALID           = (1 << 0);
    constexpr usize                  TABLE           = (1 << 1);
    [[maybe_unused]] constexpr usize BLOCK           = (0 << 1);
    constexpr usize                  PAGE            = Bit(1);

    constexpr usize                  USER            = Bit(6);

    [[maybe_unused]] constexpr usize RW              = (0 << 7);
    constexpr usize                  RO              = (1 << 7);

    constexpr usize                  ACCESS          = (1 << 10);
    constexpr usize                  NOTGLOBAL       = (1 << 11);
    constexpr usize                  EXECNEVER       = (1ul << 54);

    [[maybe_unused]] constexpr usize NONSHARE        = (0 << 8);
    constexpr usize                  OUTSHARE        = (0b10 << 8);
    constexpr usize                  INSHARE         = (0b11 << 8);

    constexpr usize                  WB              = (0b00 << 2) | INSHARE;
    [[maybe_unused]] constexpr usize NC              = (0b01 << 2) | OUTSHARE;
    [[maybe_unused]] constexpr usize WT              = (0b10 << 2) | OUTSHARE;

    static usize                     vaWidth         = 0;
    static usize                     pageSize        = 0;
    static usize                     lPageSize       = 0;
    static usize                     llPageSize      = 0;

    uintptr_t                        pteAddressMask  = 0;
    uintptr_t                        defaultPteFlags = VALID | TABLE;

    void                             Initialize()
    {
        MMFR0 mmfr0;
        MMFR2 mmfr2;
        u64   tcrEl1 = 0;

        __asm__ volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(mmfr0.value));
        __asm__ volatile("mrs %0, id_aa64mmfr2_el1" : "=r"(mmfr2.value));
        __asm__ volatile("mrs %0, tcr_el1" : "=r"(tcrEl1));

        bool featLpa = (mmfr0.physRange == PA_RANGE_BITS52);
        bool featLva = (mmfr2.virtRange == 0b0001);

        if (mmfr0.tGran4 != 0b1111) pageSize = PAGE_SIZE_4KIB;
        else if (mmfr0.tGran16 != 0b0000) pageSize = PAGE_SIZE_16KIB;
        else if (mmfr0.tGran64 == 0b0000) pageSize = PAGE_SIZE_64KIB;
        else Panic("VMM: Unknown page size");

        if (BootInfo::GetPagingMode() == 1
            && (tcrEl1 & Bit(59) && featLpa
                && ((pageSize == PAGE_SIZE_64KIB && featLva)
                    || (pageSize == PAGE_SIZE_16KIB && mmfr0.tGran16 == 0b0010)
                    || (pageSize == PAGE_SIZE_4KIB && mmfr0.tGran4 == 0b0001))))
            vaWidth = 52;
        else vaWidth = 48;

        std::pair<std::pair<usize, usize>, uintptr_t> map[]
            = {{{PAGE_SIZE_4KIB, 48}, 0x0000FFFFFFFFF000},
               {{PAGE_SIZE_16KIB, 48}, 0x0000FFFFFFFFC000},
               {{PAGE_SIZE_64KIB, 48}, 0x0000FFFFFFFF0000},

               {{PAGE_SIZE_4KIB, 52}, 0x0003FFFFFFFFF000},
               {{PAGE_SIZE_16KIB, 52}, 0x0003FFFFFFFFC000},
               {{PAGE_SIZE_64KIB, 52}, 0x0000FFFFFFFF0000}};

        for (usize i = 0; i < std::size(map); i++)
        {
            auto& entry = map[i];
            if (entry.first.first == pageSize && entry.first.second == vaWidth)
                pteAddressMask = entry.second;
        }

        Assert(pteAddressMask);
    }

    void*          AllocatePageTable() { return new TTBR; }
    void           DestroyPageMap(PageMap* pageMap) { ToDo(); }

    PageAttributes FromNativeFlags(usize flags)
    {
        PageAttributes attributes{};

        if (flags & VALID) attributes |= PageAttributes::eRead;
        if (!(flags & RO)) attributes |= PageAttributes::eWrite;
        if (!(flags & EXECNEVER)) attributes |= PageAttributes::eExecutable;
        if (flags & USER) attributes |= PageAttributes::eUser;
        if (!(flags & NOTGLOBAL)) attributes |= PageAttributes::eGlobal;

        if (flags & WB) attributes |= PageAttributes::eWriteBack;

        return attributes;
    }
    usize ToNativeFlags(PageAttributes flags)
    {
        usize ret = VALID | ACCESS;
        if ((flags & PageAttributes::eWrite) == 0) ret |= RO;
        if ((flags & PageAttributes::eExecutable) == 0) ret |= EXECNEVER;

        if (flags & PageAttributes::eUser) ret |= USER;
        if ((flags & PageAttributes::eGlobal) == 0) ret |= NOTGLOBAL;

        if ((flags & PageAttributes::eLPage) == 0
            && (flags & PageAttributes::eLLPage) == 0)
            ret |= PAGE;

        ret |= WB;
        return ret;
    }

    uintptr_t GetAddressMask() { return pteAddressMask; }
    usize     GetPageSize(PageAttributes flags)
    {
        if (flags & PageAttributes::eLLPage) return llPageSize;
        if (flags & PageAttributes::eLPage) return lPageSize;

        return pageSize;
    }
}; // namespace Arch::VMM
using namespace Arch::VMM;

namespace VirtualMemoryManager
{
    void SaveCurrentPageMap(PageMap& pageMap)
    {
        u64 value = 0;
        __asm__ volatile("mrs %0, ttbr0_el1" : "=r"(value));
        pageMap.GetTopLevel()->ttbr0 = ToHigherHalfAddress<TTBR*>(value);
        __asm__ volatile("mrs %0, ttbr1_el1" : "=r"(value));
        pageMap.GetTopLevel()->ttbr1 = ToHigherHalfAddress<TTBR*>(value);
    }
    void LoadPageMap(PageMap& pageMap, bool hh = true)
    {
        __asm__ volatile(
            "msr ttbr0_el1, %0" ::"r"(FromHigherHalfAddress<uintptr_t>(
                reinterpret_cast<uintptr_t>(pageMap.GetTopLevel()->ttbr0))));
        if (hh == true)
            __asm__ volatile("msr ttbr1_el1, %0" ::"r"(
                FromHigherHalfAddress<uintptr_t>(reinterpret_cast<uintptr_t>(
                    pageMap.GetTopLevel()->ttbr1))));
    }
}; // namespace VirtualMemoryManager

using namespace VirtualMemoryManager;

PageMap::PageMap()
    : topLevel(new PageTable{new TTBR, nullptr})
{
    llPageSize     = Arch::VMM::pageSize * 512 * 512;
    lPageSize      = Arch::VMM::pageSize * 512;
    this->pageSize = Arch::VMM::pageSize;

    if (!VMM::GetKernelPageMap()) topLevel->ttbr1 = new TTBR;
    else topLevel->ttbr1 = VMM::GetKernelPageMap()->topLevel->ttbr1;
}

bool            PageTableEntry::IsValid() { return GetFlag(VALID); }
bool            PageTableEntry::IsLarge() { return !GetFlag(TABLE); }

PageTableEntry* PageMap::Virt2Pte(PageTable* topLevel, uintptr_t virt,
                                  bool allocate, u64 pageSize)
{
    usize pml5Entry = (virt & (0x1ffull << 48)) >> 48;
    usize pml4Entry = (virt & (0x1ffull << 39)) >> 39;
    usize pml3Entry = (virt & (0x1ffull << 30)) >> 30;
    usize pml2Entry = (virt & (0x1ffull << 21)) >> 21;
    usize pml1Entry = (virt & (0x1ffull << 12)) >> 12;

    TTBR* half = (virt & (1ull << 63ull)) ? topLevel->ttbr1 : topLevel->ttbr0;
    if (!half) return nullptr;

    TTBR* pml4 = static_cast<TTBR*>(
        (BootInfo::GetPagingMode() == 1)
            ? GetNextLevel(half->entries[pml5Entry], allocate)
            : half);
    if (!pml4) return nullptr;

    TTBR* pml3 = static_cast<TTBR*>(
        GetNextLevel(pml4->entries[pml4Entry], allocate, pageSize));
    if (!pml3) return nullptr;
    if (pageSize == llPageSize /*|| pml3->entries[pml3Entry].IsLarge()*/)
        return &pml3->entries[pml3Entry];

    TTBR* pml2 = static_cast<TTBR*>(
        GetNextLevel(pml3->entries[pml3Entry], allocate, virt));
    if (!pml2) return nullptr;

    if (pageSize == lPageSize /*|| pml2->entries[pml2Entry].IsLarge()*/)
        return &pml2->entries[pml2Entry];

    TTBR* pml1 = static_cast<TTBR*>(
        GetNextLevel(pml2->entries[pml2Entry], allocate, virt));
    if (!pml1) return nullptr;

    return &pml1->entries[pml1Entry];
}

uintptr_t PageMap::Virt2Phys(uintptr_t virt, PageAttributes flags)
{
    std::unique_lock guard(lock);

    auto             pSize    = GetPageSize(flags);
    PageTableEntry*  pmlEntry = Virt2Pte(topLevel, virt, false, pSize);
    if (!pmlEntry || !pmlEntry->GetFlag(VALID)) return -1;

    return pmlEntry->GetAddress() + (virt % pSize);
}
bool PageMap::InternalMap(uintptr_t virt, uintptr_t phys, PageAttributes flags)
{
    PageTableEntry* pmlEntry
        = Virt2Pte(topLevel, virt, true, GetPageSize(flags));
    if (!pmlEntry)
    {
        LogError("VMM: Could not get page map entry for address {:#x}", virt);
        return false;
    }

    auto nativeFlags = ToNativeFlags(flags);

    pmlEntry->Clear();
    pmlEntry->SetAddress(phys);
    pmlEntry->SetFlags(nativeFlags, true);
    return true;
}

bool PageMap::InternalUnmap(uintptr_t virt, PageAttributes flags)
{
    PageTableEntry* pmlEntry
        = Virt2Pte(topLevel, virt, false, GetPageSize(flags));
    if (!pmlEntry)
    {
        LogError("VMM: Could not get page map entry for address 0x{:X}", virt);
        return false;
    }

    pmlEntry->Clear();

    usize addr = (0ull << 48ull) | (virt >> 12ul);
    __asm__ volatile(
        "dsb st; \n\t"
        "tlbi vale1, %0;\n\t"
        "dsb sy; isb" ::"r"(addr)
        : "memory");
    return true;
}

bool PageMap::SetFlags(uintptr_t virt, PageAttributes flags)
{
    PageTableEntry* pmlEntry
        = Virt2Pte(topLevel, virt, false, GetPageSize(flags));
    if (!pmlEntry)
    {
        LogError("VMM: Could not get page map entry for address {:#x}", virt);
        return false;
    }

    auto nativeFlags = ToNativeFlags(flags);
    auto addr        = pmlEntry->GetAddress();

    pmlEntry->Clear();
    pmlEntry->SetAddress(addr);
    pmlEntry->SetFlags(nativeFlags, true);
    return true;
}

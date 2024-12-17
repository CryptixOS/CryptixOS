/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "Memory/VMM.hpp"

#include "Common.hpp"

constexpr usize                  PTE_PRESENT    = Bit(0);
constexpr usize                  PTE_WRITEABLE  = Bit(1);
constexpr usize                  PTE_USER_SUPER = Bit(2);
constexpr usize                  PTE_PWT        = Bit(3);
constexpr usize                  PTE_PCD        = Bit(4);
[[maybe_unused]] constexpr usize PTE_ACCESSED   = Bit(5);
constexpr usize                  PTE_LPAGE      = Bit(7);
constexpr usize                  PTE_PAT4K      = Bit(7);
constexpr usize                  PTE_GLOBAL     = Bit(8);
[[maybe_unused]] constexpr usize PTE_CUSTOM0    = Bit(9);
[[maybe_unused]] constexpr usize PTE_CUSTOM1    = Bit(10);
[[maybe_unused]] constexpr usize PTE_CUSTOM2    = Bit(11);
constexpr usize                  PTE_PATLG      = Bit(12);
constexpr usize                  PTE_NOEXEC     = Bit(63ull);

struct PageTable
{
    PageTableEntry entries[512];
} __attribute__((packed));
static bool gib1Pages = false;

namespace Arch::VMM
{
    constexpr uintptr_t PTE_ADDRESS_MASK = 0x000ffffffffff000;
    uintptr_t defaultPteFlags   = PTE_PRESENT | PTE_WRITEABLE | PTE_USER_SUPER;

    // 4KiB
    constexpr usize PAGE_SIZE   = 0x1000;
    // 2MiB
    constexpr usize LPAGE_SIZE  = 0x200000;
    // 1GiB
    constexpr usize LLPAGE_SIZE = 0x40000000;

    static void     DestroyLevel(PageMap* pageMap, PageTable* pml, usize start,
                                 usize end, usize level)
    {
        if (!level || !pml) return;

        for (usize i = start; i < end; i++)
        {
            auto next = static_cast<PageTable*>(
                pageMap->GetNextLevel(pml->entries[i], false));
            if (!next) continue;

            DestroyLevel(pageMap, next, 0, 512, level - 1);
        }
        delete pml;
    }

    void Initialize()
    {
        // TODO(v1tr10l7): Check support for 1gib pages

        gib1Pages = true;
    }

    void* AllocatePageTable() { return new PageTable; }
    void  DestroyPageMap(PageMap* pageMap)
    {
        DestroyLevel(pageMap, pageMap->GetTopLevel(), 0, 256,
                     BootInfo::GetPagingMode() == 0 ? 4 : 5);
    }

    PageAttributes FromNativeFlags(uintptr_t flags)
    {
        PageAttributes ret;
        if (flags & PTE_PRESENT) ret |= PageAttributes::eRead;
        if (flags & PTE_WRITEABLE) ret |= PageAttributes::eWrite;
        if (!(flags & PTE_NOEXEC)) ret |= PageAttributes::eExecutable;
        if (flags & PTE_USER_SUPER) ret |= PageAttributes::eUser;
        if (flags & PTE_GLOBAL) ret |= PageAttributes::eGlobal;

        usize patbit = (flags & PTE_LPAGE ? PTE_PATLG : PTE_PAT4K);

        if ((flags & (patbit | PTE_PCD)) == (patbit | PTE_PCD))
            ret |= PageAttributes::eWriteBack;
        else if ((flags & (PTE_PCD | PTE_PWT)) == (PTE_PCD | PTE_PWT))
            ret |= PageAttributes::eWriteCombining;

        return ret;
    }
    usize ToNativeFlags(PageAttributes attribs)
    {
        uintptr_t pteFlags = PTE_PRESENT;
        if (attribs & PageAttributes::eWrite) pteFlags |= PTE_WRITEABLE;

        if (!(attribs & PageAttributes::eExecutable)) pteFlags |= PTE_NOEXEC;
        if (attribs & PageAttributes::eUser) pteFlags |= PTE_USER_SUPER;
        if (attribs & PageAttributes::eGlobal) pteFlags |= PTE_GLOBAL;
        if (attribs & PageAttributes::eLPage
            || attribs & PageAttributes::eLLPage)
            pteFlags |= PTE_LPAGE;

        bool largePages = pteFlags & PTE_LPAGE;
        u64  patbit     = (largePages ? PTE_PATLG : PTE_PAT4K);

        if (attribs & PageAttributes::eUncacheableStrong) pteFlags |= PTE_PCD;
        if (attribs & PageAttributes::eWriteCombining)
        {
            pteFlags |= PTE_PCD | PTE_PWT;
            pteFlags &= ~patbit;
        }
        else if (attribs & PageAttributes::eWriteThrough) pteFlags |= patbit;
        else if (attribs & PageAttributes::eWriteProtected)
            pteFlags |= patbit | PTE_PWT;
        else if (attribs & PageAttributes::eWriteBack)
            pteFlags |= patbit | PTE_PCD;
        else if (attribs & PageAttributes::eUncacheable)
            pteFlags |= patbit | PTE_PCD | PTE_PWT;

        return pteFlags | PTE_USER_SUPER;
    }

    uintptr_t GetAddressMask() { return PTE_ADDRESS_MASK; }
    usize     GetPageSize(PageAttributes flags)
    {
        if (flags & PageAttributes::eLLPage) return LLPAGE_SIZE;
        if (flags & PageAttributes::eLPage) return LPAGE_SIZE;

        return PAGE_SIZE;
    }

} // namespace Arch::VMM

using namespace Arch::VMM;
bool            PageTableEntry::IsValid() { return GetFlag(PTE_PRESENT); }
bool            PageTableEntry::IsLarge() { return GetFlag(PTE_LPAGE); }

PageTableEntry* PageMap::Virt2Pte(PageTable* topLevel, uintptr_t virt,
                                  bool allocate, usize pageSize)
{
    usize pml5Entry = (virt >> 48) & 0x1ffull;
    usize pml4Entry = (virt >> 39) & 0x1ffull;
    usize pml3Entry = (virt >> 30) & 0x1ffull;
    usize pml2Entry = (virt >> 21) & 0x1ffull;
    usize pml1Entry = (virt >> 12) & 0x1ffull;

    if (!topLevel) return nullptr;

    PageTable* pml4
        = BootInfo::GetPagingMode() == LIMINE_PAGING_MODE_X86_64_5LVL
            ? static_cast<PageTable*>(
                GetNextLevel(topLevel->entries[pml5Entry], allocate))
            : topLevel;
    if (!pml4) return nullptr;

    PageTable* pml3 = static_cast<PageTable*>(
        GetNextLevel(pml4->entries[pml4Entry], allocate));
    if (!pml3) return nullptr;

    if (pageSize == LLPAGE_SIZE || pml3->entries[pml3Entry].IsLarge())
        return &pml3->entries[pml3Entry];

    PageTable* pml2 = static_cast<PageTable*>(
        GetNextLevel(pml3->entries[pml3Entry], allocate, virt));
    if (!pml2) return nullptr;

    if (pageSize == LPAGE_SIZE || (pml2->entries[pml2Entry].IsLarge()))
        return &pml2->entries[pml2Entry];

    PageTable* pml1 = static_cast<PageTable*>(
        GetNextLevel(pml2->entries[pml2Entry], allocate, virt));

    if (!pml1) return nullptr;

    return &pml1->entries[pml1Entry];
}

uintptr_t PageMap::Virt2Phys(uintptr_t virt, PageAttributes flags)
{
    std::unique_lock guard(lock);

    auto             pageSize = GetPageSize(flags);
    PageTableEntry*  pmlEntry = Virt2Pte(topLevel, virt, false, pageSize);
    if (!pmlEntry || !pmlEntry->GetFlag(PTE_PRESENT)) return -1;

    return pmlEntry->GetAddress() + (virt % pageSize);
}

bool PageMap::InternalMap(uintptr_t virt, uintptr_t phys, PageAttributes flags)
{

    auto pageSize = GetPageSize(flags);
    if (pageSize == LLPAGE_SIZE && !gib1Pages)
    {
        flags &= ~PageAttributes::eLLPage;
        flags |= PageAttributes::eLPage;

        for (usize i = 0; i < 1_gib; i += 2_mib)
            if (!InternalMap(virt + i, phys + i, flags)) return false;
        return true;
    }

    PageTableEntry* pmlEntry = Virt2Pte(topLevel, virt, true, pageSize);
    if (!pmlEntry)
    {
        LogError("VMM: Could not get page map entry for address {:#x}", virt);
        return false;
    }

    pmlEntry->Clear();
    pmlEntry->SetAddress(phys);
    pmlEntry->SetFlags(ToNativeFlags(flags) | PTE_USER_SUPER, true);
    return true;
}

bool PageMap::InternalUnmap(uintptr_t virt, PageAttributes flags)
{
    auto unmapOne = [this](uintptr_t virt, usize pageSize)
    {
        PageTableEntry* pmlEntry = Virt2Pte(topLevel, virt, false, pageSize);
        if (!pmlEntry)
        {
            LogError("VMM: Could not get page map entry for address {:#x}",
                     virt);
            return false;
        }

        pmlEntry->Clear();
        __asm__ volatile("invlpg (%0);" ::"r"(virt) : "memory");
        return true;
    };

    auto pageSize = GetPageSize(flags);
    if (pageSize == LLPAGE_SIZE && !gib1Pages)
    {
        flags &= ~PageAttributes::eLLPage;
        flags |= PageAttributes::eLPage;

        for (usize i = 0; i < 1_gib; i += 2_mib)
            if (!unmapOne(virt + i, 2_mib)) return false;
        return true;
    }

    return unmapOne(virt, pageSize);
}

bool PageMap::SetFlags(uintptr_t virt, PageAttributes flags)
{
    std::unique_lock guard(lock);

    auto             pageSize = GetPageSize(flags);
    PageTableEntry*  pmlEntry = Virt2Pte(topLevel, virt, true, pageSize);
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

namespace VirtualMemoryManager
{
    void SaveCurrentPageMap(PageMap& out)
    {
        uintptr_t cr3 = 0;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3)::"memory");

        out = ToHigherHalfAddress<uintptr_t>(cr3);
    }

    void LoadPageMap(PageMap& pageMap, bool)
    {
        uintptr_t topLevel = FromHigherHalfAddress<uintptr_t>(
            reinterpret_cast<uintptr_t>(pageMap.GetTopLevel()));

        __asm__ volatile("mov %0, %%cr3" ::"r"(topLevel));
    }
}; // namespace VirtualMemoryManager

PageMap::PageMap()
    : topLevel(new PageTable)
{
    if (!VMM::GetKernelPageMap())
    {
        for (usize i = 256; i < 512; i++)
            GetNextLevel(topLevel->entries[i], true);

        usize pat = (0x07ull << 56ull) | (0x06ull << 48ull) | (0x05ull << 40ull)
                  | (0x04ull << 32ull) | (0x01ull << 24ull)
                  | (0x00ull << 16ull);

        u32 edx = pat >> 32;
        u32 eax = static_cast<u32>(pat);
        __asm__ volatile("wrmsr" ::"a"(eax), "d"(edx), "c"(0x277) : "memory");
        return;
    }

    // Map Kernel
    for (usize i = 256; i < 512; i++)
        topLevel->entries[i] = VMM::GetKernelPageMap()->topLevel->entries[i];
}

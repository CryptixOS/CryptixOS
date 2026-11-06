/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Arch/CPU.hpp>
#include <Library/Locking/Spinlock.hpp>
#include <Memory/MM.hpp>
#include <Memory/VMM.hpp>
#include <System/InterruptManager.hpp>

#include <Arch/x86_64/Drivers/Time/Lapic.hpp>

#include <Prism/Utility/Atomic.hpp>

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

struct [[gnu::packed]] PageTable
{
    PageTableEntry entries[512];
};
static bool s_1GibPages = false;

namespace Arch::VMM
{
    constexpr u64 PTE_ADDRESS_MASK = 0x000ffffffffff000;
    u64 g_DefaultPteFlags       = PTE_PRESENT | PTE_WRITEABLE | PTE_USER_SUPER;

    // 4KiB
    constexpr usize PAGE_SIZE   = 0x1000;
    // 2MiB
    constexpr usize LPAGE_SIZE  = 0x200000;
    // 1GiB
    constexpr usize LLPAGE_SIZE = 0x40000000;

    // CPUID.80000001h:EDX[26] - 1GiB pages (Page1GB / pdpe1gb)
    constexpr u64   CPUID_EDX_PAGE1GB = Bit(26);

    inline void     InvalidatePage(Pointer virt)
    {
        __asm__ volatile("invlpg (%0);" ::"r"(virt.Raw<upointer>())
                         : "memory");
    }
    inline void FlushLocalTlb()
    {
        // Reloading CR3 drops every non-global translation.
        uintptr_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r"(cr3)::"memory");
        __asm__ volatile("mov %0, %%cr3" ::"r"(cr3) : "memory");
    }

    // `pml` is a page table at `level` (1 = PT ... 5 = PML5). Entries at
    // level 1, and large entries at any level, point at data pages rather than
    // page tables, so they must not be descended into (or freed).
    static void DestroyLevel(PageMap* pageMap, PageTable* pml, usize start,
                             usize end, usize level)
    {
        if (!level || !pml) return;

        for (usize i = start; i < end && level > 1; i++)
        {
            auto& entry = pml->entries[i];
            if (!entry.IsValid() || entry.IsLarge()) continue;

            auto next
                = static_cast<PageTable*>(pageMap->NextLevel(entry, false));
            DestroyLevel(pageMap, next, 0, 512, level - 1);
        }

        PMM::FreePages(Pointer(pml).FromHigherHalf<void*>(), 1);
    }

    void Initialize()
    {
        ::CPU::ID id(0x80000001);
        s_1GibPages = id.rdx & CPUID_EDX_PAGE1GB;
        LogTrace("VMM: 1GiB pages are {}supported", s_1GibPages ? "" : "not ");
    }

    // Page tables are exactly one page, so take them straight from the PMM:
    // the kernel heap would spend an extra metadata page on every table.
    void* AllocatePageTable()
    {
        Pointer phys = PMM::CallocatePages(1);
        AssertMsg(phys, "VMM: Out of memory while allocating a page table");
        return phys.ToHigherHalf<void*>();
    }
    void DestroyPageMap(PageMap* pageMap)
    {
        DestroyLevel(pageMap, pageMap->TopLevel(), 0, 256,
                     MM::PagingMode() == PagingMode::eLevel4 ? 4 : 5);
    }

    PageAttributes FromNativeFlags(uintptr_t flags)
    {
        PageAttributes ret{};
        if (flags & PTE_PRESENT) ret |= PageAttributes::eRead;
        if (flags & PTE_WRITEABLE) ret |= PageAttributes::eWrite;
        if (!(flags & PTE_NOEXEC)) ret |= PageAttributes::eExecutable;
        if (flags & PTE_USER_SUPER) ret |= PageAttributes::eUser;
        if (flags & PTE_GLOBAL) ret |= PageAttributes::eGlobal;

        usize patbit   = (flags & PTE_LPAGE ? PTE_PATLG : PTE_PAT4K);
        usize patIndex = (flags & patbit ? 4 : 0) | (flags & PTE_PCD ? 2 : 0)
                       | (flags & PTE_PWT ? 1 : 0);

        // Mirrors the PAT layout programmed by CPU::EnablePAT().
        switch (patIndex)
        {
            case 3: ret |= PageAttributes::eWriteCombining; break;
            case 4: ret |= PageAttributes::eWriteThrough; break;
            case 5: ret |= PageAttributes::eWriteProtected; break;
            case 6: ret |= PageAttributes::eWriteBack; break;
            case 7: ret |= PageAttributes::eUncacheable; break;
            default: ret |= PageAttributes::eUncacheableStrong; break;
        }

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

        // The cache types are multi-bit *encodings* (see PageTableEntry.hpp),
        // so they must be compared as a whole: testing them with `&` matches
        // any overlapping bit (e.g. eUncacheable would hit eWriteCombining).
        // The resulting PAT indices follow the layout of CPU::EnablePAT().
        auto cacheType = attribs & PageAttributes::eUncacheable;
        if (cacheType == ToUnderlying(PageAttributes::eUncacheableStrong))
            pteFlags |= PTE_PCD; // PAT2: UC
        else if (cacheType == ToUnderlying(PageAttributes::eWriteCombining))
            pteFlags |= PTE_PCD | PTE_PWT; // PAT3: WC
        else if (cacheType == ToUnderlying(PageAttributes::eWriteThrough))
            pteFlags |= patbit; // PAT4: WT
        else if (cacheType == ToUnderlying(PageAttributes::eWriteProtected))
            pteFlags |= patbit | PTE_PWT; // PAT5: WP
        else if (cacheType == ToUnderlying(PageAttributes::eWriteBack))
            pteFlags |= patbit | PTE_PCD; // PAT6: WB
        else if (cacheType == ToUnderlying(PageAttributes::eUncacheable))
            pteFlags |= patbit | PTE_PCD | PTE_PWT; // PAT7: UC-
        // No cache type requested: PAT0 (UC).

        return pteFlags;
    }

    uintptr_t GetAddressMask() { return PTE_ADDRESS_MASK; }
    usize     GetPageSize(PageAttributes flags)
    {
        if (flags & PageAttributes::eLLPage) return LLPAGE_SIZE;
        if (flags & PageAttributes::eLPage) return LPAGE_SIZE;

        return PAGE_SIZE;
    }

    //--------------------------------------------------------------------------
    // TLB shootdown
    //
    // invlpg only affects the executing CPU, so after unmapping or reducing
    // permissions every other CPU must drop its stale translations too.
    // An initiator bumps a global generation, IPIs all other CPUs and waits
    // until each one has acknowledged that generation (after flushing).
    // While waiting for the shootdown lock, CPUs service pending requests
    // themselves, so two initiators can never wait on each other.
    //--------------------------------------------------------------------------
    namespace
    {
        Atomic<u64>              s_FlushGeneration = 0;
        Atomic<u64>*             s_FlushAcks       = nullptr;
        usize                    s_FlushAckCount   = 0;
        Atomic<bool>             s_ShootdownBusy   = false;
        u8                       s_ShootdownVector = 0;
        Ref<InterruptDispatcher> s_ShootdownDispatcher;

        constexpr usize          SHOOTDOWN_SPIN_LIMIT = 100'000'000;

        void                     AcknowledgePendingFlush()
        {
            auto id = ::CPU::GetCurrent()->ID;
            if (id >= s_FlushAckCount) return;

            u64 generation = s_FlushGeneration.Load();
            if (s_FlushAcks[id].Load() >= generation) return;

            FlushLocalTlb();
            s_FlushAcks[id].Store(generation);
        }
    } // namespace

    void InitializeTlbShootdown()
    {
        usize cpuCount = ::CPU::GetOnlineCPUsCount();
        if (cpuCount < 2 || s_ShootdownVector) return;

        s_FlushAcks     = new Atomic<u64>[cpuCount];
        s_FlushAckCount = cpuCount;

        s_ShootdownDispatcher
            = InterruptManager::AllocateHandler(0xd0, nullptr, "cryptix");
        AssertMsg(s_ShootdownDispatcher,
                  "VMM: Failed to allocate the TLB shootdown vector");
        s_ShootdownDispatcher->SetHandler(
            [](Device*, ExecutionContext*) -> IrqResult
            {
                AcknowledgePendingFlush();
                return IrqResult::eHandled;
            });

        s_ShootdownVector = s_ShootdownDispatcher->IrqLine() + 0x20;
        LogTrace("VMM: TLB shootdown IPI installed on vector {:#x}",
                 s_ShootdownVector);
    }

    void ShootdownTlb()
    {
        if (!s_ShootdownVector) return;

        while (s_ShootdownBusy.Exchange(true))
        {
            AcknowledgePendingFlush();
            Arch::Pause();
        }

        u64  generation = s_FlushGeneration.FetchAdd(1) + 1;
        auto self       = ::CPU::GetCurrent()->ID;
        if (self < s_FlushAckCount) s_FlushAcks[self].Store(generation);

        // Fixed delivery to all CPUs excluding self (shorthand 0b11).
        Lapic::Instance()->SendIpi(s_ShootdownVector | (0b11 << 18), 0);

        for (usize id = 0; id < s_FlushAckCount; id++)
        {
            if (id == self) continue;

            usize spins = 0;
            while (s_FlushAcks[id].Load() < generation)
            {
                if (++spins > SHOOTDOWN_SPIN_LIMIT)
                {
                    LogWarn("VMM: CPU {} did not acknowledge a TLB shootdown",
                            id);
                    break;
                }
                Arch::Pause();
            }
        }

        s_ShootdownBusy.Store(false);
    }
} // namespace Arch::VMM

using namespace Arch::VMM;
bool            PageTableEntry::IsValid() { return GetFlag(PTE_PRESENT); }
bool            PageTableEntry::IsLarge() { return GetFlag(PTE_LPAGE); }

PageTableEntry* PageMap::Virt2Pte(PageTable* topLevel, Pointer virt,
                                  bool allocate, usize pageSize)
{
    usize pml5Entry = (virt.Raw() >> 48) & 0x1ffull;
    usize pml4Entry = (virt.Raw() >> 39) & 0x1ffull;
    usize pml3Entry = (virt.Raw() >> 30) & 0x1ffull;
    usize pml2Entry = (virt.Raw() >> 21) & 0x1ffull;
    usize pml1Entry = (virt.Raw() >> 12) & 0x1ffull;

    if (!topLevel) return nullptr;

    PageTable* pml4 = MM::PagingMode() == PagingMode::eLevel5
                        ? static_cast<PageTable*>(
                              NextLevel(topLevel->entries[pml5Entry], allocate))
                        : topLevel;
    if (!pml4) return nullptr;

    PageTable* pml3 = static_cast<PageTable*>(
        NextLevel(pml4->entries[pml4Entry], allocate));
    if (!pml3) return nullptr;

    if (pageSize == LLPAGE_SIZE || pml3->entries[pml3Entry].IsLarge())
        return &pml3->entries[pml3Entry];

    PageTable* pml2 = static_cast<PageTable*>(
        NextLevel(pml3->entries[pml3Entry], allocate, virt));
    if (!pml2) return nullptr;

    if (pageSize == LPAGE_SIZE || (pml2->entries[pml2Entry].IsLarge()))
        return &pml2->entries[pml2Entry];

    PageTable* pml1 = static_cast<PageTable*>(
        NextLevel(pml2->entries[pml2Entry], allocate, virt));

    if (!pml1) return nullptr;

    return &pml1->entries[pml1Entry];
}

Pointer PageMap::Virt2Phys(Pointer virt, PageAttributes)
{
    ScopedLock guard(m_Lock, true);
    if (!m_TopLevel) return u64(-1);

    // Walk down to the leaf ourselves, so the offset is taken relative to the
    // size of the page that actually maps `virt` (4KiB, 2MiB or 1GiB).
    usize      pml5Entry = (virt.Raw() >> 48) & 0x1ffull;
    usize      pml4Entry = (virt.Raw() >> 39) & 0x1ffull;
    usize      pml3Entry = (virt.Raw() >> 30) & 0x1ffull;
    usize      pml2Entry = (virt.Raw() >> 21) & 0x1ffull;
    usize      pml1Entry = (virt.Raw() >> 12) & 0x1ffull;

    PageTable* pml4      = MM::PagingMode() == PagingMode::eLevel5
                             ? static_cast<PageTable*>(
                                   NextLevel(m_TopLevel->entries[pml5Entry], false))
                             : m_TopLevel;
    if (!pml4) return u64(-1);

    auto pml3 = static_cast<PageTable*>(
        NextLevel(pml4->entries[pml4Entry], false));
    if (!pml3) return u64(-1);

    PageTableEntry* leaf     = &pml3->entries[pml3Entry];
    usize           pageSize = LLPAGE_SIZE;
    if (leaf->IsValid() && !leaf->IsLarge())
    {
        auto pml2 = static_cast<PageTable*>(NextLevel(*leaf, false));
        leaf      = &pml2->entries[pml2Entry];
        pageSize  = LPAGE_SIZE;

        if (leaf->IsValid() && !leaf->IsLarge())
        {
            auto pml1 = static_cast<PageTable*>(NextLevel(*leaf, false));
            leaf      = &pml1->entries[pml1Entry];
            pageSize  = PAGE_SIZE;
        }
    }
    if (!leaf->IsValid()) return u64(-1);

    // For large pages bit 12 is the PAT bit, not part of the address.
    upointer frame = leaf->Address().Raw<upointer>() & ~(pageSize - 1);
    return frame + (virt.Raw<upointer>() & (pageSize - 1));
}

bool PageMap::InternalMap(Pointer virt, Pointer phys, PageAttributes flags)
{
    auto pageSize = GetPageSize(flags);
    if (pageSize == LLPAGE_SIZE && !s_1GibPages)
    {
        // No 1GiB page support: map the range with 2MiB pages instead.
        flags &= ~PageAttributes::eLLPage;
        flags |= PageAttributes::eLPage;

        for (usize i = 0; i < LLPAGE_SIZE; i += LPAGE_SIZE)
            if (!InternalMap(virt.Offset(i), phys.Offset(i), flags))
                return false;

        return true;
    }

    PageTableEntry* pmlEntry = Virt2Pte(m_TopLevel, virt, true, pageSize);
    if (!pmlEntry)
    {
        LogError("VMM: Could not get page map entry for address {:#x}", virt);
        return false;
    }

    if (pmlEntry->Address())
    {
        LogWarn(
            "VMM: page at {:#x} is already mapped to {:#x}\ntried to map to => "
            "{:#x}",
            virt.Raw(), pmlEntry->Address().Raw(), phys.Raw());
        Stacktrace::Print(8);
#define CTOS_HALT_ON_COLLISION 1
#if CTOS_HALT_ON_COLLISION
        for (;;) Arch::Halt();
#endif
    }

    pmlEntry->Clear();
    pmlEntry->SetAddress(phys);
    pmlEntry->SetFlags(ToNativeFlags(flags), true);
    return true;
}

bool PageMap::InternalUnmap(Pointer virt, PageAttributes flags)
{
    auto unmapOne = [this](uintptr_t virt, usize pageSize)
    {
        PageTableEntry* pmlEntry = Virt2Pte(m_TopLevel, virt, false, pageSize);
        if (!pmlEntry)
        {
            LogError("VMM: Could not get page map entry for address {:#x}",
                     virt);
            return false;
        }

        pmlEntry->Clear();
        InvalidatePage(virt);
        return true;
    };

    auto pageSize = GetPageSize(flags);
    if (pageSize == LLPAGE_SIZE && !s_1GibPages)
    {
        for (usize i = 0; i < LLPAGE_SIZE; i += LPAGE_SIZE)
            if (!unmapOne(virt.Offset(i), LPAGE_SIZE)) return false;
        return true;
    }

    return unmapOne(virt, pageSize);
}

// Replaces the flags of the leaf entry mapping `virt`, keeping its address.
// The entry must be mapped at the granularity implied by `flags`: rewriting
// a large entry with 4KiB flags would clear PS and turn the huge page into a
// bogus page table pointer (and vice versa).
static bool ReplaceLeafFlags(PageTableEntry* entry, Pointer virt,
                             PageAttributes flags)
{
    if (!entry || !entry->IsValid()) return false;

    // Bit 7 is PS only in directory entries; in a 4KiB PTE it is the PAT bit
    // (set for write-back mappings), so a 4KiB leaf can't be asked whether it
    // is large. Callers check the directory entry above it instead, see
    // MappedByLargePage().
    usize pageSize = GetPageSize(flags);
    if (pageSize != Arch::VMM::PAGE_SIZE && !entry->IsLarge()) return false;

    // For large entries bit 12 is the PAT bit; ToNativeFlags sets it anew.
    upointer frame = entry->Address().Raw<upointer>() & ~(pageSize - 1);

    entry->Clear();
    entry->SetAddress(frame);
    entry->SetFlags(ToNativeFlags(flags), true);
    InvalidatePage(virt);
    return true;
}

// Whether `virt` is mapped by a 2MiB/1GiB entry: a 4KiB request must not
// rewrite such an entry (it would clear PS and turn the huge page into a
// bogus page-table pointer). Asked of the directory entry, where bit 7 is PS.
static bool MappedByLargePage(PageTableEntry* directoryEntry)
{
    return directoryEntry && directoryEntry->IsValid()
        && directoryEntry->IsLarge();
}

bool PageMap::InternalProtect(Pointer virt, PageAttributes flags)
{
    usize pageSize = Arch::VMM::GetPageSize(flags);
    if (pageSize == Arch::VMM::PAGE_SIZE
        && MappedByLargePage(Virt2Pte(m_TopLevel, virt, false, LPAGE_SIZE)))
        return false;
    return ReplaceLeafFlags(Virt2Pte(m_TopLevel, virt, false, pageSize), virt,
                            flags);
}

bool PageMap::SetFlags(Pointer virt, PageAttributes flags)
{
    bool ok;
    {
        ScopedLock guard(m_Lock, true);

        // Never allocate here: setting flags on an unmapped address would
        // otherwise create a present mapping of physical address 0.
        auto pageSize = GetPageSize(flags);
        if (pageSize == Arch::VMM::PAGE_SIZE
            && MappedByLargePage(Virt2Pte(m_TopLevel, virt, false, LPAGE_SIZE)))
            ok = false;
        else
            ok = ReplaceLeafFlags(
                Virt2Pte(m_TopLevel, virt, false, pageSize), virt, flags);
    }
    if (ok) Arch::VMM::ShootdownTlb();
    return ok;
}

namespace VMM
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
            reinterpret_cast<uintptr_t>(pageMap.TopLevel()));

        __asm__ volatile("mov %0, %%cr3" ::"r"(topLevel));
    }
}; // namespace VMM

PageMap::PageMap()
    : m_TopLevel(static_cast<PageTable*>(Arch::VMM::AllocatePageTable()))
{
    if (!VMM::GetKernelPageMap())
    {
        // Pre-allocate every kernel half PML4 entry, so later kernel mappings
        // are automatically visible in all page maps that copy these entries.
        for (usize i = 256; i < 512; i++)
            NextLevel(m_TopLevel->entries[i], true);

        usize pat = (0x07ull << 56ull) | (0x06ull << 48ull) | (0x05ull << 40ull)
                  | (0x04ull << 32ull) | (0x01ull << 24ull);

        u32 edx = pat >> 32;
        u32 eax = static_cast<u32>(pat);
        __asm__ volatile("wrmsr" ::"a"(eax), "d"(edx), "c"(0x277) : "memory");
        return;
    }

    // Map Kernel
    for (usize i = 256; i < 512; i++)
        m_TopLevel->entries[i]
            = VMM::GetKernelPageMap()->m_TopLevel->entries[i];
}

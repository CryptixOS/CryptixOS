/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Debug/Kasan.hpp>
#include <Debug/Panic.hpp>

#include <Memory/MM.hpp>
#include <Memory/PMM.hpp>
#include <Memory/VMM.hpp>

#include <Prism/Utility/Atomic.hpp>
#include <Prism/Utility/Math.hpp>

extern "C" symbol limine_requests_addr_start;
extern "C" symbol data_end_addr;

// The runtime itself must never be instrumented: a check inside a check
// would recurse forever. Everything below works on plain integers, without
// helpers from elsewhere that could be instrumented (at -O0 nothing is
// inlined).
#if defined(__clang__)
    #pragma clang attribute push(__attribute__((no_sanitize("address", "kernel-address"))), apply_to = function)
#endif

namespace Kasan
{
    // As the compiler lays them out (__asan_global in LLVM)
    struct __asan_global
    {
        u64         Address;
        u64         ValidSize;
        u64         TotalSize;
        const char* Name;
        const char* ModuleName;
        u64         HasDynamicInit;
        void*       Location;
        u64         OdrIndicator;
    };

    namespace
    {
        constexpr u64 SHADOW_SHIFT        = 3;
        constexpr u64 SHADOW_GRANULE_SIZE = 1 << SHADOW_SHIFT;
        constexpr u64 SHADOW_GRANULE_MASK = SHADOW_GRANULE_SIZE - 1;

        // A covered address range and where its shadow starts:
        // shadow(address) = ShadowBase + (address - Start) / 8
        struct ShadowRange
        {
            u64 Start      = 0;
            u64 End        = 0;
            u64 ShadowBase = 0;
        };

        // The direct map and the kernel image
        CTOS_RO_AFTER_INIT constinit ShadowRange s_Ranges[2]{};
        CTOS_RO_AFTER_INIT constinit usize       s_RangeCount = 0;
        constinit bool        s_Initialized = false;
        constinit Atomic<bool> s_Reporting  = false;

        // Globals register themselves from the global constructors, which
        // run before the shadow exists; they are applied in Initialize()
        struct GlobalBatch
        {
            __asan_global* Globals;
            usize                 Count;
        };
        constexpr usize      MAX_PENDING_GLOBALS = 1024;
        constinit GlobalBatch s_PendingGlobals[MAX_PENDING_GLOBALS]{};
        constinit usize       s_PendingGlobalCount = 0;
        constinit usize       s_DroppedGlobals     = 0;

        u8*                   ShadowOf(u64 address)
        {
            for (usize i = 0; i < s_RangeCount; i++)
            {
                auto& range = s_Ranges[i];
                if (address >= range.Start && address < range.End)
                    return reinterpret_cast<u8*>(
                        range.ShadowBase
                        + ((address - range.Start) >> SHADOW_SHIFT));
            }
            return nullptr;
        }

        void FillShadow(u64 address, u64 size, u8 value)
        {
            for (u64 offset = 0; offset < size; offset += SHADOW_GRANULE_SIZE)
            {
                u8* shadow = ShadowOf(address + offset);
                if (shadow) *shadow = value;
            }
        }

        const char* ShadowName(u8 value)
        {
            switch (value)
            {
                case 0xf9: return "global redzone";
                case 0xfa: return "heap redzone (out of bounds)";
                case 0xfb: return "freed heap object (use after free)";
                case 0xff: return "free page";
                default: break;
            }
            return value < SHADOW_GRANULE_SIZE ? "partially addressable"
                                               : "unknown";
        }

        [[noreturn]] void Report(u64 address, u64 size, bool write,
                                 u64 badAddress, u8 shadow, u64 caller)
        {
            EarlyPanic(
                "KASAN: invalid %s of %llu bytes at %#llx: %#llx is %s "
                "(shadow %#x), from %#llx",
                write ? "write" : "read", size, address, badAddress,
                ShadowName(shadow), shadow, caller);
        }

        void CheckAccess(u64 address, u64 size, bool write, u64 caller)
        {
            if (!s_Initialized || size == 0) [[unlikely]]
                return;

            u64 last = address + size - 1;
            for (u64 current = address; current <= last;)
            {
                u64 granuleEnd = current | SHADOW_GRANULE_MASK;
                u8* shadow     = ShadowOf(current);
                if (shadow && *shadow)
                {
                    // 1-7: only that many leading bytes are addressable
                    u8  value      = *shadow;
                    u64 firstValid = current & ~SHADOW_GRANULE_MASK;
                    u64 accessEnd  = last < granuleEnd ? last : granuleEnd;
                    if (value >= SHADOW_GRANULE_SIZE
                        || accessEnd >= firstValid + value)
                    {
                        // Panic once, even if the report itself trips
                        if (s_Reporting.Exchange(true)) return;
                        s_Initialized = false;
                        Report(address, size, write,
                               value >= SHADOW_GRANULE_SIZE
                                   ? current
                                   : firstValid + value,
                               value, caller);
                    }
                }

                if (granuleEnd == ~0ull) break;
                current = granuleEnd + 1;
            }
        }

        void MarkRegion(u64 address, u64 validSize, u64 totalSize, u8 value)
        {
            u64 fullGranules = validSize & ~SHADOW_GRANULE_MASK;
            FillShadow(address, fullGranules, 0);
            if (validSize & SHADOW_GRANULE_MASK)
            {
                u8* shadow = ShadowOf(address + fullGranules);
                if (shadow) *shadow = validSize & SHADOW_GRANULE_MASK;
                fullGranules += SHADOW_GRANULE_SIZE;
            }
            if (totalSize > fullGranules)
                FillShadow(address + fullGranules, totalSize - fullGranules,
                           value);
        }
    } // namespace

    namespace
    {
        void RegisterGlobals(__asan_global* globals, usize count)
        {
            for (usize i = 0; i < count; i++)
                MarkRegion(globals[i].Address, globals[i].ValidSize,
                           globals[i].TotalSize,
                           ToUnderlying(ShadowType::eGlobalRedzone));
        }

        void PoisonFreePages(Pointer base, usize pageCount)
        {
            FillShadow(base.Raw() + MM::HigherHalfOffset(),
                       pageCount * PMM::PAGE_SIZE,
                       ToUnderlying(ShadowType::ePageFree));
        }

        bool IsRam(MemoryZoneType type)
        {
            switch (type)
            {
                case MemoryZoneType::eUsable:
                case MemoryZoneType::eBootloaderReclaimable:
                case MemoryZoneType::eKernelAndModules:
                case MemoryZoneType::eACPI_Reclaimable:
                case MemoryZoneType::eACPI_NVS: return true;
                default: break;
            }
            return false;
        }

        // Maps `size` bytes of shadow at `shadowBase` for the memory starting
        // at `start`. Shadow pages whose memory isn't RAM (holes, MMIO) are
        // all mapped to one read-only zero page, like Linux's
        // kasan_early_shadow_page: they read as addressable and are never
        // written, since only RAM is ever poisoned.
        bool MapShadow(u64 shadowBase, u64 size, u64 start, bool directMap,
                       Pointer zeroPage)
        {
            auto  pageMap = VMM::GetKernelPageMap();
            auto  zones   = PMM::InitialMemoryZones();
            u64   hhdm    = MM::HigherHalfOffset();
            // The memory one shadow page describes
            u64   span    = PMM::PAGE_SIZE << SHADOW_SHIFT;

            for (u64 offset = 0; offset < size; offset += PMM::PAGE_SIZE)
            {
                bool backed = !directMap;
                if (directMap)
                {
                    u64 physStart = start - hhdm + (offset << SHADOW_SHIFT);
                    u64 physEnd   = physStart + span;
                    for (auto& zone : zones)
                    {
                        u64 zoneStart = zone.Base().Raw();
                        u64 zoneEnd   = zoneStart + zone.Length();
                        if (IsRam(zone.Type()) && zoneStart < physEnd
                            && zoneEnd > physStart)
                        {
                            backed = true;
                            break;
                        }
                    }
                }

                Pointer        phys = zeroPage;
                PageAttributes attributes
                    = PageAttributes::eRead | PageAttributes::eWriteBack;
                if (backed)
                {
                    phys = PMM::CallocatePages(1);
                    if (!phys) return false;
                    attributes |= PageAttributes::eWrite;
                }

                if (!pageMap->Map(shadowBase + offset, phys, attributes))
                    return false;
            }
            return true;
        }
    } // namespace

    void Initialize()
    {
        if constexpr (!ENABLED) return;

        u64 ramTop = 0;
        for (auto& zone : PMM::InitialMemoryZones())
            if (IsRam(zone.Type()))
                ramTop = Max(ramTop, zone.Base().Raw() + zone.Length());

        u64 hhdm       = MM::HigherHalfOffset();
        u64 imageStart = Pointer(limine_requests_addr_start).Raw();
        u64 imageEnd   = Pointer(data_end_addr).Raw();

        auto shadowSize = [](u64 bytes)
        {
            return Math::AlignUp((bytes >> SHADOW_SHIFT) + 1, PMM::PAGE_SIZE);
        };
        u64 directMapShadow = shadowSize(ramTop);
        u64 imageShadow     = shadowSize(imageEnd - imageStart);

        Pointer zeroPage    = PMM::CallocatePages(1);
        Pointer shadowBase  = VMM::AllocateSpace(
            directMapShadow + imageShadow, PMM::PAGE_SIZE, false);
        if (!zeroPage || !shadowBase
            || !MapShadow(shadowBase.Raw(), directMapShadow, hhdm, true,
                          zeroPage)
            || !MapShadow(shadowBase.Raw() + directMapShadow, imageShadow,
                          imageStart, false, zeroPage))
        {
            LogError("KASAN: Failed to map the shadow memory, disabled");
            return;
        }

        s_Ranges[0]   = {hhdm, hhdm + ramTop, shadowBase.Raw()};
        s_Ranges[1]   = {imageStart, imageEnd,
                         shadowBase.Raw() + directMapShadow};
        s_RangeCount  = 2;

        PMM::ForEachFreeRange(PoisonFreePages);
        for (usize i = 0; i < s_PendingGlobalCount; i++)
            RegisterGlobals(s_PendingGlobals[i].Globals,
                            s_PendingGlobals[i].Count);
        if (s_DroppedGlobals)
            LogWarn("KASAN: {} global batches registered too early were lost",
                    s_DroppedGlobals);

        s_Initialized = true;
        LogInfo("KASAN: Enabled, {} MiB of shadow memory for {} MiB of RAM",
                (directMapShadow + imageShadow) / 1_mib, ramTop / 1_mib);
    }
    bool IsInitialized() { return s_Initialized; }

    void PoisonImpl(Pointer address, usize size, ShadowType type)
    {
        if (s_RangeCount) FillShadow(address.Raw(), size, ToUnderlying(type));
    }
    void UnpoisonImpl(Pointer address, usize size)
    {
        if (s_RangeCount) MarkRegion(address.Raw(), size, size, 0);
    }
    usize AddressableSizeImpl(Pointer object, usize capacity)
    {
        usize size = 0;
        while (size < capacity)
        {
            u8* shadow = ShadowOf(object.Raw() + size);
            if (!shadow || *shadow == 0)
            {
                size += SHADOW_GRANULE_SIZE;
                continue;
            }
            if (*shadow < SHADOW_GRANULE_SIZE) size += *shadow;
            break;
        }
        return size < capacity ? size : capacity;
    }

    // The entry points the compiler calls. With
    // -asan-instrumentation-with-call-threshold=0 every access is a call,
    // so these are the only places the shadow is read.
    extern "C"
    {
#define CTOS_KASAN_ACCESS(size)                                                \
    void __asan_load##size##_noabort(u64 address)                              \
    {                                                                          \
        CheckAccess(address, size, false,                                      \
                    reinterpret_cast<u64>(__builtin_return_address(0)));       \
    }                                                                          \
    void __asan_store##size##_noabort(u64 address)                             \
    {                                                                          \
        CheckAccess(address, size, true,                                       \
                    reinterpret_cast<u64>(__builtin_return_address(0)));       \
    }                                                                          \
    void __asan_load##size(u64 address)                                        \
    {                                                                          \
        CheckAccess(address, size, false,                                      \
                    reinterpret_cast<u64>(__builtin_return_address(0)));       \
    }                                                                          \
    void __asan_store##size(u64 address)                                       \
    {                                                                          \
        CheckAccess(address, size, true,                                       \
                    reinterpret_cast<u64>(__builtin_return_address(0)));       \
    }                                                                          \
    void __asan_report_load##size##_noabort(u64 address)                       \
    {                                                                          \
        CheckAccess(address, size, false,                                      \
                    reinterpret_cast<u64>(__builtin_return_address(0)));       \
    }                                                                          \
    void __asan_report_store##size##_noabort(u64 address)                      \
    {                                                                          \
        CheckAccess(address, size, true,                                       \
                    reinterpret_cast<u64>(__builtin_return_address(0)));       \
    }

        CTOS_KASAN_ACCESS(1)
        CTOS_KASAN_ACCESS(2)
        CTOS_KASAN_ACCESS(4)
        CTOS_KASAN_ACCESS(8)
        CTOS_KASAN_ACCESS(16)
#undef CTOS_KASAN_ACCESS

        void __asan_loadN_noabort(u64 address, u64 size)
        {
            CheckAccess(address, size, false,
                        reinterpret_cast<u64>(__builtin_return_address(0)));
        }
        void __asan_storeN_noabort(u64 address, u64 size)
        {
            CheckAccess(address, size, true,
                        reinterpret_cast<u64>(__builtin_return_address(0)));
        }
        void __asan_loadN(u64 address, u64 size)
        {
            CheckAccess(address, size, false,
                        reinterpret_cast<u64>(__builtin_return_address(0)));
        }
        void __asan_storeN(u64 address, u64 size)
        {
            CheckAccess(address, size, true,
                        reinterpret_cast<u64>(__builtin_return_address(0)));
        }
        void __asan_report_load_n_noabort(u64 address, u64 size)
        {
            CheckAccess(address, size, false,
                        reinterpret_cast<u64>(__builtin_return_address(0)));
        }
        void __asan_report_store_n_noabort(u64 address, u64 size)
        {
            CheckAccess(address, size, true,
                        reinterpret_cast<u64>(__builtin_return_address(0)));
        }

        void __asan_register_globals(__asan_global* globals, u64 count)
        {
            if (s_Initialized)
            {
                RegisterGlobals(globals, count);
                return;
            }
            if (s_PendingGlobalCount < MAX_PENDING_GLOBALS)
                s_PendingGlobals[s_PendingGlobalCount++] = {globals, count};
            else ++s_DroppedGlobals;
        }
        void __asan_unregister_globals(__asan_global*, u64) {}

        void __asan_handle_no_return() {}
        void __asan_before_dynamic_init(const char*) {}
        void __asan_after_dynamic_init() {}
    }
}; // namespace Kasan

#if defined(__clang__)
    #pragma clang attribute pop
#endif

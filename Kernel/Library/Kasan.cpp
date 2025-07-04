/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Common.hpp>

#include <Boot/BootInfo.hpp>

#include <Debug/Assertions.hpp>
#include <Debug/Panic.hpp>

#include <Library/Kasan.hpp>
#include <Prism/Utility/Math.hpp>

#include <magic_enum/magic_enum.hpp>

namespace Kasan
{
    constexpr usize SHADOW_SCALE_OFFSET = 3;
    constexpr usize SHADOW_SCALE        = Bit(SHADOW_SCALE_OFFSET);
    constexpr usize SHADOW_MASK         = SHADOW_SCALE - 1;

    constexpr usize ALLOCA_REDZONE_SIZE = 32;

    enum class AccessType
    {
        eLoad,
        eStore,
    };

    static bool    s_Initialized = false;
    static Pointer s_ShadowBase;
    static usize   s_ShadowOffset;

    void           Initialize(Pointer shadowBase)
    {
        Pointer hhdmOffset = BootInfo::GetHHDMOffset();

        s_ShadowBase       = shadowBase;
        s_ShadowOffset     = s_ShadowBase - (hhdmOffset >> SHADOW_SCALE_OFFSET);
        s_Initialized      = true;
    }

    static void PrintViolation(Pointer address, usize size,
                               AccessType accessType, ShadowType shadowType,
                               Pointer returnAddress)
    {
        Panic(
            "KASAN: Invalid {}-byte {} access to {}, which is marked as '{}' "
            "[at {}]",
            size, magic_enum::enum_name(accessType).data() + 1, address.Raw(),
            magic_enum::enum_name(shadowType).data() + 1, returnAddress.Raw());
    }

    static ShadowType* va_to_shadow(Pointer address)
    {
        return Pointer(address.Raw<u64>() >> SHADOW_SCALE_OFFSET)
            .Offset<Pointer>(s_ShadowOffset)
            .As<ShadowType>();
    }

    void FillShadow(Pointer address, usize size, ShadowType type)
    {
        if (!s_Initialized) [[unlikely]]
            return;

        Assert((address.Raw() % SHADOW_SCALE) == 0);
        Assert((size % SHADOW_SCALE) == 0);

        auto* shadow     = va_to_shadow(address);
        auto  shadowSize = size >> SHADOW_SCALE_OFFSET;
        std::memset(shadow, std::to_underlying(type), shadowSize);
    }
    void MarkRegion(Pointer address, usize validSize, usize totalSize,
                    ShadowType type)
    {
        if (!s_Initialized) [[unlikely]]
            return;

        Assert((address.Raw() % SHADOW_SCALE) == 0);
        Assert((totalSize % SHADOW_SCALE) == 0);

        auto* shadow          = va_to_shadow(address);
        auto  validShadowSize = validSize >> SHADOW_SCALE_OFFSET;
        std::memset(shadow, std::to_underlying(ShadowType::eUnpoisoned8Bytes),
                    validShadowSize);
        auto unalignedSize = validSize & SHADOW_MASK;
        if (unalignedSize)
            *(shadow + validShadowSize)
                = static_cast<ShadowType>(unalignedSize);

        auto poisonedShadowSize
            = (totalSize - Math::RoundUpToPowerOfTwo(validSize, SHADOW_SCALE))
           >> SHADOW_SCALE_OFFSET;
        std::memset(shadow + validShadowSize + (unalignedSize != 0),
                    std::to_underlying(type), poisonedShadowSize);
    }

    static bool Check1ShadowByte(Pointer address, ShadowType& shadowType)
    {
        const auto shadow = *va_to_shadow(address);
        const i8   minimalValidShadow
            = (address & Pointer(SHADOW_MASK)).Offset(1);

        if (shadow == ShadowType::eUnpoisoned8Bytes
            || (minimalValidShadow <= std::to_underlying(shadow))) [[likely]]
            return true;

        shadowType = shadow;
        return false;
    }
    static bool Check2ShadowBytes(Pointer address, ShadowType& shadowType)
    {
        if ((address.Raw<u64>() >> SHADOW_SCALE_OFFSET)
            != (address.Offset<u64>(1) >> SHADOW_SCALE_OFFSET)) [[unlikely]]
            return Check1ShadowByte(address, shadowType)
                && Check1ShadowByte(address.Offset(1), shadowType);

        const auto shadow = *va_to_shadow(address);
        const i8   minimalValidShadow
            = (address.Offset<Pointer>(1) & SHADOW_MASK).Offset(1);

        if (shadow == ShadowType::eUnpoisoned8Bytes
            || (minimalValidShadow <= std::to_underlying(shadow))) [[likely]]
            return true;

        shadowType = shadow;
        return false;
    }
    static bool Check4ShadowBytes(Pointer address, ShadowType& shadowType)
    {
        if ((address.Raw<u64>() >> SHADOW_SCALE_OFFSET)
            != (address.Offset<usize>(3) >> SHADOW_SCALE_OFFSET)) [[unlikely]]
            return Check2ShadowBytes(address, shadowType)
                && Check2ShadowBytes(address.Offset<Pointer>(2), shadowType);

        const auto shadow = *va_to_shadow(address);
        const i8   minimalValidShadow
            = (address.Offset<Pointer>(3) & SHADOW_MASK).Offset(1);

        if (shadow == ShadowType::eUnpoisoned8Bytes
            || (minimalValidShadow <= std::to_underlying(shadow))) [[likely]]
            return true;

        shadowType = shadow;
        return false;
    }
    static bool Check8ShadowBytes(Pointer address, ShadowType& shadowType)
    {
        if ((address.Raw<u64>() >> SHADOW_SCALE_OFFSET)
            != (address.Offset<usize>(7)) >> SHADOW_SCALE_OFFSET) [[unlikely]]
            return Check4ShadowBytes(address, shadowType)
                && Check4ShadowBytes(address.Offset<Pointer>(4), shadowType);

        const auto shadow = *va_to_shadow(address);
        const i8   minimalValidShadow
            = (address.Offset<Pointer>(7) & SHADOW_MASK).Offset(1);

        if (shadow == ShadowType::eUnpoisoned8Bytes
            || (minimalValidShadow <= std::to_underlying(shadow))) [[likely]]
            return true;

        shadowType = shadow;
        return false;
    }
    static bool CheckN_ShadowBytes(Pointer address, usize n,
                                   ShadowType& shadowType)
    {
        while ((address.Raw<u64>() % 8) && (n > 0))
        {
            if (!Check1ShadowByte(address, shadowType)) return false;

            ++address;
            --n;
        }
        while (n >= 8)
        {
            if (!Check8ShadowBytes(address, shadowType)) return false;

            address = address.Offset<Pointer>(8);
            n -= 8;
        }

        while (n > 0)
        {
            if (!Check1ShadowByte(address, shadowType)) return false;

            ++address;
            --n;
        }
        return true;
    }

    static void CheckShadowBytes(Pointer address, usize size,
                                 AccessType accessType, Pointer returnAddress)
    {
        if (size == 0) [[unlikely]]
            return;
        if (!s_Initialized) [[unlikely]]
            return;
        if (address < BootInfo::GetHHDMOffset() || address >= s_ShadowBase)
            [[unlikely]]
            return;

        bool       valid      = false;
        ShadowType shadowType = ShadowType::eUnpoisoned8Bytes;
        switch (size)
        {
            case 1: valid = Check1ShadowByte(address, shadowType); break;
            case 2: valid = Check2ShadowBytes(address, shadowType); break;
            case 4: valid = Check4ShadowBytes(address, shadowType); break;
            case 8: valid = Check8ShadowBytes(address, shadowType); break;
            default:
                valid = CheckN_ShadowBytes(address, size, shadowType);
                break;
        }

        if (valid) [[likely]]
            return;

        PrintViolation(address, size, accessType, shadowType, returnAddress);
    }

    extern "C"
    {
// Define a macro to easily declare the KASAN load and store callbacks for
// the various sizes of data type.
//
#define ADDRESS_SANITIZER_LOAD_STORE(size)                                     \
    void __asan_load##size(uintptr_t);                                         \
    void __asan_load##size(uintptr_t address)                                  \
    {                                                                          \
        CheckShadowBytes(address, size, AccessType::eLoad,                     \
                         __builtin_return_address(0));                         \
    }                                                                          \
    void __asan_load##size##_noabort(uintptr_t);                               \
    void __asan_load##size##_noabort(uintptr_t address)                        \
    {                                                                          \
        CheckShadowBytes(address, size, AccessType::eLoad,                     \
                         __builtin_return_address(0));                         \
    }                                                                          \
    void __asan_store##size(uintptr_t);                                        \
    void __asan_store##size(uintptr_t address)                                 \
    {                                                                          \
        CheckShadowBytes(address, size, AccessType::eStore,                    \
                         __builtin_return_address(0));                         \
    }                                                                          \
    void __asan_store##size##_noabort(uintptr_t);                              \
    void __asan_store##size##_noabort(uintptr_t address)                       \
    {                                                                          \
        CheckShadowBytes(address, size, AccessType::eStore,                    \
                         __builtin_return_address(0));                         \
    }                                                                          \
    void __asan_report_load##size(uintptr_t);                                  \
    void __asan_report_load##size(uintptr_t address)                           \
    {                                                                          \
        PrintViolation(address, size, AccessType::eLoad, ShadowType::eGeneric, \
                       __builtin_return_address(0));                           \
    }                                                                          \
    void __asan_report_load##size##_noabort(uintptr_t);                        \
    void __asan_report_load##size##_noabort(uintptr_t address)                 \
    {                                                                          \
        PrintViolation(address, size, AccessType::eLoad, ShadowType::eGeneric, \
                       __builtin_return_address(0));                           \
    }                                                                          \
    void __asan_report_store##size(uintptr_t);                                 \
    void __asan_report_store##size(uintptr_t address)                          \
    {                                                                          \
        PrintViolation(address, size, AccessType::eStore,                      \
                       ShadowType::eGeneric, __builtin_return_address(0));     \
    }                                                                          \
    void __asan_report_store##size##_noabort(uintptr_t);                       \
    void __asan_report_store##size##_noabort(uintptr_t address)                \
    {                                                                          \
        PrintViolation(address, size, AccessType::eStore,                      \
                       ShadowType::eGeneric, __builtin_return_address(0));     \
    }

        ADDRESS_SANITIZER_LOAD_STORE(1);
        ADDRESS_SANITIZER_LOAD_STORE(2);
        ADDRESS_SANITIZER_LOAD_STORE(4);
        ADDRESS_SANITIZER_LOAD_STORE(8);
        ADDRESS_SANITIZER_LOAD_STORE(16);

#undef ADDRESS_SANITIZER_LOAD_STORE

        void __asan_loadN(uintptr_t, usize);
        void __asan_loadN(uintptr_t address, usize size)
        {
            CheckShadowBytes(address, size, AccessType::eLoad,
                             __builtin_return_address(0));
        }

        void __asan_loadN_noabort(uintptr_t, usize);
        void __asan_loadN_noabort(uintptr_t address, usize size)
        {
            CheckShadowBytes(address, size, AccessType::eLoad,
                             __builtin_return_address(0));
        }

        void __asan_storeN(uintptr_t, usize);
        void __asan_storeN(uintptr_t address, usize size)
        {
            CheckShadowBytes(address, size, AccessType::eStore,
                             __builtin_return_address(0));
        }

        void __asan_storeN_noabort(uintptr_t, usize);
        void __asan_storeN_noabort(uintptr_t address, usize size)
        {
            CheckShadowBytes(address, size, AccessType::eStore,
                             __builtin_return_address(0));
        }

        void __asan_report_load_n(uintptr_t, usize);
        void __asan_report_load_n(uintptr_t address, usize size)
        {
            PrintViolation(address, size, AccessType::eLoad,
                           ShadowType::eGeneric, __builtin_return_address(0));
        }

        void __asan_report_load_n_noabort(uintptr_t, usize);
        void __asan_report_load_n_noabort(uintptr_t address, usize size)
        {
            PrintViolation(address, size, AccessType::eLoad,
                           ShadowType::eGeneric, __builtin_return_address(0));
        }

        void __asan_report_store_n(uintptr_t, usize);
        void __asan_report_store_n(uintptr_t address, usize size)
        {
            PrintViolation(address, size, AccessType::eStore,
                           ShadowType::eGeneric, __builtin_return_address(0));
        }

        void __asan_report_store_n_noabort(uintptr_t, usize);
        void __asan_report_store_n_noabort(uintptr_t address, usize size)
        {
            PrintViolation(address, size, AccessType::eStore,
                           ShadowType::eGeneric, __builtin_return_address(0));
        }

        // As defined in the compiler
        struct __asan_global_source_location
        {
            char const* Filename;
            int         LineNumber;
            int         ColumnNumber;
        };
        struct __asan_global
        {
            uintptr_t                      Address;
            usize                          ValidSize;
            usize                          TotalSize;
            char const*                    Name;
            char const*                    ModuleName;
            usize                          HasDynamicInit;
            __asan_global_source_location* Location;
            usize                          OdrIndicator;
        };

        void __asan_register_globals(struct __asan_global*, usize);
        void __asan_register_globals(struct __asan_global* globals, usize count)
        {
            for (auto i = 0u; i < count; ++i)
                MarkRegion(globals[i].Address, globals[i].ValidSize,
                           globals[i].TotalSize, ShadowType::eGeneric);
        }

        void __asan_unregister_globals(struct __asan_global*, usize);
        void __asan_unregister_globals(struct __asan_global* globals,
                                       usize                 count)
        {
            for (auto i = 0u; i < count; ++i)
                MarkRegion(globals[i].Address, globals[i].TotalSize,
                           globals[i].TotalSize, ShadowType::eUnpoisoned8Bytes);
        }

        void __asan_alloca_poison(uintptr_t, usize);
        void __asan_alloca_poison(uintptr_t address, usize size)
        {
            Assert(address % ALLOCA_REDZONE_SIZE == 0);
            auto roundedSize
                = Math::RoundUpToPowerOfTwo(size, ALLOCA_REDZONE_SIZE);
            FillShadow(address - ALLOCA_REDZONE_SIZE, ALLOCA_REDZONE_SIZE,
                       ShadowType::eStackLeft);
            MarkRegion(address, size, roundedSize, ShadowType::eStackMiddle);
            FillShadow(address + roundedSize, ALLOCA_REDZONE_SIZE,
                       ShadowType::eStackRight);
        }

        void __asan_allocas_unpoison(uintptr_t, usize);
        void __asan_allocas_unpoison(uintptr_t start, usize end)
        {
            Assert(start >= end);
            auto size = end - start;
            Assert(size % SHADOW_SCALE == 0);
            FillShadow(start, size, ShadowType::eUnpoisoned8Bytes);
        }

        void __asan_poison_stack_memory(uintptr_t, usize);
        void __asan_poison_stack_memory(uintptr_t address, usize size)
        {
            FillShadow(address, Math::RoundUpToPowerOfTwo(size, SHADOW_SCALE),
                       ShadowType::eUseAfterScope);
        }

        void __asan_unpoison_stack_memory(uintptr_t, usize);
        void __asan_unpoison_stack_memory(uintptr_t address, usize size)
        {
            FillShadow(address, Math::RoundUpToPowerOfTwo(size, SHADOW_SCALE),
                       ShadowType::eUnpoisoned8Bytes);
        }

        void __asan_handle_no_return(void);
        void __asan_handle_no_return(void) {}

        void __asan_before_dynamic_init(char const*);
        void __asan_before_dynamic_init(char const*) {}

        void __asan_after_dynamic_init();
        void __asan_after_dynamic_init() {}
    }

}; // namespace Kasan

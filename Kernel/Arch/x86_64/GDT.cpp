/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/GDT.hpp>

#include <Prism/Spinlock.hpp>

namespace GDT
{
    namespace
    {
        Spinlock s_Lock;
        struct [[gnu::packed]] SegmentDescriptor
        {
            u16 LimitLow;
            u16 BaseLow;
            u8  BaseMiddle;
            u8  Access;
            u8  LimitHigh : 4;
            u8  Flags     : 4;
            u8  BaseHigh;
        };
        struct [[gnu::packed]] TaskStateSegmentDescriptor
        {
            u16 Length;
            u16 BaseLow;
            u8  BaseMiddle1;
            u8  Flags1;
            u8  Flags2;
            u8  BaseMiddle2;
            u32 BaseHigh;
            u32 Reserved;
        };

        struct [[gnu::packed]] GDTEntries
        {
            SegmentDescriptor          Null;
            SegmentDescriptor          KernelCode;
            SegmentDescriptor          KernelData;
            SegmentDescriptor          UserCode;
            SegmentDescriptor          UserData;
            TaskStateSegmentDescriptor TSS;
        } s_GdtEntries;

        constexpr usize GDT_SEGMENT_PRESENT = Bit(7);
        constexpr usize GDT_RING3           = Bit(5) | Bit(6);
        constexpr usize GDT_CODE_OR_DATA    = Bit(4);
        constexpr usize GDT_CODE_SEGMENT    = Bit(3);
        constexpr usize GDT_CODE_READABLE   = Bit(1);
        constexpr usize GDT_DATA_WRITEABLE  = Bit(1);
    }; // namespace

#define GDTWriteEntry(_entry, _base, _limit, _access, _flags)                  \
    {                                                                          \
        (_entry)->LimitLow   = _limit & 0xffff;                                \
        (_entry)->BaseLow    = _base & 0xffff;                                 \
        (_entry)->BaseMiddle = (_base >> 16) & 0xff;                           \
        (_entry)->Access     = _access | GDT_SEGMENT_PRESENT;                  \
        (_entry)->LimitHigh  = (_limit >> 16) & 0xf;                           \
        (_entry)->Flags      = _flags;                                         \
        (_entry)->BaseHigh   = (_base >> 24) & 0xff;                           \
    }
#define TSSWriteEntry(_entry, _base)                                           \
    {                                                                          \
        (_entry)->Length      = sizeof(TaskStateSegment);                      \
        (_entry)->BaseLow     = (u16)(_base);                                  \
        (_entry)->BaseMiddle1 = (u8)(_base >> 16);                             \
        (_entry)->Flags1      = ((1 << 3) << 4) | (1 << 3 | 1 << 0);           \
        (_entry)->Flags2      = 0;                                             \
        (_entry)->BaseMiddle2 = (u8)(_base >> 24);                             \
        (_entry)->BaseHigh    = (u32)(_base >> 32);                            \
        (_entry)->Reserved    = 0;                                             \
    }

    void Initialize()
    {
        memset(&s_GdtEntries.Null, 0, sizeof(SegmentDescriptor));

        u8 userCodeAccess = GDT_RING3 | GDT_CODE_OR_DATA | GDT_CODE_SEGMENT
                          | GDT_CODE_READABLE;
        u8 userDataAccess   = GDT_DATA_WRITEABLE | GDT_CODE_OR_DATA | GDT_RING3;

        userCodeAccess      = 0xf2;
        userDataAccess      = 0xfa;
        u8 kernelCodeAccess = 0x9a;
        u8 kernelDataAccess = 0x92;
        GDTWriteEntry(&s_GdtEntries.KernelCode, 0, 0xffffffff, kernelCodeAccess,
                      0xa);

        GDTWriteEntry(&s_GdtEntries.KernelData, 0, 0xffffffff, kernelDataAccess,
                      0xa);
        GDTWriteEntry(&s_GdtEntries.UserCode, 0, 0, userCodeAccess, 0xa);
        GDTWriteEntry(&s_GdtEntries.UserData, 0, 0, userDataAccess, 0xa);
        TSSWriteEntry(&s_GdtEntries.TSS, 0ull);
    }

    void Load(u64 id)
    {
        struct [[gnu::packed]]
        {
            u16       Limit;
            uintptr_t Base;
        } gdtr;
        gdtr.Limit = sizeof(s_GdtEntries) - 1;
        gdtr.Base  = reinterpret_cast<uintptr_t>(&s_GdtEntries);

        __asm__ volatile(
            "lgdt %0\n"
            "mov %%rsp, %%rbx\n"
            "push %1\n"
            "push %%rbx\n"
            "pushf\n"
            "push %2\n"
            "lea 1f(%%rip), %%rax\n"
            "push %%rax\n"
            "iretq\n"
            "1:\n"
            "mov %1, %%ds\n"
            "mov %1, %%es\n"
            "mov %1, %%fs\n"
            "mov %1, %%gs\n"
            "mov %1, %%ss"
            :
            : "m"(gdtr), "r"(KERNEL_DATA_SELECTOR), "r"(KERNEL_CODE_SELECTOR)
            : "rbx", "memory", "%rax", "rbx");

        __asm__ volatile("ltr %0" ::"r"(static_cast<u16>(TSS_SELECTOR)));
        EarlyLogInfo("GDT: Loaded on cpu #%zu", id);
    }
    void LoadTSS(TaskStateSegment* tss)
    {
        ScopedLock guard(s_Lock);
        Assert(tss);

        TSSWriteEntry(&s_GdtEntries.TSS, reinterpret_cast<uintptr_t>(tss));
        __asm__ volatile("ltr %0" : : "rm"((uint16_t)TSS_SELECTOR) : "memory");
    }

}; // namespace GDT

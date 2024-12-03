/*
 * Created by v1tr10l7 on 17.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "GDT.hpp"

#include <mutex>

namespace GDT
{
    namespace
    {
        std::mutex lock;
        struct SegmentDescriptor
        {
            u16 limitLow;
            u16 baseLow;
            u8  baseMiddle;
            u8  access;
            u8  limitHigh : 4;
            u8  flags     : 4;
            u8  baseHigh;
        } __attribute__((packed));
        struct TaskStateSegmentDescriptor
        {
            u16 length;
            u16 baseLow;
            u8  baseMiddle1;
            u8  flags1;
            u8  flags2;
            u8  baseMiddle2;
            u32 baseHigh;
            u32 reserved;
        } __attribute__((packed));

        struct GDTEntries
        {
            SegmentDescriptor          null;
            SegmentDescriptor          kernelCode;
            SegmentDescriptor          kernelData;
            SegmentDescriptor          userCode;
            SegmentDescriptor          userData;
            TaskStateSegmentDescriptor tss;
        } __attribute__((packed)) gdtEntries;

        constexpr usize GDT_SEGMENT_PRESENT = Bit(7);
        constexpr usize GDT_RING3           = Bit(5) | Bit(6);
        constexpr usize GDT_CODE_OR_DATA    = Bit(4);
        constexpr usize GDT_CODE_SEGMENT    = Bit(3);
        constexpr usize GDT_CODE_READABLE   = Bit(1);
        constexpr usize GDT_DATA_WRITEABLE  = Bit(1);
    }; // namespace

#define GDTWriteEntry(_entry, _base, _limit, _access, _flags)                  \
    {                                                                          \
        (_entry)->limitLow   = _limit & 0xffff;                                \
        (_entry)->baseLow    = _base & 0xffff;                                 \
        (_entry)->baseMiddle = (_base >> 16) & 0xff;                           \
        (_entry)->access     = _access | GDT_SEGMENT_PRESENT;                  \
        (_entry)->limitHigh  = (_limit >> 16) & 0xf;                           \
        (_entry)->flags      = _flags;                                         \
        (_entry)->baseHigh   = (_base >> 24) & 0xff;                           \
    }
#define TSSWriteEntry(_entry, _base)                                           \
    {                                                                          \
        (_entry)->length      = sizeof(TaskStateSegment);                      \
        (_entry)->baseLow     = (u16)(_base);                                  \
        (_entry)->baseMiddle1 = (u8)(_base >> 16);                             \
        (_entry)->flags1      = ((1 << 3) << 4) | (1 << 3 | 1 << 0);           \
        (_entry)->flags2      = 0;                                             \
        (_entry)->baseMiddle2 = (u8)(_base >> 24);                             \
        (_entry)->baseHigh    = (u32)(_base >> 32);                            \
        (_entry)->reserved    = 0;                                             \
    }

    void Initialize()
    {
        memset(&gdtEntries.null, 0, sizeof(SegmentDescriptor));

        u8 userCodeAccess = GDT_RING3 | GDT_CODE_OR_DATA | GDT_CODE_SEGMENT
                          | GDT_CODE_READABLE;
        u8 userDataAccess   = GDT_DATA_WRITEABLE | GDT_CODE_OR_DATA | GDT_RING3;

        userCodeAccess      = 0xf2;
        userDataAccess      = 0xfa;
        u8 kernelCodeAccess = 0x9a;
        u8 kernelDataAccess = 0x92;
        GDTWriteEntry(&gdtEntries.kernelCode, 0, 0xffffffff, kernelCodeAccess,
                      0xa);

        GDTWriteEntry(&gdtEntries.kernelData, 0, 0xffffffff, kernelDataAccess,
                      0xa);
        GDTWriteEntry(&gdtEntries.userCode, 0, 0, userCodeAccess, 0xa);
        GDTWriteEntry(&gdtEntries.userData, 0, 0, userDataAccess, 0xa);
        TSSWriteEntry(&gdtEntries.tss, 0ull);
    }

    void Load(u64 id)
    {
        struct
        {
            u16       limit;
            uintptr_t base;
        } __attribute__((packed)) gdtr;
        gdtr.limit = sizeof(gdtEntries) - 1;
        gdtr.base  = reinterpret_cast<uintptr_t>(&gdtEntries);

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
        std::unique_lock guard(lock);
        Assert(tss);

        TSSWriteEntry(&gdtEntries.tss, reinterpret_cast<uintptr_t>(tss));
        __asm__ volatile("ltr %0" : : "rm"((uint16_t)TSS_SELECTOR) : "memory");
    }

}; // namespace GDT

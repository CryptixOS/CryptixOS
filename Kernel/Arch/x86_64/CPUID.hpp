/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Common.hpp"

inline static constexpr const u32 CPUID_FUNCTION_GET_FEATURE = 0x01;

inline static constexpr const u32 CPU_FEAT_EBX_SMEP          = BIT(7);
inline static constexpr const u32 CPU_FEAT_EBX_AVX512        = BIT(16);
inline static constexpr const u32 CPU_FEAT_EBX_SMAP          = BIT(20);

inline static constexpr const u32 CPU_FEAT_ECX_SSE3          = BIT(0);
inline static constexpr const u32 CPU_FEAT_ECX_PCLMUL        = BIT(1);
inline static constexpr const u32 CPU_FEAT_ECX_DTES64        = BIT(2);
inline static constexpr const u32 CPU_FEAT_ECX_UMIP          = BIT(2);
inline static constexpr const u32 CPU_FEAT_ECX_MONITOR       = BIT(3);
inline static constexpr const u32 CPU_FEAT_ECX_DS_CPL        = BIT(4);
inline static constexpr const u32 CPU_FEAT_ECX_VMX           = BIT(5);
inline static constexpr const u32 CPU_FEAT_ECX_SMX           = BIT(6);
inline static constexpr const u32 CPU_FEAT_ECX_EST           = BIT(7);
inline static constexpr const u32 CPU_FEAT_ECX_TM2           = BIT(8);
inline static constexpr const u32 CPU_FEAT_ECX_SSSE3         = BIT(9);
inline static constexpr const u32 CPU_FEAT_ECX_CID           = BIT(10);
inline static constexpr const u32 CPU_FEAT_ECX_SDBG          = BIT(11);
inline static constexpr const u32 CPU_FEAT_ECX_FMA           = BIT(12);
inline static constexpr const u32 CPU_FEAT_ECX_CX16          = BIT(13);
inline static constexpr const u32 CPU_FEAT_ECX_XTPR          = BIT(14);
inline static constexpr const u32 CPU_FEAT_ECX_PDCM          = BIT(15);
inline static constexpr const u32 CPU_FEAT_ECX_PCID          = BIT(17);
inline static constexpr const u32 CPU_FEAT_ECX_DCA           = BIT(18);
inline static constexpr const u32 CPU_FEAT_ECX_SSE4_1        = BIT(19);
inline static constexpr const u32 CPU_FEAT_ECX_SSE4_2        = BIT(20);
inline static constexpr const u32 CPU_FEAT_ECX_X2APIC        = BIT(21);
inline static constexpr const u32 CPU_FEAT_ECX_MOVBE         = BIT(22);
inline static constexpr const u32 CPU_FEAT_ECX_POPCNT        = BIT(23);
inline static constexpr const u32 CPU_FEAT_ECX_TSC           = BIT(24);
inline static constexpr const u32 CPU_FEAT_ECX_AES           = BIT(25);
inline static constexpr const u32 CPU_FEAT_ECX_XSAVE         = BIT(26);
inline static constexpr const u32 CPU_FEAT_ECX_OSXSAVE       = BIT(27);
inline static constexpr const u32 CPU_FEAT_ECX_AVX           = BIT(28);
inline static constexpr const u32 CPU_FEAT_ECX_F16C          = BIT(29);
inline static constexpr const u32 CPU_FEAT_ECX_RDRAND        = BIT(30);
inline static constexpr const u32 CPU_FEAT_ECX_HYPERVISOR    = BIT(31);

inline static constexpr const u32 CPU_FEAT_EDX_FPU           = BIT(0);
inline static constexpr const u32 CPU_FEAT_EDX_VME           = BIT(1);
inline static constexpr const u32 CPU_FEAT_EDX_DE            = BIT(2);
inline static constexpr const u32 CPU_FEAT_EDX_PSE           = BIT(3);
inline static constexpr const u32 CPU_FEAT_EDX_TSC           = BIT(4);
inline static constexpr const u32 CPU_FEAT_EDX_MSR           = BIT(5);
inline static constexpr const u32 CPU_FEAT_EDX_PAE           = BIT(6);
inline static constexpr const u32 CPU_FEAT_EDX_MCE           = BIT(7);
inline static constexpr const u32 CPU_FEAT_EDX_CX8           = BIT(8);
inline static constexpr const u32 CPU_FEAT_EDX_APIC          = BIT(9);
inline static constexpr const u32 CPU_FEAT_EDX_SEP           = BIT(11);
inline static constexpr const u32 CPU_FEAT_EDX_MTRR          = BIT(12);
inline static constexpr const u32 CPU_FEAT_EDX_PGE           = BIT(13);
inline static constexpr const u32 CPU_FEAT_EDX_MCA           = BIT(14);
inline static constexpr const u32 CPU_FEAT_EDX_CMOV          = BIT(15);
inline static constexpr const u32 CPU_FEAT_EDX_PAT           = BIT(16);
inline static constexpr const u32 CPU_FEAT_EDX_PSE36         = BIT(17);
inline static constexpr const u32 CPU_FEAT_EDX_PSN           = BIT(18);
inline static constexpr const u32 CPU_FEAT_EDX_CLFLUSH       = BIT(19);
inline static constexpr const u32 CPU_FEAT_EDX_DS            = BIT(21);
inline static constexpr const u32 CPU_FEAT_EDX_ACPI          = BIT(22);
inline static constexpr const u32 CPU_FEAT_EDX_MMX           = BIT(23);
inline static constexpr const u32 CPU_FEAT_EDX_FXSR          = BIT(24);
inline static constexpr const u32 CPU_FEAT_EDX_SSE           = BIT(25);
inline static constexpr const u32 CPU_FEAT_EDX_SSE2          = BIT(26);
inline static constexpr const u32 CPU_FEAT_EDX_SS            = BIT(27);
inline static constexpr const u32 CPU_FEAT_EDX_HTT           = BIT(28);
inline static constexpr const u32 CPU_FEAT_EDX_TM            = BIT(29);
inline static constexpr const u32 CPU_FEAT_EDX_IA64          = BIT(30);
inline static constexpr const u32 CPU_FEAT_EDX_PBE           = BIT(31);

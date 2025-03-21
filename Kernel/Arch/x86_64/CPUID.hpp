/*
 * Created by v1tr10l7 on 25.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

constexpr usize CPUID_CHECK_FEATURES       = 0x01;
constexpr usize CPUID_CHECK_XSAVE_FEATURES = 0x0d;

constexpr usize CPU_FEAT_EBX_SMEP          = Bit(7);
constexpr usize CPU_FEAT_EBX_AVX512        = Bit(16);
constexpr usize CPU_FEAT_EBX_SMAP          = Bit(20);

constexpr usize CPU_FEAT_ECX_SSE3          = Bit(0);
constexpr usize CPU_FEAT_ECX_PCLMUL        = Bit(1);
constexpr usize CPU_FEAT_ECX_DTES64        = Bit(2);
constexpr usize CPU_FEAT_ECX_UMIP          = Bit(2);
constexpr usize CPU_FEAT_ECX_MONITOR       = Bit(3);
constexpr usize CPU_FEAT_ECX_DS_CPL        = Bit(4);
constexpr usize CPU_FEAT_ECX_VMX           = Bit(5);
constexpr usize CPU_FEAT_ECX_SMX           = Bit(6);
constexpr usize CPU_FEAT_ECX_EST           = Bit(7);
constexpr usize CPU_FEAT_ECX_TM2           = Bit(8);
constexpr usize CPU_FEAT_ECX_SSSE3         = Bit(9);
constexpr usize CPU_FEAT_ECX_CID           = Bit(10);
constexpr usize CPU_FEAT_ECX_SDBG          = Bit(11);
constexpr usize CPU_FEAT_ECX_FMA           = Bit(12);
constexpr usize CPU_FEAT_ECX_CX16          = Bit(13);
constexpr usize CPU_FEAT_ECX_XTPR          = Bit(14);
constexpr usize CPU_FEAT_ECX_PDCM          = Bit(15);
constexpr usize CPU_FEAT_ECX_PCID          = Bit(17);
constexpr usize CPU_FEAT_ECX_DCA           = Bit(18);
constexpr usize CPU_FEAT_ECX_SSE4_1        = Bit(19);
constexpr usize CPU_FEAT_ECX_SSE4_2        = Bit(20);
constexpr usize CPU_FEAT_ECX_X2APIC        = Bit(21);
constexpr usize CPU_FEAT_ECX_MOVBE         = Bit(22);
constexpr usize CPU_FEAT_ECX_POPCNT        = Bit(23);
constexpr usize CPU_FEAT_ECX_TSC           = Bit(24);
constexpr usize CPU_FEAT_ECX_AES           = Bit(25);
constexpr usize CPU_FEAT_ECX_XSAVE         = Bit(26);
constexpr usize CPU_FEAT_ECX_OSXSAVE       = Bit(27);
constexpr usize CPU_FEAT_ECX_AVX           = Bit(28);
constexpr usize CPU_FEAT_ECX_F16C          = Bit(29);
constexpr usize CPU_FEAT_ECX_RDRAND        = Bit(30);
constexpr usize CPU_FEAT_ECX_HYPERVISOR    = Bit(31);

constexpr usize CPU_FEAT_EDX_FPU           = Bit(0);
constexpr usize CPU_FEAT_EDX_VME           = Bit(1);
constexpr usize CPU_FEAT_EDX_DE            = Bit(2);
constexpr usize CPU_FEAT_EDX_PSE           = Bit(3);
constexpr usize CPU_FEAT_EDX_TSC           = Bit(4);
constexpr usize CPU_FEAT_EDX_MSR           = Bit(5);
constexpr usize CPU_FEAT_EDX_PAE           = Bit(6);
constexpr usize CPU_FEAT_EDX_MCE           = Bit(7);
constexpr usize CPU_FEAT_EDX_CX8           = Bit(8);
constexpr usize CPU_FEAT_EDX_APIC          = Bit(9);
constexpr usize CPU_FEAT_EDX_SEP           = Bit(11);
constexpr usize CPU_FEAT_EDX_MTRR          = Bit(12);
constexpr usize CPU_FEAT_EDX_PGE           = Bit(13);
constexpr usize CPU_FEAT_EDX_MCA           = Bit(14);
constexpr usize CPU_FEAT_EDX_CMOV          = Bit(15);
constexpr usize CPU_FEAT_EDX_PAT           = Bit(16);
constexpr usize CPU_FEAT_EDX_PSE36         = Bit(17);
constexpr usize CPU_FEAT_EDX_PSN           = Bit(18);
constexpr usize CPU_FEAT_EDX_CLFLUSH       = Bit(19);
constexpr usize CPU_FEAT_EDX_DS            = Bit(21);
constexpr usize CPU_FEAT_EDX_ACPI          = Bit(22);
constexpr usize CPU_FEAT_EDX_MMX           = Bit(23);
constexpr usize CPU_FEAT_EDX_FXSR          = Bit(24);
constexpr usize CPU_FEAT_EDX_SSE           = Bit(25);
constexpr usize CPU_FEAT_EDX_SSE2          = Bit(26);
constexpr usize CPU_FEAT_EDX_SS            = Bit(27);
constexpr usize CPU_FEAT_EDX_HTT           = Bit(28);
constexpr usize CPU_FEAT_EDX_TM            = Bit(29);
constexpr usize CPU_FEAT_EDX_IA64          = Bit(30);
constexpr usize CPU_FEAT_EDX_PBE           = Bit(31);

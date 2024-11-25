/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "CPU.hpp"

#include "Scheduler/Process.hpp"
#include "Scheduler/Thread.hpp"
#include "Utility/BootInfo.hpp"

namespace CPU
{
    static std::vector<CPU>                   cpus;
    static u64                                bspLapicId      = 0;
    static usize                              onlineCPUsCount = 1;

    extern "C" void                           handleSyscall(CPUContext*);
    extern "C" __attribute__((noreturn)) void syscall_entry();

    static void InitializeCPU(limine_smp_info* cpu)
    {
        CPU* current = reinterpret_cast<CPU*>(cpu->extra_argument);

        GDT::Load(current->lapicID);
        IDT::Load();

        EnablePAT();
        VMM::GetKernelPageMap()->Load();

        if (current->lapicID != bspLapicId)
            current->tss.rsp[0] = ToHigherHalfAddress<uintptr_t>(
                PMM::AllocatePages<u64>(KERNEL_STACK_SIZE / PMM::PAGE_SIZE));
        else
        {

            uintptr_t stack;
            __asm__ volatile("mov %%rsp, %0" : "=r"(stack));
            current->tss.rsp[0] = stack;
        }
        GDT::LoadTSS(&current->tss);

        SetKernelGSBase(cpu->extra_argument);
        SetGSBase(cpu->extra_argument);

        // TODO(v1tr10l7): Enable SSE, SMEP, SMAP, UMIP
        // TODO(v1tr10l7): Initialize the FPU

        WriteMSR(MSR::IA32_EFER,
                 ReadMSR(MSR::IA32_EFER) | MSR::IA32_EFER_SYSCALL_ENABLE);
        WriteMSR(MSR::STAR, (GDT::KERNEL_DATA_SELECTOR << 48ull)
                                | (GDT::KERNEL_CODE_SELECTOR << 32));

        WriteMSR(MSR::LSTAR, reinterpret_cast<uintptr_t>(syscall_entry));
        WriteMSR(MSR::SFMASK, ~Bit(2));
        // TODO(v1tr10l7): Initialize Lapic

        GetCurrent()->tss.ist[0]
            = ToHigherHalfAddress<uintptr_t>(
                  PMM::AllocatePages(KERNEL_STACK_SIZE / PMM::PAGE_SIZE))
            + KERNEL_STACK_SIZE;
        IDT::SetIST(0x20, 1);
        IDT::SetIST(14, 2);

        static std::atomic<pid_t> idlePids(-1);
        Process*                  process = new Process;
        process->pid                      = idlePids--;
        process->name                     = "Idle Process";
        process->pageMap                  = VMM::GetKernelPageMap();

        auto idleThread                   = new ::Thread(
            process, reinterpret_cast<uintptr_t>(Arch::Halt), false);
        idleThread->state  = ThreadState::eReady;
        GetCurrent()->idle = idleThread;
    }

    void InitializeBSP()
    {
        limine_smp_response* smp      = BootInfo::GetSMP_Response();
        usize                cpuCount = smp->cpu_count;

        cpus.resize(cpuCount);
        bspLapicId = smp->bsp_lapic_id;

        LogTrace("BSP: Initializing...");
        for (usize i = 0; i < cpuCount; i++)
        {
            limine_smp_info* smpInfo = smp->cpus[i];
            if (smpInfo->lapic_id != bspLapicId) continue;

            smpInfo->extra_argument = reinterpret_cast<u64>(&cpus[i]);

            cpus[i].lapicID         = smpInfo->lapic_id;
            cpus[i].id              = i;

            GDT::Initialize();
            IDT::Initialize();
            InitializeCPU(smpInfo);
        }

        LogInfo("BSP: Initialized");
    }

    bool ID(u64 leaf, u64 subleaf, u64& rax, u64& rbx, u64& rcx, u64& rdx)
    {
        u32 cpuidMax;
        __asm__ volatile("cpuid"
                         : "=a"(cpuidMax)
                         : "a"(leaf & 0x80000000)
                         : "rbx", "rcx", "rdx");
        if (leaf > cpuidMax) return false;

        __asm__ volatile("cpuid"
                         : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx)
                         : "a"(leaf), "c"(subleaf));
        return true;
    }

    bool GetInterruptFlag()
    {
        u64 rflags;
        __asm__ volatile(
            "pushf\n"
            "pop %0"
            : "=m"(rflags));

        return rflags & Bit(9);
    }
    void SetInterruptFlag(bool enabled)
    {
        if (enabled) __asm__ volatile("sti");
        else __asm__ volatile("cli");
    }
    void Halt() { __asm__ volatile("hlt"); }

    void WriteMSR(u32 msr, u64 value)
    {
        const u64 rdx = value >> 32;
        const u64 rax = value;
        __asm__ volatile("wrmsr" : : "a"(rax), "d"(rdx), "c"(msr) : "memory");
    }
    u64 ReadMSR(u32 msr)
    {
        u64 rdx = 0;
        u64 rax = 0;
        __asm__ volatile("rdmsr" : "=a"(rax), "=d"(rdx) : "c"(msr) : "memory");
        return (rdx << 32) | rax;
    }

    void WriteXCR(u64 reg, u64 value)
    {
        u32 a = value;
        u32 d = value >> 32;
        __asm__ volatile("xsetbv" ::"a"(a), "d"(d), "c"(reg) : "memory");
    }
    u64 ReadCR0()
    {
        u64 ret;
        __asm__ volatile("mov %%cr0, %0" : "=r"(ret)::"memory");

        return ret;
    }

    u64 ReadCR2()
    {
        u64 ret;
        __asm__ volatile("mov %%cr2, %0" : "=r"(ret)::"memory");

        return ret;
    }

    u64 ReadCR3()
    {
        u64 ret;
        __asm__ volatile("mov %%cr3, %0" : "=r"(ret)::"memory");

        return ret;
    }
    void WriteCR0(u64 value)
    {
        __asm__ volatile("mov %0, %%cr0" ::"r"(value) : "memory");
    }

    void WriteCR2(u64 value)
    {
        __asm__ volatile("mov %0, %%cr2" ::"r"(value) : "memory");
    }
    void WriteCR3(u64 value)
    {
        __asm__ volatile("mov %0, %%cr3" ::"r"(value) : "memory");
    }

    void WriteCR4(u64 value)
    {
        __asm__ volatile("mov %0, %%cr4" ::"r"(value) : "memory");
    }

    u64 ReadCR4()
    {
        u64 ret;
        __asm__ volatile("mov %%cr4, %0" : "=r"(ret)::"memory");

        return ret;
    }
    void Stac()
    {
        if (ReadCR4() & BIT(21)) __asm__ volatile("stac" ::: "cc");
    }
    void Clac()
    {
        if (ReadCR4() & BIT(21)) __asm__ volatile("clac" ::: "cc");
    }

    uintptr_t GetFSBase() { return ReadMSR(0xc0000100); }
    uintptr_t GetGSBase() { return ReadMSR(0xc0000101); }
    uintptr_t GetKernelGSBase() { return ReadMSR(0xc0000102); }

    void      SetFSBase(uintptr_t address) { WriteMSR(0xc0000100, address); }
    void      SetGSBase(uintptr_t address) { WriteMSR(0xc0000101, address); }
    void SetKernelGSBase(uintptr_t address) { WriteMSR(0xc0000102, address); }

    std::vector<CPU>& GetCPUs() { return cpus; }

    u64               GetOnlineCPUsCount() { return onlineCPUsCount; }
    u64               GetBSPID() { return bspLapicId; }
    CPU&              GetBSP() { return cpus[0]; }

    u64               GetCurrentID()
    {
        return GetOnlineCPUsCount() > 1 ? GetCurrent()->id : 0;
    }
    CPU* GetCurrent()
    {
        usize id;
        __asm__ volatile("mov %%gs:0, %0" : "=r"(id)::"memory");

        return &cpus[id];
    }
    Thread* GetCurrentThread()
    {
        Thread* currentThread;
        __asm__ volatile("mov %%gs:8, %0" : "=r"(currentThread)::"memory");

        return currentThread;
    }

    void PrepareThread(Thread* thread, uintptr_t pc)
    {
        thread->ctx.rflags = 0x202;
        thread->ctx.rip    = pc;

        uintptr_t pkstack
            = PMM::AllocatePages<uintptr_t>(KERNEL_STACK_SIZE / PMM::PAGE_SIZE);
        thread->kstack
            = ToHigherHalfAddress<uintptr_t>(pkstack) + KERNEL_STACK_SIZE;
        GetCurrent()->kernelStack = thread->kstack;

        // uintptr_t ppfstack
        //     = PMM::AllocatePages<uintptr_t>(KERNEL_STACK_SIZE /
        //     PMM::PAGE_SIZE);
        thread->pfstack
            = ToHigherHalfAddress<uintptr_t>(pkstack) + KERNEL_STACK_SIZE;

        thread->gsBase = reinterpret_cast<uintptr_t>(thread);
        if (thread->user)
        {
            thread->ctx.cs = GDT::USERLAND_CODE_SELECTOR;
            thread->ctx.ss = GDT::USERLAND_DATA_SELECTOR;
            thread->ctx.ds = thread->ctx.es = thread->ctx.ss;
            thread->ctx.rsp                 = thread->stack;
        }
        else
        {
            thread->ctx.cs  = GDT::KERNEL_CODE_SELECTOR;
            thread->ctx.ss  = GDT::KERNEL_DATA_SELECTOR;
            thread->ctx.rsp = thread->stack = thread->kstack;
        }
        thread->ctx.rsp = thread->stack = thread->kstack;
    }
    void SaveThread(Thread* thread, CPUContext* ctx)
    {
        thread->ctx    = *ctx;

        thread->gsBase = GetKernelGSBase();
        thread->fsBase = GetFSBase();
    }
    void LoadThread(Thread* thread, CPUContext* ctx)
    {
        thread->runningOn        = GetCurrent()->id;

        GetCurrent()->tss.ist[1] = thread->pfstack;
        GetCurrent()->tss.rsp[0] = thread->pfstack;

        thread->parent->pageMap->Load();
        SetGSBase(reinterpret_cast<u64>(thread));
        SetKernelGSBase(thread->gsBase);
        SetFSBase(thread->fsBase);

        *ctx = thread->ctx;
    }
    void Reschedule(usize ms)
    {
        // TODO(v1tr10l7): Reschedule
    }

    void EnablePAT()
    {
        // write-combining/write-protect
        u64 pat = ReadMSR(MSR::IA32_PAT);
        pat &= 0xffffffffull;
        pat |= 0x0105ull << 32ull;
        WriteMSR(MSR::IA32_PAT, pat);
    }
}; // namespace CPU

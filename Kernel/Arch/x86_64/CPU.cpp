/*
 * Created by v1tr10l7 on 25.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/CPU.hpp>
#include <Arch/x86_64/Drivers/Timers/PIT.hpp>

#include <Boot/BootInfo.hpp>
#include <Debug/Panic.hpp>

#include <Prism/Utility/Math.hpp>

#include <Scheduler/Process.hpp>
#include <Scheduler/Scheduler.hpp>
#include <Scheduler/Thread.hpp>

extern u8 g_ScheduleVector;
u8        g_PanicIpiVector = 255;

namespace CPU
{
    namespace
    {
        inline void XSave(uintptr_t ctx)
        {
            __asm__ volatile("xsave (%0)"
                             :
                             : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                             : "memory");
        }
        inline void XRestore(uintptr_t ctx)
        {
            __asm__ volatile("xrstor (%0)"
                             :
                             : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                             : "memory");
        }

        inline void FXSave(uintptr_t ctx)
        {
            __asm__ volatile("fxsave (%0)" : : "r"(ctx) : "memory");
        }
        inline void FXRestore(uintptr_t ctx)
        {
            __asm__ volatile("fxrstor (%0)" : : "r"(ctx) : "memory");
        }

        Vector<CPU> s_CPUs;
        CPU*        s_BSP             = nullptr;
        u64         s_BspLapicId      = 0;
        usize       s_OnlineCPUsCount = 1;
    } // namespace

    extern "C" __attribute__((noreturn)) void syscall_entry();

    static void                               InitializeFPU()
    {
        if (!EnableSSE()) return;
        CPU* current = GetCurrent();
        bool isBSP   = current->LapicID == s_BspLapicId;

        ID   cpuid(CPUID_CHECK_FEATURES, 0);
        if (cpuid.rcx & CPU_FEAT_ECX_XSAVE)
        {
            if (isBSP) LogInfo("FPU: XSave supported");

            WriteCR4(ReadCR4() | CR4::OSXSAVE);
            u64 xcr0 = 0;
            if (isBSP) LogInfo("FPU: Saving x87 state using XSave");
            xcr0 |= XCR0::X87;
            if (isBSP) LogInfo("FPU: Saving SSE state using XSave");
            xcr0 |= XCR0::SSE;

            if (cpuid.rcx & CPU_FEAT_ECX_AVX)
            {
                if (isBSP) LogInfo("FPU: Saving AVX state using XSave");
                xcr0 |= XCR0::AVX;
            }

            if (cpuid(7) && (cpuid.rbx & CPU_FEAT_EBX_AVX512))
            {
                if (isBSP) LogInfo("FPU: Saving AVX-512 state using xsave");
                xcr0 |= XCR0::OPMASK;
                xcr0 |= XCR0::ZMM_HI256;
                xcr0 |= XCR0::ZMM_HI16;
            }

            WriteXCR(0, xcr0);
            if (!cpuid(0xd)) Panic("CPUID failure");

            current->FpuStorageSize = cpuid.rcx;
            current->FpuRestore     = XRestore;
            current->FpuSave        = XSave;
        }
        else if (cpuid.rdx & 0x01000000)
        {
            WriteCR4(ReadCR4() | CR4::OSFXSR);

            current->FpuStorageSize = 512;
            current->FpuSave        = FXSave;
            current->FpuRestore     = FXRestore;
        }
        else Panic("SIMD: Unavailable");

        LogInfo("FPU: Initialized on cpu[{}]", current->ID);
    }

    static void InitializeCPU(limine_mp_info* cpu)
    {
        CPU* current = reinterpret_cast<CPU*>(cpu->extra_argument);

        if (current->LapicID != s_BspLapicId)
        {
            EnablePAT();
            VMM::GetKernelPageMap()->Load();

            GDT::Initialize();
            GDT::Load(current->ID);

            current->TSS.rsp[0] = Pointer(PMM::CallocatePages(KERNEL_STACK_SIZE
                                                              / PMM::PAGE_SIZE))
                                      .ToHigherHalf<Pointer>()
                                      .Offset<uintptr_t>(KERNEL_STACK_SIZE);
            GDT::LoadTSS((&current->TSS));
            IDT::Load();

            SetKernelGSBase(cpu->extra_argument);
            SetGSBase(cpu->extra_argument);
        }

        EnablePAT();
        EnableSMEP();
        EnableSMAP();
        EnableUMIP();

        InitializeFPU();

        // Setup syscalls
        WriteMSR(MSR::IA32_EFER,
                 ReadMSR(MSR::IA32_EFER) | MSR::IA32_EFER_SYSCALL_ENABLE);

        // Userland CS = MSR::STAR(63:48) + 16
        // Userland SS = MSR::STAR(63:48) + 8
        WriteMSR(
            MSR::STAR,
            (reinterpret_cast<uint64_t>(GDT::KERNEL_DATA_SELECTOR | 0x03) << 48)
                | (reinterpret_cast<uint64_t>(GDT::KERNEL_CODE_SELECTOR)
                   << 32));

        // Syscall EntryPoint
        WriteMSR(MSR::LSTAR, reinterpret_cast<uintptr_t>(syscall_entry));
        WriteMSR(MSR::SFMASK, ~u32(2));

        Lapic::Instance()->Initialize();

        GetCurrent()->TSS.ist[0]
            = ToHigherHalfAddress<uintptr_t>(PMM::AllocatePages<uintptr_t>(
                  KERNEL_STACK_SIZE / PMM::PAGE_SIZE))
            + KERNEL_STACK_SIZE;
        IDT::SetIST(14, 2);
    }

    void InitializeBSP()
    {
        limine_mp_response* smp      = BootInfo::GetSMP_Response();
        usize               cpuCount = smp->cpu_count;

        s_CPUs.Resize(cpuCount);
        s_BspLapicId = smp->bsp_lapic_id;

        LogTrace("BSP: Initializing...");
        for (usize i = 0; i < cpuCount; i++)
        {
            limine_mp_info* smpInfo = smp->cpus[i];
            // Find the bsp
            if (smpInfo->lapic_id != s_BspLapicId) continue;

            smpInfo->extra_argument = Pointer(&s_CPUs[i]);
            s_CPUs[i].LapicID       = smpInfo->lapic_id;
            s_CPUs[i].ID            = i;
            s_CPUs[i].Lock          = new Spinlock;

            CPU* current = reinterpret_cast<CPU*>(smpInfo->extra_argument);
            s_BSP        = current;

            GDT::Initialize();
            GDT::Load(current->LapicID);

            current->TSS.rsp[0] = Pointer(PMM::CallocatePages(KERNEL_STACK_SIZE
                                                              / PMM::PAGE_SIZE))
                                      .ToHigherHalf<Pointer>()
                                      .Offset<uintptr_t>(KERNEL_STACK_SIZE);
            GDT::LoadTSS(&current->TSS);

            IDT::Initialize();
            IDT::Load();

            SetKernelGSBase(smpInfo->extra_argument);
            SetGSBase(smpInfo->extra_argument);

            Lapic::Instance()->Initialize();
            InitializeCPU(smpInfo);
            current->IsOnline = true;
        }

        IDT::SetIST(g_ScheduleVector, 1);
        LogInfo("BSP: Initialized");
    }
    void StartAPs()
    {
        LogTrace("SMP: Launching APs");

        auto apEntryPoint = [](limine_mp_info* cpu)
        {
            CPU* current = reinterpret_cast<CPU*>(cpu->extra_argument);
            static Spinlock s_Lock;
            {
                ScopedLock guard(s_Lock);
                InitializeCPU(cpu);

                current->IsOnline = true;
                LogInfo("SMP: CPU {} is up", current->ID);
            }

            Scheduler::PrepareAP();
            Halt();
        };

        auto smpResponse = BootInfo::GetSMP_Response();
        for (usize i = 0; i < smpResponse->cpu_count; i++)
        {
            auto cpu = smpResponse->cpus[i];
            if (cpu->lapic_id == s_BspLapicId) continue;

            cpu->extra_argument = reinterpret_cast<uintptr_t>(&s_CPUs[i]);
            s_CPUs[i].LapicID   = cpu->lapic_id;
            s_CPUs[i].ID        = i;
            s_CPUs[i].Lock      = new Spinlock;

            cpu->goto_address   = apEntryPoint;
            while (!s_CPUs[i].IsOnline) Arch::Pause();
        }

        auto handler = IDT::GetHandler(255);
        handler->Reserve();
        handler->SetInterruptVector(255);
        handler->SetHandler(HaltAndCatchFire);
    }

    ID::ID(u64 leaf, u64 subleaf)
    {
        __asm__ volatile("cpuid"
                         : "=a"(rax), "=b"(rbx), "=c"(rcx), "=d"(rdx)
                         : "a"(leaf), "c"(subleaf));
    }
    bool ID::operator()(u64 leaf, u64 subleaf)
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

    void DumpRegisters(CPUContext* ctx)
    {
        LogInfo("RAX: {:#x}, RBX: {:#x}, RCX: {:#x}, RDX: {:#x}", ctx->rax,
                ctx->rbx, ctx->rcx, ctx->rdx);
        LogInfo("RSI: {:#x}, RDI: {:#x}, RBP: {:#x}, RSP: {:#x}", ctx->rsi,
                ctx->rdi, ctx->rbp, ctx->rsp);
        LogInfo("R8: {:#x}, R9: {:#x}, R10: {:#x}, R11: {:#x}", ctx->r8,
                ctx->r9, ctx->r10, ctx->r11);
        LogInfo("R12: {:#x}, R13: {:#x}, R14: {:#x}, R15: {:#x}", ctx->r12,
                ctx->r13, ctx->r14, ctx->r15);
        LogInfo("CS: {:#x}, SS: {:#x}, DS: {:#x}, ES: {:#x}", ctx->cs, ctx->ss,
                ctx->ds, ctx->es);
        LogInfo("RIP: {:#x}, RFLAGS: {:#b}", ctx->rip, ctx->rflags);
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
    bool SwapInterruptFlag(bool enabled)
    {
        bool interruptFlag = GetInterruptFlag();

        if (enabled) SetInterruptFlag(true);
        else SetInterruptFlag(false);

        return interruptFlag;
    }
    void Halt() { __asm__ volatile("hlt"); }
    void HaltAll()
    {
        Lapic::Instance()->SendIpi(g_PanicIpiVector | (0b10 < 18), 0);
    }
    void WakeUp(usize id, bool everyone)
    {
        if (everyone)
            Lapic::Instance()->SendIpi(g_ScheduleVector | (0b10 << 18), 0);
        else
            Lapic::Instance()->SendIpi(g_ScheduleVector | s_CPUs[id].LapicID,
                                       0);
    }

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
        if (ReadCR4() & Bit(21)) __asm__ volatile("stac" ::: "cc");
    }
    void Clac()
    {
        if (ReadCR4() & Bit(21)) __asm__ volatile("clac" ::: "cc");
    }

    uintptr_t GetFSBase() { return ReadMSR(MSR::FS_BASE); }
    uintptr_t GetGSBase() { return ReadMSR(MSR::GS_BASE); }
    uintptr_t GetKernelGSBase() { return ReadMSR(MSR::KERNEL_GS_BASE); }

    void      SetFSBase(uintptr_t address) { WriteMSR(MSR::FS_BASE, address); }
    void      SetGSBase(uintptr_t address) { WriteMSR(MSR::GS_BASE, address); }
    void      SetKernelGSBase(uintptr_t address)
    {
        WriteMSR(MSR::KERNEL_GS_BASE, address);
    }

    Vector<CPU>& GetCPUs() { return s_CPUs; }

    u64          GetOnlineCPUsCount() { return s_OnlineCPUsCount; }
    u64          GetBspId() { return s_BspLapicId; }
    CPU&         GetBsp() { return *s_BSP; }

    u64          GetCurrentID()
    {
        return GetOnlineCPUsCount() > 1 ? GetCurrent()->ID : s_BspLapicId;
    }
    CPU* GetCurrent()
    {
        usize id;
        __asm__ volatile("mov %%gs:0, %0" : "=r"(id)::"memory");

        return &s_CPUs[id];
    }
    Thread* GetCurrentThread()
    {
        Thread* currentThread;
        __asm__ volatile("mov %%gs:8, %0" : "=r"(currentThread)::"memory");

        return currentThread;
    }

    void PrepareThread(Thread* thread, uintptr_t pc, uintptr_t arg)
    {
        CPU* current       = GetCurrent();
        thread->ctx.rflags = 0x202;
        thread->ctx.rip    = pc;
        thread->ctx.rdi    = arg;

        uintptr_t kernelStack
            = PMM::AllocatePages<uintptr_t>(KERNEL_STACK_SIZE / PMM::PAGE_SIZE);
        thread->kernelStack
            = ToHigherHalfAddress<uintptr_t>(kernelStack) + KERNEL_STACK_SIZE;
        current->KernelStack = thread->kernelStack;

        uintptr_t pfStack
            = PMM::AllocatePages<uintptr_t>(KERNEL_STACK_SIZE / PMM::PAGE_SIZE);
        thread->pageFaultStack
            = ToHigherHalfAddress<uintptr_t>(pfStack) + KERNEL_STACK_SIZE;

        usize fpuPageCount = thread->fpuStoragePageCount
            = Math::DivRoundUp(current->FpuStorageSize, PMM::PAGE_SIZE);
        thread->fpuStorage
            = ToHigherHalfAddress<uintptr_t>(PMM::CallocatePages(fpuPageCount));

        thread->SetGsBase(reinterpret_cast<uintptr_t>(thread));
        if (thread->IsUser())
        {
            thread->ctx.cs = GDT::USERLAND_CODE_SELECTOR | 0x03;
            thread->ctx.ss = GDT::USERLAND_DATA_SELECTOR | 0x03;
            thread->ctx.ds = thread->ctx.es = thread->ctx.ss;

            thread->ctx.rsp                 = thread->stack;
            current->FpuRestore(thread->fpuStorage);

            u16 defaultFcw = 0b1100111111;
            __asm__ volatile("fldcw %0" ::"m"(defaultFcw) : "memory");
            u32 defaultMxCsr = 0b1111110000000;
            asm volatile("ldmxcsr %0" ::"m"(defaultMxCsr) : "memory");

            current->FpuSave(thread->fpuStorage);
        }
        else
        {
            thread->ctx.cs = GDT::KERNEL_CODE_SELECTOR;
            thread->ctx.ss = GDT::KERNEL_DATA_SELECTOR;
            thread->ctx.ds = thread->ctx.es = thread->ctx.ss;

            thread->ctx.rsp = thread->stack = thread->kernelStack;
        }
    }
    void SaveThread(Thread* thread, CPUContext* ctx)
    {
        thread->ctx = *ctx;

        thread->SetGsBase(GetKernelGSBase());
        thread->SetFsBase(GetFSBase());

        GetCurrent()->FpuSave(thread->fpuStorage);
    }
    void LoadThread(Thread* thread, CPUContext* ctx)
    {
        thread->runningOn        = GetCurrent()->ID;

        GetCurrent()->TSS.ist[1] = thread->pageFaultStack;
        GetCurrent()->FpuRestore(thread->fpuStorage);

        thread->GetParent()->PageMap->Load();

        SetGSBase(reinterpret_cast<u64>(thread));
        SetKernelGSBase(thread->GetGsBase());
        SetFSBase(thread->GetFsBase());

        *ctx = thread->ctx;
    }
    void Reschedule(TimeStep interval)
    {
        Assert(Lapic::Instance()->Start(TimerMode::eOneShot, interval));
    }

    bool EnableSSE()
    {
        ID   cpuid;
        bool sseAvailable
            = cpuid(CPUID_CHECK_FEATURES) && (cpuid.rdx & CPU_FEAT_EDX_SSE);

        if (!sseAvailable)
        {
            LogError("SSE: Not Supported");
            return false;
        }

        u64 cr0 = ReadCR0() & ~CR0::EM;
        WriteCR0(cr0 | CR0::MP);

        u64 cr4 = ReadCR4();
        WriteCR4(cr4 | CR4::OSFXSR | CR4::OSXMMEXCPT);

        LogInfo("SSE: Enabled on cpu[{}]", GetCurrent()->ID);
        return true;
    }
    void EnablePAT()
    {

        // write-combining/write-protect
        u64 pat = ReadMSR(MSR::IA32_PAT);
        pat &= 0xffffffffull;
        pat |= 0x0105ull << 32ull;

        pat = (PAT::eUncacheable << 56) | (PAT::eWriteBack << 48)
            | (PAT::eWriteProtected << 40) | (PAT::eWriteThrough << 32)
            | (PAT::eWriteCombining << 24) | (PAT::eUncacheableStrong << 16);

        WriteMSR(MSR::IA32_PAT, pat);
    }
    void EnableSMEP()
    {
        ID id(7, 0);
        if (id.rbx & CPU_FEAT_EBX_SMEP) WriteCR4(ReadCR4() | CR4::SMEP);
    }
    void EnableSMAP()
    {
        ID id(7, 0);
        if (id.rbx & CPU_FEAT_EBX_SMAP)
        {
            WriteCR4(ReadCR4() | CR4::SMAP);
            Clac();
        }
    }
    void EnableUMIP()
    {
        ID id(7, 0);
        if (id.rcx & CPU_FEAT_ECX_UMIP) WriteCR4(ReadCR4() | CR4::UMIP);
    }

    UserMemoryProtectionGuard::UserMemoryProtectionGuard() { Stac(); }
    UserMemoryProtectionGuard::~UserMemoryProtectionGuard() { Clac(); }

    bool DuringSyscall()
    {
        auto       cpu = GetCurrent();
        ScopedLock guard(*cpu->Lock);

        return cpu->DuringSyscall;
    }
    void OnSyscallEnter(usize index)
    {
        auto       cpu = GetCurrent();
        ScopedLock guard(*cpu->Lock);

        cpu->DuringSyscall = true;
        cpu->LastSyscallID = index;
    }
    void OnSyscallLeave()
    {
        auto       cpu = GetCurrent();
        ScopedLock guard(*cpu->Lock);

        cpu->DuringSyscall = false;
    }
}; // namespace CPU

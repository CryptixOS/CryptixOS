/*
 * Created by v1tr10l7 on 09.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Boot/BootModuleInfo.hpp>
#include <Firmware/ACPI/ACPI.hpp>
#include <Firmware/ACPI/SRAT.hpp>

#include <Library/ELF.hpp>
#include <Library/Module.hpp>

#include <System/System.hpp>
#include <VFS/DirectoryEntry.hpp>
#include <VFS/VFS.hpp>

extern "C" ModuleHeader module_init_start_addr[];
extern "C" ModuleHeader module_init_end_addr[];

namespace System
{
    namespace
    {
        BootModuleInfo                      s_KernelExecutable;
        ELF::Image                          s_KernelImage;
        RedBlackTree<StringView, u64>       s_KernelSymbols;
        Span<BootModuleInfo, DynamicExtent> s_BootModules;

        static constexpr Pointer            MODULE_LOAD_BASE = nullptr;
        Module::List                        s_Modules;
    }; // namespace

    ErrorOr<void> LoadKernelSymbols(const BootModuleInfo& kernelExecutable)
    {
        LogTrace("System: Loading kernel symbols...");

        s_KernelExecutable    = kernelExecutable;
        Pointer kernelAddress = kernelExecutable.LoadAddress;
        usize   kernelSize    = kernelExecutable.Size;

        if (!kernelAddress || !kernelSize)
        {
            LogTrace("System: Failed to locate the kernel executable");
            return Error(ENOENT);
        }

        LogTrace(
            "System: Loaded the kernel executable with size `{}`KiB at `{:#x}`",
            kernelSize / 1024, kernelAddress.Raw());

        auto status = s_KernelImage.LoadFromMemory(kernelAddress, kernelSize);
        if (!status)
        {
            LogError("System: Failed to load the kernel image");
            return Error(ENOEXEC);
        }

        ELF::Image::SymbolEnumerator it;
        it.BindLambda(
            [&](StringView name, Pointer value) -> bool
            {
                if (name.Empty()) return true;

                s_KernelSymbols[name] = value;
                return true;
            });

        s_KernelImage.ForEachSymbol(it);
        LogTrace("System: Successfully loaded the kernel symbols");

        Stacktrace::Initialize();
        return {};
    }
    void PrepareBootModules(Span<BootModuleInfo> bootModules)
    {
        s_BootModules = bootModules;
    }
    const BootModuleInfo* FindBootModule(StringView name)
    {
        for (const auto& module : s_BootModules)
            if (module.Name == name) return &module;

        return nullptr;
    }
    void InitializeNumaDomains()
    {
        using SRAT        = ACPI::SRAT::SRAT;
        using EntryHeader = ACPI::SRAT::EntryHeader;
        using EntryType   = ACPI::SRAT::Type;

        LogTrace("System: Retrieving numa domains information from SRAT table");
        auto    srat      = ACPI::GetTable<SRAT>("SRAT");
        usize   length    = srat->Header.Length;

        Pointer start     = srat;
        Pointer end       = start.Offset(length);
        Pointer current   = start.Offset(sizeof(SRAT));

        usize   nodeIndex = 0;
        while (current < end)
        {
            auto entry = current.As<EntryHeader>();
            LogTrace("System: NUMA Node[{}] => `{}`", nodeIndex++,
                     ToString(entry->Type));

            switch (entry->Type)
            {
                case EntryType::eCPUAffinity:
                {
                    auto  cpuEntry = current.As<ACPI::SRAT::CPU_Affinity>();
                    usize proximityDomain = 0;
                    for (usize i = 0; i < 3; i++)
                        proximityDomain = (proximityDomain << (i << 3))
                                        | cpuEntry->ProximityDomainHigh[i];
                    proximityDomain
                        = (proximityDomain << 8) | cpuEntry->ProximityDomainLow;

                    LogMessage("\t\tProximity Domain: {}\n", proximityDomain);
                    LogMessage("\t\tAPIC ID: {}\n", cpuEntry->ApicID);
                    LogMessage("\t\tLocal SAPIC EID: {}\n",
                               cpuEntry->LocalSapicEID);
                    LogMessage("\t\tClock Domain: {}\n", cpuEntry->ClockDomain);
                    break;
                }
                case EntryType::eMemoryAffinity:
                {
                    auto memoryEntry = current.As<ACPI::SRAT::MemoryAffinity>();
                    usize base       = memoryEntry->BaseAddress;
                    usize length     = memoryEntry->Length;

                    LogMessage("\t\tProximity Domain: {}\n",
                               memoryEntry->ProximityDomain);
                    LogMessage("\t\tMemory range: <{:#x}-{:#x}>\n", base,
                               length);
                    break;
                }
                case EntryType::eX2ApicAffinity:
                {
                    auto x2apic = current.As<ACPI::SRAT::X2ApicAffinity>();
                    LogMessage("\t\tProximity Domain: {}\n",
                               x2apic->ProximityDomain);
                    LogMessage("\t\tX2APIC ID: {}\n", x2apic->ApicID);
                    LogMessage("\t\tClock Domain: {}\n", x2apic->ClockDomain);
                    break;
                }
                case EntryType::eGiccAffinity:
                {
                    auto gicc = current.As<ACPI::SRAT::GiccAffinity>();
                    LogMessage("\t\tProximity Domain: {}\n",
                               gicc->ProximityDomain);
                    LogMessage("\t\tProcessor UID: {}\n", gicc->ProcessorUID);
                    LogMessage("\t\tClock Domain: {}\n", gicc->ClockDomain);
                    break;
                }
                case EntryType::eGicItsAffinity:
                {
                    auto gicits = current.As<ACPI::SRAT::GicItsAffinity>();
                    LogMessage("\t\tProximity Domain: {}\n",
                               gicits->ProximityDomain);
                    LogMessage("\t\tItsID: {}\n", gicits->ItsID);
                    break;
                }
                case EntryType::eGenericAffinity:
                {
                    auto generic = current.As<ACPI::SRAT::GenericAffinity>();
                    LogMessage("\t\tProximity Domain: {}\n",
                               generic->ProximityDomain);
                    LogMessage("\t\tDevice Handle Type: {}\n",
                               generic->DeviceHandleType);
                    // LogMessage("\t\tDevice Handle: {}\n",
                    // generic->DeviceHandle);
                    break;
                }
                case EntryType::eGenericPortAffinity:
                {
                    auto genericPort
                        = current.As<ACPI::SRAT::GenericAffinity>();
                    LogMessage("\t\tProximity Domain: {}\n",
                               genericPort->ProximityDomain);
                    break;
                }
                case EntryType::eRintcAffinity:
                {
                    auto rintc = current.As<ACPI::SRAT::RintcAffinity>();
                    LogMessage("\t\tProximity Domain: {}\n",
                               rintc->ProximityDomain);
                    LogMessage("\t\tProcessor UID: {}\n", rintc->ProcessorUID);
                    LogMessage("\t\tClock Domain: {}\n", rintc->ClockDomain);
                    break;
                }

                default: break;
            }

            current += entry->Length;
        }
    }

    ErrorOr<void> LoadModules()
    {

        auto modulesStart = module_init_start_addr;
        auto modulesEnd   = module_init_end_addr;
        for (ModuleHeader* moduleHeader = modulesStart;
             moduleHeader < modulesEnd; moduleHeader++)
        {
            auto module         = CreateRef<Module>();
            module->Name        = moduleHeader->Name;
            module->Initialized = false;
            module->Failed      = false;
            module->Initialize  = moduleHeader->Initialize;
            module->Terminate   = moduleHeader->Terminate;

            auto status         = System::LoadModule(module);
            if (!status)
                ;
        }

        return {};
    }

    ErrorOr<void> LoadModule(PathView path)
    {
        auto maybePathRes = VFS::ResolvePath(nullptr, path);
        RetOnError(maybePathRes);

        auto pathRes = maybePathRes.value();
        auto entry   = pathRes.Entry;

        if (!entry) return Error(ENOENT);
        return LoadModule(entry);
    }
    ErrorOr<void> LoadModule(Ref<DirectoryEntry> entry)
    {
        if (!entry) return Error(EFAULT);

        auto module            = new Module;

        module->Name           = entry->Name();
        module->Initialized    = false;
        module->Failed         = false;

        Ref<ELF::Image> image  = CreateRef<ELF::Image>();
        auto            status = image->Load(entry->INode(), MODULE_LOAD_BASE);
        if (!status)
        {
            LogError("System: Failed to load the module located at `{}`",
                     entry->Path());
            delete module;
            return Error(status.error());
        }

        module->Image = image;
#if 0
        LogTrace("Module: Dumping symbols...");
        image->DumpSymbols();
#endif

        ELF::Image::SymbolEntryEnumerator it;
        it.BindLambda(
            [&](ELF::Symbol& symbol, StringView name) -> bool
            {
                // if (symbol.SectionIndex != 0) return true;

                if (name == "ModuleInit")
                {
                    LogInfo("System: Found module's init entry point => {:#x}",
                            symbol.Value);
                    module->Initialize
                        = reinterpret_cast<ModuleInitProc>(symbol.Value);
                }
                // u64 resolvedValue = System::LookupKernelSymbol(name);
                // LogTrace("Resolved symbol =>\n\tName: {}, Value: {:#x}",
                // name,
                //          resolvedValue);

                return true;
            });

        ELF::Image::SymbolLookup lookup;
        lookup.Bind<LookupKernelSymbol>();
        status = image->ApplyRelocations(lookup);
        // image->ForEachSymbolEntry(it);
        // status = image->ResolveSymbols(lookup);
        if (!status)
            LogError("System: Failed to resolve symbols of module `{}`",
                     module->Name);
        else
            LogInfo("System: Successfully resolved symbols of module `{}`",
                    module->Name);
        if (module->Initialize) module->Initialize();
        else
            LogError("System: Module `{}` doesn't have any entry point",
                     module->Name);

        // TODO(v1tr10l7): Load the module

        // delete module;
        return Error(ENOSYS);
    }
    ErrorOr<void> LoadModule(Ref<Module> module)
    {
        StringView name = module->Name;
        LogTrace("System: Loading module `{}`...", name);

        if (FindModule(name))
        {
            LogError("System: Module `{}` is already loaded", name);
            return Error(EEXIST);
        }

        s_Modules.PushBack(module);
        LogTrace("System: Dispatching module `{}`...", name);

        auto status = module->Initialize();
        if (!status)
        {
            LogError("System: Failed to initialize module `{}`", name);
            return Error(ENODEV);
        }

        LogInfo("System: Module `{}` successfully loaded", name);
        return {};
    }

    Module::List& Modules() { return s_Modules; }
    Ref<Module>   FindModule(StringView name)
    {
        for (auto& module : s_Modules)
            if (module.Name == name) return &module;

        return nullptr;
    }

    PathView          KernelExecutablePath() { return s_KernelExecutable.Path; }
    const ELF::Image& KernelImage() { return s_KernelImage; }
    const RedBlackTree<StringView, u64>& KernelSymbols()
    {
        return s_KernelSymbols;
    }

    u64 LookupKernelSymbol(StringView name)
    {
        auto it = s_KernelSymbols.Find(name);
        if (it == s_KernelSymbols.end()) return 0;

        return it->Value;
    }
}; // namespace System

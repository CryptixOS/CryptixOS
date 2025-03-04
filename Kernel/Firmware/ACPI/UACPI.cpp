/*
 * Created by v1tr10l7 on 02.12.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Drivers/PCI/Device.hpp>
#include <Drivers/PCI/HostController.hpp>
#include <Drivers/PCI/PCI.hpp>
#include <Firmware/ACPI/ACPI.hpp>

#include <Library/Mutex.hpp>
#include <Memory/MMIO.hpp>

#include <Time/Time.hpp>

#include <Prism/Math.hpp>
#include <Prism/Spinlock.hpp>

#include <uacpi/kernel_api.h>

namespace uACPI
{
    extern "C"
    {
        uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr* outRsdp)
        {
            uacpi_phys_addr rsdp = BootInfo::GetRSDPAddress();

            if (rsdp)
            {
                *outRsdp = rsdp;
                return UACPI_STATUS_OK;
            }

            return UACPI_STATUS_NOT_FOUND;
        }

        uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address,
                                                  uacpi_u8        byteWidth,
                                                  uacpi_u64*      outValue)
        {
            uacpi_status status = UACPI_STATUS_OK;
            void*        virt   = uacpi_kernel_map(address, byteWidth);

            switch (byteWidth)
            {
                case 1: *outValue = MMIO::Read<u8>(virt); break;
                case 2: *outValue = MMIO::Read<u16>(virt); break;
                case 4: *outValue = MMIO::Read<u32>(virt); break;
                case 8: *outValue = MMIO::Read<u64>(virt); break;

                default: status = UACPI_STATUS_INVALID_ARGUMENT; break;
            }

            uacpi_kernel_unmap(virt, byteWidth);
            return status;
        }
        uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address,
                                                   uacpi_u8        byteWidth,
                                                   uacpi_u64       inValue)
        {
            uacpi_status status = UACPI_STATUS_OK;
            void*        virt   = uacpi_kernel_map(address, byteWidth);
            switch (byteWidth)
            {
                case 1: MMIO::Write<u8>(virt, inValue); break;
                case 2: MMIO::Write<u16>(virt, inValue); break;
                case 4: MMIO::Write<u32>(virt, inValue); break;
                case 8: MMIO::Write<u64>(virt, inValue); break;

                default: status = UACPI_STATUS_INVALID_ARGUMENT; break;
            }

            uacpi_kernel_unmap(virt, byteWidth);
            return status;
        }

        uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr address,
                                              uacpi_u8      byteWidth,
                                              uacpi_u64*    outValue)
        {
            u16 port = address;
            switch (byteWidth)
            {
                case 1: *outValue = IO::In<u8>(port); break;
                case 2: *outValue = IO::In<u16>(port); break;
                case 4: *outValue = IO::In<u32>(port); break;
                default: return UACPI_STATUS_INVALID_ARGUMENT;
            }

            return UACPI_STATUS_OK;
        }
        uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr address,
                                               uacpi_u8      byteWidth,
                                               uacpi_u64     inValue)
        {
            u16 port = address;
            switch (byteWidth)
            {
                case 1: IO::Out<u8>(port, inValue); break;
                case 2: IO::Out<u16>(port, inValue); break;
                case 4: IO::Out<u32>(port, inValue); break;
                default: return UACPI_STATUS_INVALID_ARGUMENT;
            }

            return UACPI_STATUS_OK;
        }

        uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address address,
                                                  uacpi_handle*     outHandle)
        {
            PCI::DeviceAddress* addr = new PCI::DeviceAddress;
            addr->Domain             = address.segment;
            addr->Bus                = address.bus;
            addr->Function           = address.function;
            addr->Slot               = address.device;

            *outHandle               = addr;
            return UACPI_STATUS_OK;
        }
        void uacpi_kernel_pci_device_close(uacpi_handle handle)
        {
            delete reinterpret_cast<PCI::DeviceAddress*>(handle);
        }

        uacpi_status uacpi_kernel_pci_read8(uacpi_handle device,
                                            uacpi_size offset, uacpi_u8* value)
        {
            auto& addr = *reinterpret_cast<PCI::DeviceAddress*>(device);
            *value = PCI::GetHostController(addr.Domain)->Read(addr, offset, 1);

            return UACPI_STATUS_OK;
        }
        uacpi_status uacpi_kernel_pci_read16(uacpi_handle device,
                                             uacpi_size   offset,
                                             uacpi_u16*   value)
        {
            auto& addr = *reinterpret_cast<PCI::DeviceAddress*>(device);
            *value = PCI::GetHostController(addr.Domain)->Read(addr, offset, 2);

            return UACPI_STATUS_OK;
        }
        uacpi_status uacpi_kernel_pci_read32(uacpi_handle device,
                                             uacpi_size   offset,
                                             uacpi_u32*   value)
        {
            auto& addr = *reinterpret_cast<PCI::DeviceAddress*>(device);
            *value = PCI::GetHostController(addr.Domain)->Read(addr, offset, 4);

            return UACPI_STATUS_OK;
        }
        uacpi_status uacpi_kernel_pci_write8(uacpi_handle device,
                                             uacpi_size offset, uacpi_u8 value)
        {
            auto& addr = *reinterpret_cast<PCI::DeviceAddress*>(device);
            PCI::GetHostController(addr.Domain)->Write(addr, offset, value, 1);

            return UACPI_STATUS_OK;
        }
        uacpi_status uacpi_kernel_pci_write16(uacpi_handle device,
                                              uacpi_size   offset,
                                              uacpi_u16    value)
        {
            auto& addr = *reinterpret_cast<PCI::DeviceAddress*>(device);
            PCI::GetHostController(addr.Domain)->Write(addr, offset, value, 2);

            return UACPI_STATUS_OK;
        }
        uacpi_status uacpi_kernel_pci_write32(uacpi_handle device,
                                              uacpi_size   offset,
                                              uacpi_u32    value)
        {
            auto& addr = *reinterpret_cast<PCI::DeviceAddress*>(device);
            PCI::GetHostController(addr.Domain)->Write(addr, offset, value, 4);

            return UACPI_STATUS_OK;
        }

        uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len,
                                         uacpi_handle* outHandle)
        {
            *outHandle = reinterpret_cast<uacpi_handle>(base);

            return UACPI_STATUS_OK;
        }
        void         uacpi_kernel_io_unmap(uacpi_handle) {}

        uacpi_status uacpi_kernel_io_read8(uacpi_handle handle,
                                           uacpi_size offset, uacpi_u8* out)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_read(addr + offset, 1,
                                            reinterpret_cast<uacpi_u64*>(out));
        }
        uacpi_status uacpi_kernel_io_read16(uacpi_handle handle,
                                            uacpi_size offset, uacpi_u16* out)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_read(addr + offset, 2,
                                            reinterpret_cast<uacpi_u64*>(out));
        }
        uacpi_status uacpi_kernel_io_read32(uacpi_handle handle,
                                            uacpi_size offset, uacpi_u32* out)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_read(addr + offset, 4,
                                            reinterpret_cast<uacpi_u64*>(out));
        }
        uacpi_status uacpi_kernel_io_write8(uacpi_handle handle,
                                            uacpi_size offset, uacpi_u8 value)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_write(addr + offset, 1, value);
        }
        uacpi_status uacpi_kernel_io_write16(uacpi_handle handle,
                                             uacpi_size offset, uacpi_u16 value)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_write(addr + offset, 2, value);
        }
        uacpi_status uacpi_kernel_io_write32(uacpi_handle handle,
                                             uacpi_size offset, uacpi_u32 value)
        {
            auto addr = reinterpret_cast<uacpi_io_addr>(handle);
            return uacpi_kernel_raw_io_write(addr + offset, 4, value);
        }

        void* uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
        {
            PageMap*  pageMap = VMM::GetKernelPageMap();

            uintptr_t phys    = Math::AlignDown(addr, PMM::PAGE_SIZE);
            uintptr_t virt    = VMM::AllocateSpace(len, PMM::PAGE_SIZE, true);
            len = Math::AlignUp(addr - phys + len, PMM::PAGE_SIZE);
            pageMap->MapRange(virt, phys, len);
            return reinterpret_cast<u8*>(virt) + (addr - phys);
        }
        void uacpi_kernel_unmap(void* addr, uacpi_size len)
        {
            PageMap*  pageMap = VMM::GetKernelPageMap();
            uintptr_t virt = Math::AlignDown(reinterpret_cast<uintptr_t>(addr),
                                             PMM::PAGE_SIZE);
            len = Math::AlignUp(reinterpret_cast<uintptr_t>(addr) - virt + len,
                                PMM::PAGE_SIZE);

            pageMap->UnmapRange(virt, len);
        }

        void* uacpi_kernel_alloc(uacpi_size size) { return new u8[size]; }
        void* uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
        {
            return new u8[count * size];
        }

        void uacpi_kernel_free(void* memory)
        {
            delete[] reinterpret_cast<u8*>(memory);
        }

        void uacpi_kernel_log(uacpi_log_level level, const uacpi_char* message)
        {
            char* c = const_cast<char*>(message);
            for (; *c != 0 && *c != '\n'; c++)
                ;
            *c = 0;
            switch (level)
            {
                case UACPI_LOG_DEBUG: EarlyLogDebug("%s", message); break;
                case UACPI_LOG_TRACE: EarlyLogTrace("%s", message); break;
                case UACPI_LOG_INFO: EarlyLogInfo("%s", message); break;
                case UACPI_LOG_WARN: EarlyLogWarn("%s", message); break;
                case UACPI_LOG_ERROR: EarlyLogError("%s", message); break;
            }
        }

        uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void)
        {
            return Time::GetEpoch() - BootInfo::GetDateAtBoot();
        }
        void uacpi_kernel_stall(uacpi_u8 usec)
        {
            CtosUnused(usec);
            ToDoWarn();
        }
        void uacpi_kernel_sleep(uacpi_u64 msec)
        {
            CtosUnused(msec);
            ToDoWarn();
        }

        uacpi_handle uacpi_kernel_create_mutex(void)
        {
            auto mutex = new std::mutex;

            return reinterpret_cast<uacpi_handle>(mutex);
        }
        void uacpi_kernel_free_mutex(uacpi_handle handle)
        {
            delete reinterpret_cast<std::mutex*>(handle);
        }

        uacpi_handle uacpi_kernel_create_event(void)
        {
            Event* event = new Event;

            return reinterpret_cast<uacpi_handle>(event);
        }
        void uacpi_kernel_free_event(uacpi_handle handle)
        {
            delete reinterpret_cast<Event*>(handle);
        }

        uacpi_thread_id uacpi_kernel_get_thread_id(void)
        {
            auto thread = Thread::GetCurrent();
            if (thread)
                return reinterpret_cast<uacpi_thread_id>(thread->GetTid());

            return UACPI_THREAD_ID_NONE;
        }

        uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle,
                                                uacpi_u16    timeout)
        {
            auto& mutex = *reinterpret_cast<std::mutex*>(handle);
            mutex.lock();

            return UACPI_STATUS_OK;
        }
        void uacpi_kernel_release_mutex(uacpi_handle handle)
        {
            auto& mutex = *reinterpret_cast<std::mutex*>(handle);
            mutex.unlock();
        }

        uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16)
        {
            auto&      event = *reinterpret_cast<Event*>(handle);

            std::array evs   = {&event};
            Event::Await(evs);

            return true;
        }
        void uacpi_kernel_signal_event(uacpi_handle handle)
        {
            auto& event = *reinterpret_cast<Event*>(handle);
            Event::Trigger(&event);
        }
        void uacpi_kernel_reset_event(uacpi_handle handle)
        {
            auto& event   = *reinterpret_cast<Event*>(handle);
            event.Pending = 0;
            event.Listeners.clear();
        }
        uacpi_status
        uacpi_kernel_handle_firmware_request(uacpi_firmware_request*)
        {
            ToDoWarn();
            return UACPI_STATUS_UNIMPLEMENTED;
        }

        uacpi_interrupt_handler s_Handlers[255]{};
        uacpi_handle            s_Contexts[255]{};
        void                    OnInterrupt(CPUContext* ctx)
        {
            auto handler = s_Handlers[ctx->interruptVector];
            if (handler) handler(s_Contexts[ctx->interruptVector]);
        }

        uacpi_status uacpi_kernel_install_interrupt_handler(
            uacpi_u32 irq, uacpi_interrupt_handler uhandler, uacpi_handle ctx,
            uacpi_handle* outIrqHandle)
        {
            auto handler = InterruptManager::AllocateHandler(irq);
            handler->Reserve();
            handler->SetHandler(OnInterrupt);
            s_Handlers[irq] = uhandler;
            s_Contexts[irq] = ctx;
            InterruptManager::Unmask(irq);

            *reinterpret_cast<usize*>(outIrqHandle)
                = handler->GetInterruptVector();
            return UACPI_STATUS_OK;
        }

        uacpi_status
        uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler,
                                                 uacpi_handle irqHandle)
        {
            CtosUnused(irqHandle);
            ToDoWarn();
            return UACPI_STATUS_UNIMPLEMENTED;
        }

        uacpi_handle uacpi_kernel_create_spinlock(void)
        {
            Spinlock* spinlock = new Spinlock;

            return reinterpret_cast<uacpi_handle>(spinlock);
        }
        void uacpi_kernel_free_spinlock(uacpi_handle spinlock)
        {
            delete reinterpret_cast<Spinlock*>(spinlock);
        }

        uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle)
        {
            uacpi_cpu_flags interruptState = CPU::GetInterruptFlag();

            reinterpret_cast<Spinlock*>(handle)->Acquire();
            return interruptState;
        }
        void uacpi_kernel_unlock_spinlock(uacpi_handle    spinlock,
                                          uacpi_cpu_flags interruptState)
        {
            reinterpret_cast<Spinlock*>(spinlock)->Release();

            CPU::SetInterruptFlag(interruptState);
        }

        uacpi_status uacpi_kernel_schedule_work(uacpi_work_type,
                                                uacpi_work_handler,
                                                uacpi_handle ctx)
        {
            CtosUnused(ctx);
            ToDoWarn();

            return UACPI_STATUS_UNIMPLEMENTED;
        }
        uacpi_status uacpi_kernel_wait_for_work_completion(void)
        {
            ToDoWarn();

            return UACPI_STATUS_UNIMPLEMENTED;
        }
    };
}; // namespace uACPI

/*
 * Created by v1tr10l7 on 25.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/CPU.hpp>
#include <Arch/x86_64/Drivers/PCSpeaker.hpp>
#include <Arch/x86_64/IO.hpp>

#include <Drivers/CharacterDevice.hpp>
#include <Drivers/DeviceManager.hpp>
#include <Drivers/GenericDriver.hpp>
#include <Library/Module.hpp>

#include <Prism/Core/Types.hpp>
#include <VFS/DevTmpFs/DevTmpFs.hpp>

#include <VFS/VFS.hpp>

class PCSpeakerDevice : public CharacterDevice
{
  public:
    PCSpeakerDevice()
        : CharacterDevice("pcspk", MakeDevice(AllocateMajor().Value(), 0))
    {
    }

    virtual StringView     Name() const noexcept override { return m_Name; }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        return -1;
    }
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        if (!src || bytes < sizeof(u32)) return_err(-1, EINVAL);

        struct Command
        {
            u16 Tone         = 0;
            u16 Milliseconds = 0;
        } command
            = CPU::AsUser([src]() -> Command
                          { return *reinterpret_cast<const Command*>(src); });

        if (command.Tone < 20 || command.Tone > 20'000
            || command.Milliseconds == 0)
            return_err(-1, EINVAL);

        PCSpeaker::ToneOn(command.Tone);
        IO::Delay(command.Milliseconds);
        PCSpeaker::ToneOff();

        return sizeof(Command);
    }

    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        return Read(out.Raw(), offset, count);
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        return Write(in.Raw(), offset, count);
    }

    virtual i32 IoCtl(usize request, uintptr_t argp) override
    {
        return_err(-1, ENOSYS);
    }
};

namespace
{
    static PCSpeakerDevice* s_PCSpeaker = nullptr;
    ErrorOr<void>           Probe()
    {
        s_PCSpeaker = new PCSpeakerDevice();
        if (!s_PCSpeaker) return Error(ENOMEM);

        TryOrRet(DeviceManager::RegisterCharDevice(s_PCSpeaker));
        return {};
    }

    GenericDriver s_PCSpeakerDriver = {
        .Name   = "pcspk",
        .Probe  = Probe,
        .Remove = nullptr,
    };

    bool ModuleInit() { return RegisterGenericDriver(s_PCSpeakerDriver); }
} // namespace
MODULE_INIT(pcspk, ModuleInit);

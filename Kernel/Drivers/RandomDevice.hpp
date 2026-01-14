/*
 * Created by v1tr10l7 on 09.01.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/DeviceIDs.hpp>
#include <Drivers/Core/CharacterDevice.hpp>
#include <Prism/Algorithm/Random.hpp>

#include <Time/Time.hpp>

class RandomDevice final : public CharacterDevice
{
  public:
    RandomDevice()
        : CharacterDevice("random", API::DeviceMajor::MEMORY, 8)
        , m_Engine(SeedFromKernel())
    {
    }
    virtual StringView     Name() const noexcept override { return m_Name; }

    virtual ErrorOr<isize> Read(void* dest, off_t offset, usize bytes) override
    {
        auto* out = static_cast<u8*>(dest);
        for (usize i = 0; i < bytes; ++i) out[i] = NextByte();

        return bytes;
    }
    virtual ErrorOr<isize> Read(const UserBuffer& out, usize count,
                                isize offset = -1) override
    {
        for (isize i = 0; i < static_cast<isize>(count); ++i)
        {
            u8 b = NextByte();
            if (const_cast<UserBuffer&>(out).Write(&b, sizeof(u8), 1) == 0)
                return Error(EFAULT);
        }

        return count;
    }
    virtual ErrorOr<isize> Write(const void* src, off_t offset,
                                 usize bytes) override
    {
        const u8* in = static_cast<const u8*>(src);
        for (usize i = 0; i < bytes; ++i)
        {
            // XOR input into generated output stream
            m_Buffer ^= static_cast<u32>(in[i]) << ((i & 3) * 8);
            m_Engine.Discard(1);
        }

        return bytes;
    }
    virtual ErrorOr<isize> Write(const UserBuffer& in, usize count,
                                 isize offset = -1) override
    {
        for (isize i = 0; i < static_cast<isize>(count); ++i)
        {
            u8 byte = 0;
            if (const_cast<UserBuffer&>(in).Read(&byte, sizeof(u8), i) == 0)
                break;

            // XOR input into generated output stream
            m_Buffer ^= static_cast<u32>(byte) << ((i & 3) * 8);
            m_Engine.Discard(1);
        }

        return count;
    }

    virtual ErrorOr<isize> IoCtl(usize request, upointer argp) override
    {
        return Error(ENOSYS);
    }

  private:
    Algorithm::mt19937 m_Engine;
    u32                m_Buffer{0};
    u8                 m_Used{4};

    u8                 NextByte()
    {
        if (m_Used >= 4)
        {
            m_Buffer = m_Engine();
            m_Used   = 0;
        }

        return static_cast<u8>(m_Buffer >> (8 * m_Used++));
    }

    static u32 SeedFromKernel()
    {
#ifdef CTOS_TARGET_X86_64
        u64 tsc = CPU::ReadTsc();
#else
        u64 tsc = 0;
#endif
        u64 time  = Time::GetRealTime();
        u64 cpu   = CPU::Current()->ID;

        // collapse entropy to 32 bits (mt19937 seed width)
        u64 mixed = tsc ^ (time << 1) ^ (cpu << 32);
        mixed ^= mixed >> 33;
        mixed *= 0xff51afd7ed558ccdULL;
        mixed ^= mixed >> 33;

        return static_cast<u32>(mixed);
    }
};

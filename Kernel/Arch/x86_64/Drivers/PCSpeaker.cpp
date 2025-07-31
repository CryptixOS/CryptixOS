/*
 * Created by v1tr10l7 on 29.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Arch/x86_64/Drivers/PCSpeaker.hpp>
#include <Arch/x86_64/Drivers/Time/PIT.hpp>
#include <Arch/x86_64/IO.hpp>

namespace PCSpeaker
{
    void ToneOn(u64 frequency)
    {
        IO::Out<byte>(PIT::COMMAND, PIT::SELECT_CHANNEL2 | PIT::SEND_WORD
                                        | PIT::Mode::eSquareWave);
        u16 reloadValue = PIT::BASE_FREQUENCY / frequency;

        IO::Out<byte>(PIT::CHANNEL2_DATA, reloadValue);
        IO::Out<byte>(PIT::CHANNEL2_DATA, reloadValue >> 8);

        IO::Out<byte>(0x61, IO::In<byte>(0x61) | 3);
    }

    void ToneOff() { IO::Out<byte>(0x61, IO::In<byte>(0x61) & ~3); }
}; // namespace PCSpeaker

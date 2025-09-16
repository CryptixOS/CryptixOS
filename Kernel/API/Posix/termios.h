/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

using cc_t           = unsigned char;
using speed_t        = unsigned int;
using tcflag_t       = unsigned int;

constexpr usize NCCS = 0x20;

struct termios
{
    /* input mode flags */
    tcflag_t c_iflag;
    /* output mode flags */
    tcflag_t c_oflag;
    /* control mode flags */
    tcflag_t c_cflag;
    /* local mode flags */
    tcflag_t c_lflag;
    /* line discipline */
    cc_t     c_line;
    /* control characters */
    cc_t     c_cc[NCCS];
};
struct ktermios
{
    /* input mode flags */
    tcflag_t c_iflag;
    /* output mode flags */
    tcflag_t c_oflag;
    /* control mode flags */
    tcflag_t c_cflag;
    /* local mode flags */
    tcflag_t c_lflag;
    /* line discipline */
    cc_t     c_line;
    /* control characters */
    cc_t     c_cc[NCCS];
    /* input speed */
    speed_t  c_ispeed;
    /* output speed */
    speed_t  c_ospeed;
};

// c_cc
constexpr usize VINTR      = 0;
constexpr usize VQUIT      = 1;
constexpr usize VERASE     = 2;
constexpr usize VKILL      = 3;
constexpr usize VEOF       = 4;
constexpr usize VTIME      = 5;
constexpr usize VMIN       = 6;
constexpr usize VSWTC      = 7;
constexpr usize VSTART     = 8;
constexpr usize VSTOP      = 9;
constexpr usize VSUSP      = 10;
constexpr usize VEOL       = 11;
constexpr usize VREPRINT   = 12;
constexpr usize VDISCARD   = 13;
constexpr usize VWERASE    = 14;
constexpr usize VLNEXT     = 15;
constexpr usize VEOL2      = 16;

// c_iflag
// ignore break condition
constexpr usize IGNBRK     = 000001;
// Signal interrupt on break
constexpr usize BRKINT     = 000002;
// Ignore characters with parity errors
constexpr usize IGNPAR     = 000004;
// Mark parity and framing errors
constexpr usize PARMRK     = 000010;
// Enable input parity check
constexpr usize INPCK      = 000020;
// Strip 8th bit off characters
constexpr usize ISTRIP     = 000040;
// Map NL to CR on input
constexpr usize INLCR      = 000100;
// Ignore CR
constexpr usize IGNCR      = 000200;
// Map CR to NL on input
constexpr usize ICRNL      = 000400;
// Map uppercase characters to lowercase on input(not in POSIX)
constexpr usize IUCLC      = 001000;
// Enable start/stop output control
constexpr usize IXON       = 0002000;
// Enable any character to restart output
constexpr usize IXANY      = 0004000;
// Enable start/stop input control
constexpr usize IXOFF      = 0010000;
// Ring bell when input queue is full(not in POSIX)
constexpr usize IMAXBEL    = 0020000;
// Input is UTF-8
constexpr usize IUTF8      = 0040000;

// c_oflag
// Post-process output
constexpr usize OPOST      = 0000001;
// Map lowercase characters to uppercase on output (not in POSIX)
constexpr usize OLCUC      = 0000002;
// Map NL to CR-NL on output
constexpr usize ONLCR      = 0000004;
// Map CR to NL on output
constexpr usize OCRNL      = 0000010;
// No CR output at column 0
constexpr usize ONOCR      = 0000020;
// NL performs CR function
constexpr usize ONLRET     = 0000020;
// Use fill characters for delay
constexpr usize OFILL      = 0000100;
// Fill is DEL
constexpr usize OFDEL      = 00002000;
constexpr usize XTABS      = 0014000;

// c_cflag
// Hang up
constexpr usize B0         = 0000000;
constexpr usize B50        = 0000001;
constexpr usize B75        = 0000002;
constexpr usize B110       = 0000003;
constexpr usize B134       = 0000004;
constexpr usize B150       = 0000005;
constexpr usize B200       = 0000006;
constexpr usize B300       = 0000007;
constexpr usize B600       = 0000010;
constexpr usize B1200      = 0000011;
constexpr usize B1800      = 0000012;
constexpr usize B2400      = 0000013;
constexpr usize B4800      = 0000014;
constexpr usize B9600      = 0000015;
constexpr usize B19200     = 0000016;
constexpr usize B38400     = 0000017;

/* Extra output baud rates (not in POSIX).  */
constexpr usize B57600     = 0010001;
constexpr usize B115200    = 0010002;
constexpr usize B230400    = 0010003;
constexpr usize B460800    = 0010004;
constexpr usize B500000    = 0010005;
constexpr usize B576000    = 0010006;
constexpr usize B921600    = 0010007;
constexpr usize B1000000   = 0010010;
constexpr usize B1152000   = 0010011;
constexpr usize B1500000   = 0010012;
constexpr usize B2000000   = 0010013;
constexpr usize B2500000   = 0010014;
constexpr usize B3000000   = 0010015;
constexpr usize B3500000   = 0010016;
constexpr usize B4000000   = 0010017;
constexpr usize __MAX_BAUD = B4000000;

constexpr usize CSIZE      = 0000060;
constexpr usize CS5        = 0000000;
constexpr usize CS6        = 0000020;
constexpr usize CS7        = 0000040;
constexpr usize CS8        = 0000060;
constexpr usize CSTOPB     = 0000100;
constexpr usize CREAD      = 0000200;
constexpr usize PARENB     = 0000400;
constexpr usize PARODD     = 0001000;
constexpr usize HUPCL      = 0002000;
constexpr usize CLOCAL     = 0004000;

// c_lflag
// Enable signals
constexpr usize ISIG       = 0000001;
// Canonical input (erase and kill processing)
constexpr usize ICANON     = 0000002;
// Enable echo
constexpr usize ECHO       = 0000010;
// Echo erase character as error-correcting backspace
constexpr usize ECHOE      = 0000020;
// Echo KILL
constexpr usize ECHOK      = 0000040;
// Echo NL
constexpr usize ECHONL     = 0000100;
// Disable flush after interrupt or quit
constexpr usize NOFLSH     = 0000200;
// Send SIGTTOU for background output
constexpr usize TOSTOP     = 0000400;
// If ECHO is also set, terminal special characters other than TAB, NL, START
// and STOP are echoed as ^X, where X is the character with ASCII code 40
// greater than the special character (not in POSIX). Enable
// implementation-defined input processing
constexpr usize ECHOCTL    = 0001000;
// If ICANON and ECHO are also set, characters are printed as they are being
// erased (not in POSIX).
constexpr usize ECHOPRT    = 0002000;
// If ICANON is also set, KILL is echoed by erasing each character on the line,
// as specified by ECHOE and ECHOPRT (not in POSIX).
constexpr usize ECHOKE     = 0004000;
constexpr usize IEXTEN     = 100000;

constexpr usize TCOOFF     = 0;
constexpr usize TCOON      = 1;
constexpr usize TCIOFF     = 2;
constexpr usize TCION      = 3;

constexpr usize TCIFLUSH   = 0;
constexpr usize TCOFLUSH   = 1;
constexpr usize TCIOFLUSH  = 2;

constexpr usize TCSANOW    = 0;
constexpr usize TCSADRAIN  = 1;
constexpr usize TCSAFLUSH  = 2;

struct winsize
{
    u16 ws_row;
    u16 ws_col;
    u16 ws_xpixel;
    u16 ws_ypixel;
};

#include <API/Posix/asm/termbits.h>

template <>
struct fmt::formatter<termios2> : fmt::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const termios2& termios, FormatContext& ctx) const
    {
        tcflag_t    iflag          = termios.c_iflag;
        tcflag_t    oflag          = termios.c_oflag;
        tcflag_t    cflag          = termios.c_cflag;
        tcflag_t    lflag          = termios.c_lflag;
        const auto& cc             = termios.c_cc;

        auto        iflagFormatted = fmt::format(
            "IFLAG = {{ IGNBRK: {}, BRKINT: {}, IGNPAR: {}, PARMRK: {}, "
                   "INPCK: "
                   "{}, "
                   "ISTRIP: {}, INLCR: {}, IGNCR: {}, ICRNL: {}, IUCLC: {}, "
                   "IXON: {}, IXANY: {}, IXOFF: {}, IMAXBEL: {}, IUTF: {} }}",
            (iflag & IGNBRK) == IGNBRK, (iflag & BRKINT) == BRKINT,
            (iflag & IGNPAR) == IGNPAR, (iflag & PARMRK) == PARMRK,
            (iflag & INPCK) == INPCK, (iflag & ISTRIP) == ISTRIP,
            (iflag & INLCR) == INLCR, (iflag & IGNCR) == IGNCR,
            (iflag & ICRNL) == ICRNL, (iflag & IUCLC) == IUCLC,
            (iflag & IXON) == IXON, (iflag & IXANY) == IXANY,
            (iflag & IXOFF) == IXOFF, (iflag & IMAXBEL) == IMAXBEL,
            (iflag & IUTF8) == IUTF8);
        ;

        auto oflagFormatted = fmt::format(
            "OFLAG = {{ OPOST: {}, OLCUC: {}, ONLCR: {}, OCRNL: {}, ONOCR: {}, "
            "ONLRET: {}, OFILL: {}, OFDEL: {}, XTABS: {} }}",
            (oflag & OPOST) == OPOST, (oflag & OLCUC) == OLCUC,
            (oflag & ONLCR) == ONLCR, (oflag & OCRNL) == OCRNL,
            (oflag & ONOCR) == ONOCR, (oflag & ONLRET) == ONLRET,
            (oflag & OFILL) == OFILL, (oflag & OFDEL) == OFDEL,
            (oflag & XTABS) == XTABS);

        auto cflagFormatted = fmt::format(
            "CFLAG = {{ CSTOPB: {}, CREAD: {}, PARENB: {}, PARODD: {}, HUPCL: "
            "{}, "
            "CLOCAL: {} }}",
            (cflag & CSTOPB) == CSTOPB, (cflag & CREAD) == CREAD,
            (cflag & PARENB) == PARENB, (cflag & PARODD) == PARODD,
            (cflag & HUPCL) == HUPCL, (cflag & CLOCAL) == CLOCAL);
        auto lflagFormatted = fmt::format(
            "LFLAG = {{ ISIG: {}, ICANON: {}, ECHO: {}, ECHOE: {}, ECHOK: {}, "
            "ECHONL: {}, NOFLSH: {}, TOSTOP: {}, ECHOCTL: {}, ECHOPRT: {}, "
            "ECHOKE: "
            "{}, IEXTEN: {} }}",
            (lflag & ISIG) == ISIG, (lflag & ICANON) == ICANON,
            (lflag & ECHO) == ECHO, (lflag & ECHOE) == ECHOE,
            (lflag & ECHOK) == ECHOK, (lflag & ECHONL) == ECHONL,
            (lflag & NOFLSH) == NOFLSH, (lflag & TOSTOP) == TOSTOP,
            (lflag & ECHOCTL) == ECHOCTL, (lflag & ECHOPRT) == ECHOPRT,
            (lflag & ECHOKE) == ECHOKE, (lflag & IEXTEN) == IEXTEN);
        auto ccFormatted = fmt::format(
            "CC = {{ VINTR: {}, VQUIT: {}, VERASE: {}, VKILL: {}, VEOF: {}, "
            "VTIME: {}, VMIN: {}, VSWTC: {}, VSTART: {}, VSTOP: {}, VSUSP: {}, "
            "VEOL: {}, VREPRINT: {}, VDISCARD: {}, VWERASE: {}, VLNEXT: {}, "
            "VEOL2: {} }}",
            cc[VINTR], cc[VQUIT], cc[VERASE], cc[VKILL], cc[VEOF], cc[VTIME],
            cc[VMIN], cc[VSWTC], cc[VSTART], cc[VSTOP], cc[VSUSP], cc[VEOL],
            cc[VREPRINT], cc[VDISCARD], cc[VWERASE], cc[VLNEXT], cc[VEOL2]);

        return fmt::formatter<std::string>::format(
            fmt::format("{}\n{}\n{}\n{}\n{}", iflagFormatted, oflagFormatted,
                        cflagFormatted, lflagFormatted, ccFormatted),
            ctx);
    }
};

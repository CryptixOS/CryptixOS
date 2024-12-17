/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include "Utility/Types.hpp"

using cc_t           = u32;
using speed_t        = u32;
using tcflag_t       = u32;

constexpr usize NCCS = 11;

struct termios
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t     c_cc[NCCS];
    speed_t  ibaud;
    speed_t  obaud;
};

struct winsize
{
    u16 ws_row;
    u16 ws_col;
    u16 ws_xpixel;
    u16 ws_ypixel;
};

constexpr usize TCGETS       = 0x5401;
constexpr usize TCSETS       = 0x5402;
constexpr usize TCSETSW      = 0x5403;
constexpr usize TCSETSF      = 0x5404;
constexpr usize TCGETA       = 0x5405;
constexpr usize TCSETA       = 0x5406;
constexpr usize TCSETAW      = 0x5407;
constexpr usize TCSETAF      = 0x5408;
constexpr usize TCSBRK       = 0x5409;
constexpr usize TCXONC       = 0x540A;
constexpr usize TCFLSH       = 0x540B;
constexpr usize TIOCEXCL     = 0x540C;
constexpr usize TIOCNXCL     = 0x540D;
constexpr usize TIOCSCTTY    = 0x540E;
constexpr usize TIOCGPGRP    = 0x540F;
constexpr usize TIOCSPGRP    = 0x5410;
constexpr usize TIOCOUTQ     = 0x5411;
constexpr usize TIOCSTI      = 0x5412;
constexpr usize TIOCGWINSZ   = 0x5413;
constexpr usize TIOCSWINSZ   = 0x5414;
constexpr usize TIOCMGET     = 0x5415;
constexpr usize TIOCMBIS     = 0x5416;
constexpr usize TIOCMBIC     = 0x5417;
constexpr usize TIOCMSET     = 0x5418;
constexpr usize TIOCGSOFTCAR = 0x5419;
constexpr usize TIOCSSOFTCAR = 0x541A;
constexpr usize FIONREAD     = 0x541B;
#define TIOCINQ FIONREAD
constexpr usize TIOCLINUX   = 0x541C;
constexpr usize TIOCCONS    = 0x541D;
constexpr usize TIOCGSERIAL = 0x541E;
constexpr usize TIOCSSERIAL = 0x541F;
constexpr usize TIOCPKT     = 0x5420;
constexpr usize FIONBIO     = 0x5421;
constexpr usize TIOCNOTTY   = 0x5422;
constexpr usize TIOCSETD    = 0x5423;
constexpr usize TIOCGETD    = 0x5424;
constexpr usize TCSBRKP     = 0x5425; /* Needed for POSIX tcsendbreak() */
constexpr usize TIOCSBRK    = 0x5427; /* BSD compatibility */
constexpr usize TIOCCBRK    = 0x5428; /* BSD compatibility */
constexpr usize TIOCGSID    = 0x5429; /* Return the session ID of FD */
#define TCGETS2  _IOR('T', 0x2A, struct termios2)
#define TCSETS2  _IOW('T', 0x2B, struct termios2)
#define TCSETSW2 _IOW('T', 0x2C, struct termios2)
#define TCSETSF2 _IOW('T', 0x2D, struct termios2)
constexpr usize TIOCGRS485 = 0x542E;
#ifndef TIOCSRS485
constexpr usize TIOCSRS485 = 0x542F;
#endif
#define TIOCGPTN                                                               \
    _IOR('T', 0x30, unsigned int)       /* Get Pty Number (of pty-mux device) */
#define TIOCSPTLCK _IOW('T', 0x31, int) /* Lock/unlock Pty */
#define TIOCGDEV                                                               \
    _IOR('T', 0x32, unsigned int) /* Get primary device node of /dev/console   \
                                   */
constexpr usize TCGETX  = 0x5432; /* SYS5 TCGETX compatibility */
constexpr usize TCSETX  = 0x5433;
constexpr usize TCSETXF = 0x5434;
constexpr usize TCSETXW = 0x5435;
#define TIOCSIG _IOW('T', 0x36, int) /* pty: generate signal */
constexpr usize TIOCVHANGUP = 0x5437;
#define TIOCGPKT     _IOR('T', 0x38, int) /* Get packet mode state */
#define TIOCGPTLCK   _IOR('T', 0x39, int) /* Get Pty lock state */
#define TIOCGEXCL    _IOR('T', 0x40, int) /* Get exclusive mode state */
#define TIOCGPTPEER  _IO('T', 0x41)       /* Safely open the slave */
#define TIOCGISO7816 _IOR('T', 0x42, struct serial_iso7816)
#define TIOCSISO7816 _IOWR('T', 0x43, struct serial_iso7816)

constexpr usize FIONCLEX        = 0x5450;
constexpr usize FIOCLEX         = 0x5451;
constexpr usize FIOASYNC        = 0x5452;
constexpr usize TIOCSERCONFIG   = 0x5453;
constexpr usize TIOCSERGWILD    = 0x5454;
constexpr usize TIOCSERSWILD    = 0x5455;
constexpr usize TIOCGLCKTRMIOS  = 0x5456;
constexpr usize TIOCSLCKTRMIOS  = 0x5457;
constexpr usize TIOCSERGSTRUCT  = 0x5458; /* For debugging only */
constexpr usize TIOCSERGETLSR   = 0x5459; /* Get line status register */
constexpr usize TIOCSERGETMULTI = 0x545A; /* Get multiport config  */
constexpr usize TIOCSERSETMULTI = 0x545B; /* Set multiport config */

constexpr usize TIOCMIWAIT
    = 0x545C; /* wait for a change on serial input line(s) */
constexpr usize TIOCGICOUNT
    = 0x545D; /* read serial port __inline__ interrupt counts */

/*
 * Some arches already define FIOQSIZE due to a historical
 * conflict with a Hayes modem-specific ioctl value.
 */
#ifndef FIOQSIZE
constexpr usize FIOQSIZE = 0x5460;
#endif

/* Used for packet mode */
#define TIOCPKT_DATA       0
#define TIOCPKT_FLUSHREAD  1
#define TIOCPKT_FLUSHWRITE 2
#define TIOCPKT_STOP       4
#define TIOCPKT_START      8
#define TIOCPKT_NOSTOP     16
#define TIOCPKT_DOSTOP     32
#define TIOCPKT_IOCTL      64

constexpr usize TIOCSER_TEMT = 0x01; /* Transmitter physically empty */

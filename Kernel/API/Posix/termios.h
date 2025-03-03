/*
 * Created by v1tr10l7 on 28.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

using cc_t           = u8;
using speed_t        = u32;
using tcflag_t       = u32;

constexpr usize NCCS = 0x20;

struct termios
{
    tcflag_t c_iflag;
    tcflag_t c_oflag;
    tcflag_t c_cflag;
    tcflag_t c_lflag;
    cc_t     c_cc[NCCS];
    speed_t  c_ispeed;
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
constexpr usize VINFO      = 17;

// c_iflag
// ignore break condition
constexpr usize IGNBRK     = 0x000001;
// Signal interrupt on break
constexpr usize BRKINT     = 0x000002;
// Ignore characters with parity errors
constexpr usize IGNPAR     = 0x000004;
// Mark parity and framing errors
constexpr usize PARMRK     = 0x000010;
// Enable input parity check
constexpr usize INPCK      = 0x000020;
// Strip 8th bit off characters
constexpr usize ISTRIP     = 0x000040;
// Map NL to CR on input
constexpr usize INLCR      = 0x000100;
// Ignore CR
constexpr usize IGNCR      = 0x000200;
// Map CR to NL on input
constexpr usize ICRNL      = 0x000400;
// Map uppercase characters to lowercase on input(not in POSIX)
constexpr usize IUCLC      = 0x001000;
// Enable start/stop output control
constexpr usize IXON       = 0x0002000;
// Enable any character to restart output
constexpr usize IXANY      = 0x0004000;
// Enable start/stop input control
constexpr usize IXOFF      = 0x0010000;
// Ring bell when input queue is full(not in POSIX)
constexpr usize IMAXBEL    = 0x0020000;
// Input is UTF-8
constexpr usize IUTF8      = 0x0040000;

// c_oflag
// Post-process output
constexpr usize OPOST      = 0x0000001;
// Map lowercase characters to uppercase on output (not in POSIX)
constexpr usize OLCUC      = 0x0000002;
// Map NL to CR-NL on output
constexpr usize ONLCR      = 0x0000004;
// Map CR to NL on output
constexpr usize OCRNL      = 0x0000010;
// No CR output at column 0
constexpr usize ONOCR      = 0x0000020;
// NL performs CR function
constexpr usize ONLRET     = 0x0000020;
// Use fill characters for delay
constexpr usize OFILL      = 0x0000100;
// Fill is DEL
constexpr usize OFDEL      = 0x00002000;
constexpr usize XTABS      = 0014000;

// c_cflag
// Hang up
constexpr usize B0         = 0x0000000;
constexpr usize B50        = 0x0000001;
constexpr usize B75        = 0x0000002;
constexpr usize B110       = 0x0000003;
constexpr usize B134       = 0x0000004;
constexpr usize B150       = 0x0000005;
constexpr usize B200       = 0x0000006;
constexpr usize B300       = 0x0000007;
constexpr usize B600       = 0x0000010;
constexpr usize B1200      = 0x0000011;
constexpr usize B1800      = 0x0000012;
constexpr usize B2400      = 0x0000013;
constexpr usize B4800      = 0x0000014;
constexpr usize B9600      = 0x0000015;
constexpr usize B19200     = 0x0000016;
constexpr usize B38400     = 0x0000017;

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

constexpr usize CSIZE      = 0x0000060;
constexpr usize CS5        = 0x0000000;
constexpr usize CS6        = 0x0000020;
constexpr usize CS7        = 0x0000040;
constexpr usize CS8        = 0x0000060;
constexpr usize CSTOPB     = 0x0000100;
constexpr usize CREAD      = 0x0000200;
constexpr usize PARENB     = 0x0000400;
constexpr usize PARODD     = 0x0001000;
constexpr usize HUPCL      = 0x0002000;
constexpr usize CLOCAL     = 0x0004000;

// c_lflag
// Enable signals
constexpr usize ISIG       = 0x0000001;
// Canonical input (erase and kill processing)
constexpr usize ICANON     = 0x0000002;
// Enable echo
constexpr usize ECHO       = 0x0000010;
// Echo erase character as error-correcting backspace
constexpr usize ECHOE      = 0x0000020;
// Echo KILL
constexpr usize ECHOK      = 0x0000040;
// Echo NL
constexpr usize ECHONL     = 0x0000100;
// Disable flush after interrupt or quit
constexpr usize NOFLSH     = 0x0000200;
// Send SIGTTOU for background output
constexpr usize TOSTOP     = 0x0000400;
// If ECHO is also set, terminal special characters other than TAB, NL, START
// and STOP are echoed as ^X, where X is the character with ASCII code 0x40
// greater than the special character (not in POSIX). Enable
// implementation-defined input processing
constexpr usize ECHOCTL    = 0001000;
// If ICANON and ECHO are also set, characters are printed as they are being
// erased (not in POSIX).
constexpr usize ECHOPRT    = 0002000;
// If ICANON is also set, KILL is echoed by erasing each character on the line,
// as specified by ECHOE and ECHOPRT (not in POSIX).
constexpr usize ECHOKE     = 0004000;
constexpr usize IEXTEN     = 0x100000;

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

constexpr usize TCGETS       = 0x5401;
constexpr usize TCSETS       = 0x5402;
constexpr usize TCSETSW      = 0x5403;
constexpr usize TCSETSF      = 0x5404;
constexpr usize TCGETA       = 0x5405;
constexpr usize TCSETA       = 0x5406;
constexpr usize TCSETAW      = 0x5407;
constexpr usize TCSETAF      = 0x5408;
constexpr usize TCSBRK       = 0x5409;
constexpr usize TCXONC       = 0x540a;
constexpr usize TCFLSH       = 0x540b;
constexpr usize TIOCEXCL     = 0x540c;
constexpr usize TIOCNXCL     = 0x540d;
constexpr usize TIOCSCTTY    = 0x540e;
constexpr usize TIOCGPGRP    = 0x540f;
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

#define TIOCINQ  FIONREAD
#define TCGETS2  _IOR('T', 0x2A, struct termios2)
#define TCSETS2  _IOW('T', 0x2B, struct termios2)
#define TCSETSW2 _IOW('T', 0x2C, struct termios2)
#define TCSETSF2 _IOW('T', 0x2D, struct termios2)
#ifndef TIOCSRS485
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
constexpr usize FIOQSIZE           = 0x5460;

/* Used for packet mode */
constexpr usize TIOCPKT_DATA       = 0;
constexpr usize TIOCPKT_FLUSHREAD  = 1;
constexpr usize TIOCPKT_FLUSHWRITE = 2;
constexpr usize TIOCPKT_STOP       = 4;
constexpr usize TIOCPKT_START      = 8;
constexpr usize TIOCPKT_NOSTOP     = 16;
constexpr usize TIOCPKT_DOSTOP     = 32;
constexpr usize TIOCPKT_IOCTL      = 64;

constexpr usize TIOCSER_TEMT       = 0x01; /* Transmitter physically empty */

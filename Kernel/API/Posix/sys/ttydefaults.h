/*
 * Created by v1tr10l7 on 18.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/termios.h>

/*
 * Defaults on "first" open.
 */
constexpr usize TTYDEF_IFLAG = BRKINT | ISTRIP | ICRNL | IMAXBEL | IXON | IXANY;
constexpr usize TTYDEF_OFLAG = OPOST | ONLCR | XTABS;
constexpr usize TTYDEF_LFLAG
    = ECHO | ICANON | ISIG | IEXTEN | ECHOE | ECHOK | ECHOCTL;
constexpr usize TTYDEF_CFLAG = CREAD | CS8 | PARENB | HUPCL;
constexpr usize TTYDEF_SPEED = B9600;

/*
 * Control Character Defaults
 */
constexpr usize CTRL(u64 c) { return c & 037; }
constexpr usize CEOF     = CTRL('d');
constexpr usize CEOL     = 0;
constexpr usize CERASE   = 0177;
constexpr usize CINTR    = CTRL('c');
constexpr usize CSTATUS  = 0;
constexpr usize CKILL    = CTRL('u');
constexpr usize CMIN     = 1;
constexpr usize CQUIT    = 034;
constexpr usize CSUSP    = CTRL('z');
constexpr usize CTIME    = 0;
constexpr usize CDSUSP   = CTRL('y');
constexpr usize CSTART   = CTRL('q');
constexpr usize CSTOP    = CTRL('s');
constexpr usize CLNEXT   = CTRL('v');
constexpr usize CDISCARD = CTRL('o');
constexpr usize CWERASE  = CTRL('w');
constexpr usize CREPRINT = CTRL('r');
constexpr usize CEOT     = CEOF;
constexpr usize CBRK     = CEOL;
constexpr usize CRPRNT   = CREPRINT;
constexpr usize CFLUSH   = CDISCARD;

constexpr usize CSWTC    = 0;
constexpr usize CEOL2    = CEOL;

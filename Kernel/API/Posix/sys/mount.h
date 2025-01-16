/*
 * Created by v1tr10l7 on 16.01.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Common.hpp>

constexpr usize MS_RDONLY      = Bit(0);
constexpr usize MS_NOSUID      = Bit(1);
constexpr usize MS_NODEV       = Bit(2);
constexpr usize MS_NOEXEC      = Bit(3);
constexpr usize MS_SYNCHRONOUS = Bit(4);
constexpr usize MS_REMOUNT     = Bit(5);
constexpr usize MS_MANDLOCK    = Bit(6);
constexpr usize MS_DIRSYNC     = Bit(7);
constexpr usize MS_NOSYFOLLOW  = Bit(8);
constexpr usize MS_NOATIME     = Bit(9);
constexpr usize MS_DIRATIME    = Bit(10);
constexpr usize MS_BIND        = Bit(11);
constexpr usize MS_MOVE        = Bit(12);
constexpr usize MS_REC         = Bit(13);
constexpr usize MS_VERBOSE     = Bit(14);
constexpr usize MS_SILENT      = Bit(15);
constexpr usize MS_POSIXACL    = Bit(16);
constexpr usize MS_UNBINDABLE  = Bit(17);
constexpr usize MS_PRIVATE     = Bit(18);
constexpr usize MS_SLAVE       = Bit(19);
constexpr usize MS_SHARED      = Bit(20);
constexpr usize MS_RELATIME    = Bit(21);
constexpr usize MS_KERNMOUNT   = Bit(22);
constexpr usize MS_I_VERSION   = Bit(23);
constexpr usize MS_STRICTATIME = Bit(24);
constexpr usize MS_LAZYTIME    = Bit(25);

/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/sys/stat.h>
#include <Prism/Core/Types.hpp>

using ssize_t     = isize;

using pid_t       = int;
using tid_t       = int;

using ProcessID   = pid_t;
using ThreadID    = tid_t;

using DeviceID    = dev_t;
using DeviceMajor = u32;
using DeviceMinor = u32;

using INodeMode   = mode_t;
using LinkCount   = nlink_t;
using UserID      = uid_t;
using GroupID     = gid_t;

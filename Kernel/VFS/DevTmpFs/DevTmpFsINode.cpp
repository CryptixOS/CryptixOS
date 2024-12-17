/*
 * Created by v1tr10l7 on 20.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include "DevTmpFsINode.hpp"

isize DevTmpFsINode::Read(void* buffer, off_t offset, usize bytes)
{
    if (!device) return_err(-1, ENOTTY);

    return device->Read(buffer, offset, bytes);
}
isize DevTmpFsINode::Write(const void* buffer, off_t offset, usize bytes)
{
    if (!device) return_err(-1, ENOTTY);

    return device->Write(buffer, offset, bytes);
}

i32 DevTmpFsINode::IoCtl(usize request, usize arg)
{
    if (!device) return_err(-1, ENOTTY);

    return device->IoCtl(request, arg);
}

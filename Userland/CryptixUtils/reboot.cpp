/*
 * Created by vitriol1744 on 19.02.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdio.h>
#include <stdlib.h>

#include <cryptix/reboot.hpp>
#include <cryptix/syscall.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

using namespace cryptix;
int main()
{
    printf("The system will reboot...\n");
    Syscall(SYS_REBOOT, RebootCmd::eRestart);
    // reboot(LINUX_REBOOT_CMD_RESTART);

    return EXIT_FAILURE;
}

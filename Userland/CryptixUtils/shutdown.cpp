/*
 * Created by vitriol1744 on 21.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <Vitriol1744@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <stdio.h>
#include <stdlib.h>

#include <linux/reboot.h>
#include <sys/reboot.h>

int main()
{
    printf("The system will be shutdown...\n");
    reboot(LINUX_REBOOT_CMD_POWER_OFF);

    return EXIT_FAILURE;
}

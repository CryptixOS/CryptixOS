/*
 * Created by vitriol1744 on 19.02.2025.
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
    printf("The system will reboot...\n");
    reboot(LINUX_REBOOT_CMD_RESTART);

    return EXIT_FAILURE;
}

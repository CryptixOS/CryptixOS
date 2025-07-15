/*
 * Created by v1tr10l7 on 12.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

//--------------------------------------------------------------------------
// Any debug options land here
//--------------------------------------------------------------------------

#ifndef CTOS_PMM_DUMP_MEMORY_MAP
    #define CTOS_PMM_DUMP_MEMORY_MAP 0
#endif

#ifndef CTOS_DUMP_PCI_IRQ_ROUTES
    #define CTOS_DUMP_PCI_IRQ_ROUTES 0
#endif

#ifndef CTOS_DUMP_DEVICE_TREE
    #define CTOS_DUMP_DEVICE_TREE 0
#endif

#ifndef CTOS_DUMP_EFI_MEMORY_MAP
    #define CTOS_DUMP_EFI_MEMORY_MAP 0
#endif

#ifndef CTOS_TTY_LOG_TERMIOS
    #define CTOS_TTY_LOG_TERMIOS 0
#endif

#ifndef CTOS_DEBUG_ECHFS
    #define CTOS_DEBUG_ECHFS 0
#endif

#ifndef CTOS_DUMP_INIT_ARRAY
    #define CTOS_DUMP_INIT_ARRAY 0
#endif

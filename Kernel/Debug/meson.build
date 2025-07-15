#*
#* Created by v1tr10l7 on 15.07.2025.
#* Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
srcs += files(
  'Panic.cpp',
)

debugConfig = configuration_data()
debugConfig.set('CTOS_MM_DUMP_MEMMAP', '0')
debugConfig.set('CTOS_PCI_DUMP_IRQ_ROUTES', '0')
debugConfig.set('CTOS_DTB_DUMP', '0')
debugConfig.set('CTOS_EFI_DUMP_MEMMAP', '0')
debugConfig.set('CTOS_TTY_LOG_TERMIOS', '0')
debugConfig.set('CTOS_DUMP_INIT_ARRAY', '1')

configure_file(
  input: 'Debug.hpp.in', 
  output: 'Debug.hpp',
  configuration: debugConfig)

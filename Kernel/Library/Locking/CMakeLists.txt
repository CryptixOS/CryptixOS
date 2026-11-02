#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    ConditionalVariable.cppm
    Mutex.cppm
    RecursiveMutex.cppm
    RecursiveSpinlock.cppm
    Semaphore.cppm
    SequenceLock.cppm
    Spinlock.cppm
    SpinlockProtected.cppm
    UniqueGuard.cppm
    WaitQueue.cppm
    ConditionalVariable.cpp
    Mutex.cpp
    RecursiveMutex.cpp
    Semaphore.cpp
    SequenceLock.cpp
    Spinlock.cpp
)

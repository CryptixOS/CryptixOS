#
# Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
#
# SPDX-License-Identifier: GPL-3
#

target_sources(cryptix_kernel
  PUBLIC FILE_SET CXX_MODULES FILES
    Fifo.cppm
    File.cppm
    FileDescriptor.cppm
    FileDescriptorTable.cppm
    Filesystem.cppm
    FilesystemView.cppm
    INode.cppm
    MountPoint.cppm
    PathResolver.cppm
    SequenceFile.cppm
    SynthFsINode.cppm
    VFS.cppm
    DirectoryEntry.cpp
    Fifo.cpp
    File.cpp
    FileDescriptor.cpp
    FileDescriptorTable.cpp
    INode.cpp
    MountPoint.cpp
    PathResolver.cpp
    SynthFsINode.cpp
    VFS.cpp
)

add_subdirectory(DevPtsFs)
add_subdirectory(DevTmpFs)
add_subdirectory(EchFs)
add_subdirectory(Ext2Fs)
add_subdirectory(Fat32Fs)
add_subdirectory(Initrd)
add_subdirectory(ProcFs)
add_subdirectory(TmpFs)

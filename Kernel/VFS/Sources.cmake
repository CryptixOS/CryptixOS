#*
#* Created by v1tr10l7 on 07.11.2026.
#* Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
ctos_kernel_sources(
    DirectoryEntry.cpp
    Pipe.cpp
    File.cpp
    FileDescriptor.cpp
    FileDescriptorTable.cpp
    INode.cpp
    MountPoint.cpp
    PathResolver.cpp
    SynthFsINode.cpp
    VFS.cpp
)

ctos_subdir(DevPtsFs)
ctos_subdir(DevTmpFs)
ctos_subdir(EchFs)
ctos_subdir(Ext2Fs)
ctos_subdir(Fat32Fs)
ctos_subdir(Initrd)
ctos_subdir(ProcFs)
ctos_subdir(TmpFs)

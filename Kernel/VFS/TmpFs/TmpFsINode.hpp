/*
 * Created by v1tr10l7 on 19.11.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Library/Logger.hpp>
#include <Prism/Memory/Buffer.hpp>

#include <VFS/DirectoryEntry.hpp>
#include <VFS/SynthFsINode.hpp>

extern bool g_LogTmpFs;

#define CTOS_DEBUG_TMPFS 1
#if CTOS_DEBUG_TMPFS != 0
    #define TmpFsTrace(...)                                                    \
        if (g_LogTmpFs) LogMessage("[\e[35mTmpFs\e[0m]: {}\n", __VA_ARGS__)
#else
    #define TmpFsTrace(...)
#endif

class TmpFsINode final : public SynthFsINode
{
  public:
    TmpFsINode(StringView name, class Filesystem* fs, INodeID id,
               INodeMode mode);

  private:
    friend class TmpFs;
};

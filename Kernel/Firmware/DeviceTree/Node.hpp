/*
 * Created by v1tr10l7 on 20.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Endian.hpp>

#include <string_view>
#include <unordered_map>

namespace DeviceTree
{
    class Property
    {
      public:
        BigEndian<u32> m_Length;
        BigEndian<u32> m_NameOffset;
    };

    class Node
    {
      public:
        enum class Tag : u32
        {
            eBeginNode = 1,
            eEndNode   = 2,
            eProperty  = 3,
            eNop       = 4,
            eEnd       = 9,
        };

      private:
        std::unordered_map<std::string_view, Node*> m_Children;
        std::unordered_map<std::string_view, Node*> m_Properties;
    };
}; // namespace DeviceTree

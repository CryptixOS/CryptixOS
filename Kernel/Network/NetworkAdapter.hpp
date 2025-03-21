/*
 * Created by v1tr10l7 on 21.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Network/MacAddress.hpp>

#include <Prism/Core/Types.hpp>

class NetworkAdapter
{
  public:
    NetworkAdapter()          = default;
    virtual ~NetworkAdapter() = default;

    const MacAddress& GetMacAddress() const { return m_MacAddress; }

    static bool       RegisterNIC(NetworkAdapter* nic);

  protected:
    MacAddress m_MacAddress;
};

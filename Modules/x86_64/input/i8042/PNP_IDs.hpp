/*
 * Created by v1tr10l7 on 09.08.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Array.hpp>
#include <Prism/String/StringView.hpp>

static auto s_MatchIDs = ToArray<StringView>({
    "PNP0300", "PNP0301", "PNP0302", "PNP0303", "PNP0304", "PNP0305", "PNP0306",
    "PNP0309", "PNP030a", "PNP030b", "PNP0320", "PNP0343", "PNP0344", "PNP0345",
    "CPQA0D7", "AUI0200", "FJC6000", "FJC6001", "PNP0f03", "PNP0f0b", "PNP0f0e",
    "PNP0f12", "PNP0f13", "PNP0f19", "PNP0f1c", "SYN0801",
});

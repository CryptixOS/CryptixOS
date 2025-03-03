/*
 * Created by v1tr10l7 on 02.03.2025.
 *
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>
#include <owner_less.hpp>

#include <array>
#include <atomic>
#include <concepts>
#include <enable_shared_from_this.hpp>
#include <flanterm.h>
#include <limine.h>
#include <nostd/string.hpp>
#include <optional>
#include <parallel_hashmap/phmap.h>
#include <span>
#include <tuple>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>
#include <utility>
#include <veque.hpp>

/*
 * Created by v1tr10l7 on 07.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>

constexpr usize _IOC_NRBITS    = 8;
constexpr usize _IOC_TYPEBITS  = 8;
constexpr usize _IOC_SIZEBITS  = 14;
constexpr usize _IOC_DIRBITS   = 2;

constexpr usize _IOC_NRMASK    = (1 << _IOC_NRBITS) - 1;
constexpr usize _IOC_TYPEMASK  = (1 << _IOC_TYPEBITS) - 1;
constexpr usize _IOC_SIZEMASK  = (1 << _IOC_SIZEBITS) - 1;
constexpr usize _IOC_DIRMASK   = (1 << _IOC_DIRBITS) - 1;

constexpr usize _IOC_NRSHIFT   = 0;
constexpr usize _IOC_TYPESHIFT = _IOC_NRSHIFT + _IOC_NRBITS;
constexpr usize _IOC_SIZESHIFT = _IOC_TYPESHIFT + _IOC_TYPEBITS;
constexpr usize _IOC_DIRSHIFT  = _IOC_SIZESHIFT + _IOC_SIZEBITS;

constexpr usize _IOC_NONE      = 0u;
constexpr usize _IOC_WRITE     = 1u;
constexpr usize _IOC_READ      = 2u;

template <usize dir, usize type, usize nr, usize size>
consteval usize _IOC()
{
    return (dir << _IOC_DIRSHIFT) | (type << _IOC_TYPESHIFT)
         | (nr << _IOC_NRSHIFT) | (size << _IOC_SIZESHIFT);
}

template <typename T>
consteval usize _IOC_TYPECHECK()
{
    return sizeof(T);
}

template <usize type, usize nr>
consteval usize _IO()
{
    return _IOC<_IOC_NONE, type, nr>();
}
template <usize type, usize nr, typename argtype>
consteval usize _IOR()
{
    return _IOC<_IOC_READ, type, nr, _IOC_TYPECHECK<argtype>()>();
}
template <usize type, usize nr, typename argtype>
consteval usize _IOW()
{
    return _IOC<_IOC_WRITE, type, nr, _IOC_TYPECHECK<argtype>()>();
}
template <usize type, usize nr, typename argtype>
consteval usize _IOWR()
{
    return _IOC<_IOC_READ | _IOC_WRITE, type, nr, _IOC_TYPECHECK<argtype>()>();
}

template <usize type, usize nr, typename argtype>
consteval usize _IOR_BAD()
{
    return _IOC<_IOC_READ, type, nr, sizeof(argtype)>();
}
template <usize type, usize nr, typename argtype>
consteval usize _IOW_BAD()
{
    return _IOC<_IOC_WRITE, type, nr, sizeof(argtype)>();
}

template <usize type, usize nr, typename argtype>
consteval usize _IOWR_BAD()
{
    return _IOC<_IOC_READ | _IOC_WRITE, type, nr, sizeof(argtype)>();
}

consteval usize _IOC_DIR(usize nr)
{
    return (nr >> _IOC_DIRSHIFT) & _IOC_DIRMASK;
}
consteval usize _IOC_TYPE(usize nr)
{
    return (nr >> _IOC_TYPESHIFT) & _IOC_TYPEMASK;
}
consteval usize _IOC_NR(usize nr) { return (nr >> _IOC_NRSHIFT) & _IOC_NRMASK; }
consteval usize _IOC_SIZE(usize nr)
{
    return (nr >> _IOC_SIZESHIFT) & _IOC_SIZEMASK;
}

constexpr usize IOC_IN        = _IOC_WRITE << _IOC_DIRSHIFT;
constexpr usize IOC_OUT       = _IOC_READ << _IOC_DIRSHIFT;
constexpr usize IOC_INOUT     = (_IOC_WRITE | _IOC_READ) << _IOC_DIRSHIFT;
constexpr usize IOCSIZE_MASK  = _IOC_SIZEMASK << _IOC_SIZESHIFT;
constexpr usize IOCSIZE_SHIFT = _IOC_SIZESHIFT;

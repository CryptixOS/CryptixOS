/*
 * Created by v1tr10l7 on 09.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Utility/Math.hpp>

class Color
{
  public:
    constexpr static u32 RED_BITS_MASK   = 0x00'ff'00'00;
    constexpr static u32 GREEN_BITS_MASK = 0x00'00'ff'00;
    constexpr static u32 BLUE_BITS_MASK  = 0x00'00'00'ff;
    constexpr static u32 ALPHA_BITS_MASK = 0xff'00'00'00;
    constexpr static u32 RGB_BITS_MASK
        = RED_BITS_MASK | GREEN_BITS_MASK | BLUE_BITS_MASK;

    constexpr static u32 RED_BITS_SHIFT   = 16;
    constexpr static u32 GREEN_BITS_SHIFT = 8;
    constexpr static u32 BLUE_BITS_SHIFT  = 0;
    constexpr static u32 ALPHA_BITS_SHIFT = 24;

    constexpr Color()                     = default;
    explicit inline constexpr Color(u32 rgba)
    {
        m_Red   = (rgba & RED_BITS_MASK) >> RED_BITS_SHIFT;
        m_Green = (rgba & GREEN_BITS_MASK) >> GREEN_BITS_SHIFT;
        m_Blue  = (rgba & BLUE_BITS_MASK) >> BLUE_BITS_SHIFT;
        m_Alpha = (rgba & ALPHA_BITS_MASK) >> ALPHA_BITS_SHIFT;
    }
    inline constexpr Color(u8 r, u8 g, u8 b, u8 a = 0)
        : m_Red(r)
        , m_Green(g)
        , m_Blue(b)
        , m_Alpha(a)
    {
    }
    inline constexpr Color(const Color& other, u8 alpha)
        : m_Red(other.m_Red)
        , m_Green(other.m_Green)
        , m_Blue(other.m_Blue)
        , m_Alpha(alpha)
    {
    }

    inline constexpr u32 RGB() const
    {
        return (m_Red << RED_BITS_SHIFT) | (m_Green << GREEN_BITS_SHIFT)
             | m_Blue << BLUE_BITS_SHIFT;
    }
    inline constexpr u32 RGBA() const
    {
        return RGB() | m_Alpha << ALPHA_BITS_SHIFT;
    }

    inline constexpr u8   Red() const { return m_Red; }
    inline constexpr u8   Green() const { return m_Green; }
    inline constexpr u8   Blue() const { return m_Blue; }
    inline constexpr u8   Alpha() const { return m_Alpha; }

    inline constexpr void SetRGB(u32 rgb)
    {
        m_Red   = rgb >> RED_BITS_SHIFT;
        m_Green = rgb >> GREEN_BITS_SHIFT;
        m_Blue  = rgb >> BLUE_BITS_SHIFT;
    }
    inline constexpr void SetRGBA(u32 rgba)
    {
        SetRGB(rgba);
        m_Alpha = rgba >> ALPHA_BITS_SHIFT;
    }

    inline constexpr void  SetRed(u8 r) { m_Red = r; }
    inline constexpr void  SetGreen(u8 g) { m_Green = g; }
    inline constexpr void  SetBlue(u8 b) { m_Blue = b; }
    inline constexpr void  SetAlpha(u8 a) { m_Alpha = a; }

    inline constexpr       operator u32() const { return RGBA(); }

    inline constexpr Color Blend(Color foreground) const
    {
        u32 alpha        = 255 - (foreground.m_Alpha << ALPHA_BITS_SHIFT);
        u32 inverseAlpha = foreground.m_Alpha + 1;

        u8  r = (alpha * foreground.m_Red + inverseAlpha * m_Red) / 256;
        u8  g = (alpha * foreground.m_Green + inverseAlpha * m_Green) / 256;
        u8  b = (alpha * foreground.m_Blue + inverseAlpha * m_Blue) / 256;

        return Color(r, g, b);
    }

    inline constexpr static auto AnsiBlack() { return Color(0xa0'00'00'00); }
    inline constexpr static auto AnsiRed() { return Color(0xa0'aa'00'00); }
    inline constexpr static auto AnsiGreen() { return Color(0xa0'00'aa'00); }
    inline constexpr static auto AnsiYellow() { return Color(0xa0'aa'55'00); }
    inline constexpr static auto AnsiBlue() { return Color(0xa0'00'00'aa); }
    inline constexpr static auto AnsiMagenta() { return Color(0xa0'aa'00'aa); }
    inline constexpr static auto AnsiCyan() { return Color(0xa0'00'aa'aa); }
    inline constexpr static auto AnsiWhite() { return Color(0xa0'aa'aa'aa); }

    inline constexpr static auto AnsiBrightBlack()
    {
        return Color(0xa0'55'55'55);
    }
    inline constexpr static auto AnsiBrightRed()
    {
        return Color(0xa0'ff'55'55);
    }
    inline constexpr static auto AnsiBrightGreen()
    {
        return Color(0xa0'55'ff'55);
    }
    inline constexpr static auto AnsiBrightYellow()
    {
        return Color(0xa0'ff'ff'55);
    }
    inline constexpr static auto AnsiBrightBlue()
    {
        return Color(0xa0'55'55'ff);
    }
    inline constexpr static auto AnsiBrightMagenta()
    {
        return Color(0xa0'ff'55'ff);
    }
    inline constexpr static auto AnsiBrightCyan()
    {
        return Color(0xa0'55'ff'ff);
    }
    inline constexpr static auto AnsiBrightWhite()
    {
        return Color(0xa0'ff'ff'ff);
    }

  private:
    u8 m_Red   = 0;
    u8 m_Green = 0;
    u8 m_Blue  = 0;
    u8 m_Alpha = 0;
};

inline constexpr Color operator""_rgb(unsigned long long rgb)
{
    return Color(rgb >> Color::RED_BITS_SHIFT, rgb >> Color::GREEN_BITS_SHIFT,
                 rgb >> Color::BLUE_BITS_SHIFT);
}
inline constexpr Color operator""_rgba(unsigned long long rgba)
{
    return Color(rgba);
}

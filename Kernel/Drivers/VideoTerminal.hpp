/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Boot/BootInfo.hpp>

#include <Drivers/Terminal.hpp>
#include <Library/Image.hpp>

#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Core/Types.hpp>

struct Rectangle
{
    usize          X;
    usize          Y;
    usize          Width;
    usize          Height;

    constexpr bool InBounds(usize x, usize y)
    {
        return (x >= X && x < X + Width) && (y >= Y && y < Y + Height);
    }
};

class VideoTerminal final : public Terminal
{
  public:
    VideoTerminal() = default;
    VideoTerminal(const limine_framebuffer& framebuffer)
    {
        Initialize(framebuffer);
    }

    bool Initialize(const limine_framebuffer& framebuffer) override;

  protected:
    virtual void Clear(u32 = 0xffffffff, bool clear = true) override;

    virtual void RawPutChar(u8 c) override;
    virtual void MoveCharacter(usize newX, usize newY, usize oldX,
                               usize oldY) override;

    virtual void Scroll(isize count = 1) override;

    virtual void ScrollDown() override;
    virtual void ScrollUp() override;

    virtual void Refresh() override;
    virtual void Flush() override;

    virtual void ShowCursor() override;
    virtual bool HideCursor() override;

    virtual std::pair<usize, usize> GetCursorPos() override;
    virtual void SetCursorPos(usize xpos, usize ypos) override;

    virtual void SaveState() override;
    virtual void RestoreState() override;

    virtual void SwapPalette() override;

    virtual void SetTextForeground(AnsiColor color) override;
    virtual void SetTextBackground(AnsiColor color) override;

    virtual void SetTextForegroundRgb(u32 color) override;
    virtual void SetTextBackgroundRgb(u32 color) override;

    virtual void SetTextForegroundDefault() override;
    virtual void SetTextBackgroundDefault() override;

    virtual void SetTextForegroundDefaultBright() override;
    virtual void SetTextBackgroundDefaultBright() override;

    virtual void Destroy() override;

  private:
    struct Framebuffer
    {
        Pointer     Address;
        u64         Width;
        u64         Height;
        u64         Pitch;
        u16         BitsPerPixel;
        u8          RedMaskSize;
        u8          RedMaskShift;
        u8          GreenMaskSize;
        u8          GreenMaskShift;
        u8          BlueMaskSize;
        u8          BlueMaskShift;

        inline void PutPixel(usize x, usize y, u32 color)
        {
            Address.As<volatile u32>()[y * (Pitch / sizeof(u32)) + x] = color;
        }

    } m_Framebuffer = {};

    struct Font
    {
        Pointer Address;
        usize   Width;
        usize   Height;
        usize   Spacing;
        usize   ScaleX;
        usize   ScaleY;
        usize   GlyphWidth;
        usize   GlyphHeight;
    } m_Font;

    struct Image
    {
        Pointer Address;
        usize   Width;
        usize   Height;
        usize   Size;
    } m_Canvas;
    PNG::Image m_Image;

    struct ConsoleState
    {
        usize CursorX;
        usize CursorY;
        usize TextForeground;
        usize TextBackground;
        usize Bold;
        usize BackgroundBold;
        usize ReverseVideo;
        usize Charset;
        usize Primary;
        usize Background;
    };

    constexpr static usize DEFAULT_TEXT_FOREGROUND        = 0x00aaaaaa;
    constexpr static usize DEFAULT_TEXT_BACKGROUND        = 0xffffffff;
    constexpr static usize DEFAULT_TEXT_FOREGROUND_BRIGHT = 0xa0555555;
    constexpr static usize DEFAULT_TEXT_BACKGROUND_BRIGHT = 0x00aaaaaa;

    ConsoleState           m_CurrentState;
    ConsoleState           m_SavedState;

    usize                  m_OldCursorX     = 0;
    usize                  m_OldCursorY     = 0;
    usize                  m_OffsetX        = 0;
    usize                  m_OffsetY        = 0;

    u16                    m_Margin         = 0;
    u16                    m_MarginGradient = 0;

    struct Character
    {
        u32            CodePoint;
        u32            Foreground;
        u32            Background;

        constexpr bool operator==(const Character& other) const
        {
            return !(CodePoint != other.CodePoint
                     || Foreground != other.Foreground
                     || Background != other.Background);
        }
    };

    struct QueueItem
    {
        usize     X;
        usize     Y;
        Character Character;
    };

    // We use vectors here, so we are cache friendly,
    // as those fields aren't likely to be resized any way
    Vector<QueueItem>  m_Queue;
    Vector<QueueItem*> m_Map;
    Vector<Character>  m_Grid;
    Vector<bool>       m_Glyphs;

    void               PlotChar(Character* c, usize xpos, usize ypos);
    void               EnqueueChar(Character* c, usize x, usize y);

    void               DrawCursor();
    void               SetFont(const Font& font);
    void               SetCanvas(u8* image, usize size);

    void               GenerateGradient();
    u32                BlendMargin(usize x, usize y, u32 orig);

    constexpr u32      ConvertColor(u32 color)
    {
        u32 r = (color >> 16) & 0xff;
        u32 g = (color >> 8) & 0xff;
        u32 b = color & 0xff;

        return (r << m_Framebuffer.RedMaskShift)
             | (g << m_Framebuffer.GreenMaskShift)
             | (b << m_Framebuffer.BlueMaskShift);
    }
    constexpr bool VerifyBounds(usize x, usize y)
    {
        return x < m_Size.ws_col && y < m_Size.ws_row;
    }
};

/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once
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
    static VideoTerminal* Create(Framebuffer& framebuffer);
    bool                  Initialize(const Framebuffer& framebuffer) override;

  protected:
    VideoTerminal() = default;
    explicit VideoTerminal(const Framebuffer& framebuffer)
    {
        Initialize(framebuffer);
    }

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

    virtual void SetTextForegroundRgb(Color color) override;
    virtual void SetTextBackgroundRgb(Color color) override;

    virtual void Destroy() override;

  private:
    Framebuffer m_Framebuffer = {};

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
        Color TextForeground;
        Color TextBackground;
        usize Bold;
        usize BackgroundBold;
        usize ReverseVideo;
        usize Charset;
        usize Primary;
        Color Background;
    };

    constexpr static Color DEFAULT_TEXT_FOREGROUND = Color(0x00'aa'aa'aa);
    constexpr static Color DEFAULT_TEXT_BACKGROUND = Color(0xff'ff'ff'ff);
    constexpr static Color DEFAULT_TEXT_FOREGROUND_BRIGHT
        = Color(0xa0'55'55'55);
    constexpr static Color DEFAULT_TEXT_BACKGROUND_BRIGHT
        = Color(0x00'aa'aa'aa);

    ConsoleState m_CurrentState;
    ConsoleState m_SavedState;

    usize        m_OldCursorX     = 0;
    usize        m_OldCursorY     = 0;
    usize        m_OffsetX        = 0;
    usize        m_OffsetY        = 0;

    u16          m_Margin         = 0;
    u16          m_MarginGradient = 0;

    struct Character
    {
        u32            CodePoint;
        Color          Foreground;
        Color          Background;

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
    Color              BlendMargin(usize x, usize y, Color orig);

    constexpr bool     VerifyBounds(usize x, usize y)
    {
        return x < m_Size.ws_col && y < m_Size.ws_row;
    }
};

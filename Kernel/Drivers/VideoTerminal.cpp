/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Drivers/VideoTerminal.hpp>

#include <Library/Logger.hpp>

constexpr u32   RED_BITS_MASK      = 0x00ff0000;
constexpr u32   GREEN_BITS_MASK    = 0x0000ff00;
constexpr u32   BLUE_BITS_MASK     = 0x000000ff;
constexpr u32   ALPHA_BITS_MASK    = 0xff000000;

constexpr u32   RED_BITS_SHIFT     = 16;
constexpr u32   GREEN_BITS_SHIFT   = 8;
constexpr u32   BLUE_BITS_SHIFT    = 0;
constexpr u32   ALPHA_BITS_SHIFT   = 24;

constexpr usize MAX_FONT_GLYPHS    = 256;
constexpr usize DEFAULT_BACKGROUND = 0xa0000000;
constexpr usize DEFAULT_BACKDROP   = 0x00000000;
extern "C"
{
    extern u8    _binary____Meta_fonts_font_bin_start[];
    extern u8    _binary____Meta_fonts_font_bin_end[];
    extern usize _binary____Meta_fonts_font_bin_size;
}

constexpr u32 BlendColors(u32 fg, u32 bg)
{
    u32   alpha        = 255 - (fg & ALPHA_BITS_MASK);
    u32   inverseAlpha = (fg >> ALPHA_BITS_SHIFT) + 1;

    u8    fgR          = (fg & RED_BITS_MASK) >> RED_BITS_SHIFT;
    u8    fgG          = (fg & GREEN_BITS_MASK) >> GREEN_BITS_SHIFT;
    u8    fgB          = (fg & BLUE_BITS_MASK) >> BLUE_BITS_SHIFT;

    u8    bgR          = (bg & RED_BITS_MASK) >> RED_BITS_SHIFT;
    u8    bgG          = (bg & GREEN_BITS_MASK) >> GREEN_BITS_SHIFT;
    u8    bgB          = (bg & BLUE_BITS_MASK) >> BLUE_BITS_SHIFT;

    u8    r     = static_cast<u8>((alpha * fgR + inverseAlpha * bgR) / 256);
    u8    g     = static_cast<u8>((alpha * fgG + inverseAlpha * bgG) / 256);
    u8    b     = static_cast<u8>((alpha * fgB + inverseAlpha * bgB) / 256);

    usize final = (r << RED_BITS_SHIFT) | (g << GREEN_BITS_SHIFT) | b;
    return final;
}

constexpr usize BUILTIN_FONT_WIDTH  = 8;
constexpr usize BUILTIN_FONT_HEIGHT = 16;

bool            VideoTerminal::Initialize(const limine_framebuffer& framebuffer)
{
    if (!framebuffer.address) return false;
    else if (m_Initialized) return true;

    m_Framebuffer = {.Address        = framebuffer.address,
                     .Width          = framebuffer.width,
                     .Height         = framebuffer.height,
                     .Pitch          = framebuffer.pitch,
                     .BitsPerPixel   = framebuffer.bpp,
                     .RedMaskSize    = framebuffer.red_mask_size,
                     .RedMaskShift   = framebuffer.red_mask_shift,
                     .GreenMaskSize  = framebuffer.green_mask_size,
                     .GreenMaskShift = framebuffer.green_mask_shift,
                     .BlueMaskSize   = framebuffer.blue_mask_size,
                     .BlueMaskShift  = framebuffer.blue_mask_shift};

    auto fontAddress
        = reinterpret_cast<uintptr_t>(&_binary____Meta_fonts_font_bin_start);
    auto fontModule = BootInfo::FindModule("font");
    if (fontModule)
        fontAddress = reinterpret_cast<uintptr_t>(fontModule->address);

    auto backgroundModule = BootInfo::FindModule("background");
    if (!backgroundModule) LogError("Terminal background not found");
    else
        SetCanvas(reinterpret_cast<u8*>(backgroundModule->address),
                  backgroundModule->size);

    SetTextForegroundDefault();
    SetTextBackgroundDefault();

    m_Margin         = 64;
    m_MarginGradient = 4;
    SetFont({
        .Address     = fontAddress,
        .Width       = BUILTIN_FONT_WIDTH,
        .Height      = BUILTIN_FONT_HEIGHT,
        .Spacing     = 1,
        .ScaleX      = 1,
        .ScaleY      = 1,
        .GlyphWidth  = BUILTIN_FONT_WIDTH * 1,
        .GlyphHeight = BUILTIN_FONT_HEIGHT * 1,
    });

    m_OffsetX = m_Margin
              + ((m_Framebuffer.Width - m_Margin * 2) % m_Font.GlyphWidth) / 2;
    m_OffsetY
        = m_Margin
        + ((m_Framebuffer.Height - m_Margin * 2) % m_Font.GlyphHeight) / 2;

    usize queueEntryCount = m_Size.ws_row * m_Size.ws_col;
    m_Grid.Resize(queueEntryCount);

    for (usize i = 0; i < queueEntryCount; i++)
    {
        m_Grid[i].CodePoint  = ' ';
        m_Grid[i].Foreground = m_CurrentState.TextForeground;
        m_Grid[i].Background = m_CurrentState.TextBackground;
    }

    m_Queue.Reserve(queueEntryCount);
    m_Map.Resize(queueEntryCount);

    Reset();
    Refresh();

    LogInfo(
        "VT: ws_col: {}, ws_row: {}, scrollMarginBottom: {}, scrollMarginTop: "
        "{}",
        m_Size.ws_col, m_Size.ws_row, m_ScrollBottomMargin, m_ScrollTopMargin);
    return (m_Initialized = true);
}

void VideoTerminal::Clear(u32 color, bool move)
{
    Character empty;
    empty.CodePoint  = ' ';
    empty.Foreground = m_CurrentState.TextForeground;
    empty.Background = m_CurrentState.TextBackground;

    for (usize i = 0; i < m_Size.ws_row * m_Size.ws_col; i++)
        EnqueueChar(&empty, i % m_Size.ws_col, i / m_Size.ws_col);

    if (move)
    {
        m_CurrentState.CursorX = 0;
        m_CurrentState.CursorY = 0;
    }
}

void VideoTerminal::RawPutChar(u8 c)
{
    if (m_CurrentState.CursorX >= m_Size.ws_col)
    {
        m_CurrentState.CursorX = 0;
        m_CurrentState.CursorY++;
        if (m_CurrentState.CursorY == m_ScrollBottomMargin)
        {
            m_CurrentState.CursorY--;
            ScrollDown();
        }
        if (m_CurrentState.CursorY >= m_Size.ws_col)
            m_CurrentState.CursorY = m_Size.ws_col - 1;
    }

    Character ch;
    ch.CodePoint  = c;
    ch.Foreground = m_CurrentState.TextForeground;
    ch.Background = m_CurrentState.TextBackground;
    EnqueueChar(&ch, m_CurrentState.CursorX++, m_CurrentState.CursorY);
}
void VideoTerminal::MoveCharacter(usize newX, usize newY, usize oldX,
                                  usize oldY)
{
    if (oldX >= m_Size.ws_col || oldY >= m_Size.ws_row || newX >= m_Size.ws_col
        || newY >= m_Size.ws_row)
        return;

    usize      i = oldX + oldY * m_Size.ws_col;

    QueueItem* q = m_Map[i];
    Character* c = q ? &q->Character : &m_Grid[i];

    EnqueueChar(c, newX, newY);
}

void VideoTerminal::Scroll(isize count)
{
    bool revScroll = false;
    if (count < 0)
    {
        revScroll = true;
        count *= -1;
    }

    for (isize i = 0; i < count; i++) revScroll ? ScrollUp() : ScrollDown();
}

void VideoTerminal::ScrollDown()
{
    for (usize i = (m_ScrollTopMargin + 1) * m_Size.ws_col;
         i < m_ScrollBottomMargin * m_Size.ws_col; i++)
    {
        QueueItem* q = m_Map[i];
        Character* c = q ? &q->Character : &m_Grid[i];
        EnqueueChar(c, (i - m_Size.ws_col) % m_Size.ws_col,
                    (i - m_Size.ws_col) / m_Size.ws_col);
    }

    Character empty;
    empty.CodePoint  = ' ';
    empty.Foreground = m_CurrentState.TextForeground;
    empty.Background = m_CurrentState.TextBackground;
    for (usize i = 0; i < m_Size.ws_col; i++)
        EnqueueChar(&empty, i, m_ScrollBottomMargin - 1);
}
void VideoTerminal::ScrollUp()
{
    for (usize i = (m_ScrollBottomMargin - 1) * m_Size.ws_col - 1;
         i >= m_ScrollTopMargin * m_Size.ws_col; i--)
    {
        if (i == static_cast<usize>(-1)) break;
        QueueItem* q = m_Map[i];
        Character* c = q ? &q->Character : &m_Grid[i];
        EnqueueChar(c, (i + m_Size.ws_col) % m_Size.ws_col,
                    (i + m_Size.ws_col) / m_Size.ws_col);
    }

    Character empty;
    empty.CodePoint  = ' ';
    empty.Foreground = m_CurrentState.TextForeground;
    empty.Background = m_CurrentState.TextBackground;
    for (usize i = 0; i < m_Size.ws_col; i++)
        EnqueueChar(&empty, i, m_ScrollTopMargin);
}

void VideoTerminal::Refresh()
{
    u32 bgColor = 0xffffffff;
    for (usize y = 0; y < m_Framebuffer.Height; y++)
    {
        for (usize x = 0; x < m_Framebuffer.Width; x++)
        {
            if (m_Canvas.Address)
                bgColor = m_Canvas.Address
                              .As<volatile u32>()[y * m_Framebuffer.Width + x];
            m_Framebuffer.PutPixel(x, y, bgColor);
        }
    }

    for (usize i = 0; i < m_Size.ws_row * m_Size.ws_col; i++)
    {
        usize x = i % m_Size.ws_col;
        usize y = i / m_Size.ws_col;

        PlotChar(&m_Grid[i], x, y);
    }

    DrawCursor();
}
void VideoTerminal::Flush()
{
    DrawCursor();
    while (!m_Queue.Empty())
    {
        QueueItem q      = m_Queue.PopBackElement();

        usize     offset = q.Y * m_Size.ws_col + q.X;
        if (!m_Map[offset]) continue;

        PlotChar(&q.Character, q.X, q.Y);
        m_Grid[offset] = q.Character;
        m_Map[offset]  = nullptr;
    }

    if (m_OldCursorX != m_CurrentState.CursorX
        || m_OldCursorY != m_CurrentState.CursorY)
    {
        if (m_OldCursorX < m_Size.ws_col && m_OldCursorY < m_Size.ws_row)
            PlotChar(&m_Grid[m_OldCursorX + m_OldCursorY * m_Size.ws_col],
                     m_OldCursorX, m_OldCursorY);
    }

    m_OldCursorX = m_CurrentState.CursorX;
    m_OldCursorY = m_CurrentState.CursorY;
}

void                    VideoTerminal::ShowCursor() {}
bool                    VideoTerminal::HideCursor() { return true; }

std::pair<usize, usize> VideoTerminal::GetCursorPos()
{
    usize x = m_CurrentState.CursorX >= m_Size.ws_col ? m_Size.ws_col - 1
                                                      : m_CurrentState.CursorX;
    usize y = m_CurrentState.CursorY >= m_Size.ws_row ? m_Size.ws_row - 1
                                                      : m_CurrentState.CursorY;

    return {x, y};
}
void VideoTerminal::SetCursorPos(usize xpos, usize ypos)
{
    if (xpos >= m_Size.ws_col)
        xpos = static_cast<i32>(xpos) < 0 ? 0 : m_Size.ws_col - 1;
    if (ypos >= m_Size.ws_row)
        ypos = static_cast<i32>(ypos) < 0 ? 0 : m_Size.ws_row - 1;

    m_CurrentState.CursorX = xpos;
    m_CurrentState.CursorY = ypos;
}

void VideoTerminal::SaveState() { m_SavedState = m_CurrentState; }
void VideoTerminal::RestoreState() { m_CurrentState = m_SavedState; }

void VideoTerminal::SwapPalette()
{
    std::swap(m_CurrentState.TextForeground, m_CurrentState.TextBackground);
}

constexpr usize s_AnsiColors[]
    = {0x00000000, 0x00aa0000, 0x0000aa00, 0x00aa5500, 0x000000aa, 0x00aa00aa,
       0x0000aaaa, 0x00aaaaaa, 0x00555555, 0x00ff5555, 0x0055ff55, 0x00ffff55,
       0x005555ff, 0x00ff55ff, 0x0055ffff, 0x00ffffff};

static_assert(std::size(s_AnsiColors) == 16);

void VideoTerminal::SetTextForeground(AnsiColor color)
{
    m_CurrentState.TextForeground
        = ConvertColor(s_AnsiColors[std::to_underlying(color)]);
}
void VideoTerminal::SetTextBackground(AnsiColor color)
{
    m_CurrentState.TextBackground
        = ConvertColor(s_AnsiColors[std::to_underlying(color)]);
}

void VideoTerminal::SetTextForegroundRgb(u32 rgb)
{
    m_CurrentState.TextForeground = ConvertColor(rgb);
}
void VideoTerminal::SetTextBackgroundRgb(u32 rgb)
{
    m_CurrentState.TextBackground = ConvertColor(rgb);
}

void VideoTerminal::SetTextForegroundDefault()
{
    SetTextForegroundRgb(DEFAULT_TEXT_FOREGROUND);
}
void VideoTerminal::SetTextBackgroundDefault()
{
    m_CurrentState.TextBackground = 0xffffffff;
}

void VideoTerminal::SetTextForegroundDefaultBright()
{
    SetTextForegroundRgb(DEFAULT_TEXT_FOREGROUND_BRIGHT);
}
void VideoTerminal::SetTextBackgroundDefaultBright()
{
    SetTextBackgroundRgb(DEFAULT_TEXT_BACKGROUND_BRIGHT);
}

void VideoTerminal::Destroy()
{
    if (m_Canvas.Address) delete[] m_Canvas.Address.As<u8>();
}

void VideoTerminal::PlotChar(Character* c, usize xpos, usize ypos)
{
    if (!VerifyBounds(xpos, ypos)) return;
    u32 defaultBg = 0xffffffff;

    xpos          = m_OffsetX + xpos * m_Font.GlyphWidth;
    ypos          = m_OffsetY + ypos * m_Font.GlyphHeight;

    bool* glyph   = &m_Glyphs[c->CodePoint * m_Font.Height * m_Font.Width];

    for (usize gy = 0; gy < m_Font.GlyphHeight; gy++)
    {
        u8   fy         = gy / m_Font.ScaleY;
        u32* canvasLine = m_Canvas.Address.As<u32>() + xpos
                        + (ypos + gy) * m_Framebuffer.Width;
        for (usize fx = 0; fx < m_Font.Width; fx++)
        {
            bool draw = glyph[fy * m_Font.Width + fx];
            for (usize i = 0; i < m_Font.ScaleX; i++)
            {
                usize gx = m_Font.ScaleX * fx + i;
                u32   bg
                    = c->Background == 0xffffffff ? defaultBg : c->Background;
                u32 fg
                    = c->Foreground == 0xffffffff ? defaultBg : c->Foreground;

                if (m_Canvas.Address)
                {
                    bg = c->Background == 0xffffffff ? canvasLine[gx]
                                                     : c->Background;
                    fg = c->Foreground == 0xffffffff ? canvasLine[gx]
                                                     : c->Foreground;
                }
                m_Framebuffer.PutPixel(xpos + gx, ypos + gy, draw ? fg : bg);
            }
        }
    }
}

void VideoTerminal::EnqueueChar(Character* c, usize x, usize y)
{
    if (x >= m_Size.ws_col || y >= m_Size.ws_row) return;
    usize      i = y * m_Size.ws_col + x;

    QueueItem* q = m_Map[i];
    if (!q)
    {
        if (m_Grid[i] == *c) return;
        q        = &m_Queue.EmplaceBack();
        q->X     = x;
        q->Y     = y;
        m_Map[i] = q;
    }

    q->Character = *c;
}

void VideoTerminal::DrawCursor()
{
    if (!VerifyBounds(m_CurrentState.CursorX, m_CurrentState.CursorY)) return;
    usize i = m_CurrentState.CursorX + m_CurrentState.CursorY * m_Size.ws_col;

    Character  c;
    QueueItem* q = m_Map[i];
    if (q) c = q->Character;
    else c = m_Grid[i];

    std::swap(c.Foreground, c.Background);
    PlotChar(&c, m_CurrentState.CursorX, m_CurrentState.CursorY);
    if (q)
    {
        m_Grid[i] = q->Character;
        m_Map[i]  = nullptr;
    }
}

void VideoTerminal::SetFont(const Font& font)
{
    m_Font = font;

    if (m_Font.ScaleX < 1) m_Font.ScaleX = 1;
    if (m_Font.ScaleY < 1) m_Font.ScaleY = 1;

    if (m_Framebuffer.Width >= (1920 + 1920 / 3)
        && m_Framebuffer.Height >= (1080 + 1080 / 3))
    {
        m_Font.ScaleX = 2;
        m_Font.ScaleY = 2;
    }
    if (m_Framebuffer.Width >= (3840 + 3840 / 3)
        && m_Framebuffer.Height >= (2160 + 2160 / 3))
    {
        m_Font.ScaleX = 4;
        m_Font.ScaleY = 4;
    }

    m_Font.Width += m_Font.Spacing;
    m_Glyphs.Resize(MAX_FONT_GLYPHS * m_Font.Height * m_Font.Width
                    * sizeof(u32));

    for (usize i = 0; i < MAX_FONT_GLYPHS; i++)
    {
        u8* glyph = m_Font.Address.Offset<Pointer>(i * m_Font.Height).As<u8>();
        for (usize y = 0; y < m_Font.Height; y++)
        {
            for (usize x = 0; x < 8; x++)
            {
                usize offset
                    = i * m_Font.Height * m_Font.Width + y * m_Font.Width + x;

                m_Glyphs[offset] = glyph[y] & (0x80 >> x);
            }

            for (usize x = 8; x < m_Font.Width; x++)
            {
                usize offset
                    = i * m_Font.Height * m_Font.Width + y * m_Font.Width + x;

                m_Glyphs[offset] = (i >= 0xc0 && i <= 0xdf) && glyph[y] & 1;
            }
        }
    }

    m_Font.GlyphWidth  = m_Font.Width * m_Font.ScaleX;
    m_Font.GlyphHeight = m_Font.Height * m_Font.ScaleY;

    m_Size.ws_col = (m_Framebuffer.Width - m_Margin * 2) / m_Font.GlyphWidth;
    m_Size.ws_row = (m_Framebuffer.Height - m_Margin * 2) / m_Font.GlyphHeight;
    m_Size.ws_xpixel = m_Framebuffer.Width;
    m_Size.ws_ypixel = m_Framebuffer.Height;
}
void VideoTerminal::SetCanvas(u8* image, usize size)
{
    if (!image || !m_Image.LoadFromMemory(image, size))
    {
        LogError("VT: Failed to load the canvas!");
        return;
    }

    m_Canvas.Width   = m_Image.Width();
    m_Canvas.Height  = m_Image.Height();

    m_Margin         = 64;
    m_MarginGradient = 4;

    if (m_Framebuffer.Width > m_Canvas.Width
        || m_Framebuffer.Height > m_Canvas.Height)
    {
        // TODO(v1tr10l7): Stretch
        m_Canvas.Width  = m_Framebuffer.Width;
        m_Canvas.Height = m_Framebuffer.Height;
    }

    usize bgCanvasSize   = m_Framebuffer.Width * m_Framebuffer.Height;
    u32*  bgCanvas       = new u32[bgCanvasSize];
    m_Canvas.Address     = bgCanvas;
    m_Canvas.Size        = bgCanvasSize;

    i64 marginNoGradient = static_cast<i64>(m_Margin) - m_MarginGradient;
    if (marginNoGradient < 0) marginNoGradient = 0;

    GenerateGradient();
}
void VideoTerminal::GenerateGradient()
{
    Pointer     pixels        = m_Image.Pixels();
    const usize imagePitch    = m_Image.Pitch();
    const usize columnSize    = m_Image.BitsPerPixel() / 8;
    const usize displacementX = m_Framebuffer.Width / 2 - m_Image.Width() / 2;
    const usize displacementY = m_Framebuffer.Height / 2 - m_Image.Height() / 2;

    Rectangle   gradientArea(60, 60, m_Framebuffer.Width - 60 * 2,
                             m_Framebuffer.Height - 60 * 2);

    for (usize y = 0; y < m_Framebuffer.Height; y++)
    {
        usize       imageY       = y - displacementY;
        const usize offset       = imagePitch * imageY;
        usize       canvasOffset = m_Framebuffer.Width * y;
        for (usize x = 0; x < m_Framebuffer.Width; x++)
        {
            usize   imageX     = (x - displacementX);
            Pointer imagePixel = pixels.Offset(imageX * columnSize + offset);

            u32     color      = imageX >= m_Image.Width() ? DEFAULT_BACKDROP
                                                           : *imagePixel.As<u32>();
            if (gradientArea.InBounds(x, y))
                color = BlendColors(DEFAULT_BACKGROUND, color);
            else color = BlendMargin(x, y, color);
            m_Canvas.Address.As<u32>()[canvasOffset + x] = color;
        }
    }
}

u32 VideoTerminal::BlendMargin(usize x, usize y, u32 backgroundPixel)
{
    usize gradientStopX = m_Framebuffer.Width - m_Margin;
    usize gradientStopY = m_Framebuffer.Height - m_Margin;
    usize distanceX     = x - gradientStopX;
    usize distanceY     = y - gradientStopY;

    if (x < m_Margin) distanceX = m_Margin - x;
    if (y < m_Margin) distanceY = m_Margin - y;

    usize distance = Math::Sqrt(
        static_cast<u64>(distanceX) * static_cast<u64>(distanceX)
        + static_cast<u64>(distanceY) * static_cast<u64>(distanceY));
    if (x >= m_Margin && x < gradientStopX) distance = distanceY;
    else if (y >= m_Margin && y < gradientStopY) distance = distanceX;

    if (distance > m_MarginGradient) return backgroundPixel;

    u8 gradientStep = (0xff - (DEFAULT_BACKGROUND >> 24)) / m_MarginGradient;
    u8 newAlpha     = (DEFAULT_BACKGROUND >> 24) + gradientStep * distance;

    return BlendColors((DEFAULT_BACKGROUND & 0xffffff) | (newAlpha << 24),
                       backgroundPixel);
}

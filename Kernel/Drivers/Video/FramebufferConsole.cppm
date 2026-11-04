/*
 * Created by v1tr10l7 on 30.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */

module;

#include <API/Posix/termios.h>
#include <Common.hpp>
#include <Debug/Assertions.hpp>
#include <Prism/String/StringView.hpp>
#include <Prism/Debug/Assertions.hpp>
#include <Library/Logger.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Debug/Log.hpp>
#include <cstdarg>
#include <Prism/String/String.hpp>
#include <compare>
#include <API/UnixTypes.hpp>
#include <Compiler.hpp>
#if defined(CTOS_TARGET_X86_64)
#include <Arch/x86_64/CPUIntrinsics.hpp>
#endif
#include <API/Posix/sys/mman.h>
#include <Prism/Core/Core.hpp>
#include <Prism/Memory/Pointer.hpp>
#include <Prism/Memory/Ref.hpp>
#include <Prism/Containers/Span.hpp>
#include <Arch/Arch.hpp>
#include <API/Posix/time.h>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/NonCopyable.hpp>
#include <Prism/Core/NonMovable.hpp>
#include <Prism/Utility/Atomic.hpp>
#include <Prism/Containers/KeyValuePair.hpp>
#include <Prism/Core/Error.hpp>
#include <Prism/Utility/Math.hpp>
#include <Prism/Containers/IntrusiveRefList.hpp>
#include <Prism/Utility/Time.hpp>
#if !(defined(CTOS_TARGET_X86_64)) && defined(CTOS_TARGET_AARCH64)
#include <Arch/aarch64/CPUIntrinsics.hpp>
#endif
#include <Prism/Memory/Memory.hpp>
#include <Prism/Utility/Path.hpp>
#include <Prism/Containers/Bitmap.hpp>
#include <Prism/Utility/Optional.hpp>
#include <API/Posix/dirent.h>
#include <API/Posix/fcntl.h>
#include <API/Posix/unistd.h>
#include <Prism/Containers/Deque.hpp>
#include <Prism/Utility/Delegate.hpp>
#include <API/Posix/sys/stat.h>
#include <Prism/Containers/UnorderedMap.hpp>
#include <Prism/Memory/WeakRef.hpp>
#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/CircularQueue.hpp>
#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/Memory/ByteStream.hpp>

export module Cryptix:Drivers.Video.FramebufferConsole;

export import :Drivers.TTY.VirtualConsole;
export import :Library.Image;

export struct Rectangle
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

export class FramebufferConsole final : public VirtualConsole
{
  public:
    static FramebufferConsole* Create(Framebuffer& framebuffer,
                                      StringView   name  = "fbcon",
                                      usize        minor = 0);
    bool Initialize(const Framebuffer& framebuffer) override;
    bool Load();

  protected:
    FramebufferConsole() = default;
    explicit FramebufferConsole(const Framebuffer& framebuffer, StringView name,
                                usize minor);

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

    virtual KeyValuePair<usize, usize> GetCursorPos() override;
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
    bool        m_Loaded      = false;
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

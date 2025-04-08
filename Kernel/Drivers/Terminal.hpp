/*
 * Created by v1tr10l7 on 23.05.2024.
 * Copyright (c) 2024-2024, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <API/Posix/termios.h>
#include <Boot/BootInfo.hpp>

#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>
#include <Library/Spinlock.hpp>

#include <string_view>
#include <unordered_map>

namespace AnsiColor
{
    constexpr const char* FOREGROUND_BLACK   = "\u001b[30m";
    constexpr const char* FOREGROUND_RED     = "\u001b[31m";
    constexpr const char* FOREGROUND_GREEN   = "\u001b[32m";
    constexpr const char* FOREGROUND_YELLOW  = "\u001b[33m";
    constexpr const char* FOREGROUND_BLUE    = "\u001b[34m";
    constexpr const char* FOREGROUND_MAGENTA = "\u001b[35m";
    constexpr const char* FOREGROUND_CYAN    = "\u001b[36m";
    constexpr const char* FOREGROUND_WHITE   = "\u001b[37m";

    constexpr const char* BACKGROUND_BLACK   = "\u001b[40m";
    constexpr const char* BACKGROUND_RED     = "\u001b[41m";
    constexpr const char* BACKGROUND_GREEN   = "\u001b[42m";
    constexpr const char* BACKGROUND_YELLOW  = "\u001b[43m";
    constexpr const char* BACKGROUND_BLUE    = "\u001b[44m";
    constexpr const char* BACKGROUND_MAGENTA = "\u001b[45m";
    constexpr const char* BACKGROUND_CYAN    = "\u001b[46m";
    constexpr const char* BACKGROUND_WHITE   = "\u001b[47m";
}; // namespace AnsiColor

class Terminal
{
  public:
    Terminal();

    virtual bool          Initialize(const Framebuffer& m_Framebuffer) = 0;

    inline const winsize& GetSize() const { return m_Size; }

    inline bool           GetCursorKeyMode() const { return m_CursorKeyMode; }
    void                  Resize(const winsize& newDimensions);

    virtual void          Clear(u32 color = 0xffffffff, bool move = true) = 0;

    void                  PutChar(u64 c);
    void                  PutCharImpl(u64 c);

    void                  PrintString(std::string_view str);

    void                  Bell();

    static Terminal*      GetPrimary();
    static const Vector<Terminal*>& EnumerateTerminals();

  protected:
    bool     m_Initialized = false;
    winsize  m_Size;
    usize    m_ScrollBottomMargin = 0;
    usize    m_ScrollTopMargin    = 0;

    Spinlock m_Lock;

    enum class AnsiColor
    {
        eBlack,
        eRed,
        eGreen,
        eYellow,
        eBlue,
        eMagenta,
        eCyan,
        eWhite,

        eBrightBlack,
        eBrightRed,
        eBrightGreen,
        eBrightYellow,
        eBrightMagenta,
        eBrightCyan,
        eBrightWhite,
    };

    virtual void RawPutChar(u8 c) = 0;
    virtual void MoveCharacter(usize newX, usize newY, usize oldX, usize oldY)
        = 0;

    virtual void                    Scroll(isize count = 1)              = 0;
    virtual void                    ScrollDown()                         = 0;
    virtual void                    ScrollUp()                           = 0;

    virtual void                    Refresh()                            = 0;
    virtual void                    Flush()                              = 0;

    virtual void                    ShowCursor()                         = 0;
    virtual bool                    HideCursor()                         = 0;

    virtual std::pair<usize, usize> GetCursorPos()                       = 0;
    virtual void                    SetCursorPos(usize xpos, usize ypos) = 0;

    virtual void                    SaveState()                          = 0;
    virtual void                    RestoreState()                       = 0;

    virtual void                    SwapPalette()                        = 0;

    virtual void                    SetTextForeground(AnsiColor color)   = 0;
    virtual void                    SetTextBackground(AnsiColor color)   = 0;

    virtual void                    SetTextForegroundRgb(u32 color)      = 0;
    virtual void                    SetTextBackgroundRgb(u32 color)      = 0;

    virtual void                    SetTextForegroundDefault()           = 0;
    virtual void                    SetTextBackgroundDefault()           = 0;

    virtual void                    SetTextForegroundDefaultBright()     = 0;
    virtual void                    SetTextBackgroundDefaultBright()     = 0;

    void                            Reset();
    virtual void                    Destroy() = 0;

  private:
    enum class State
    {
        eNormal,
        eEscape,
        eControlSequenceEntry,
        eControlSequenceParams,
        eCsiExpectParameters,
        eOsCommand,
        eDecPrivate,
        eDecParameters,
        eDecCursorMode,
    } m_State;

    constexpr static usize              MAX_ESC_PARAMETER_COUNT = 16;
    Array<u32, MAX_ESC_PARAMETER_COUNT> m_EscapeValues;
    usize                               m_EscapeValueCount = 0;
    u8                                  m_TabSize          = 4;

    bool                                m_CursorKeyMode    = false;

    void                                OnEscapeChar(char c);
    void                                OnCsi(char c);
    bool                                DecSpecialPrint(u8 c);

    static Vector<Terminal*>            s_Terminals;
};

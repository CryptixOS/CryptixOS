/*
 * Created by v1tr10l7 on 02.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Endian.hpp>

#include <Prism/Memory/Buffer.hpp>
#include <Prism/Memory/ByteStream.hpp>
#include <Prism/Memory/Pointer.hpp>

#include <Prism/Utility/Math.hpp>

namespace PNG
{
    constexpr const char SIGNATURE[] = "\x89PNG\r\n\x1a\n";
    struct [[gnu::packed]] Header
    {
        u32 Width;
        u32 Height;
        u8  BitDepth;
        u8  ColorType;
        u8  Compression;
        u8  Filter;
        u8  Interlace;
    };

    struct Chunk
    {
        u32 Length;
        u32 Type;
    };

    enum class ColorType : u8
    {
        eGrayscale          = 0,
        eTrueColor          = 2,
        eIndexed            = 3,
        eGrayscaleWithAlpha = 4,
        eTrueColorWithAlpha = 6,
    };
    enum class CompressionMethod : u8
    {
        eDeflate = 0,
    };
    enum class FilterType : u8
    {
        eNone    = 0,
        eSub     = 1,
        eUp      = 2,
        eAverage = 3,
        ePaeth   = 4,
    };
    enum class InterlaceMethod : u8
    {
        eNone  = 0,
        eAdam7 = 1,
    };

    struct [[gnu::packed]] PaletteEntry
    {
        u8 Red;
        u8 Green;
        u8 Blue;
        u8 Alpha;
    };

    inline constexpr u8 PredictPaeth(u8 a, u8 b, u8 c)
    {
        i32 p  = a + b - c;
        i32 pa = Math::Abs(p - a);
        i32 pb = Math::Abs(p - b);
        i32 pc = Math::Abs(p - c);

        if (pa <= pb && pa <= pc) return a;
        if (pb <= pc) return b;
        return c;
    }

    class Loader
    {
      public:
        Loader() = default;
        Loader(u8* data, usize size)
            : m_Data(data)
            , m_Size(size)
        {
        }

        bool                 LoadFromMemory(u8* data, usize size);

        inline const Buffer& Pixels() const { return m_Pixels; }
        inline i32           Width() const { return m_Width; }
        inline i32           Height() const { return m_Height; }

      private:
        [[maybe_unused]] u8*   m_Data   = nullptr;
        [[maybe_unused]] usize m_Size   = 0;

        i32                    m_Width  = 0;
        i32                    m_Height = 0;
        u8                     m_Depth  = 0;
        u8                     m_Color;
        ColorType              m_ColorType;
        CompressionMethod      m_CompressionMethod;
        u8                     m_Filter;
        InterlaceMethod        m_Interlace;

        usize                  m_ChannelCount = 0;
        Vector<PaletteEntry>   m_Palette;

        Buffer                 m_CompressedData;
        usize                  m_IdatSize = 0;

        Buffer                 m_Pixels;

        Buffer Defilter(ByteStream<Endian::eLittle> inputStream);
        void   UnfilterScanline(u8* currentRow, const u8* previousRow,
                                i32 bytesPerPixel, i32 length, FilterType filter);

        bool   ParseIHDR(ByteStream<Endian::eBig>& stream, Chunk& chunk);
        bool   ParsePLTE(ByteStream<Endian::eBig>& stream, Chunk& chunk);
        bool   ParseTRNS(ByteStream<Endian::eBig>& stream, Chunk& chunk);
        bool   ParseIDAT(ByteStream<Endian::eBig>& stream, Chunk& chunk);
    };

    class Image
    {
      public:
        Image() = default;
        inline Image(u8* data, usize size) { LoadFromMemory(data, size); }

        inline ~Image() {}

        bool           LoadFromMemory(u8* data, usize size);

        inline Pointer Address() const { return m_Pixels.Raw(); }
        inline u8*   Pixels() const { return const_cast<u8*>(m_Pixels.Raw()); }
        inline       operator bool() const { return m_Pixels.Raw() != nullptr; }

        inline usize Size() const { return m_Width * m_Height * m_Pitch; }
        inline usize Width() const { return m_Width; }
        inline usize Height() const { return m_Height; }
        inline isize Pitch() const { return m_Pitch; }

        inline isize BitsPerPixel() const { return m_BitsPerPixel; }

      private:
        Buffer m_Pixels;
        usize  m_Width        = 0;
        usize  m_Height       = 0;
        i32    m_Pitch        = 0;
        i32    m_BitsPerPixel = 0;
    };
}; // namespace PNG

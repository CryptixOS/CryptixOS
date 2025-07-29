/*
 * Created by v1tr10l7 on 06.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Array.hpp>
#include <Prism/Containers/Span.hpp>
#include <Prism/Containers/Vector.hpp>
#include <Prism/Core/Types.hpp>

#include <Prism/Memory/BitStream.hpp>
#include <Prism/Memory/Endian.hpp>
#include <Prism/Memory/Pointer.hpp>

namespace ZLib
{
    enum class CompressionMethod : u8
    {
        eDeflate = 8,
    };
    struct Header
    {
        union
        {
            struct
            {
                CompressionMethod CompressionMethod : 4;
                u8                CompressionInfo   : 4;

                u8                CheckBits         : 5;
                bool              DictionaryPresent : 1;
                u8                CompressionLevel  : 2;
            };
            BigEndian<u16> Data;
        };

        Header() = default;
        explicit Header(u16 header)
            : Data(header)
        {
        }
    };
    static_assert(sizeof(Header) == sizeof(u16));

    enum class BlockType : u8
    {
        eUncompressed       = 0b00,
        eFixedHuffmanCode   = 0b01,
        eDynamicHuffmanCode = 0b10,
        eReserved           = 0b11,
    };

    struct HuffmanTree
    {
        inline static constexpr i32 FAST_BITS   = 9;
        inline static constexpr i32 FAST_MASK   = (1 << FAST_BITS) - 1;
        inline static constexpr i32 MAX_SYMBOLS = 288;

        Array<u16, 1 << FAST_BITS>  FastLookupTable;
        Array<u16, 16>              FirstCodeOfLength;
        Array<i32, 17>              MaxCodeInclusive;
        Array<u16, 16>              FirstSymbolIndexOfLength;
        Array<u8, MAX_SYMBOLS>      SymbolBitLengths;
        Array<u16, MAX_SYMBOLS>     SymbolValues;

        HuffmanTree() = default;

        bool Build(Span<u8, DynamicExtent> bitLengths);
        i32  Decode(BitStream<>& stream);
        i32  DecodeSlowPath(BitStream<>& stream);
    };

    class Decompressor
    {
      public:
        Decompressor(Pointer data, usize size)
            : m_InputData(data)
            , m_InputSize(size)
        {
        }
        ~Decompressor();

        bool       Decompress(usize initialSize = 0);

        inline u8* DecompressedData() const
        {
            return reinterpret_cast<u8*>(m_OutputBuffer);
        }
        inline usize DecompressedSize() const
        {
            return m_OutputPointer - m_OutputBuffer;
        }

      private:
        Pointer     m_InputData     = nullptr;
        usize       m_InputSize     = 0;

        u8*         m_InputEnd      = nullptr;

        u8*         m_OutputBuffer  = nullptr;
        usize       m_OutputSize    = 0;

        u8*         m_OutputPointer = nullptr;
        u8*         m_OutputEnd     = nullptr;

        HuffmanTree m_LiteralLengthTree;
        HuffmanTree m_DistanceTree;

        // Ensures that output buffer can hold @count additional bytes
        bool        EnsureCapacity(usize count);

        bool        DecompressStream(BitStream<>& stream);
        bool        ParseBlock(BitStream<>& stream, BlockType blockType);
        bool        DecodeCanonicalCodes(BitStream<>& stream);

        bool        DecodeBlock(BitStream<>& stream);
    };
}; // namespace ZLib

/*
 * Created by v1tr10l7 on 06.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Debug/Assertions.hpp>

#include <Library/Logger.hpp>
#include <Library/ZLib.hpp>

#include <Prism/Memory/ByteStream.hpp>
#include <Prism/Utility/Checksum.hpp>

namespace ZLib
{
    inline constexpr i32 ReverseBits(i32 value, i32 count)
    {
        Assert(count <= 16);

        i32 reversedBits = ((value & 0xaaaa) >> 1) | ((value & 0x5555) << 1);
        reversedBits
            = ((reversedBits & 0xcccc) >> 2) | ((reversedBits & 0x3333) << 2);
        reversedBits
            = ((reversedBits & 0xf0f0) >> 4) | ((reversedBits & 0x0f0f) << 4);
        reversedBits
            = ((reversedBits & 0xff00) >> 8) | ((reversedBits & 0x00ff) << 8);

        return reversedBits >> (16 - count);
    }

    bool HuffmanTree::Build(Span<u8, DynamicExtent> bitLengths)
    {
        i32            totalSymbolsAssigned = 0;
        Array<i32, 16> nextCodeForLength    = {};
        Array<i32, 17> codeLengthCounts     = {};

        FastLookupTable.Fill(0);
        codeLengthCounts.Fill(0);

        for (usize i = 0; i < bitLengths.Size(); ++i)
            ++codeLengthCounts[bitLengths[i]];
        codeLengthCounts[0] = 0;
        for (i32 length = 1; length < 16; ++length)
            if (codeLengthCounts[length] > (1 << length))
            {
                LogError("ZLib: Invalid code lengths");
                return false;
            }

        auto huffmanCode = 0;
        for (i32 length = 1; length < 16; ++length)
        {
            nextCodeForLength[length]        = huffmanCode;
            FirstCodeOfLength[length]        = huffmanCode;
            FirstSymbolIndexOfLength[length] = totalSymbolsAssigned;
            huffmanCode = (huffmanCode + codeLengthCounts[length]);
            if (codeLengthCounts[length] && (huffmanCode - 1ull >= Bit(length)))
            {
                LogError("ZLib: Invalid canonical code");
                return false;
            }
            MaxCodeInclusive[length] = huffmanCode << (16 - length);
            huffmanCode <<= 1;
            totalSymbolsAssigned += codeLengthCounts[length];
        }
        MaxCodeInclusive[16] = 0x10000;
        for (usize symbol = 0; symbol < bitLengths.Size(); ++symbol)
        {
            i32 length = bitLengths[symbol];
            if (bitLengths[symbol] == 0) continue;

            i32 symbolIndex = nextCodeForLength[length]
                            - FirstCodeOfLength[length]
                            + FirstSymbolIndexOfLength[length];
            u16 fastTableValue            = (u16)((length << 9) | symbol);

            SymbolBitLengths[symbolIndex] = (u8)length;
            SymbolValues[symbolIndex]     = (u16)symbol;
            if (length <= FAST_BITS)
            {
                i32 reversedBits
                    = ReverseBits(nextCodeForLength[length], length);
                while (reversedBits < (1 << FAST_BITS))
                {
                    FastLookupTable[reversedBits] = fastTableValue;
                    reversedBits += (1 << length);
                }
            }
            ++nextCodeForLength[length];
        }
        return true;
    }
    i32 HuffmanTree::Decode(BitStream<>& stream)
    {
        i32 bits      = stream.PeekBits(16);
        i32 fastEntry = FastLookupTable[bits & FAST_MASK];
        if (fastEntry)
        {
            i32 codeLength = fastEntry >> 9;
            stream.DropBits(codeLength);
            return fastEntry & 511;
        }
        return DecodeSlowPath(stream);
    }
    i32 HuffmanTree::DecodeSlowPath(BitStream<>& stream)
    {
        i32 reversedBits = ReverseBits(stream.PeekBits(16), 16);

        i32 codeLength   = 0;
        for (codeLength = FAST_BITS + 1;; ++codeLength)
            if (reversedBits < MaxCodeInclusive[codeLength]) break;
        if (codeLength >= 16) return -1;
        i32 symbolIndex = (reversedBits >> (16 - codeLength))
                        - FirstCodeOfLength[codeLength]
                        + FirstSymbolIndexOfLength[codeLength];
        if (symbolIndex >= MAX_SYMBOLS) return -1;
        if (SymbolBitLengths[symbolIndex] != codeLength) return -1;
        stream.DropBits(codeLength);
        return SymbolValues[symbolIndex];
    }

    Decompressor::~Decompressor()
    {
        if (m_OutputBuffer)
        {
            delete[] m_OutputBuffer;
            m_OutputBuffer = nullptr;
            m_OutputSize   = 0;
        }
    }

    bool Decompressor::Decompress(usize initialSize)
    {
        if (initialSize == 0) initialSize = m_InputSize;
        EnsureCapacity(initialSize);
        BitStream stream(m_InputData.As<u8>(), m_InputSize);
        m_InputEnd = m_InputData.Offset<u8*>(m_InputSize);

        Header header(stream.Read<u16>());
        if (header.CompressionMethod != CompressionMethod::eDeflate)
        {
            LogError("ZLib: Non-Deflate compression is not supported");
            return false;
        }
        if (header.DictionaryPresent)
        {
            LogError(
                "ZLib: Predefined dictionaries are currently not supported");
            return false;
        }
        if (header.Data.Load() % 31)
        {
            LogError("ZLib: Invalid checksum");
            return false;
        }

        if (stream.IsEndOfStream())
        {
            LogError("ZLib: End of stream");
            return false;
        }

        if (!DecompressStream(stream))
        {
            delete[] m_OutputBuffer;
            m_OutputBuffer = nullptr;
            m_OutputSize   = 0;
            return false;
        }
        m_OutputSize = m_OutputPointer - m_OutputBuffer;
        return true;
    }

    bool Decompressor::EnsureCapacity(usize count)
    {
        usize currentOffset
            = m_OutputBuffer ? m_OutputPointer - m_OutputBuffer : 0;
        usize newSize = m_OutputBuffer == 0 ? count : m_OutputSize;

        while (newSize < currentOffset + count) newSize <<= 1;

        if (m_OutputBuffer) delete[] m_OutputBuffer;
        m_OutputBuffer  = new u8[newSize];
        m_OutputSize    = newSize;

        m_OutputPointer = m_OutputBuffer + currentOffset;
        m_OutputEnd     = m_OutputBuffer + newSize;
        return true;
    }

    bool Decompressor::DecompressStream(BitStream<>& stream)
    {
        bool final = true;
        do {
            final          = stream.ReadBits(1);

            BlockType type = static_cast<BlockType>(stream.ReadBits(2));

            if (!ParseBlock(stream, type)) return false;
            if (!DecodeBlock(stream)) return false;
        } while (!final);

        u32 checksum = 0;
        Memory::Copy(&checksum, m_InputData.Offset<u8*>(m_InputSize - 4), 4);
        checksum = ConvertEndian<Endian::eNative, Endian::eBig>(checksum);

        const u32 computedAdler32
            = Adler32::DoChecksum(m_OutputBuffer, DecompressedSize());
        if (checksum != computedAdler32)
            LogError(
                "ZLib: Invalid adler32 checksum, adler32: {:#x}, "
                "computedAdler32: {:#x}",
                checksum, computedAdler32);

        return true;
    }
    bool Decompressor::ParseBlock(BitStream<>& stream, BlockType blockType)
    {
        constexpr Array<u8, HuffmanTree::MAX_SYMBOLS> defaultLengths = {
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8};

        constexpr Array<u8, 32> defaultDistances
            = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
               5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
        static_assert(defaultLengths.Size() == HuffmanTree::MAX_SYMBOLS);
        static_assert(defaultDistances.Size() == 32);

        switch (blockType)
        {
            case BlockType::eUncompressed:
            {
                stream.AlignToByteBoundary();
                u16 length        = stream.Read<u16>();
                u16 negatedLength = stream.Read<u16>();
                if ((length ^ 0xffff) != negatedLength)
                {
                    LogError("ZLib: (length ^ 0xffff) != negatedLength");
                    return false;
                }
                if (m_OutputPointer + length >= m_OutputEnd)
                    if (!EnsureCapacity(length * 2)) return false;
                stream.Read(m_OutputPointer, length);

                m_OutputPointer += length;
            }
            break;
            case BlockType::eFixedHuffmanCode:
                if (!m_LiteralLengthTree.Build(Span<u8, DynamicExtent>{
                        const_cast<u8*>(defaultLengths.Raw()),
                        defaultLengths.Size()})
                    || !m_DistanceTree.Build(Span<u8, DynamicExtent>{
                        const_cast<u8*>(defaultDistances.Raw()),
                        defaultDistances.Size()}))
                {
                    LogError("ZLib: Failed to build fixed huffman codes");
                    return false;
                }
                break;
            case BlockType::eDynamicHuffmanCode:
                if (!DecodeCanonicalCodes(stream))
                {
                    LogError(
                        "ZLib: Failed to build dynamic huffman canonical "
                        "codes");
                    return false;
                }
                break;

            default:
                LogError("ZLib: Invalid block type, blockType: {}",
                         ToUnderlying(blockType));
                return false;
        }

        return true;
    }
    bool Decompressor::DecodeCanonicalCodes(BitStream<>& stream)
    {
        constexpr u8 codeLengthOrder[19] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                            11, 4,  12, 3, 13, 2, 14, 1, 15};
        HuffmanTree  codeLengthTree;
        u8           allCodeLengths[320];
        u8           codeLengthCodeLengths[19];

        i32          literalCodeCount    = stream.ReadBits(5) + 257;
        i32          distanceCodeCount   = stream.ReadBits(5) + 1;
        i32          codeLengthCodeCount = stream.ReadBits(4) + 4;
        i32          totalCodes          = literalCodeCount + distanceCodeCount;

        Memory::Fill(codeLengthCodeLengths, 0, sizeof(codeLengthCodeLengths));
        for (i32 i = 0; i < codeLengthCodeCount; ++i)
            codeLengthCodeLengths[codeLengthOrder[i]] = stream.ReadBits(3);
        if (!codeLengthTree.Build({codeLengthCodeLengths, 19})) return false;

        i32 i = 0;
        for (i = 0; i < totalCodes;)
        {
            i32 symbol = codeLengthTree.Decode(stream);

            i32 repeat = 0;
            i32 value  = 0;
            switch (symbol)
            {
                case 0 ... 15: allCodeLengths[i++] = (u8)symbol; continue;
                case 16:
                    value  = allCodeLengths[i - 1];
                    repeat = stream.ReadBits(2) + 3;
                    break;
                case 17: repeat = stream.ReadBits(3) + 3; break;
                case 18: repeat = stream.ReadBits(7) + 11; break;
                default: LogError("ZLib: Invalid code length"); return false;
            }

            Memory::Fill(allCodeLengths + i, value, repeat);
            i += repeat;
        }
        if (i != totalCodes)
        {
            LogError("ZLib: Invalid number of code lengths");
            return false;
        }

        if (!m_LiteralLengthTree.Build(
                {allCodeLengths, (usize)literalCodeCount}))
            return false;
        if (!m_DistanceTree.Build(
                {allCodeLengths + literalCodeCount, (usize)distanceCodeCount}))
            return false;
        return true;
    }

    bool Decompressor::DecodeBlock(BitStream<>& stream)
    {
        u8* currentPointer = m_OutputPointer;
        for (;;)
        {
            i32 symbol = m_LiteralLengthTree.Decode(stream);
            if (symbol < 0)
            {
                LogError("ZLib: Bad huffman code");
                return false;
            }
            else if (symbol < 256)
            {
                if (currentPointer >= m_OutputEnd)
                {
                    if (!EnsureCapacity(4096)) return false;
                    currentPointer = m_OutputPointer;
                }
                *currentPointer++ = symbol;
                continue;
            }

            if (symbol == 256)
            {
                m_OutputPointer = currentPointer;
                return true;
            }
            if (symbol >= 286)
            {
                LogError("ZLib: Bad huffman code");
                return false;
            }
            symbol -= 257;
            constexpr i32 lengthBase[] = {
                3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

            constexpr i32 lengthExtraBits[]
                = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                   2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

            i32 len = lengthBase[symbol];
            if (lengthExtraBits[symbol])
                len += stream.ReadBits(lengthExtraBits[symbol]);
            symbol = m_DistanceTree.Decode(stream);
            if (symbol < 0 || symbol >= 30)
            {
                LogError("ZLib: Bad huffman code");
                return false;
            }
            constexpr i32 distBase[] = {
                1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
                33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
                1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

            constexpr i32 distExtraBits[]
                = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                   6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

            i32 distance = distBase[symbol];
            if (distExtraBits[symbol])
                distance += stream.ReadBits(distExtraBits[symbol]);
            if (currentPointer - m_OutputBuffer < distance)
            {
                LogError("ZLib: Bad distance");
                return false;
            }
            if (currentPointer + len >= m_OutputEnd)
            {
                if (!EnsureCapacity(len * 2)) return false;
                currentPointer = m_OutputPointer;
            }
            u8* src = reinterpret_cast<u8*>(currentPointer - distance);
            while (len--) *currentPointer++ = distance == 1 ? *src : *src++;
        }
    }
}; // namespace ZLib

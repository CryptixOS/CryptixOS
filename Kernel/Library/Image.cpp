/*
 * Created by v1tr10l7 on 02.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Debug/Assertions.hpp>

#include <Library/Image.hpp>
#include <Library/Logger.hpp>
#include <Library/ZLib.hpp>

#include <Prism/Containers/Vector.hpp>
#include <Prism/Memory/ByteStream.hpp>
#include <Prism/Utility/Checksum.hpp>
#include <Prism/Utility/Math.hpp>

#include <magic_enum/magic_enum.hpp>

namespace PNG
{
    constexpr usize ToChunkType(const char type[4])
    {
        return (type[0] << 24) | (type[1] << 16) | (type[2] << 8) | type[3];
    }
    enum class ChunkType
    {
        // Critical Chunks
        eIHDR = ToChunkType("IHDR"),
        eIDAT = ToChunkType("IDAT"),
        ePLTE = ToChunkType("PLTE"),
        eIEND = ToChunkType("IEND"),

        // Ancillary Chunks
        ebKGD = ToChunkType("bKGD"),
        ecHRM = ToChunkType("cHRM"),
        ecICP = ToChunkType("cICP"),
        edSIG = ToChunkType("dSIG"),
        eeXIf = ToChunkType("eXIf"),
        egAMA = ToChunkType("gAMA"),
        ehIST = ToChunkType("hIST"),
        eiCCP = ToChunkType("iCCP"),
        eiTXt = ToChunkType("iTXt"),
        epHYs = ToChunkType("pHYs"),
        esBIT = ToChunkType("sBIT"),
        esPLT = ToChunkType("sPLT"),
        esRGB = ToChunkType("sRGB"),
        esTER = ToChunkType("sTER"),
        etEXt = ToChunkType("tEXt"),
        etIME = ToChunkType("tIME"),
        etRNS = ToChunkType("tRNS"),
        ezTXt = ToChunkType("zTXt"),
        eCgBI = ToChunkType("CgBI"),
    };

    bool Loader::LoadFromMemory(u8* data, usize size)
    {
        ByteStream<Endian::eBig> stream(data, size);
        u8                       signature[8];
        stream >> signature;
        if (std::memcmp(signature, PNG::SIGNATURE, 8) != 0)
        {
            LogError("PNG: Invalid signature -> '{:#02x}'",
                     fmt::join(signature, "\\"));
            return false;
        }

        Chunk ih;
        stream >> ih.Length;
        auto ihdrCrcStart = stream.Raw() + stream.Offset();
        stream >> ih.Type;

        if (ih.Type != ToUnderlying(ChunkType::eIHDR))
        {
            LogError(
                "PNG: First chunk should be 'IHDR' type, but it's not,"
                " the image is corrupt, type: {:#08x}",
                ih.Type);
            return false;
        }

        if (!ParseIHDR(stream, ih))
        {
            LogError("PNG: Failed to parse the IHDR chunk");
            return false;
        }

        auto ihdrCrcEnd = stream.Raw() + stream.Offset();
        u32  crc32;
        stream >> crc32;
        u32 computedCrc32
            = CRC32::DoChecksum(ihdrCrcStart, ihdrCrcEnd - ihdrCrcStart);
        if (crc32 != computedCrc32)
            LogError("Failed to validate crc32 checksum!");

        for (;;)
        {
            Chunk chunk;
            stream >> chunk.Length;
            // CRC is only over chunk type + chunk data
            auto crcStart = stream.Raw() + stream.Offset();
            stream >> chunk.Type;

            switch (static_cast<ChunkType>(chunk.Type))
            {
                case ChunkType::eCgBI: stream.Skip(chunk.Length); break;
                case ChunkType::eIHDR:
                    LogError(
                        "PNG: Multiple IHDR chunks, the image is corrupt!");
                    return false;
                case ChunkType::ePLTE:
                    if (!ParsePLTE(stream, chunk)) return false;
                    break;
                case ChunkType::etRNS:
                    if (!ParseTRNS(stream, chunk))
                        LogError("PNG: Failed to parse tRNS");
                    break;
                case ChunkType::eIDAT:
                    if (!ParseIDAT(stream, chunk)) return false;
                    break;

                case ChunkType::eIEND: break;
                default:
                    if ((chunk.Type & (1 << 29)) == 0)
                    {
                        LogError("PNG: Unknown PNG chunk type -> '{:#x}'",
                                 chunk.Type);
                        return false;
                    }
                    else
                    {
                        char chunkType[5] = {0};
                        chunkType[3] = static_cast<char>(chunk.Type & 0xff);
                        chunkType[2]
                            = static_cast<char>((chunk.Type >> 8) & 0xff);
                        chunkType[1]
                            = static_cast<char>((chunk.Type >> 16) & 0xff);
                        chunkType[0]
                            = static_cast<char>((chunk.Type >> 24) & 0xff);
                        chunkType[4] = 0;
                        LogWarn("PNG: '{}' chunks are currently not supported",
                                chunkType);
                    }
                    stream.Skip(chunk.Length);
                    break;
            }

            // end of PNG chunk, verify the CRC32
            auto crcEnd = stream.Raw() + stream.Offset();
            stream >> crc32;
            if (crc32 != CRC32::DoChecksum(crcStart, crcEnd - crcStart))
                LogError("PNG: Failed to verify crc32 checksum of chunk: '{}'",
                         chunk.Type);
            if (static_cast<ChunkType>(chunk.Type) == ChunkType::eIEND) break;
        }

        if (m_CompressedData.Size() == 0)
        {
            LogError("PNG: No IDAT");
            return false;
        }

        ZLib::Decompressor zlibDecompressor(m_CompressedData.Raw(), m_IdatSize);
        usize              stride
            = Math::DivRoundUp(m_ChannelCount * m_Width * m_Depth + 7, 8) + 1;
        usize rawSize = stride * m_Height;
        zlibDecompressor.Decompress(rawSize);

        auto decompressedData = zlibDecompressor.DecompressedData();
        rawSize               = zlibDecompressor.DecompressedSize();
        if (!decompressedData) return false;

        m_Pixels = Defilter({decompressedData, rawSize});
        return !m_Pixels.Empty();
    }

    Buffer Loader::Defilter(ByteStream<Endian::eLittle> inputStream)
    {
        constexpr usize outputChannelCount = 4;

        usize           bytesPerSample     = m_Depth == 16 ? 2 : 1;
        usize           bitsPerPixel       = bytesPerSample * m_ChannelCount;
        usize           stride = m_Width * outputChannelCount * bytesPerSample;

        usize           imageRowBytes
            = Math::DivRoundUp(m_ChannelCount * m_Width * m_Depth, 8);
        usize expectedSize = (imageRowBytes + 1) * m_Height;

        if (inputStream.Size() < expectedSize)
        {
            LogError("PNG: Not enough pixels, the image is corrupted");

            return {};
        }

        Buffer reconstructedScanlines(m_Width * m_Height * outputChannelCount);
        ByteStream<Endian::eLittle> outStream(reconstructedScanlines);

        u8*                         prevRow = nullptr;
        for (i32 y = 0; y < m_Height; ++y)
        {
            FilterType filter;
            inputStream >> filter;

            if (!magic_enum::enum_contains(filter))
            {
                LogError("PNG: Invalid filter, the image is corrupted");
                return {};
            }

            outStream.Write(inputStream, imageRowBytes);
            u8* currentRow = &reconstructedScanlines[stride * y];
            UnfilterScanline(currentRow, prevRow, bitsPerPixel, imageRowBytes,
                             filter);

            for (i32 x = m_Width - 1;
                 m_ChannelCount < 4 && m_Depth == 8 && x >= 0; --x)
            {
                u8* src = currentRow + x * m_ChannelCount;
                u8* dst = currentRow + x * outputChannelCount;
                std::memmove(dst, src, m_ChannelCount);
                if (m_ChannelCount == 3) dst[3] = 255;
            }

            for (usize i = 0; m_Depth == 16 && i < stride; i += 2)
                std::swap(currentRow[i], currentRow[i + 1]);

            prevRow = currentRow;
        }

        return std::move(reconstructedScanlines);
    }
    void Loader::UnfilterScanline(u8* currentRow, const u8* previousRow,
                                  i32 bytesPerPixel, i32 length,
                                  FilterType filter)
    {
        for (i32 x = 0; x < length; ++x)
        {
            u8 left  = x >= bytesPerPixel ? currentRow[x - bytesPerPixel] : 0;
            u8 above = previousRow ? previousRow[x] : 0;
            u8 upperLeft = x >= bytesPerPixel && previousRow
                             ? previousRow[x - bytesPerPixel]
                             : 0;

            switch (filter)
            {
                case PNG::FilterType::eNone: break;
                case PNG::FilterType::eSub: currentRow[x] += left; break;
                case PNG::FilterType::eUp: currentRow[x] += above; break;
                case PNG::FilterType::eAverage:
                    currentRow[x] += static_cast<u8>((left + above) >> 1);
                    break;
                case PNG::FilterType::ePaeth:
                    currentRow[x] += PNG::PredictPaeth(left, above, upperLeft);
                    break;
            }
        }
    }

    bool Loader::ParseIHDR(ByteStream<Endian::eBig>& stream, Chunk& chunk)
    {
        if (chunk.Length != 13)
        {
            LogError("PNG: Bad IHDR length -> {}", chunk.Length);
            return false;
        }

        stream >> m_Width;
        stream >> m_Height;

        if (m_Width == 0 || m_Width > std::numeric_limits<i32>::max()
            || m_Height == 0 || m_Height > std::numeric_limits<i32>::max())
        {
            LogError("PNG: Invalid image size, width: {}, height: {}", m_Width,
                     m_Height);
            return false;
        }

        stream >> m_Depth;
        if (m_Depth > 16 || !Math::IsPowerOfTwo(m_Depth))
        {
            LogError(
                "PNG: Invalid bit depth -> {}\n"
                "Supported depths are: 1/2/4/8/16 bits",
                m_Depth);
            return false;
        }
        stream >> m_Color;
        m_ColorType = static_cast<ColorType>(m_Color);
        if (!magic_enum::enum_contains(m_ColorType))
        {
            LogError("PNG: Invalid color type -> {}", m_Color);
            return false;
        }

        constexpr auto validDepthsForColorType  = ToArray({
            ToArray({1, 2, 4, 8, 16}),
            {},
            {8, 16},
            {1, 2, 4, 8},
            {8, 16},
            {},
            {8, 16},
        });
        constexpr auto channelCountForColorType = ToArray({
            1,
            0,
            3,
            1,
            2,
            0,
            4,
        });

        bool           validDepth               = false;
        for (const auto depth : validDepthsForColorType[m_Color])
            if (depth == m_Depth) validDepth = true;
        if (!validDepth)
        {
            LogError("PNG: Invalid depth({}) for color type '{}'", m_Depth,
                     magic_enum::enum_name(m_ColorType).data() + 1);
            return false;
        }
        m_ChannelCount = channelCountForColorType[m_Color];

        stream >> m_CompressionMethod;
        if (m_CompressionMethod != CompressionMethod::eDeflate)
        {
            LogError("PNG: Bad compression method -> {}",
                     ToUnderlying(m_CompressionMethod));
            return false;
        }
        stream >> m_Filter;
        if (m_Filter != 0)
        {
            LogError("PNG: Bad filter method -> {}", m_Filter);
            return false;
        }
        stream >> m_Interlace;
        if (!magic_enum::enum_contains(m_Interlace))
        {
            LogError("PNG: Bad interlace method -> {}",
                     ToUnderlying(m_Interlace));
            return false;
        }

        return true;
    }
    bool Loader::ParsePLTE(ByteStream<Endian::eBig>& stream, Chunk& chunk)
    {
        if (m_ColorType == ColorType::eGrayscale
            || m_ColorType == ColorType::eGrayscaleWithAlpha)
        {
            LogError(
                "PNG: Encountered PLTE chunk, but the color type is -> '{}'"
                "\nPLTE chunk can only appear in images with ColorType:\n"
                "- TrueColor\n",
                "- Indexed\n", "- TrueColorWithAlpha",
                magic_enum::enum_name(m_ColorType).data() + 1);
            return false;
        }
        if (chunk.Length > 256 * 3)
        {
            LogError(
                "PNG: Invalid PLTE, contains more than 256 palette "
                "entries");
            return false;
        }

        usize paletteEntryCount = chunk.Length / 3;
        if (paletteEntryCount * 3 != chunk.Length)
        {
            LogError("PNG: Invalid PLTE size");
            return false;
        }

        m_Palette.Reserve(paletteEntryCount);
        for (usize i = 0; i < paletteEntryCount; ++i)
        {
            auto& entry = m_Palette.EmplaceBack();

            stream >> entry.Red;
            stream >> entry.Green;
            stream >> entry.Blue;
            entry.Alpha = 0;
        }

        return true;
    }
    bool Loader::ParseIDAT(ByteStream<Endian::eBig>& stream, Chunk& chunk)
    {
        if (m_ColorType == ColorType::eIndexed && m_Palette.Empty())
        {
            LogError(
                "PNG: The color type is Indexed, and IDAT chunk was found "
                "before PLTE");
            return false;
        }
        if (chunk.Length > (1u << 30))
        {
            LogError("PNG: IDAT size limit, idat section is larger than 2^30");
            return false;
        }
        if (static_cast<i32>(m_IdatSize + chunk.Length)
            < static_cast<i32>(m_IdatSize))
            return false;
        if (m_IdatSize + chunk.Length > m_CompressedData.Size())
        {
            usize newSize = m_CompressedData.Size();
            if (newSize == 0)
                newSize = chunk.Length > 4096 ? chunk.Length : 4096;
            while (m_IdatSize + chunk.Length > newSize) newSize <<= 1;

            m_CompressedData.Resize(newSize);
        }

        stream.Read(m_CompressedData.Raw() + m_IdatSize, chunk.Length);
        m_IdatSize += chunk.Length;
        return true;
    }
    bool Loader::ParseTRNS(ByteStream<Endian::eBig>& stream, Chunk& chunk)
    {
        if (m_CompressedData.Size())
        {
            LogError("PNG: tRNS after IDAT, image is corrupt");
            return false;
        }

        switch (m_ColorType)
        {
            case ColorType::eGrayscale:
            case ColorType::eTrueColor:
                LogWarn(
                    "PNG: tRNS for color types Grayscale and TrueColor is "
                    "currently not supported");
                return true;
            case ColorType::eIndexed:
                if (m_Palette.Empty())
                {
                    LogError(
                        "PNG: The image color type is Indexed, and tRNS "
                        "chunk "
                        "appeared before PLTE");
                    return false;
                }
                if (chunk.Length > m_Palette.Size())
                {
                    LogError(
                        "PNG: The color type is Indexed, and there are "
                        "more "
                        "tRNS alpha entries than PLTE");
                    return false;
                }
                for (auto& entry : m_Palette) stream >> entry.Alpha;
                return true;
            case ColorType::eGrayscaleWithAlpha:
            case ColorType::eTrueColorWithAlpha:
                LogWarn("PNG: tRNS chunk shouldn't appear for color type: '{}'",
                        magic_enum::enum_name(m_ColorType).data() + 1);
                break;

            default: break;
        }

        return false;
    }

    bool Image::LoadFromMemory(u8* data, usize size)
    {
        Loader pngLoader(data, size);
        if (!pngLoader.LoadFromMemory(data, size))
        {
            LogError("PNG: Failed to load the image file");
            return false;
        }

        auto& pixels = pngLoader.Pixels();
        if (pixels.Empty()) return false;

        m_Pixels.Resize(pixels.Size());
        std::memcpy(m_Pixels.Raw(), pixels.Raw(), pixels.Size());
        m_Width         = pngLoader.Width();
        m_Height        = pngLoader.Height();
        m_Pitch         = m_Width * 4;
        m_BitsPerPixel  = 32;

        u32* pixelArray = reinterpret_cast<u32*>(m_Pixels.Raw());
        for (u32 i = 0; i < m_Width * m_Height; i++)
            pixelArray[i] = (pixelArray[i] & 0x0000ff00)
                          | ((pixelArray[i] & 0x00ff0000) >> 16)
                          | ((pixelArray[i] & 0x000000ff) << 16);
        return true;
    }
}; // namespace PNG

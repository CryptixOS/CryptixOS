/*
 * Created by v1tr10l7 on 06.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ZLib.hpp>

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "Vectors.hpp"

namespace
{
    // Must match text() and noise() in gen-vectors.py
    void GenerateText(std::vector<std::uint8_t>& out, std::size_t size)
    {
        std::size_t start = out.size();
        for (std::size_t line = 0; out.size() - start < size; ++line)
        {
            std::string entry = "drwxr-xr-x root root "
                              + std::to_string(line * 37 % 9973)
                              + " usr/lib/file-" + std::to_string(line % 113)
                              + ".so\n";
            out.insert(out.end(), entry.begin(), entry.end());
        }
        out.resize(start + size);
    }
    void GenerateNoise(std::vector<std::uint8_t>& out, std::size_t size,
                       std::uint32_t seed)
    {
        std::uint32_t state = seed;
        for (std::size_t i = 0; i < size; ++i)
        {
            state = state * 1103515245u + 12345u;
            out.push_back(state >> 24);
        }
    }

    std::vector<std::uint8_t> OriginalData(const Case& test)
    {
        std::vector<std::uint8_t> data;
        for (std::size_t i = 0; i < test.ChunkCount; ++i)
        {
            const Chunk& chunk = test.Chunks[i];
            if (chunk.Kind == 0) GenerateText(data, chunk.Size);
            else GenerateNoise(data, chunk.Size, chunk.Seed);
        }
        return data;
    }
} // namespace

class ZLibDecompress : public testing::TestWithParam<Case>
{
};

TEST_P(ZLibDecompress, MatchesTheOriginal)
{
    const Case&        test     = GetParam();
    auto               expected = OriginalData(test);

    ZLib::Decompressor decompressor(
        Pointer(const_cast<std::uint8_t*>(test.Compressed)),
        test.CompressedSize);
    ASSERT_TRUE(decompressor.Decompress(test.InitialSize));
    ASSERT_EQ(decompressor.DecompressedSize(), expected.size());
    if (!expected.empty())
    {
        EXPECT_EQ(std::memcmp(decompressor.DecompressedData(),
                              expected.data(), expected.size()),
                  0);
    }
}

INSTANTIATE_TEST_SUITE_P(Vectors, ZLibDecompress, testing::ValuesIn(Cases),
                         [](const testing::TestParamInfo<Case>& info)
                         { return std::string(info.param.Name); });

TEST(ZLibDecompress, RejectsABadHeaderCheck)
{
    // A valid header with its check bits broken: (CMF * 256 + FLG) % 31 != 0
    std::uint8_t stream[] = {0x78, 0x9d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01};
    ZLib::Decompressor decompressor(Pointer(stream), sizeof(stream));
    EXPECT_FALSE(decompressor.Decompress());
}

TEST(ZLibDecompress, RejectsANonDeflateMethod)
{
    // CM == 15 is reserved; 0x7f07 % 31 == 0, so only the method is wrong
    std::uint8_t stream[] = {0x7f, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01};
    ASSERT_EQ(((stream[0] << 8) | stream[1]) % 31, 0);
    ZLib::Decompressor decompressor(Pointer(stream), sizeof(stream));
    EXPECT_FALSE(decompressor.Decompress());
}

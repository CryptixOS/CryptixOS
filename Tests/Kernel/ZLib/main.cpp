/*
 * Created by v1tr10l7 on 06.11.2026.
 * Copyright (c) 2024-2026, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#include <Library/ZLib.hpp>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Vectors.hpp"

// Must match text() and noise() in gen-vectors.py
static void GenerateText(std::vector<std::uint8_t>& out, std::size_t size)
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
static void GenerateNoise(std::vector<std::uint8_t>& out, std::size_t size,
                          std::uint32_t seed)
{
    std::uint32_t state = seed;
    for (std::size_t i = 0; i < size; ++i)
    {
        state = state * 1103515245u + 12345u;
        out.push_back(state >> 24);
    }
}

static int RunCase(const Case& test)
{
    std::vector<std::uint8_t> expected;
    for (std::size_t i = 0; i < test.ChunkCount; ++i)
    {
        const Chunk& chunk = test.Chunks[i];
        if (chunk.Kind == 0) GenerateText(expected, chunk.Size);
        else GenerateNoise(expected, chunk.Size, chunk.Seed);
    }

    ZLib::Decompressor decompressor(
        Pointer(const_cast<std::uint8_t*>(test.Compressed)),
        test.CompressedSize);
    if (!decompressor.Decompress(test.InitialSize))
    {
        std::printf("FAIL %s: Decompress() returned false\n", test.Name);
        return 1;
    }
    if (decompressor.DecompressedSize() != expected.size())
    {
        std::printf("FAIL %s: size %zu, expected %zu\n", test.Name,
                    static_cast<std::size_t>(decompressor.DecompressedSize()),
                    expected.size());
        return 1;
    }
    if (!expected.empty()
        && std::memcmp(decompressor.DecompressedData(), expected.data(),
                       expected.size()))
    {
        std::printf("FAIL %s: contents differ\n", test.Name);
        return 1;
    }

    std::printf("PASS %s (%zu -> %zu bytes)\n", test.Name, test.CompressedSize,
                expected.size());
    return 0;
}

static int RunRejectsBadHeader()
{
    // A valid header with its check bits broken: (CMF * 256 + FLG) % 31 != 0
    std::uint8_t       stream[] = {0x78, 0x9d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01};
    ZLib::Decompressor decompressor(Pointer(stream), sizeof(stream));
    if (decompressor.Decompress())
    {
        std::printf("FAIL RejectsBadHeader: accepted a corrupt header\n");
        return 1;
    }
    std::printf("PASS RejectsBadHeader\n");
    return 0;
}

int main()
{
    int failures = 0;
    for (const Case& test : Cases) failures += RunCase(test);
    failures += RunRejectsBadHeader();

    return failures ? 1 : 0;
}

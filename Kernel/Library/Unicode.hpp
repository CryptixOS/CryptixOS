/*
 * Created by v1tr10l7 on 07.04.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Prism/Containers/Array.hpp>
#include <Prism/Core/Types.hpp>

namespace Unicode
{
    constexpr i32 ToCodePage437(u64 codePoint)
    {
        switch (codePoint)
        {
            case 0x263a: return 1;
            case 0x263b: return 2;
            case 0x2665: return 3;
            case 0x2666: return 4;
            case 0x2663: return 5;
            case 0x2660: return 6;
            case 0x2022: return 7;
            case 0x25d8: return 8;
            case 0x25cb: return 9;
            case 0x25d9: return 10;
            case 0x2642: return 11;
            case 0x2640: return 12;
            case 0x266a: return 13;
            case 0x266b: return 14;
            case 0x263c: return 15;
            case 0x25ba: return 16;
            case 0x25c4: return 17;
            case 0x2195: return 18;
            case 0x203c: return 19;
            case 0x00b6: return 20;
            case 0x00a7: return 21;
            case 0x25ac: return 22;
            case 0x21a8: return 23;
            case 0x2191: return 24;
            case 0x2193: return 25;
            case 0x2192: return 26;
            case 0x2190: return 27;
            case 0x221f: return 28;
            case 0x2194: return 29;
            case 0x25b2: return 30;
            case 0x25bc: return 31;

            case 0x2302: return 127;
            case 0x00c7: return 128;
            case 0x00fc: return 129;
            case 0x00e9: return 130;
            case 0x00e2: return 131;
            case 0x00e4: return 132;
            case 0x00e0: return 133;
            case 0x00e5: return 134;
            case 0x00e7: return 135;
            case 0x00ea: return 136;
            case 0x00eb: return 137;
            case 0x00e8: return 138;
            case 0x00ef: return 139;
            case 0x00ee: return 140;
            case 0x00ec: return 141;
            case 0x00c4: return 142;
            case 0x00c5: return 143;
            case 0x00c9: return 144;
            case 0x00e6: return 145;
            case 0x00c6: return 146;
            case 0x00f4: return 147;
            case 0x00f6: return 148;
            case 0x00f2: return 149;
            case 0x00fb: return 150;
            case 0x00f9: return 151;
            case 0x00ff: return 152;
            case 0x00d6: return 153;
            case 0x00dc: return 154;
            case 0x00a2: return 155;
            case 0x00a3: return 156;
            case 0x00a5: return 157;
            case 0x20a7: return 158;
            case 0x0192: return 159;
            case 0x00e1: return 160;
            case 0x00ed: return 161;
            case 0x00f3: return 162;
            case 0x00fa: return 163;
            case 0x00f1: return 164;
            case 0x00d1: return 165;
            case 0x00aa: return 166;
            case 0x00ba: return 167;
            case 0x00bf: return 168;
            case 0x2310: return 169;
            case 0x00ac: return 170;
            case 0x00bd: return 171;
            case 0x00bc: return 172;
            case 0x00a1: return 173;
            case 0x00ab: return 174;
            case 0x00bb: return 175;
            case 0x2591: return 176;
            case 0x2592: return 177;
            case 0x2593: return 178;
            case 0x2502: return 179;
            case 0x2524: return 180;
            case 0x2561: return 181;
            case 0x2562: return 182;
            case 0x2556: return 183;
            case 0x2555: return 184;
            case 0x2563: return 185;
            case 0x2551: return 186;
            case 0x2557: return 187;
            case 0x255d: return 188;
            case 0x255c: return 189;
            case 0x255b: return 190;
            case 0x2510: return 191;
            case 0x2514: return 192;
            case 0x2534: return 193;
            case 0x252c: return 194;
            case 0x251c: return 195;
            case 0x2500: return 196;
            case 0x253c: return 197;
            case 0x255e: return 198;
            case 0x255f: return 199;
            case 0x255a: return 200;
            case 0x2554: return 201;
            case 0x2569: return 202;
            case 0x2566: return 203;
            case 0x2560: return 204;
            case 0x2550: return 205;
            case 0x256c: return 206;
            case 0x2567: return 207;
            case 0x2568: return 208;
            case 0x2564: return 209;
            case 0x2565: return 210;
            case 0x2559: return 211;
            case 0x2558: return 212;
            case 0x2552: return 213;
            case 0x2553: return 214;
            case 0x256b: return 215;
            case 0x256a: return 216;
            case 0x2518: return 217;
            case 0x250c: return 218;
            case 0x2588: return 219;
            case 0x2584: return 220;
            case 0x258c: return 221;
            case 0x2590: return 222;
            case 0x2580: return 223;
            case 0x03b1: return 224;
            case 0x00df: return 225;
            case 0x0393: return 226;
            case 0x03c0: return 227;
            case 0x03a3: return 228;
            case 0x03c3: return 229;
            case 0x00b5: return 230;
            case 0x03c4: return 231;
            case 0x03a6: return 232;
            case 0x0398: return 233;
            case 0x03a9: return 234;
            case 0x03b4: return 235;
            case 0x221e: return 236;
            case 0x03c6: return 237;
            case 0x03b5: return 238;
            case 0x2229: return 239;
            case 0x2261: return 240;
            case 0x00b1: return 241;
            case 0x2265: return 242;
            case 0x2264: return 243;
            case 0x2320: return 244;
            case 0x2321: return 245;
            case 0x00f7: return 246;
            case 0x2248: return 247;
            case 0x00b0: return 248;
            case 0x2219: return 249;
            case 0x00b7: return 250;
            case 0x221a: return 251;
            case 0x207f: return 252;
            case 0x00b2: return 253;
            case 0x25a0: return 254;
        }

        return -1;
    }

    struct Interval
    {
        u32            First;
        u32            Last;

        constexpr bool operator<(const u32& ucs) const { return Last < ucs; }
    };

    constexpr bool IsInCombining(u32 ucs, const auto& table)
    {
        auto it = std::lower_bound(table.begin(), table.end(), ucs);
        if (it == table.end()) return false;

        return ucs >= it->First && ucs <= it->Last;
    }
    constexpr i32 GetWidth(u32 ucs)
    {
        /* sorted list of non-overlapping intervals of non-spacing characters */
        /* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B
         * c" */
        constexpr const auto combining{ToArray<Interval>(
            {{0x0300, 0x036f},   {0x0483, 0x0486},   {0x0488, 0x0489},
             {0x0591, 0x05bd},   {0x05bf, 0x05bf},   {0x05c1, 0x05c2},
             {0x05c4, 0x05c5},   {0x05c7, 0x05c7},   {0x0600, 0x0603},
             {0x0610, 0x0615},   {0x064b, 0x065e},   {0x0670, 0x0670},
             {0x06d6, 0x06e4},   {0x06e7, 0x06e8},   {0x06ea, 0x06ed},
             {0x070f, 0x070f},   {0x0711, 0x0711},   {0x0730, 0x074a},
             {0x07a6, 0x07b0},   {0x07eb, 0x07f3},   {0x0901, 0x0902},
             {0x093c, 0x093c},   {0x0941, 0x0948},   {0x094d, 0x094d},
             {0x0951, 0x0954},   {0x0962, 0x0963},   {0x0981, 0x0981},
             {0x09bc, 0x09bc},   {0x09c1, 0x09c4},   {0x09cd, 0x09cd},
             {0x09e2, 0x09e3},   {0x0a01, 0x0a02},   {0x0a3c, 0x0a3c},
             {0x0a41, 0x0a42},   {0x0a47, 0x0a48},   {0x0a4b, 0x0a4d},
             {0x0a70, 0x0a71},   {0x0a81, 0x0a82},   {0x0abc, 0x0abc},
             {0x0ac1, 0x0ac5},   {0x0ac7, 0x0ac8},   {0x0acd, 0x0acd},
             {0x0ae2, 0x0ae3},   {0x0b01, 0x0b01},   {0x0b3c, 0x0b3c},
             {0x0b3f, 0x0b3f},   {0x0b41, 0x0b43},   {0x0b4d, 0x0b4d},
             {0x0b56, 0x0b56},   {0x0b82, 0x0b82},   {0x0bc0, 0x0bc0},
             {0x0bcd, 0x0bcd},   {0x0c3e, 0x0c40},   {0x0c46, 0x0c48},
             {0x0c4a, 0x0c4d},   {0x0c55, 0x0c56},   {0x0cbc, 0x0cbc},
             {0x0cbf, 0x0cbf},   {0x0cc6, 0x0cc6},   {0x0ccc, 0x0ccd},
             {0x0ce2, 0x0ce3},   {0x0d41, 0x0d43},   {0x0d4d, 0x0d4d},
             {0x0dcA, 0x0dca},   {0x0dd2, 0x0dd4},   {0x0dd6, 0x0dd6},
             {0x0e31, 0x0e31},   {0x0e34, 0x0e3a},   {0x0e47, 0x0e4e},
             {0x0eb1, 0x0eb1},   {0x0eb4, 0x0eb9},   {0x0ebb, 0x0ebc},
             {0x0ec8, 0x0ecd},   {0x0f18, 0x0f19},   {0x0f35, 0x0f35},
             {0x0f37, 0x0f37},   {0x0f39, 0x0f39},   {0x0f71, 0x0f7e},
             {0x0f80, 0x0f84},   {0x0f86, 0x0f87},   {0x0f90, 0x0f97},
             {0x0f99, 0x0fbc},   {0x0fc6, 0x0fc6},   {0x102d, 0x1030},
             {0x1032, 0x1032},   {0x1036, 0x1037},   {0x1039, 0x1039},
             {0x1058, 0x1059},   {0x1160, 0x11ff},   {0x135f, 0x135f},
             {0x1712, 0x1714},   {0x1732, 0x1734},   {0x1752, 0x1753},
             {0x1772, 0x1773},   {0x17b4, 0x17b5},   {0x17b7, 0x17bd},
             {0x17c6, 0x17c6},   {0x17c9, 0x17d3},   {0x17dd, 0x17dd},
             {0x180b, 0x180d},   {0x18a9, 0x18a9},   {0x1920, 0x1922},
             {0x1927, 0x1928},   {0x1932, 0x1932},   {0x1939, 0x193b},
             {0x1a17, 0x1a18},   {0x1b00, 0x1b03},   {0x1b34, 0x1b34},
             {0x1b36, 0x1b3a},   {0x1b3c, 0x1b3c},   {0x1b42, 0x1b42},
             {0x1B6b, 0x1b73},   {0x1dc0, 0x1dca},   {0x1dfe, 0x1dff},
             {0x200b, 0x200f},   {0x202a, 0x202e},   {0x2060, 0x2063},
             {0x206a, 0x206f},   {0x20d0, 0x20ef},   {0x302a, 0x302f},
             {0x3099, 0x309a},   {0xa806, 0xa806},   {0xa80b, 0xa80b},
             {0xa825, 0xa826},   {0xfb1e, 0xfb1e},   {0xfe00, 0xfe0f},
             {0xfe20, 0xfe23},   {0xfeff, 0xfeff},   {0xfff9, 0xfffb},
             {0x10a01, 0x10a03}, {0x10a05, 0x10a06}, {0x10a0c, 0x10a0f},
             {0x10a38, 0x10a3a}, {0x10a3f, 0x10a3f}, {0x1d167, 0x1d169},
             {0x1d173, 0x1d182}, {0x1d185, 0x1d18b}, {0x1d1aa, 0x1d1ad},
             {0x1d242, 0x1d244}, {0xe0001, 0xe0001}, {0xe0020, 0xe007f},
             {0xe0100, 0xe01ef}})};

        /* test for 8-bit control characters */
        if (ucs == 0) return 0;
        if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0)) return 1;

        /* binary search in table of non-spacing characters */
        if (IsInCombining(ucs, combining)) return 0;

        /* if we arrive here, ucs is not a combining or C0/C1 control character
         */
        return 1
             + (ucs >= 0x1100
                && (ucs <= 0x115f || /* Hangul Jamo init. consonants */
                    ucs == 0x2329 || ucs == 0x232a
                    || (ucs >= 0x2e80 && ucs <= 0xa4cf && ucs != 0x303f)
                    ||                                  /* CJK ... Yi */
                    (ucs >= 0xac00 && ucs <= 0xd7a3) || /* Hangul Syllables */
                    (ucs >= 0xf900 && ucs <= 0xfaff)
                    || /* CJK Compatibility Ideographs */
                    (ucs >= 0xfe10 && ucs <= 0xfe19) || /* Vertical forms */
                    (ucs >= 0xfe30 && ucs <= 0xfe6f)
                    || /* CJK Compatibility Forms */
                    (ucs >= 0xff00 && ucs <= 0xff60) || /* Fullwidth Forms */
                    (ucs >= 0xffe0 && ucs <= 0xffe6)
                    || (ucs >= 0x20000 && ucs <= 0x2fffd)
                    || (ucs >= 0x30000 && ucs <= 0x3fffd)));
    }
}; // namespace Unicode

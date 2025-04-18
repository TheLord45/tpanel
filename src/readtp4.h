/*
 * Copyright (C) 2022 to 2024 by Andreas Theofilu <andreas@theosys.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef __READTP4__
#define __READTP4__

#include <string>
#include <cstdint>
#include <vector>

namespace reader
{
    typedef struct
    {
        unsigned char ch;
        short byte;
    } CHTABLE;

    static CHTABLE __cht[] =
    {
        {0x80,  0x20AC},
        {0x81,  0x0081},
        {0x82,  0x201A},
        {0x83,  0x0192},
        {0x84,  0x201E},
        {0x85,  0x2026},
        {0x86,  0x2020},
        {0x87,  0x2021},
        {0x88,  0x02C6},
        {0x89,  0x2030},
        {0x8A,  0x0160},
        {0x8B,  0x2039},
        {0x8C,  0x0152},
        {0x8D,  0x008d},
        {0x8E,  0x017D},
        {0x8F,  0x008f},
        {0x90,  0x0090},
        {0x91,  0x2018},
        {0x92,  0x2019},
        {0x93,  0x201C},
        {0x94,  0x201D},
        {0x95,  0x2022},
        {0x96,  0x2013},
        {0x97,  0x2014},
        {0x98,  0x02DC},
        {0x99,  0x2122},
        {0x9A,  0x0161},
        {0x9B,  0x203A},
        {0x9C,  0x0153},
        {0x9D,  0x009d},
        {0x9E,  0x017E},
        {0x9F,  0x0178},
        {0xA0,  0x00A0},
        {0xA1,  0x00A1},
        {0xA2,  0x00A2},
        {0xA3,  0x00A3},
        {0xA4,  0x00A4},
        {0xA5,  0x00A5},
        {0xA6,  0x00A6},
        {0xA7,  0x00A7},
        {0xA8,  0x00A8},
        {0xA9,  0x00A9},
        {0xAA,  0x00AA},
        {0xAB,  0x00AB},
        {0xAC,  0x00AC},
        {0xAD,  0x00AD},
        {0xAE,  0x00AE},
        {0xAF,  0x00AF},
        {0xB0,  0x00B0},
        {0xB1,  0x00B1},
        {0xB2,  0x00B2},
        {0xB3,  0x00B3},
        {0xB4,  0x00B4},
        {0xB5,  0x00B5},
        {0xB6,  0x00B6},
        {0xB7,  0x00B7},
        {0xB8,  0x00B8},
        {0xB9,  0x00B9},
        {0xBA,  0x00BA},
        {0xBB,  0x00BB},
        {0xBC,  0x00BC},
        {0xBD,  0x00BD},
        {0xBE,  0x00BE},
        {0xBF,  0x00BF},
        {0xC0,  0x00C0},
        {0xC1,  0x00C1},
        {0xC2,  0x00C2},
        {0xC3,  0x00C3},
        {0xC4,  0x00C4},
        {0xC5,  0x00C5},
        {0xC6,  0x00C6},
        {0xC7,  0x00C7},
        {0xC8,  0x00C8},
        {0xC9,  0x00C9},
        {0xCA,  0x00CA},
        {0xCB,  0x00CB},
        {0xCC,  0x00CC},
        {0xCD,  0x00CD},
        {0xCE,  0x00CE},
        {0xCF,  0x00CF},
        {0xD0,  0x00D0},
        {0xD1,  0x00D1},
        {0xD2,  0x00D2},
        {0xD3,  0x00D3},
        {0xD4,  0x00D4},
        {0xD5,  0x00D5},
        {0xD6,  0x00D6},
        {0xD7,  0x00D7},
        {0xD8,  0x00D8},
        {0xD9,  0x00D9},
        {0xDA,  0x00DA},
        {0xDB,  0x00DB},
        {0xDC,  0x00DC},
        {0xDD,  0x00DD},
        {0xDE,  0x00DE},
        {0xDF,  0x00DF},
        {0xE0,  0x00E0},
        {0xE1,  0x00E1},
        {0xE2,  0x00E2},
        {0xE3,  0x00E3},
        {0xE4,  0x00E4},
        {0xE5,  0x00E5},
        {0xE6,  0x00E6},
        {0xE7,  0x00E7},
        {0xE8,  0x00E8},
        {0xE9,  0x00E9},
        {0xEA,  0x00EA},
        {0xEB,  0x00EB},
        {0xEC,  0x00EC},
        {0xED,  0x00ED},
        {0xEE,  0x00EE},
        {0xEF,  0x00EF},
        {0xF0,  0x00F0},
        {0xF1,  0x00F1},
        {0xF2,  0x00F2},
        {0xF3,  0x00F3},
        {0xF4,  0x00F4},
        {0xF5,  0x00F5},
        {0xF6,  0x00F6},
        {0xF7,  0x00F7},
        {0xF8,  0x00F8},
        {0xF9,  0x00F9},
        {0xFA,  0x00FA},
        {0xFB,  0x00FB},
        {0xFC,  0x00FC},
        {0xFD,  0x00FD},
        {0xFE,  0x00FE},
        {0xFF,  0x00FF}
    };

    struct HEADER       // This is the first entry in the file.
    {
        unsigned char abyFileID[8];     // 0 - 7
        uint32_t listStartBlock;        // 8 - 11
    };

    struct BLOCK        // This is a block. It points to other blocks and contains the block data.
    {
        uint32_t thisBlock;             // 0 - 3
        uint32_t prevBlock;             // 4 - 7
        uint32_t nextBlock;             // 8 - 11
        uint16_t bytesUsed;             // 12 - 13
        unsigned char abyData[512];     // 14 - 525
    };

    struct USAGE_BLOCK  // This is an index entry
    {
        uint32_t thisBlock;             // 0 - 3
        uint32_t prevBlock;             // 4 - 7
        uint32_t nextBlock;             // 8 - 11
        uint16_t bytesUsed;             // 12 - 13
        unsigned char filePath[260];    // 14 - 273
        time_t tmCreate;                // 274 - 277
        time_t tmModify;                // 278 - 281
        uint32_t flags;                 // 282 - 285
        uint32_t startBlock;            // 286 - 289
        uint32_t sizeBlocks;            // 290 - 293
        uint32_t sizeBytes;             // 294 - 297
    };

    struct FILE_HEAD    // This is the pointer part of a block.
    {
        uint32_t thisBlock;             // 228 - 231
        uint32_t prevBlock;             // 232 - 235
        uint32_t nextBlock;             // 236 - 239
        uint16_t blockLen;              // 240 - 141
    };

    struct INDEX
    {
        struct USAGE_BLOCK *ublock {nullptr};
        struct INDEX *prev {nullptr};
        struct INDEX *next {nullptr};
    };

    struct MANIFEST
    {
        size_t size;
        time_t tmCreate;
        time_t tmModify;
        std::string fname;
    };

#define SIZE_HEADER         12
#define SIZE_BLOCK          526
#define SIZE_USAGE_BLOCK    298
#define SIZE_FILE_HEAD      14

    class ReadTP4
    {
            std::string fname{""};
            std::string target{"."};
            struct INDEX *idx {nullptr};

        public:
            explicit ReadTP4(const std::string& fn)
                : fname{fn}
            {}

            ReadTP4(const std::string& fn, const std::string& tg)
                : fname{fn},
                  target{tg}
            {}

            ~ReadTP4();

            bool isReady();
            bool doRead();
            std::string toHex(int num, int width);
            bool isTP5() { return tp5Type; }

        private:
            void fillBlock(struct BLOCK& bl, const unsigned char *buf);
            void fillUsageBlock(struct USAGE_BLOCK& ub, const unsigned char *buf);
            void fillFileHead(struct FILE_HEAD& fh, const unsigned char *buf);
            size_t calcBlockPos(uint32_t block);

            struct INDEX *appendUBlock(const struct USAGE_BLOCK *ub);
            void deleteIndex();

            uint32_t makeDWord(const unsigned char *buf);
            uint16_t makeWord(const unsigned char *buf);
            std::string cp1250ToUTF8(const std::string& str);
            static bool compareManifest(struct MANIFEST& m1, struct MANIFEST& m2);

            std::vector<struct MANIFEST> manifest;
            bool tp5Type{false};
    };
}

#endif

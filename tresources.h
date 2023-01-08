/*
 * Copyright (C) 2020, 2021 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TRESOURCES_H__
#define __TRESOURCES_H__

#include <deque>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <locale>
#include <codecvt>
#include <cstdint>
#include <clocale>

#ifdef __MACH__
#include <skia/core/SkImage.h>
#include <skia/core/SkString.h>
#include <skia/core/SkData.h>
#else
#include <include/core/SkImage.h>
#include <include/core/SkString.h>
#include <include/core/SkData.h>
#endif

#include "terror.h"

class SkBitmap;
class SkData;
class SkStreamAsset;
class SkFont;
class SkTypeface;

typedef enum _RESOURCE_TYPE
{
    RESTYPE_UNKNOWN,
    RESTYPE_IMAGE,
    RESTYPE_SYSIMAGE,
    RESTYPE_FONT,
    RESTYPE_SYSFONT,
    RESTYPE_CURSOR,
    RESTYPE_SYSCURSOR,
    RESTYPE_BORDER,
    RESTYPE_SYSBORDER,
    RESTYPE_SLIDER,
    RESTYPE_SYSSLIDER
}_RESOURCE_TYPE;

SkString GetResourcePath(const char* resource = "", _RESOURCE_TYPE rs = RESTYPE_IMAGE);

bool DecodeDataToBitmap(sk_sp<SkData> data, SkBitmap* dst);

sk_sp<SkData> GetResourceAsData(const char* resource, _RESOURCE_TYPE rs = RESTYPE_IMAGE);

inline bool GetResourceAsBitmap(const char* resource, SkBitmap* dst)
{
    return DecodeDataToBitmap(GetResourceAsData(resource), dst);
}

inline sk_sp<SkImage> GetResourceAsImage(const char* resource)
{
    return SkImage::MakeFromEncoded(GetResourceAsData(resource));
}

std::unique_ptr<SkStreamAsset> GetResourceAsStream(const char* resource, _RESOURCE_TYPE rs = RESTYPE_IMAGE);

sk_sp<SkTypeface> MakeResourceAsTypeface(const char* resource, int ttcIndex = 0, _RESOURCE_TYPE rs = RESTYPE_FONT);

sk_sp<SkData> readImage(const std::string& fname);
SkBitmap *allocPixels(int width, int height, SkBitmap *bm);
SkColor reverseColor(const SkColor& col);

std::string toLower(std::string& str);
std::string toUpper(std::string& str);
std::vector<std::string> StrSplit(const std::string& str, const std::string& seps, const bool trimEmpty=false);
std::string UTF8ToCp1250(const std::string& str);
std::string cp1250ToUTF8(const std::string& str);
std::string latin1ToUTF8(const std::string& str);

void *renew(char **mem, size_t old_size, size_t new_size);
std::vector<std::string> splitLine(const std::string& str);
std::vector<std::string> splitLine(const std::string& str, int width, int height, SkFont& font, SkPaint& paint);
bool isHex(int c);
int strCaseCompare(const std::string& str1, const std::string& str2);
std::string fillString(int c, int len);
bool isUTF8(const std::string& str);
size_t utf8Strlen(const std::string& str);
uint16_t getUint16(const unsigned char *p, bool big_endian=false);
uint32_t getUint32(const unsigned char *p, bool big_endian=false);
bool endsWith (const std::string &src, const std::string &end);
bool startsWith (const std::string &src, const std::string &start);
std::string dirName (const std::string &path);
std::string baseName (const std::string &path);
char *strnstr(const char *haystack, const char *needle, size_t len);
std::string getCommand(const std::string& fullCmd);
bool StrContains(const std::string& str, const std::string& part);
bool isTrue(const std::string& value);
bool isFalse(const std::string& value);
bool isNumeric(const std::string& str, bool blank=false);
bool isBigEndian();

static inline std::string &ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c);}));
    return s;
}

static inline std::string &rtrim(std::string &str)
{
    str.erase(str.find_last_not_of(" \n\r\t")+1);
    return str;
}

static inline std::string& trim(std::string& str) { return ltrim(rtrim(str)); }

#define ASSERT_MSG(cond, msg) { if (!(cond)) throw std::runtime_error("Assertion (" #cond ") failed at line " + std::to_string(__LINE__) + "! Msg: " + std::string(msg)); }
#define ASSERT(cond) ASSERT_MSG(cond, "")

class CharConvert
{
    public:
        template <typename U8StrT = std::string>
        inline static U8StrT Utf32To8(std::u32string const & s)
        {
            static_assert(sizeof(typename U8StrT::value_type) == 1, "Char byte-size should be 1 for UTF-8 strings!");
            typedef typename U8StrT::value_type VT;
            typedef uint8_t u8;
            U8StrT r;

            for (auto c: s)
            {
                size_t nby = c <= 0x7FU ? 1 : c <= 0x7FFU ? 2 : c <= 0xFFFFU ? 3 : c <= 0x1FFFFFU ? 4 : c <= 0x3FFFFFFU ? 5 : c <= 0x7FFFFFFFU ? 6 : 7;
                r.push_back(VT(
                    nby <= 1 ? u8(c) : (
                        (u8(0xFFU) << (8 - nby)) |
                        u8(c >> (6 * (nby - 1)))
                    )
                ));

                for (size_t i = 1; i < nby; ++i)
                    r.push_back(VT(u8(0x80U | (u8(0x3FU) & u8(c >> (6 * (nby - 1 - i)))))));
            }

            return r;
        }

        template <typename U8StrT>
        inline static std::u32string Utf8To32(U8StrT const & s)
        {
            static_assert(sizeof(typename U8StrT::value_type) == 1, "Char byte-size should be 1 for UTF-8 strings!");
            typedef uint8_t u8;
            std::u32string r;
            auto it = (u8 const *)s.c_str(), end = (u8 const *)(s.c_str() + s.length());

            while (it < end)
            {
                char32_t c = 0;
                if (*it <= 0x7FU)
                {
                    c = *it;
                    ++it;
                }
                else
                {
                    ASSERT((*it & 0xC0U) == 0xC0U);
                    size_t nby = 0;

                    for (u8 b = *it; (b & 0x80U) != 0; b <<= 1, ++nby) { (void)0; }
                    ASSERT(nby <= 7);
                    ASSERT((size_t)(end - it) >= nby);
                    c = *it & (u8(0xFFU) >> (nby + 1));

                    for (size_t i = 1; i < nby; ++i)
                    {
                        ASSERT((it[i] & 0xC0U) == 0x80U);
                        c = (c << 6) | (it[i] & 0x3FU);
                    }

                    it += nby;
                }

                r.push_back(c);
            }

            return r;
        }


        template <typename U16StrT = std::u16string>
        inline static U16StrT Utf32To16(std::u32string const & s)
        {
            static_assert(sizeof(typename U16StrT::value_type) == 2, "Char byte-size should be 2 for UTF-16 strings!");
            typedef typename U16StrT::value_type VT;
            typedef uint16_t u16;
            U16StrT r;

            for (auto c: s)
            {
                if (c <= 0xFFFFU)
                    r.push_back(VT(c));
                else
                {
                    ASSERT(c <= 0x10FFFFU);
                    c -= 0x10000U;
                    r.push_back(VT(u16(0xD800U | ((c >> 10) & 0x3FFU))));
                    r.push_back(VT(u16(0xDC00U | (c & 0x3FFU))));
                }
            }

            return r;
        }

        template <typename U16StrT>
        inline static std::u32string Utf16To32(U16StrT const & s)
        {
            static_assert(sizeof(typename U16StrT::value_type) == 2, "Char byte-size should be 2 for UTF-16 strings!");
            typedef uint16_t u16;
            std::u32string r;
            auto it = (u16 const *)s.c_str(), end = (u16 const *)(s.c_str() + s.length());

            while (it < end)
            {
                char32_t c = 0;

                if (*it < 0xD800U || *it > 0xDFFFU)
                {
                    c = *it;
                    ++it;
                }
                else if (*it >= 0xDC00U)
                {
                    ASSERT_MSG(false, "Unallowed UTF-16 sequence!");
                }
                else
                {
                    ASSERT(end - it >= 2);
                    c = (*it & 0x3FFU) << 10;

                    if ((it[1] < 0xDC00U) || (it[1] > 0xDFFFU))
                    {
                        ASSERT_MSG(false, "Unallowed UTF-16 sequence!");
                    }
                    else
                    {
                        c |= it[1] & 0x3FFU;
                        c += 0x10000U;
                    }

                    it += 2;
                }

                r.push_back(c);
            }

            return r;
        }

        template <typename StrT, size_t NumBytes = sizeof(typename StrT::value_type)> struct UtfHelper;
        template <typename StrT> struct UtfHelper<StrT, 1>
        {
            inline static std::u32string UtfTo32(StrT const & s) { return Utf8To32(s); }
            inline static StrT UtfFrom32(std::u32string const & s) { return Utf32To8<StrT>(s); }
        };

        template <typename StrT> struct UtfHelper<StrT, 2>
        {
            inline static std::u32string UtfTo32(StrT const & s) { return Utf16To32(s); }
            inline static StrT UtfFrom32(std::u32string const & s) { return Utf32To16<StrT>(s); }
        };

        template <typename StrT> struct UtfHelper<StrT, 4>
        {
            inline static std::u32string UtfTo32(StrT const & s)
            {
                return std::u32string((char32_t const *)(s.c_str()), (char32_t const *)(s.c_str() + s.length()));
            }

            inline static StrT UtfFrom32(std::u32string const & s)
            {
                return StrT((typename StrT::value_type const *)(s.c_str()),
                            (typename StrT::value_type const *)(s.c_str() + s.length()));
            }
        };

        template <typename StrT> inline static std::u32string UtfTo32(StrT const & s)
        {
            return UtfHelper<StrT>::UtfTo32(s);
        }

        template <typename StrT> inline static StrT UtfFrom32(std::u32string const & s)
        {
            return UtfHelper<StrT>::UtfFrom32(s);
        }

        template <typename StrToT, typename StrFromT> inline static StrToT UtfConv(StrFromT const & s)
        {
            return UtfFrom32<StrToT>(UtfTo32(s));
        }

    private:
        CharConvert() {}    // Never call this!
};

#endif  // __TRESOURCES_H__

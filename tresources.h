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

#include <include/core/SkImage.h>
#include <include/core/SkString.h>
#include <include/core/SkData.h>

class SkBitmap;
class SkData;
class SkStreamAsset;
class SkFont;
class SkTypeface;

extern sk_sp<SkData> (*gResourceFactory)(const char*);

SkString GetResourcePath(const char* resource = "");

bool DecodeDataToBitmap(sk_sp<SkData> data, SkBitmap* dst);

sk_sp<SkData> GetResourceAsData(const char* resource);

inline bool GetResourceAsBitmap(const char* resource, SkBitmap* dst)
{
    return DecodeDataToBitmap(GetResourceAsData(resource), dst);
}

inline sk_sp<SkImage> GetResourceAsImage(const char* resource)
{
    return SkImage::MakeFromEncoded(GetResourceAsData(resource));
}

std::unique_ptr<SkStreamAsset> GetResourceAsStream(const char* resource);

sk_sp<SkTypeface> MakeResourceAsTypeface(const char* resource, int ttcIndex = 0);

sk_sp<SkData> readImage(const std::string& fname);

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

#endif  // __TRESOURCES_H__

/*
 * Copyright (C) 2020 to 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include <include/core/SkBitmap.h>
#include <include/core/SkData.h>
#include <include/core/SkImage.h>
#include <include/core/SkImageGenerator.h>
#include <include/core/SkStream.h>
#include <include/core/SkFont.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkColorSpace.h>

#include <iconv.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "tresources.h"
#include "tpagemanager.h"
#include "terror.h"
#include "tconfig.h"

#if __cplusplus < 201402L
#   error "This module requires at least C++14 standard!"
#else
#   if __cplusplus < 201703L
#       include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#       warning "Support for C++14 and experimental filesystem will be removed in a future version!"
#   else
#       include <filesystem>
#       ifdef __ANDROID__
namespace fs = std::__fs::filesystem;
#       else
namespace fs = std::filesystem;
#       endif
#   endif
#endif

using std::string;
using std::endl;
using std::vector;

extern TPageManager *gPageManager;

typedef struct
{
    unsigned char ch;
    unsigned int byte;
}CHTABLE;

static CHTABLE __cht[] = {
    {0x80,	0xE282AC},
    {0x81,	0x0081},    // not used
    {0x82,	0xE2809A},
    {0x83,	0x0192},    // not used
    {0x84,	0xE2809E},
    {0x85,	0xE280A6},
    {0x86,	0xE280A0},
    {0x87,	0xE280A1},
    {0x88,	0x02C6},    // not used
    {0x89,	0xE280B0},
    {0x8A,	0xC5A0},
    {0x8B,	0xE280B9},
    {0x8C,	0xC59A},
    {0x8D,	0xC5A4},
    {0x8E,	0xC5BD},
    {0x8F,	0xC5B9},
    {0x90,	0x0090},    // not used
    {0x91,	0xE28098},
    {0x92,	0xE28099},
    {0x93,	0xE2809C},
    {0x94,	0xE2809D},
    {0x95,	0xE280A2},
    {0x96,	0xE28093},
    {0x97,	0xE28094},
    {0x98,	0x02DC},    // not used
    {0x99,	0xE284A2},
    {0x9A,	0xC5A1},
    {0x9B,	0xE280BA},
    {0x9C,	0xC59B},
    {0x9D,	0xC5A5},
    {0x9E,	0xC5BE},
    {0x9F,	0xC5BA},
    {0xA0,	0xC2A0},
    {0xA1,	0xCB87},
    {0xA2,	0xCB98},
    {0xA3,	0xC581},
    {0xA4,	0xC2A0},
    {0xA5,	0xC484},
    {0xA6,	0xC2A6},
    {0xA7,	0xC2A7},
    {0xA8,	0xC2A8},
    {0xA9,	0xC2A9},
    {0xAA,	0xC59E},
    {0xAB,	0xC2AB},
    {0xAC,	0xC2AC},
    {0xAD,	0xC2AD},
    {0xAE,	0xC2AE},
    {0xAF,	0xC5BB},
    {0xB0,	0xC2B0},
    {0xB1,	0xC2B1},
    {0xB2,	0xCB9B},
    {0xB3,	0xC582},
    {0xB4,	0xC2B4},
    {0xB5,	0xC2B5},
    {0xB6,	0xC2B6},
    {0xB7,	0xC2B7},
    {0xB8,	0xC2B8},
    {0xB9,	0xC485},
    {0xBA,	0xC59F},
    {0xBB,	0xC2BB},
    {0xBC,	0xC4BD},
    {0xBD,	0xCB9D},
    {0xBE,	0xC4BE},
    {0xBF,	0xC5BC},
    {0xC0,	0xC594},
    {0xC1,	0xC381},
    {0xC2,	0xC382},
    {0xC3,	0xC482},
    {0xC4,	0xC384},
    {0xC5,	0xC4B9},
    {0xC6,	0xC486},
    {0xC7,	0xC387},
    {0xC8,	0xC48C},
    {0xC9,	0xC389},
    {0xCA,	0xC489},
    {0xCB,	0xC38b},
    {0xCC,	0xC49A},
    {0xCD,	0xC38D},
    {0xCE,	0xC38E},
    {0xCF,	0xC48E},
    {0xD0,	0xC490},
    {0xD1,	0xC583},
    {0xD2,	0xC587},
    {0xD3,	0xC398},
    {0xD4,	0xC394},
    {0xD5,	0xC590},
    {0xD6,	0xC396},
    {0xD7,	0xC397},
    {0xD8,	0xC598},
    {0xD9,	0xC5AE},
    {0xDA,	0xC39A},
    {0xDB,	0xC5B0},
    {0xDC,	0xC39C},
    {0xDD,	0xC39D},
    {0xDE,	0xC5A2},
    {0xDF,	0xC39F},
    {0xE0,	0xC595},
    {0xE1,	0xC3A1},
    {0xE2,	0xC3A2},
    {0xE3,	0xC483},
    {0xE4,	0xC3A4},
    {0xE5,	0xC4BA},
    {0xE6,	0xC487},
    {0xE7,	0xC3A7},
    {0xE8,	0xC48D},
    {0xE9,	0xC3A9},
    {0xEA,	0xC499},
    {0xEB,	0xC3AB},
    {0xEC,	0xC49B},
    {0xED,	0xC3AD},
    {0xEE,	0xC3AE},
    {0xEF,	0xC48F},
    {0xF0,	0xC491},
    {0xF1,	0xC584},
    {0xF2,	0xC588},
    {0xF3,	0xC3B3},
    {0xF4,	0xC3B4},
    {0xF5,	0xC591},
    {0xF6,	0xC3B6},
    {0xF7,	0xC3B7},
    {0xF8,	0xC599},
    {0xF9,	0xC5AF},
    {0xFA,	0xC3BA},
    {0xFB,	0xC5B1},
    {0xFC,	0xC3BC},
    {0xFD,	0xC3BD},
    {0xFE,	0xC5A3},
    {0xFF,	0xCB99}
};

SkString GetResourcePath(const char* resource, _RESOURCE_TYPE rs)
{
    if (!resource)
        return SkString();

    if (*resource == '/')       // absolute path?
    {                           // yes, then take it as it is
        return SkString(resource);
    }

    string pth;

    switch(rs)
    {
        case RESTYPE_BORDER:    pth = "/borders/"; break;
        case RESTYPE_CURSOR:    pth = "/cursors/"; break;
        case RESTYPE_FONT:      pth = "/fonts/"; break;
        case RESTYPE_UNKNOWN:
        case RESTYPE_IMAGE:     pth = "/images/"; break;
        case RESTYPE_SLIDER:    pth = "/sliders/"; break;
        case RESTYPE_SYSBORDER: pth = "/__system/graphics/borders/"; break;
        case RESTYPE_SYSCURSOR: pth = "/__system/graphics/cursors/"; break;
        case RESTYPE_SYSFONT:   pth = "/__system/graphics/fonts/"; break;
        case RESTYPE_SYSIMAGE:  pth = "/__system/graphics/images/"; break;
        case RESTYPE_SYSSLIDER: pth = "/__system/graphics/sliders/"; break;
    }

    string projectPath = ((gPageManager && gPageManager->isSetupActive()) ? TConfig::getSystemProjectPath() : TConfig::getProjectPath());
    string path = projectPath + pth + resource;
    return SkString(path);
}

bool DecodeDataToBitmap(sk_sp<SkData> data, SkBitmap* dst)
{
    if (!data || !dst)
        return false;

    std::unique_ptr<SkImageGenerator> gen(SkImageGenerator::MakeFromEncoded(std::move(data)));
    return gen && dst->tryAllocPixels(gen->getInfo()) &&
            gen->getPixels(gen->getInfo().makeColorSpace(nullptr), dst->getPixels(), dst->rowBytes());
}

std::unique_ptr<SkStreamAsset> GetResourceAsStream(const char* resource, _RESOURCE_TYPE rs)
{
    sk_sp<SkData> data = GetResourceAsData(resource, rs);
    return data ? std::unique_ptr<SkStreamAsset>(new SkMemoryStream(std::move(data)))
    : nullptr;
}

sk_sp<SkData> GetResourceAsData(const char* resource, _RESOURCE_TYPE rs)
{
    SkString str = GetResourcePath(resource, rs);

    sk_sp<SkData> data = SkData::MakeFromFileName(str.c_str());

    if (data)
        return data;

    MSG_ERROR("GetResourceAsData: Resource \"" << str.c_str() << "\" not found.");
    TError::setError();
#ifdef SK_TOOLS_REQUIRE_RESOURCES
    SK_ABORT("GetResourceAsData: missing resource");
#endif
    return nullptr;
}

sk_sp<SkTypeface> MakeResourceAsTypeface(const char* resource, int ttcIndex, _RESOURCE_TYPE rs)
{
    return SkTypeface::MakeFromStream(GetResourceAsStream(resource, rs), ttcIndex);
}

/*
 * Read the image from a file and save it into a data buffer. This is the base
 * to convert the image.
 */
sk_sp<SkData> readImage(const string& fname)
{
    sk_sp<SkData> data = GetResourceAsData(fname.c_str());

    if (!data)
    {
        MSG_ERROR("readImage: Error loading the image " << fname);
        TError::setError();
    }

    return data;
}

SkBitmap *allocPixels(int width, int height, SkBitmap *bm)
{
    DECL_TRACER("TButton::allocPixels(int width, int height, SkBitmap *bm)");

    if (!bm)
        return nullptr;

    // Skia reads image files in the natural byte order of the CPU.
    // While on Intel CPUs the byte order is little endian it is
    // mostly big endian on other CPUs. This means that the order of
    // the colors is RGB on big endian CPUs (ARM, ...) and BGR on others.
    // To compensate this, we check the endianess of the CPU and set
    // the byte order according.
    SkImageInfo info;

    if (isBigEndian())
        info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    else
        info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);

    if (!bm->tryAllocPixels(info))
    {
        MSG_ERROR("Error allocating " << (width * height) << " pixels!");
        return nullptr;
    }

    return bm;
}

SkColor reverseColor(const SkColor& col)
{
    DECL_TRACER("reverseColor(const SkColor& col)");

    int red = SkColorGetR(col);
    int green = SkColorGetG(col);
    int blue = SkColorGetB(col);
    int alpha = SkColorGetA(col);

    return SkColorSetARGB(alpha, blue, green, red);
}

vector<string> StrSplit(const string& str, const string& seps, const bool trimEmpty)
{
    size_t pos = 0, mark = 0;
    vector<string> parts;
    string::const_iterator it, sepIt;

    if (str.empty())
        return parts;

    for (it = str.begin(); it != str.end(); ++it)
    {
        for (sepIt = seps.begin(); sepIt != seps.end(); ++sepIt)
        {
            if (pos > 0 && *it == *sepIt)
            {
                size_t len = pos - mark;

                if (len > 0 && *sepIt != '\n')
                    parts.push_back(str.substr(mark, len));
                else if (len > 0)
                    parts.push_back(str.substr(mark, len) + "\n");
                else if (*sepIt == '\n')
                    parts.push_back("\n");
                else
                    parts.push_back(string());

                mark = pos + 1;
                break;
            }
            else if (*it == *sepIt)
            {
                if (*sepIt == '\n')
                    parts.push_back("\n");

                mark = pos + 1;
            }
        }

        pos++;
    }

    parts.push_back(str.substr(mark));

    if (trimEmpty)
    {
        vector<string> nparts;

        for (auto iter = parts.begin(); iter != parts.end(); ++iter)
        {
            if (iter->empty())
                continue;

            nparts.push_back(*iter);
        }

        return nparts;
    }

    return parts;
}

string latin1ToUTF8(const string& str)
{
    DECL_TRACER("NameFormat::latin1ToUTF8(const string& str)");
    string out;

    for (size_t i = 0; i < str.length(); i++)
    {
        uint8_t ch = str.at(i);

        if (ch < 0x80)
        {
            out.push_back(ch);
        }
        else
        {
            out.push_back(0xc0 | ch >> 6);
            out.push_back(0x80 | (ch & 0x3f));
        }
    }

    return out;
}

string cp1250ToUTF8(const string& str)
{
    DECL_TRACER("cp1250ToUTF8(const string& str)");

    string out;

    for (size_t j = 0; j < str.length(); j++)
    {
        int i = -1;
        unsigned char ch = str.at(j);
        unsigned int utf = 0x80000000;

        if (ch >= 0x80)
        {
            do
            {
                i++;

                if (__cht[i].ch == ch)
                {
                    utf = __cht[i].byte;
                    break;
                }
            }
            while (__cht[i].ch != 0xff);

            if (utf == 0x80000000)
                utf = ch;
        }
        else
            utf = ch;

        if (utf > 0x00ffff)
        {
            out.push_back((utf >> 16) & 0x0000ff);
            out.push_back((utf >> 8) & 0x0000ff);
            out.push_back(utf & 0x0000ff);
        }
        else if (utf > 0x0000ff)
        {
            out.push_back((utf >> 8) & 0x0000ff);
            out.push_back(utf & 0x0000ff);
        }
        else if (ch > 0x7f)
        {
            out.push_back(0xc0 | ch >> 6);
            out.push_back(0x80 | (ch & 0x3f));
        }
        else
            out.push_back(ch);
    }

    return out;
}

string UTF8ToCp1250(const string& str)
{
    DECL_TRACER("UTF8ToCp1250(const string& str)");
#ifdef __ANDROID__
    string out;
    string::const_iterator iter;
    bool three = false;

    for (iter = str.begin(); iter != str.end(); ++iter)
    {
        unsigned int uch;

        if ((*iter & 0xc0) == 0xc0)     // If UTF8 then we need the next char also
        {
            uch = 0;

            if ((*iter & 0xe0) == 0xe0) // UTF8 consists of 3 bytes?
            {
                uch = (*iter << 16) & 0x00ff0000;
                ++iter;
                three = true;
            }

            uch |= ((*iter << 8) & 0x0000ff00);
            ++iter;
            uch |= (*iter & 0x000000ff);
        }
        else
            uch = *iter;

        if (three || uch > 0x00ff)
        {
            int i = 0;
            bool found = false;

            while(three && __cht[i].ch != 0xff)
            {
                if (__cht[i].byte == uch)
                {
                    out.push_back(__cht[i].ch);
                    found = true;
                    break;
                }

                i++;
            }

            three = false;

            if (!found)
            {
                unsigned ch = ((uch & 0x0300) >> 2) | (uch & 0x003f);
                out.push_back(ch);
            }
        }
        else
            out.push_back(uch);
    }

    return out;
#else
    char dst[1024];
    size_t srclen = 0;
    char* pIn, *pInSave;

    srclen = str.length();
    memset(&dst[0], 0, sizeof(dst));

    try
    {
        pIn = new char[srclen + 1];
        memcpy(pIn, str.c_str(), srclen);
        *(pIn+srclen) = 0;
        pInSave = pIn;
    }
    catch(std::exception& e)
    {
        MSG_ERROR("Error: " << e.what());
        return "";
    }

    size_t dstlen = sizeof(dst) - 1;
    char* pOut = (char *)dst;

    iconv_t conv = iconv_open("UTF-8", "CP1250");

    if (conv == (iconv_t)-1)
    {
        MSG_ERROR("Error opening iconv: " << strerror(errno));
        delete[] pInSave;
        return str;
    }

    size_t ret = iconv(conv, &pIn, &srclen, &pOut, &dstlen);
    iconv_close(conv);
    delete[] pInSave;

    if (ret == (size_t)-1)
    {
        MSG_ERROR("Error converting a string!");
        return str;
    }

    return string(dst);
#endif
}

std::string intToString(int num)
{
    std::stringstream ss;
    ss << num;
    return ss.str();
}

void *renew(char **mem, size_t old_size, size_t new_size)
{
    if (old_size == new_size)
        return *mem;

    if (!mem || !*mem)
        return nullptr;

    try
    {
        char *memory = new char[new_size];
        size_t len = (new_size < old_size) ? new_size : old_size;
        memcpy(memory, *mem, len);
        delete[] *mem;
        *mem = memory;
        return memory;
    }
    catch(std::exception& e)
    {
        MSG_ERROR(e.what());
        throw;
    }

    return nullptr;
}

string toUpper(string& str)
{
    string::iterator iter;

    for (iter = str.begin(); iter != str.end(); iter++)
        *iter = std::toupper(*iter);

    return str;
}

string toLower(string& str)
{
    string::iterator iter;

    for (iter = str.begin(); iter != str.end(); iter++)
        *iter = std::tolower(*iter);

    return str;
}

vector<string> splitLine(const string& str)
{
    DECL_TRACER("splitLine(const string& str)");

    vector<string> lines;
    string sl;
    string::const_iterator iter;

    if (str.empty())
        return lines;

    for (iter = str.begin(); iter != str.end(); iter++)
    {
        if (*iter == '\r')  // ignore bloating byte coming from brain death windows
            continue;

        if (*iter == '\n')
        {
            lines.push_back(sl);
            sl.clear();
            continue;
        }

        char ch[2];
        ch[0] = *iter;
        ch[1] = 0;
        sl.append(ch);
    }

    if (!sl.empty())
        lines.push_back(sl);

    return lines;
}

vector<string> splitLine(const string& str, int width, int height, SkFont& font, SkPaint& paint)
{
    DECL_TRACER("splitLine(const string& str, int width, int height, SkFont& font, SkPaint& paint)");

    SkRect rect;
    vector<string> lines, words;
    SkScalar lnHeight = font.getSize();
    int maxLines = (int)((SkScalar)height / lnHeight);
    string part, oldPart;

    if (str.empty())
        return lines;

    words = StrSplit(str, " \n");
    MSG_DEBUG("Found " << words.size() << " words.");
    vector<string>::iterator iter;

    if (words.size() == 0)
        return lines;

    for (iter = words.begin(); iter != words.end(); ++iter)
    {
        size_t pos;
        bool lineBreak = false;

        if ((pos = iter->find("\n")) != string::npos)
        {
            if (pos > 0)
                *iter = iter->substr(0, pos);
            else
                *iter = "";

            lineBreak = true;
        }

        if (part.empty())
            part += *iter;
        else
            part += " " + *iter;

        font.measureText(part.c_str(), part.length(), SkTextEncoding::kUTF8, &rect, &paint);

        if (rect.width() > (width - 8))
        {
            if (oldPart.empty())
            {
                string sample;
                size_t len = part.length();
                size_t pos = 1, start = 0;

                for (size_t i = 0; i < len; i++)
                {
                    sample = part.substr(start, pos);
                    font.measureText(sample.c_str(), sample.length(), SkTextEncoding::kUTF8, &rect, &paint);

                    if (rect.width() > (width - 8))
                    {
                        lines.push_back(sample.substr(0, sample.length() - 1)); // Cut off the last character because it is already out of bounds.
                        start = i;                                              // Set the new start of the string
                        i--;                                                    // We must repeat the last character
                        pos = 0;                                                // Reset the position counter
                        sample = sample.substr(sample.length() - 1);            // Put the last character into the part.

                        if (lines.size() >= (size_t)maxLines)                   // Break if we've reached the maximum of lines
                            return lines;
                    }

                    pos++;
                }

                oldPart.clear();
                part = sample;
                continue;
            }
            else
            {
                lines.push_back(oldPart);
                oldPart.clear();
                part = *iter;

                if (lines.size() >= (size_t)maxLines)               // Break if we've reached the maximum of lines
                    return lines;

                font.measureText(part.c_str(), part.length(), SkTextEncoding::kUTF8, &rect, &paint);

                if (rect.width() > (width - 8))
                    continue;
            }
        }
        else if (lineBreak)
        {
            lines.push_back(part);
            part.clear();
        }

        oldPart = part;
    }

    if (lines.empty())
        lines.push_back(str);
    else if (!part.empty())
        lines.push_back(part);

    return lines;
}

bool isHex(int c)
{
    if ((c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F'))
        return true;

    return false;
}

int strCaseCompare(const string& str1, const string& str2)
{
    return strcasecmp(str1.c_str(), str2.c_str());
}

std::string fillString(int c, int len)
{
    if (len < 1 || c < 1 || c > 0x00ffffff)
        return string();

    string str;
    char ch[4];

    if (c <= 0x00ff)
    {
        ch[0] = c;
        ch[1] = 0;
    }
    else if (c <= 0x00ffff)
    {
        ch[0] = c >> 8;
        ch[1] = c;
        ch[2] = 0;
    }
    else
    {
        ch[0] = c >> 16;
        ch[1] = c >> 8;
        ch[2] = c;
        ch[3] = 0;
    }

    for (int i = 0; i < len; i++)
        str.append(ch);

    return str;
}

bool isUTF8(const string& str)
{
    int c,i,ix,n,j;

    for (i=0, ix = str.length(); i < ix; i++)
    {
        c = (unsigned char) str[i];

        if (0x00 <= c && c <= 0x7f)
            n=0; // 0bbbbbbb
        else if ((c & 0xE0) == 0xC0)
            n=1; // 110bbbbb
        else if ( c==0xed && i<(ix-1) && ((unsigned char)str[i+1] & 0xa0)==0xa0)
            return false; //U+d800 to U+dfff
        else if ((c & 0xF0) == 0xE0)
            n=2; // 1110bbbb
        else if ((c & 0xF8) == 0xF0)
            n=3; // 11110bbb
        else
            return false;

        for (j=0; j<n && i<ix; j++) // n bytes matching 10bbbbbb follow ?
        {
            if ((++i == ix) || (( (unsigned char)str[i] & 0xC0) != 0x80))
                return false;
        }
    }

    return true;
}

size_t utf8Strlen(const std::string& str)
{
    int c,i,ix;
    size_t q;

    for (q = 0, i = 0, ix = str.length(); i < ix; i++, q++)
    {
        c = (unsigned char)str[i];

        if (c >= 0 && c <= 127)
            i += 0;
        else if ((c & 0xE0) == 0xC0)
            i += 1;
        else if ((c & 0xF0) == 0xE0)
            i += 2;
        else if ((c & 0xF8) == 0xF0)
            i += 3;
        else
            return 0;   //invalid utf8
    }

    return q;
}

uint16_t getUint16(const unsigned char *p, bool big_endian)
{
    if (!p)
        return 0;

    uint16_t num;
    memmove(&num, p, sizeof(uint16_t));

    if (!big_endian)
    {
        uint16_t le_num = ((num >> 8) & 0x00ff) | ((num << 8) & 0xff00);
        num = le_num;
    }

    return num;
}

uint32_t getUint32(const unsigned char *p, bool big_endian)
{
    if (!p)
        return 0;

    uint32_t num;
    memmove(&num, p, sizeof(uint32_t));

    if (!big_endian)
    {
        uint32_t le_num = ((num >> 24) & 0x000000ff) | ((num >> 8) & 0x0000ff00) | ((num << 8) & 0x00ff0000) | ((num << 24) & 0xff000000);
        num = le_num;
    }

    return num;
}

bool endsWith (const std::string &src, const std::string &end)
{
    vector<string> list;

    if (end.find("|") == string::npos)
        list.push_back(end);
    else
        list = StrSplit(end, "|", false);

    vector<string>::iterator iter;

    for (iter = list.begin(); iter != list.end(); ++iter)
    {
        size_t len = iter->length();

        if (len > src.length())
            continue;

        string part = src.substr(src.length() - len);

        if (part.compare(*iter) == 0)
            return true;
    }

    return false;
}

bool startsWith (const std::string &src, const std::string &start)
{
    vector<string> list;

    if (start.find("|") == string::npos)
        list.push_back(start);
    else
        list = StrSplit(start, "|", false);

    vector<string>::iterator iter;

    for (iter = list.begin(); iter != list.end(); ++iter)
    {
        size_t len = iter->length();

        if (len > src.length())
            continue;

        string part = src.substr(0, len);

        if (part.compare(*iter) == 0)
            return true;
    }

    return false;
}

std::string dirName (const std::string &path)
{
    if (path.empty())
        return ".";

    size_t len = path.length();

    char *buffer = new char[len+1];
    memset(buffer, 0, len+1);
    strncpy(buffer, path.c_str(), len);

    char *dir = dirname(buffer);
    string ret(dir);
    delete[] buffer;
    return ret;
}

std::string baseName (const std::string &path)
{
    if (path.empty())
        return ".";

    size_t len = path.length();

    char *buffer = new char[len+1];
    memset(buffer, 0, len+1);
    strncpy(buffer, path.c_str(), len);

    char *dir = basename(buffer);
    string ret(dir);
    delete[] buffer;
    return ret;
}

/**
 * @brief strnstr - Find the needle in a haystack
 * The function searches for a string in a larger string. In case the wanted
 * string is found it returns a pointer to the start of the string in \b haystack.
 *
 * @param haystack  The string who may contain \b needle.
 * @param needle    The string to search for in \b haystack.
 * @param len       The length of the \b haystack.
 *
 * @return If the \b needle was found in \b haystack a pointer to the start of
 * the string in \b haystack is returned. Otherwise a NULL pointer is returned.
 */
char *strnstr(const char* haystack, const char* needle, size_t len)
{
    if (!haystack || !needle)
        return nullptr;

    size_t needleLen = strlen(needle);

    if (needleLen > len)
        return nullptr;

    char *start = (char *)haystack;
    size_t pos = 0;

    while ((pos + needleLen) < len)
    {
        bool match = true;

        for (size_t i = 0; i < needleLen; i++)
        {
            if (*(start + i) != *(needle + i))
            {
                match = false;
                break;
            }
        }

        if (match)
            return start;

        pos++;
        start++;
    }

    return nullptr;
}

bool StrContains(const std::string& str, const std::string& part)
{
    return str.find(part) != string::npos;
}

std::string ReplaceString(const std::string subject, const std::string& search, const std::string& replace)
{
    size_t pos = 0;
    string sub = subject;

    while ((pos = sub.find(search, pos)) != std::string::npos)
    {
        sub.replace(pos, search.length(), replace);
        pos += replace.length();
    }

    return sub;
}

string getCommand(const string& fullCmd)
{
    DECL_TRACER("getCommand(const string& fullCmd)");

    size_t pos = fullCmd.find_first_of("-");
    string cmd;

    if (pos != string::npos)
    {
        cmd = fullCmd.substr(0, pos);
        cmd = toUpper(cmd);
    }
    else
    {
        cmd = fullCmd;
        cmd = toUpper(cmd);
    }

    return cmd;
}

bool isTrue(const std::string &value)
{
    std::string v = value;
    std::string low = toLower(v);

    if (low.find("true") != std::string::npos ||
        low.find("on") != std::string::npos ||
        low.find("yes") != std::string::npos ||
        low.find("1") != std::string::npos)
        return true;

    return false;
}

bool isFalse(const std::string &value)
{
    std::string v = value;
    std::string low = toLower(v);

    if (low.find("false") != std::string::npos ||
        low.find("off") != std::string::npos ||
        low.find("no") != std::string::npos ||
        low.find("0") != std::string::npos)
        return true;

    return false;
}

bool isNumeric(const std::string &str, bool blank)
{
    std::string::const_iterator iter;

    for (iter = str.begin(); iter != str.end(); ++iter)
    {
        if (*iter < '0' || *iter > '9')
        {
            if (blank && *iter == ' ')
                continue;

            return false;
        }
    }

    return true;
}

bool isBigEndian()
{
    union
    {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1;
}

std::string handleToString(ulong handle)
{
    ulong part1 = (handle >> 16) & 0x0000ffff;
    ulong part2 = handle & 0x0000ffff;
    return std::to_string(part1)+":"+std::to_string(part2);
}

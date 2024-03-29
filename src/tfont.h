/*
 * Copyright (C) 2020 to 2024 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TFONT_H__
#define __TFONT_H__

#include <string>
#include <map>

#include "tvalidatefile.h"

#include <include/core/SkTypeface.h>
#include <include/core/SkFontStyle.h>

typedef struct FONT_T
{
    int number{0};
    std::string file;
    int fileSize{0};
    int faceIndex{0};
    std::string name;
    std::string subfamilyName;
    std::string fullName;
    int size{0};
    int usageCount{0};
}FONT_T;

typedef enum FONT_STYLE
{
    FONT_NONE,
    FONT_NORMAL,
    FONT_ITALIC,
    FONT_BOLD,
    FONT_BOLD_ITALIC
}FONT_STYLE;

typedef enum FONT_TYPE
{
    FT_UNKNOWN,     // Unknown font type
    FT_NORMAL,      // Normal font with mostly letters
    FT_SYMBOL,      // Normal font with mostly symbols
    FT_SYM_MS       // Proprietary Microsoft symbol font
}FONT_TYPE;

class TFont : public TValidateFile
{
    public:
        TFont(const std::string& fname, bool tp=false);
        ~TFont();

        void initialize();
        bool systemFonts(bool setup=false);
        FONT_T getFont(int number);
        int getFontIDfromFile(const std::string& file);
        int getFontIDfromName(const std::string& name);
        FONT_STYLE getStyle(int number);
        FONT_STYLE getStyle(FONT_T& font);
        SkFontStyle getSkiaStyle(int number);
        sk_sp<SkTypeface> getTypeFace(int number);
        sk_sp<SkTypeface> getTypeFace(const std::string& ff);
        void setTP5(bool tp) { mIsTP5 = tp; }

        static std::vector<std::string> getFontPathList();
        static SkGlyphID *textToGlyphs(const std::string& str, sk_sp<SkTypeface>& typeFace, size_t *size);
        static FONT_TYPE isSymbol(sk_sp<SkTypeface>& typeFace);
        static size_t utf8ToUtf16(const std::string& str, uint16_t **uni, bool toSymbol = false);
        static double pixelToPoint(int dpi, int pixel);

    private:
        static void parseCmap(const unsigned char *cmaps);
        static uint16_t getGlyphIndex(SkUnichar);
        static void _freeCmap();

        std::map<int, FONT_T> mFonts;
        bool mIsTP5{false};
        std::string mFontFile;
};

#endif

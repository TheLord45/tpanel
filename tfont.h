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

#ifndef __TFONT_H__
#define __TFONT_H__

#include <string>
#include <map>

#include "tvalidatefile.h"

#include <include/core/SkTypeface.h>
#include <include/core/SkFontStyle.h>

typedef struct FONT_T
{
    int number;
    std::string file;
    int fileSize;
    int faceIndex;
    std::string name;
    std::string subfamilyName;
    std::string fullName;
    int size;
    int usageCount;
}FONT_T;

typedef enum FONT_STYLE
{
    FONT_NONE,
    FONT_NORMAL,
    FONT_ITALIC,
    FONT_BOLD,
    FONT_BOLD_ITALIC
}FONT_STYLE;

class TFont : public TValidateFile
{
    public:
        TFont();
        ~TFont();

        void initialize();
        bool systemFonts();
        FONT_T getFont(int number);
        FONT_STYLE getStyle(int number);
        FONT_STYLE getStyle(FONT_T& font);
        SkFontStyle getSkiaStyle(int number);
        sk_sp<SkTypeface> getTypeFace(int number);

        static std::vector<std::string> getFontPathList();
        static SkGlyphID *textToGlyphs(const std::string& str, sk_sp<SkTypeface>& typeFace, size_t *size);
        static bool isSymbol(sk_sp<SkTypeface>& typeFace);
        static size_t utf8ToUtf16(const std::string& str, uint16_t **uni, bool toSymbol = false);

    private:
        static void parseCmap(const unsigned char *cmaps);
        static uint16_t getGlyphIndex(SkUnichar);
        static void _freeCmap();

        std::map<int, FONT_T> mFonts;
};

#endif

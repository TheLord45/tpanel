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

#ifndef __TCOLOR_H__
#define __TCOLOR_H__

#include <string>
#include <include/core/SkColor.h>
#include "tpalette.h"

#ifdef __ANDROID__
typedef unsigned long int ulong;
#endif

class TColor
{
    public:
        typedef struct COLOR_T
        {
            int red{0};
            int green{0};
            int blue{0};
            int alpha{0};
        }COLOR_T;

        typedef enum DIRECTION_t
        {
            DIR_LIGHT_DARK = 1,
            DIR_DARK_LIGHT,
            DIR_LIGHT_DARK_LIGHT,
            DIR_DARK_LIGHT_DARK
        }DIRECTION_t;

        TColor() {};

        static COLOR_T getAMXColor(const std::string& color);
        static void setPalette(TPalette *pal) { mPalette = pal; }
        static COLOR_T splitColors(PDATA_T& pd);
        static SkColor getSkiaColor(const std::string& color);
        static ulong getColor(const std::string& color);
        static std::string colorToString(ulong color);
        static std::string colorToString(SkColor color);
        static std::vector<SkColor> colorRange(SkColor col, int width, int bandwidth, DIRECTION_t dir);
        static bool isValidAMXcolor(const std::string& color);

        static int getRed(ulong color) { return ((color >> 24) & 0x000000ff); }
        static SkColor getRed(SkColor color) { return SkColorGetR(color); }
        static int getGreen(ulong color) { return ((color >> 16) & 0x000000ff); }
        static SkColor getGreen(SkColor color) { return SkColorGetG(color); }
        static int getBlue(ulong color) { return ((color >> 8) & 0x000000ff); }
        static SkColor getBlue(SkColor color) { return SkColorGetB(color); }
        static int getAlpha(ulong color) { return (color & 0x000000ff); }
        static SkColor getAlpha(SkColor color) { return SkColorGetA(color); }

        static int setAlpha(ulong color, int alpha);
        static SkColor setAlpha(SkColor color, int alpha);
        static int calcAlpha(int alpha1, int alpha2);
        static ulong setAlphaTreshold(ulong color, int alpha);
        static SkColor setAlphaTreshold(SkColor color, int alpha);

    private:
        static TPalette *mPalette;
};

#endif

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

#include <iostream>
#include <iomanip>

#include "tcolor.h"
#include "terror.h"
#include "tresources.h"

TPalette *TColor::mPalette = nullptr;

using std::string;
using std::vector;
using std::setw;
using std::setfill;
using std::hex;
using std::dec;
using std::resetiosflags;

TColor::COLOR_T TColor::getAMXColor(const string& color)
{
    DECL_TRACER("TColor::getAMXColor(const string& color)");

    if (color.empty())
    {
        MSG_WARNING("Empty color name is ignored!");
        return TColor::COLOR_T();
    }

    // Check if we've an index number
    if (color.length() <= 3 && isdigit(color[0]))
    {
        int idx = atoi(color.c_str());

        if (idx >= 0 && idx <= 255)
        {
            PDATA_T pd = mPalette->findColor(idx);

            if (!pd.name.empty())
                return splitColors(pd);
        }
    }

    size_t pos = color.find('#');

    if (pos == string::npos)    // No # found?
    {                           // No #, then try to find color in palette
        if (!mPalette)
        {
            MSG_ERROR("No palette was set! First set a palette to be able to get any color!");
            TError::SetError();
            return TColor::COLOR_T();
        }

        PDATA_T pd = mPalette->findColor(color);

        if (!pd.name.empty())
            return splitColors(pd);

        // We fill in the default transparent color
        TColor::COLOR_T col;
        col.red = 0;
        col.green = 0;
        col.blue = 0;
        col.alpha = 0;
        return col;
    }

    // There was a # in the string. We assume that the color is encoded in hex
    // after the # symbol.
    string col = color.substr(pos + 1);

    if (col.length() < 6)
    {
        MSG_WARNING("Invalid color " << col << " found! Ignoring it.");
        return TColor::COLOR_T();
    }

    TColor::COLOR_T ct;
    ct.red = (int)strtol(col.substr(0, 2).c_str(), NULL, 16);
    ct.green = (int)strtol(col.substr(2, 2).c_str(), NULL, 16);
    ct.blue = (int)strtol(col.substr(4, 2).c_str(), NULL, 16);

    if (col.length() > 6)
        ct.alpha = (int)strtol(col.substr(6).c_str(), NULL, 16);
    else
        ct.alpha = 0x00ff;

    return ct;
}

TColor::COLOR_T TColor::splitColors(PDATA_T& pd)
{
    DECL_TRACER("TColor::splitColors(PDATA_T& pd)");

    TColor::COLOR_T ct;
    ct.red = (pd.color & 0xff000000) >> 24;
    ct.green = (pd.color & 0x00ff0000) >> 16;
    ct.blue = (pd.color & 0x0000ff00) >> 8;
    ct.alpha = (pd.color & 0x000000ff);
    return ct;
}

SkColor TColor::getSkiaColor(const std::string& color)
{
    DECL_TRACER("TColor::getSkiaColor(const std::string& color)");

    COLOR_T col = getAMXColor(color);

    if (col.alpha == 0)
        return SK_ColorTRANSPARENT;

    if (isBigEndian())
        return SkColorSetARGB(col.alpha, col.blue, col.green, col.red);
    else
        return SkColorSetARGB(col.alpha, col.red, col.green, col.blue);
}

ulong TColor::getColor(const std::string& color)
{
    DECL_TRACER("TColor::getColor(const std::string& color)");

    TColor::COLOR_T ct = getAMXColor(color);
    ulong col = ((ct.red << 24) & 0xff000000) | ((ct.green << 16) & 0x00ff0000) | ((ct.blue << 8) & 0x0000ff00) | (ct.alpha & 0x000000ff);
    return col;
}

std::string TColor::colorToString(ulong color)
{
    PDATA_T pd;
    pd.color = color;
    TColor::COLOR_T col = splitColors(pd);
    std::stringstream s;
    s << std::setw(2) << std::setfill('0') << std::hex << col.red << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << col.green << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << col.blue << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << col.alpha;
    std::string ret = s.str();
    return ret;
}

std::string TColor::colorToString(SkColor color)
{
    std::stringstream s;
    s << std::setw(2) << std::setfill('0') << std::hex << SkColorGetR(color) << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << SkColorGetG(color) << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << SkColorGetB(color) << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << SkColorGetA(color);
    std::string ret = s.str();
    return ret;
}

std::string TColor::colorToString(COLOR_T color)
{
    std::stringstream s;
    s << std::setw(2) << std::setfill('0') << std::hex << color.red << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << color.green << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << color.red << ":";
    s << std::setw(2) << std::setfill('0') << std::hex << color.alpha;
    std::string ret = s.str();
    return ret;
}

std::vector<SkColor> TColor::colorRange(SkColor col, int width, int bandwidth, DIRECTION_t dir)
{
    DECL_TRACER("TColor::colorRange(SkColor col, int width, int bandwidth)");

    int red, green, blue, alpha, colStep;
    vector<SkColor> colRange;
    SkColor color;

    alpha = SkColorGetA(col);
    // Define the start color
    if (dir == DIR_LIGHT_DARK || dir == DIR_LIGHT_DARK_LIGHT)
    {
        red = std::max((int)SkColorGetR(col) - bandwidth, 0);
        green = std::max((int)SkColorGetG(col) - bandwidth, 0);
        blue = std::max((int)SkColorGetB(col) - bandwidth, 0);

        if (dir == DIR_LIGHT_DARK)
            colStep = bandwidth / width;
        else
            colStep = bandwidth / (width / 2);
    }
    else
    {
        red = std::min((int)SkColorGetR(col) + bandwidth, 255);
        green = std::min((int)SkColorGetG(col) + bandwidth, 255);
        blue = std::min((int)SkColorGetB(col) + bandwidth, 255);

        if (dir == DIR_DARK_LIGHT)
            colStep = bandwidth / width;
        else
            colStep = bandwidth / (width / 2);
    }

    if (colStep <= 1)
    {
        colRange.push_back(col);
        return colRange;
    }

    switch(dir)
    {
        case DIR_LIGHT_DARK:
            for (int i = 0; i < width; i++)
            {
                red -= colStep * i;
                green -= colStep * i;
                blue -= colStep * i;

                if (red   < 0) red   = 0; else if (red   > 255) red   = 255;
                if (green < 0) green = 0; else if (green > 255) green = 255;
                if (blue  < 0) blue  = 0; else if (blue  > 255) blue  = 255;

                color = SkColorSetARGB(alpha, red, green, blue);
                colRange.push_back(color);
            }
        break;

        case DIR_DARK_LIGHT:
            for (int i = 0; i < width; i++)
            {
                red += colStep * i;
                green += colStep * i;
                blue += colStep * i;

                if (red   < 0) red   = 0; else if (red   > 255) red   = 255;
                if (green < 0) green = 0; else if (green > 255) green = 255;
                if (blue  < 0) blue  = 0; else if (blue  > 255) blue  = 255;

                color = SkColorSetARGB(alpha, red, green, blue);
                colRange.push_back(color);
            }
        break;

        case DIR_LIGHT_DARK_LIGHT:
            for (int i = 0; i < (width / 2); i++)
            {
                red -= colStep * i;
                green -= colStep * i;
                blue -= colStep * i;

                if (red   < 0) red   = 0; else if (red   > 255) red   = 255;
                if (green < 0) green = 0; else if (green > 255) green = 255;
                if (blue  < 0) blue  = 0; else if (blue  > 255) blue  = 255;

                color = SkColorSetARGB(alpha, red, green, blue);
                colRange.push_back(color);
            }

            for (int i = 0; i < (width / 2); i++)
            {
                red += colStep * i;
                green += colStep * i;
                blue += colStep * i;

                if (red   < 0) red   = 0; else if (red   > 255) red   = 255;
                if (green < 0) green = 0; else if (green > 255) green = 255;
                if (blue  < 0) blue  = 0; else if (blue  > 255) blue  = 255;

                color = SkColorSetARGB(alpha, red, green, blue);
                colRange.push_back(color);
            }
        break;

        case DIR_DARK_LIGHT_DARK:
            for (int i = 0; i < (width / 2); i++)
            {
                red += colStep * i;
                green += colStep * i;
                blue += colStep * i;

                if (red   < 0) red   = 0; else if (red   > 255) red   = 255;
                if (green < 0) green = 0; else if (green > 255) green = 255;
                if (blue  < 0) blue  = 0; else if (blue  > 255) blue  = 255;

                color = SkColorSetARGB(alpha, red, green, blue);
                colRange.push_back(color);
            }

            for (int i = 0; i < (width / 2); i++)
            {
                red -= colStep * i;
                green -= colStep * i;
                blue -= colStep * i;

                if (red   < 0) red   = 0; else if (red   > 255) red   = 255;
                if (green < 0) green = 0; else if (green > 255) green = 255;
                if (blue  < 0) blue  = 0; else if (blue  > 255) blue  = 255;

                color = SkColorSetARGB(alpha, red, green, blue);
                colRange.push_back(color);
            }
        break;
    }

    return colRange;
}

bool TColor::isValidAMXcolor(const string& color)
{
    DECL_TRACER("TColor::isValidAMXcolor(const string& color)");

    if (color.empty())
        return false;

    size_t pos = color.find('#');

    if (pos == string::npos)
    {
        if (!mPalette)
        {
            MSG_ERROR("No palette was set! First set a palette to be able to get any color!");
            return false;
        }

        PDATA_T pd = mPalette->findColor(color);

        if (!pd.name.empty())
            return true;

        return false;
    }

    // Check if we've an index number
    if (color.length() <= 2 && isdigit(color[0]))
    {
        int idx = atoi(color.c_str());

        if (idx < 0 || idx > 88)
            return false;

        return true;
    }

    if (color.length() >= 7)    // #RRGGBBAA
        return true;

    return false;
}

int TColor::setAlpha(ulong color, int alpha)
{
    DECL_TRACER("TColor::setAlpha(ulong color, int alpha)");

    if (alpha > 0x00ff)
        alpha = 0x00ff;
    else if (alpha < 0)
        alpha = 0;

    return ((color & 0xffffff00) | (alpha & 0x00ff));
}

SkColor TColor::setAlpha(SkColor color, int alpha)
{
    DECL_TRACER("TColor::setAlpha(SkColor color, int alpha)");

    if (alpha > 0x00ff)
        alpha = 0x00ff;
    else if (alpha < 0)
        alpha = 0;

    SkColor red = SkColorGetR(color);
    SkColor green = SkColorGetG(color);
    SkColor blue = SkColorGetB(color);
    return SkColorSetARGB(alpha, red, green, blue);
}

int TColor::calcAlpha(int alpha1, int alpha2)
{
    DECL_TRACER("TColor::calcAlpha(int alpha1, int alpha2)");

    return (alpha1 + alpha2) / 2;
}

ulong TColor::setAlphaTreshold(ulong color, int alpha)
{
    DECL_TRACER("TColor::setAlphaTreshold(ulong color, int alpha)");

    uint al = color & 0x000000ff;

    if (al > (uint)alpha)
        return (color & 0xffffff00) | (alpha & 0x00ff);

    return color;
}

SkColor TColor::setAlphaTreshold(SkColor color, int alpha)
{
    DECL_TRACER("TColor::setAlphaTreshold(SkColor color, int alpha)");

    int al = SkColorGetA(color);

    if (al > alpha)
        return SkColorSetA(color, alpha);

    return color;
}

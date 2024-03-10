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

#include "tpalette.h"
#include "terror.h"
#include "tconfig.h"
#include "tresources.h"
#include "texpat++.h"

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
using std::vector;
using std::map;
using std::pair;
using namespace Expat;

TPalette::TPalette(bool tp)
    : mIsTP5(tp)
{
    DECL_TRACER("TPalette::TPalette(bool tp)");

    mPath = TConfig::getProjectPath();
}

TPalette::TPalette(const std::string& file, bool tp)
    : mIsTP5(tp)
{
    DECL_TRACER("TPalette::TPalette(const std::string& file, bool tp)");

    mPath = TConfig::getProjectPath();
    initialize(file);
}

TPalette::~TPalette()
{
    DECL_TRACER("TPalette::~TPalette()");
}

void TPalette::initialize(const std::string& file)
{
    DECL_TRACER("TPalette::initialize(const std::string& file)");

    if (!fs::exists(mPath + "/prj.xma"))
        mPath += "/__system";

    makeFileName(mPath, file);
    string path;

    if (isValidFile())
        path = getFileName();

    TExpat xml(path);

    if (!mIsTP5)
        xml.setEncoding(ENC_CP1250);

    if (!xml.parse(false))
        return;

    int depth = 0;
    size_t index = 0;

    if ((index = xml.getElementIndex("paletteData", &depth)) == TExpat::npos)
    {
        MSG_ERROR("Element \"paletteData\" was not found!");
        TError::setError();
        return;
    }

    vector<ATTRIBUTE_t> attrs = xml.getAttributes();
    string palName = xml.getAttribute("name", attrs);

    if (havePalette(palName))
        return;

    string name, content;
    PDATA_T pal;

    while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
    {
        if (name.compare("color") != 0)
        {
            pal.clear();
            continue;
        }

        pal.index = xml.getAttributeInt("index", attrs);
        pal.name = xml.getAttribute("name", attrs);
        string color = content;

        if (color.at(0) == '#')     // Do we have a valid color value?
        {
            string sCol = "0x" + color.substr(1);
            pal.color = strtoul(sCol.c_str(), 0, 16);

            if (color.length() <= 7)
                pal.color = (pal.color << 8) | 0x000000ff;
        }

        if (pal.name.length() > 0)
        {
            if (mColors.find(pal.name) != mColors.end())    // Don't insert color if it's already in list
            {
                MSG_TRACE("Ignoring color " << pal.name << " because it was read before!");
                pal.clear();
                continue;
            }

            // Insert color into list and get next child if there is one.
            mColors.insert(pair<string, PDATA_T>(pal.name, pal));
        }

        pal.clear();
    }

    if (mColors.empty())
        addSystemColors();

    mPaletteNames.push_back(palName);
}

void TPalette::reset()
{
    DECL_TRACER("TPalette::reset()");

    mColors.clear();
    mPaletteNames.clear();
}

PDATA_T TPalette::findColor(const std::string& name)
{
    DECL_TRACER("TPalette::findColor(const std::string& name)");

    if (mColors.empty())
    {
        MSG_WARNING("Have no colors in internal table!");
        return PDATA_T();
    }

    map<string, PDATA_T>::iterator iter;

    if ((iter = mColors.find(name)) == mColors.end())
        return PDATA_T();

    return iter->second;
}

PDATA_T TPalette::findColor(int pID)
{
    DECL_TRACER("TPalette::findColor(int pID)");

    map<string, PDATA_T>::iterator iter;

    for (iter = mColors.begin(); iter != mColors.end(); ++iter)
    {
        if (iter->second.index == pID)
            return iter->second;
    }

    return PDATA_T();
}

bool TPalette::havePalette(const std::string& name)
{
    DECL_TRACER("TPalette::havePalette(const std::string& name)");

    vector<string>::iterator iter;

    for (iter = mPaletteNames.begin(); iter != mPaletteNames.end(); iter++)
    {
        if (iter->compare(name) == 0)
            return true;
    }

    return false;
}

void TPalette::addSystemColors()
{
    DECL_TRACER("TPalette::addSystemColors()");

    vector<PDATA_T>::iterator iter;
    vector<PDATA_T> palArr = {
        {  0, "VeryLightRed",    0xff0000ff },
        {  1, "LightRed",        0xdf0000ff },
        {  2, "Red",             0xbf0000ff },
        {  3, "MediumRed",       0x9f0000ff },
        {  4, "DarkRed",         0x7f0000ff },
        {  5, "VeryDarkRed",     0x5f0000ff },
        {  6, "VeryLightOrange", 0xff8000ff },
        {  7, "LightOrange",     0xdf7000ff },
        {  8, "Orange",          0xbf6000ff },
        {  9, "MediumOrange",    0x9f5000ff },
        { 10, "DarkOrange",      0x7f4000ff },
        { 11, "VeryDarkOrange",  0x5f3000ff },
        { 12, "VeryLightYellow", 0xffff00ff },
        { 13, "LightYellow",     0xdfdf00ff },
        { 14, "Yellow",          0xbfbf00ff },
        { 15, "MediumYellow",    0x9f9f00ff },
        { 16, "DarkYellow",      0x7f7f00ff },
        { 17, "VeryDarkYellow",  0x5f5f00ff },
        { 18, "VeryLightLime",   0x80ff00ff },
        { 19, "LightLime",       0x70df00ff },
        { 20, "Lime",            0x60bf00ff },
        { 21, "MediumLime",      0x509f00ff },
        { 22, "DarkLime",        0x407f00ff },
        { 23, "VeryDarkLime",    0x304f00ff },
        { 24, "VeryLightGreen",  0x00ff00ff },
        { 25, "LightGreen",      0x00df00ff },
        { 26, "Green",           0x00bf00ff },
        { 27, "MediumGreen",     0x009f00ff },
        { 28, "DarkGreen",       0x007f00ff },
        { 29, "VeryDarkGreen",   0x005f00ff },
        { 30, "VeryLightMint",   0x00ff80ff },
        { 31, "LightMint",       0x00df70ff },
        { 32, "Mint",            0x00bf60ff },
        { 33, "MediumMint",      0x009f50ff },
        { 34, "DarkMint",        0x007f40ff },
        { 35, "VeryDarkMint",    0x005f10ff },
        { 36, "VeryLightCyan",   0x00ffffff },
        { 37, "LightCyan",       0x00dfdfff },
        { 38, "Cyan",            0x00bfbfff },
        { 39, "MediumCyan",      0x009f9fff },
        { 40, "DarkCyan",        0x007f7fff },
        { 41, "VeryDarkCyan",    0x005f5fff },
        { 42, "VeryLightAqua",   0x0080ffff },
        { 43, "LightAqua",       0x0070dfff },
        { 44, "Aqua",            0x0060bfff },
        { 45, "MediumAqua",      0x00509fff },
        { 46, "DarkAqua",        0x00407fff },
        { 47, "VeryDarkAqua",    0x00305fff },
        { 48, "VeryLightBlue",   0x0000ffff },
        { 49, "LightBlue",       0x0000dfff },
        { 50, "Blue",            0x0000bfff },
        { 51, "MediumBlue",      0x00009fff },
        { 52, "DarkBlue",        0x00007fff },
        { 53, "VeryDarkBlue",    0x00005fff },
        { 54, "VeryLightPurple", 0x8000ffff },
        { 55, "LightPurple",     0x7000dfff },
        { 56, "Purple",          0x6000bfff },
        { 57, "MediumPurple",    0x50009fff },
        { 58, "DarkPurple",      0x40007fff },
        { 59, "VeryDarkPurple",  0x30005fff },
        { 60, "VeryLightMagenta",0xff00ffff },
        { 61, "LightMagenta",    0xdf00dfff },
        { 62, "Magenta",         0xbf00bfff },
        { 63, "MediumMagenta",   0x9f009fff },
        { 64, "DarkMagenta",     0x7f007fff },
        { 65, "VeryDarkMagenta", 0x5f005fff },
        { 66, "VeryLightPink",   0xff0080ff },
        { 67, "LightPink",       0xdf0070ff },
        { 68, "Pink",            0xbf0060ff },
        { 69, "MediumPink",      0x9f0050ff },
        { 70, "DarkPink",        0x7f0040ff },
        { 71, "VeryDarkPink",    0x5f0030ff },
        { 72, "White",           0xffffffff },
        { 73, "Grey1",           0xeeeeeeff },
        { 74, "Grey3",           0xccccccff },
        { 75, "Grey5",           0xaaaaaaff },
        { 76, "Grey7",           0x888888ff },
        { 77, "Grey9",           0x666666ff },
        { 78, "Grey4",           0xbbbbbbff },
        { 79, "Grey6",           0x999999ff },
        { 80, "Grey8",           0x777777ff },
        { 81, "Grey10",          0x555555ff },
        { 82, "Grey12",          0x333333ff },
        { 83, "Grey13",          0x222222ff },
        { 84, "Grey2",           0xddddddff },
        { 85, "Grey11",          0x444444ff },
        { 86, "Grey14",          0x111111ff },
        { 87, "Black",           0x000000ff },
        { 255, "Transparent",    0x63356300 }
    };

    for (iter = palArr.begin(); iter != palArr.end(); ++iter)
        mColors.insert(pair<string, PDATA_T>(iter->name, *iter));
}

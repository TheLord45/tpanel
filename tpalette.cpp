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
#include "texpat++.h"

using std::string;
using std::vector;
using std::map;
using std::pair;
using namespace Expat;

TPalette::TPalette()
{
    DECL_TRACER("TPalette::TPalette()");
}

TPalette::TPalette(const std::string& file)
{
    DECL_TRACER("TPalette::TPalette(const std::string& file)");
    initialize(file);
}

TPalette::~TPalette()
{
    DECL_TRACER("TPalette::~TPalette()");
}

void TPalette::initialize(const std::string& file)
{
    DECL_TRACER("TPalette::initialize(const std::string& file)");

    makeFileName(TConfig::getProjectPath(), file);
    string path;

    if (isValidFile())
        path = getFileName();

    TExpat xml(path);
    xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
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
    index++;

    while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
    {
        PDATA_T pal;

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
        }

        if (mColors.find(pal.name) != mColors.end())    // Don't insert color if it's already in list
        {
            MSG_TRACE("Ignoring color " << pal.name << " because it was read before!");
            pal.clear();
            continue;
        }

        // Insert color into list and get next child if there is one.
        mColors.insert(pair<string, PDATA_T>(pal.name, pal));
        pal.clear();
    }
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

    map<string, PDATA_T>::iterator iter;

    if ((iter = mColors.find(name)) == mColors.end())
        return PDATA_T();

    return iter->second;
}

PDATA_T TPalette::findColor(int pID)
{
    DECL_TRACER("TPalette::findColor(int pID)");

    map<string, PDATA_T>::iterator iter;

    for (iter = mColors.begin(); iter != mColors.end(); iter++)
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

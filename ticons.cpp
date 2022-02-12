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

#include "ticons.h"
#include "terror.h"
#include "texpat++.h"
#include "tconfig.h"

using std::string;
using std::vector;
using std::map;
using std::pair;
using namespace Expat;

TIcons::TIcons()
{
    DECL_TRACER("TIcons::TIcons()");

    initialize();
}

TIcons::~TIcons()
{
    DECL_TRACER("TIcons::~TIcons()");
}

void TIcons::initialize()
{
    DECL_TRACER("TIcons::initialize()");

    if (mIcons.size() > 0)
        mIcons.clear();

    string path = makeFileName(TConfig::getProjectPath(), "icon.xma");

    if (!isValidFile())
    {
        MSG_ERROR("File " << path << " doesn't exist or is not readable!");
        TError::setError();
        return;
    }

    TExpat xml(path);
    xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return;

    int depth = 0;
    size_t index = 0;

    if ((index = xml.getElementIndex("iconList", &depth)) == TExpat::npos)
    {
        MSG_DEBUG("File does not contain the element \"iconList\"!");
        TError::setError();
        return;
    }

    depth++;
    string name, content;
    vector<ATTRIBUTE_t> attrs;

    while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
    {
        if (name.compare("maxIcons") == 0)
            mEntries = xml.convertElementToInt(content);
        else if (name.compare("icon") == 0)
        {
            int number = xml.getAttributeInt("number", attrs);
            index = xml.getNextElementFromIndex(index, &name, &content, nullptr);

            if (index != TExpat::npos)
            {
                if (name.compare("file") == 0)
                {
                    string file = content;

                    if (number > 0 && !file.empty())
                        mIcons.insert(pair<int, string>(number, file));
                }
            }

            index++;
        }
    }
}

string TIcons::getFile(int number)
{
    DECL_TRACER("TIcons::getFile(int number)");

    if (mIcons.size() == 0)
    {
        MSG_WARNING("No icons in cache!");
        return string();
    }

    map<int, string>::iterator iter = mIcons.find(number);

    if (iter == mIcons.end())
        return string();

    return iter->second;
}

int TIcons::getNumber(const string& file)
{
    DECL_TRACER("TIcons::getNumber(const string& file)");

    if (mIcons.size() == 0)
    {
        MSG_WARNING("No icons in cache!");
        return -1;
    }

    map<int, string>::iterator iter;

    for (iter = mIcons.begin(); iter != mIcons.end(); iter++)
    {
        if (iter->second.compare(file) == 0)
            return iter->first;
    }

    return -1;
}

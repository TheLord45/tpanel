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

#include <string>
#include "tpagelist.h"
#include "texpat++.h"
#include "tconfig.h"
#include "terror.h"

using std::string;
using std::vector;
using namespace Expat;

TPageList::TPageList()
{
    DECL_TRACER("TPageList::TPageList()");
    mProject = makeFileName(TConfig::getProjectPath(), "prj.xma");
    initialize();
    // Add the virtual progress page
    PAGELIST_T pl;
    pl.name = "_progress";
    pl.pageID = 300;
    mPageList.push_back(pl);
}

TPageList::~TPageList()
{
    DECL_TRACER("TPageList::~TPageList()");
}

void TPageList::initialize()
{
    DECL_TRACER("TPageList::initialize()");

    TError::clear();

    if (mPageList.size() > 0)
        mPageList.clear();

    if (mSubPageList.size() > 0)
        mSubPageList.clear();

    if (mProject.empty() || !isValidFile(mProject))
    {
        TError::setErrorMsg("Empty or invalid project file! <" + mProject + ">");
        MSG_ERROR(TError::getErrorMsg());
        return;
    }

    TExpat xml(mProject);
    xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return;

    int depth = 0;
    size_t index = 0;

    if ((index = xml.getElementIndex("pageList", &depth)) == TExpat::npos)
    {
        MSG_ERROR("Couldn't find the section \"pageList\" in file!");
        TError::setError();
        return;
    }

    do
    {
        vector<ATTRIBUTE_t> attrs = xml.getAttributes();
        string attribute = xml.getAttribute("type", attrs);

        if (attribute.empty())
        {
            TError::setErrorMsg("Missing element \"pageList\" in file " + mProject);
            MSG_ERROR(TError::getErrorMsg());
            return;
        }
        else if (attribute.compare("page") != 0 && attribute.compare("subpage") != 0)
        {
            TError::setErrorMsg("Invalid page type " + attribute + " found!");
            MSG_ERROR(TError::getErrorMsg());
            return;
        }

        while ((index = xml.getNextElementIndex("pageEntry", depth+1)) != TExpat::npos)
        {
            PAGELIST_T pl;
            SUBPAGELIST_T spl;

            pl.clear();
            spl.clear();
            string e, content;

            while ((index = xml.getNextElementFromIndex(index, &e, &content, nullptr)) != TExpat::npos)
            {
                if (attribute.compare("page") == 0)
                {
                    if (e.compare("name") == 0)
                        pl.name = content;
                    else if (e.compare("pageID") == 0)
                        pl.pageID = xml.convertElementToInt(content);
                    else if (e.compare("file") == 0)
                        pl.file = content;
                    else if (e.compare("isValid") == 0)
                        pl.isValid = xml.convertElementToInt(content);
                }
                else if (attribute.compare("subpage") == 0)
                {
                    if (e.compare("name") == 0)
                        spl.name = content;
                    else if (e.compare("pageID") == 0)
                        spl.pageID = xml.convertElementToInt(content);
                    else if (e.compare("file") == 0)
                        spl.file = content;
                    else if (e.compare("group") == 0)
                        spl.group = content;
                    else if (e.compare("isValid") == 0)
                        spl.isValid = xml.convertElementToInt(content);
                    else if (e.compare("popupType") == 0)
                        spl.popupType = xml.convertElementToInt(content);
                }
            }

            if (attribute.compare("page") == 0)
            {
                MSG_TRACE("Added page " << pl.pageID << ", " << pl.name << " to page list.");
                mPageList.push_back(pl);
            }
            else if (attribute.compare("subpage") == 0)
            {
                MSG_TRACE("Added subpage " << spl.pageID << ", " << spl.name << " to subpage list.");
                mSubPageList.push_back(spl);
            }
        }

        attribute.clear();
        attrs.clear();
    }
    while ((index = xml.getNextElementIndex("pageList", depth)) != TExpat::npos);
}

PAGELIST_T TPageList::findPage(const std::string& name)
{
    DECL_TRACER("TPageList::findPage(const std::string& name)");

    vector<PAGELIST_T>::iterator iter;
    PAGELIST_T page;

    for (iter = mPageList.begin(); iter != mPageList.end(); iter++)
    {
        if (iter->name.compare(name) == 0)
        {
            page = *iter;
            return page;
        }
    }

    TError::setErrorMsg("Page " + name + " not found!");
    TError::setError();
    return page;
}

PAGELIST_T TPageList::findPage(int pageID)
{
    DECL_TRACER("TPageList::findPage(int pageID)");

    vector<PAGELIST_T>::iterator iter;
    PAGELIST_T page;

    for (iter = mPageList.begin(); iter != mPageList.end(); iter++)
    {
        if (iter->pageID == pageID)
        {
            page = *iter;
            return page;
        }
    }

    TError::setErrorMsg("Page " + std::to_string(pageID) + " not found!");
    TError::setError();
    return page;
}

SUBPAGELIST_T TPageList::findSubPage(const std::string& name)
{
    DECL_TRACER("TPageList::findSubPage(const std::string& name)");

    vector<SUBPAGELIST_T>::iterator iter;
    SUBPAGELIST_T page;

    for (iter = mSubPageList.begin(); iter != mSubPageList.end(); iter++)
    {
        if (iter->name.compare(name) == 0)
        {
            page = *iter;
            return page;
        }
    }

    TError::setErrorMsg("Subpage " + name + " not found!");
    TError::setError();
    return page;
}

SUBPAGELIST_T TPageList::findSubPage(int pageID)
{
    DECL_TRACER("TPageList::findSubPage(int pageID)");

    vector<SUBPAGELIST_T>::iterator iter;
    SUBPAGELIST_T page;

    for (iter = mSubPageList.begin(); iter != mSubPageList.end(); iter++)
    {
        if (iter->pageID == pageID)
        {
            page = *iter;
            return page;
        }
    }

    TError::setErrorMsg("Subpage " + std::to_string(pageID) + " not found!");
    TError::setError();
    return page;
}

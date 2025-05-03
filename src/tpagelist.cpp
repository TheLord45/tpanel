/*
 * Copyright (C) 2020 to 2025 by Andreas Theofilu <andreas@theosys.at>
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
#include "tpageinterface.h"
#include "tpagelist.h"
#include "tconfig.h"
#include "terror.h"
#include "ttpinit.h"

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
using namespace Expat;

TPageList::TPageList()
{
    DECL_TRACER("TPageList::TPageList()");

    string projectPath = TConfig::getProjectPath();
    mProject = makeFileName(projectPath, "prj.xma");

    if (fs::exists(mProject))
        initialize();

    projectPath += "/__system";
    mSystemProject = makeFileName(projectPath, "prj.xma");

    if (fs::exists(mSystemProject))
        initialize(true);

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

void TPageList::initialize(bool system)
{
    DECL_TRACER("TPageList::initialize(bool system)");

    TError::clear();
    string sProject;

    if (!system)
    {
        if (mPageList.size() > 0)
            mPageList.clear();

        if (mSubPageList.size() > 0)
            mSubPageList.clear();

        sProject = mProject;
    }
    else
    {
        if (mSystemPageList.size() > 0)
            mSystemPageList.clear();

        if (mSystemSubPageList.size() > 0)
            mSystemSubPageList.clear();

        sProject = mSystemProject;
    }

    if (sProject.empty() || !isValidFile(sProject))
    {
        TError::SetErrorMsg("Empty or invalid project file! <" + sProject + ">");
        MSG_ERROR(TError::getErrorMsg());
        return;
    }

    TExpat xml(sProject);

    if (!TTPInit::isG5())
        xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return;

    int depth = 0;
    size_t index = 0;

    if (xml.getElementIndex("pageList", &depth) == TExpat::npos)
    {
        MSG_ERROR("Couldn't find the section \"pageList\" in file!");
        TError::SetError();
        return;
    }

    do
    {
        vector<ATTRIBUTE_t> attrs = xml.getAttributes();
        string attribute = xml.getAttribute("type", attrs);

        if (attribute.empty())
        {
            TError::SetErrorMsg("Missing element \"pageList\" in file " + sProject);
            MSG_ERROR(TError::getErrorMsg());
            return;
        }
        else if (attribute.compare("page") != 0 && attribute.compare("subpage") != 0)
        {
            TError::SetErrorMsg("Invalid page type " + attribute + " found!");
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
                if (!system)
                    mPageList.push_back(pl);
                else
                    mSystemPageList.push_back(pl);
            }
            else if (attribute.compare("subpage") == 0)
            {
                if (!system)
                    mSubPageList.push_back(spl);
                else
                    mSystemSubPageList.push_back(spl);
            }
        }

        attribute.clear();
        attrs.clear();
    }
    while (xml.getNextElementIndex("pageList", depth) != TExpat::npos);

    loadSubPageSets(&xml);
}

void TPageList::loadSubPageSets(TExpat *xml)
{
    DECL_TRACER("TPageList::loadSubPageSets(TExpat *xml)");

    int depth = 0;
    size_t index = 0;
    size_t oldIndex = 0;

    if (xml->getElementIndex("subPageSets", &depth) == TExpat::npos)
    {
        MSG_WARNING("Couldn't find the section \"subPageSets\" in file!");
        return;
    }

    do
    {
        while ((index = xml->getNextElementIndex("subPageSetEntry", depth+1)) != TExpat::npos)
        {
            SUBVIEWLIST_T svl;
            SUBVIEWITEM_T svi;
            string e, content;

            vector<ATTRIBUTE_t> attrs = xml->getAttributes();
            svl.id = xml->getAttributeInt("id", attrs);

            while ((index = xml->getNextElementFromIndex(index, &e, &content, nullptr)) != TExpat::npos)
            {
                if (e.compare("name") == 0)
                    svl.name = content;
                else if (e.compare("pgWidth") == 0)
                    svl.pgWidth = xml->convertElementToInt(content);
                else if (e.compare("pgHeight") == 0)
                    svl.pgHeight = xml->convertElementToInt(content);
                else if (e.compare("items") == 0)
                {
                    string et;

                    while ((index = xml->getNextElementFromIndex(index, &et, &content, &attrs)) != TExpat::npos)
                    {
                        if (et.compare("item") == 0)
                        {
                            svi.index = xml->getAttributeInt("index", attrs);
                            string it;

                            while ((index = xml->getNextElementFromIndex(index, &it, &content, nullptr)) != TExpat::npos)
                            {
                                if (it.compare("pageID") == 0)
                                    svi.pageID = xml->convertElementToInt(content);

                                oldIndex = index;
                            }

                            svl.items.push_back(svi);
                            svi.index = svi.pageID = 0;

                            if (index == TExpat::npos)
                                index = oldIndex + 1;
                        }
                    }
                }
            }

            mSubViewList.push_back(svl);
            svl.id = svl.pgWidth = svl.pgHeight = 0;
            svl.items.clear();
            svl.name.clear();
        }
    }
    while (xml->getNextElementIndex("subPageSets", depth) != TExpat::npos);

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        vector<SUBVIEWLIST_T>::iterator iterList;
        vector<SUBVIEWITEM_T>::iterator iterItem;

        for (iterList = mSubViewList.begin(); iterList != mSubViewList.end(); ++iterList)
        {
            MSG_DEBUG("Subview container " << iterList->id << ": " << iterList->name);
            MSG_DEBUG("        pgWidth:  " << iterList->pgWidth);
            MSG_DEBUG("        pgHeight: " << iterList->pgHeight);

            for (iterItem = iterList->items.begin(); iterItem != iterList->items.end(); ++iterItem)
            {
                MSG_DEBUG("        Item:     " << iterItem->index << ", pageID: " << iterItem->pageID);
            }
        }
    }
}

PAGELIST_T TPageList::findPage(const std::string& name, bool system)
{
    DECL_TRACER("TPageList::findPage(const std::string& name, bool system)");

    vector<PAGELIST_T>::iterator iter;
    PAGELIST_T page;

    if (!system && mPageList.size() > 0)
    {
        for (iter = mPageList.begin(); iter != mPageList.end(); ++iter)
        {
            if (iter->name.compare(name) == 0)
            {
                page = *iter;
                return page;
            }
        }
    }
    else if (mSystemPageList.size() > 0)
    {
        for (iter = mSystemPageList.begin(); iter != mSystemPageList.end(); ++iter)
        {
            if (iter->name.compare(name) == 0)
            {
                page = *iter;
                return page;
            }
        }
    }

    TError::SetErrorMsg("Page " + name + " not found!");
    TError::SetError();
    return page;
}

PAGELIST_T TPageList::findPage(int pageID)
{
    DECL_TRACER("TPageList::findPage(int pageID)");

    vector<PAGELIST_T>::iterator iter;
    PAGELIST_T page;

    if (pageID < SYSTEM_PAGE_START)
    {
        for (iter = mPageList.begin(); iter != mPageList.end(); ++iter)
        {
            if (iter->pageID == pageID)
            {
                page = *iter;
                return page;
            }
        }
    }
    else
    {
        for (iter = mSystemPageList.begin(); iter != mSystemPageList.end(); ++iter)
        {
            if (iter->pageID == pageID)
            {
                page = *iter;
                return page;
            }
        }
    }

    TError::SetErrorMsg("Page " + std::to_string(pageID) + " not found!");
    TError::SetError();
    return page;
}

SUBPAGELIST_T TPageList::findSubPage(const std::string& name, bool system)
{
    DECL_TRACER("TPageList::findSubPage(const std::string& name, bool system)");

    vector<SUBPAGELIST_T>::iterator iter;
    SUBPAGELIST_T page;

    if (!system && mSubPageList.size() > 0)
    {
        for (iter = mSubPageList.begin(); iter != mSubPageList.end(); ++iter)
        {
            if (iter->name.compare(name) == 0)
            {
                page = *iter;
                return page;
            }
        }
    }
    else if (mSystemSubPageList.size() > 0)
    {
        for (iter = mSystemSubPageList.begin(); iter != mSystemSubPageList.end(); ++iter)
        {
            if (iter->name.compare(name) == 0)
            {
                page = *iter;
                return page;
            }
        }
    }

    TError::SetErrorMsg("Subpage " + name + " not found!");
    TError::SetError();
    return page;
}

SUBPAGELIST_T TPageList::findSubPage(int pageID)
{
    DECL_TRACER("TPageList::findSubPage(int pageID)");

    vector<SUBPAGELIST_T>::iterator iter;
    SUBPAGELIST_T page;

    if (pageID < SYSTEM_PAGE_START)
    {
        for (iter = mSubPageList.begin(); iter != mSubPageList.end(); ++iter)
        {
            if (iter->pageID == pageID)
            {
                page = *iter;
                return page;
            }
        }
    }
    else
    {
        for (iter = mSystemSubPageList.begin(); iter != mSystemSubPageList.end(); ++iter)
        {
            if (iter->pageID == pageID)
            {
                page = *iter;
                return page;
            }
        }
    }

    TError::SetErrorMsg("Subpage " + std::to_string(pageID) + " not found!");
    TError::SetError();
    return page;
}

/*******************************************************************************
 * SubViewList
 *
 * A subview list is a container which defines one or more subpages. All the
 * subpages in the container are displayed inside a scroll area. This area can
 * be defined to scroll vertical or horizontal. On typing on one of the
 * subpages in the scroll area the defined action is made. It behaves like a
 * normal button and sends the push notification to the NetLinx. If there are
 * some actions defined they will be executed, just like a normal button.
 *
 * Only Modero X panels are supporting this subviews! And TPanel of course :-)
 ******************************************************************************/

/**
 * @brief TPageList::findSubViewList
 * Searches the list of subviews for an entry with the ID \a id. On success
 * the type \a SUBVIEWLIST_T is returned.
 *
 * @param id    The ID of the wanted subview list.
 * @return On success returns the SUBVIEWLIST_T with all data found. Otherwise
 * an empty structure is returned.
 */
SUBVIEWLIST_T TPageList::findSubViewList(int id)
{
    DECL_TRACER("TPageList::findSubViewList(int id)");

    if (mSubViewList.empty())
        return SUBVIEWLIST_T();

    vector<SUBVIEWLIST_T>::iterator iter;

    for (iter = mSubViewList.begin(); iter != mSubViewList.end(); ++iter)
    {
        if (iter->id == id)
            return *iter;
    }

    return SUBVIEWLIST_T();
}

/**
 * @brief TPageList::findSubViewListPageID
 * Find the page ID inside of a subview list. The ID of the subview as well as
 * the index number of the item must be known to get the page ID.
 *
 * @param id        ID of subview.
 * @param index     Index number of item in the subview list.
 *
 * @return On success returns the page ID. If the page ID couldn't be found
 * either because there are no subviews or the subview ID doesn't exist or the
 * index number inside the subview doesn't exist, it returns -1.
 */
int TPageList::findSubViewListPageID(int id, int index)
{
    DECL_TRACER("TPageList::findSubViewListPageID(int id, int index)");

    if (mSubViewList.empty())
        return -1;

    SUBVIEWLIST_T slist = findSubViewList(id);

    if (slist.id == 0 || slist.items.empty())
        return -1;


    vector<SUBVIEWITEM_T>::iterator iter;

    for (iter = slist.items.begin(); iter != slist.items.end(); ++iter)
    {
        if (iter->index == index)
            return iter->pageID;
    }

    return -1;
}

/**
 * @brief TPageList::findSubViewListNextPageID
 * Searches for the first or the next page ID inside a subview list.
 * If the parameter \a *index is less then 0, the first index is returned.
 * otherwise the next index grater then \a *index is returned, if there is one.
 * If no more indexes are found it returns -1.
 *
 * @param id        The ID of the subview list.
 * @param index     The index number.
 *
 * @return On success the next page ID where the index is higher then \a index
 * is returned. If there is no more page ID with a higher index it returns -1.
 */
int TPageList::findSubViewListNextPageID(int id, int *index)
{
    DECL_TRACER("TPageList::findSubViewListNextPageID(int id, int *index)");

    if (mSubViewList.empty() || !index)
        return -1;

    SUBVIEWLIST_T slist = findSubViewList(id);

    if (slist.id == 0 || slist.items.empty())
        return -1;

    if (*index < 0)
    {
        *index = slist.items[0].index;
        return slist.items[0].pageID;
    }

    vector<SUBVIEWITEM_T>::iterator iter;

    for (iter = slist.items.begin(); iter != slist.items.end(); ++iter)
    {
        if (iter->index > *index)
            return iter->pageID;
    }

    return -1;
}

int TPageList::findSubViewListID(int pageID, int *index)
{
    DECL_TRACER("TPageList::findSubViewListID(int pageID, int *index)");

    if (mSubViewList.empty() || pageID <= 0)
        return -1;

    vector<SUBVIEWLIST_T>::iterator iter;

    for (iter = mSubViewList.begin(); iter != mSubViewList.end(); ++iter)
    {
        vector<SUBVIEWITEM_T>::iterator itItem;

        for (itItem = iter->items.begin(); itItem != iter->items.end(); ++itItem)
        {
            if (itItem->pageID == pageID)
            {
                if (index)
                    *index = itItem->index;

                return iter->id;
            }
        }
    }

    return -1;
}


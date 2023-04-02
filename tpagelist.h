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
#ifndef __TPAGELIST_H__
#define __TPAGELIST_H__

#include <string>
#include <vector>
#include "texpat++.h"
#include "tvalidatefile.h"

#define POPUPTYPE_SUBPAGE       1
#define POPUPTYPE_SUBVIEW       2

typedef struct PAGELIST_T
{
    std::string name;
    int pageID{0};
    std::string file;
    int isValid{-1};

    void clear()
    {
        name.clear();
        pageID = 0;
        file.clear();
        isValid = -1;
    }
}PAGELIST_T;

typedef struct SUBPAGELIST_T
{
    std::string name;
    int pageID{0};
    std::string file;
    std::string group;
    int isValid{-1};
    int popupType{0};

    void clear()
    {
        name.clear();
        pageID = 0;
        file.clear();
        group.clear();
        isValid = -1;
        popupType = 0;
    }
}SUBPAGELIST_T;

typedef struct SUBVIEWITEM_T
{
    int index{0};
    int pageID{0};
}SUBVIEWITEM_T;

typedef struct SUBVIEWLIST_T
{
    std::string name;
    int id{0};
    int pgWidth{0};
    int pgHeight{0};
    std::vector<SUBVIEWITEM_T> items;
}SUBVIEWLIST_T;

class TPageList : public TValidateFile
{
    public:
        TPageList();
        ~TPageList();

        PAGELIST_T findPage(const std::string& name, bool system=false);
        PAGELIST_T findPage(int pageID);
        SUBPAGELIST_T findSubPage(const std::string& name, bool system=false);
        SUBPAGELIST_T findSubPage(int pageID);

        SUBVIEWLIST_T findSubViewList(int id);
        int findSubViewListPageID(int id, int index);
        int findSubViewListNextPageID(int id, int *index);
        int findSubViewListID(int pageID, int *index);

        std::vector<PAGELIST_T>& getPagelist() { return mPageList; }
        std::vector<SUBPAGELIST_T>& getSupPageList() { return mSubPageList; }
        std::vector<PAGELIST_T>& getSystemPagelist() { return mSystemPageList; }
        std::vector<SUBPAGELIST_T>& getSystemSupPageList() { return mSystemSubPageList; }
        std::vector<SUBVIEWLIST_T>& getSubViewList() { return mSubViewList; }

    private:
        void initialize(bool system=false);
        void loadSubPageSets(Expat::TExpat *xml);
        void cleanup();

        std::string mProject;
        std::string mSystemProject;
        std::vector<PAGELIST_T> mPageList;
        std::vector<SUBPAGELIST_T> mSubPageList;
        std::vector<PAGELIST_T> mSystemPageList;
        std::vector<SUBPAGELIST_T> mSystemSubPageList;
        std::vector<SUBVIEWLIST_T> mSubViewList;
};

#endif

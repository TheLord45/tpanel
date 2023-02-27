/*
 * Copyright (C) 2020 to 2023 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TPAGE_H__
#define __TPAGE_H__

#include <string>
#include <vector>
#include "tvalidatefile.h"
#include "tsubpage.h"
#include "tpalette.h"
#include "tpageinterface.h"

#define ZORDER_INVALID      -1
#define MAX_PAGE_ID         500

class TBitmap;

typedef struct PAGECHAIN_T
{
    TSubPage *subpage{nullptr}; // Ponter to subpage
    PAGECHAIN_T *prev{nullptr}; // Pointer to previous element
    PAGECHAIN_T *next{nullptr}; // Pointer to next element
}PAGECHAIN_T;

class TPage : public TValidateFile, public TPageInterface
{
    public:
        TPage() {}
        TPage(const std::string& name);
        ~TPage();

        void initialize(const std::string& name);
        void setPalette(TPalette *pal) { mPalette = pal; }

        int getWidth() { return mPage.width; }
        int getHeight() { return mPage.height; }
        void setName(const std::string& n) { mPage.name = n; }
        std::string& getName() { return mPage.name; }
        int getNumber() { return mPage.pageID; }
        bool isVisilble() { return mVisible; }
        ulong getHandle() { return (mPage.pageID << 16) & 0xffff0000; }

        PAGECHAIN_T *addSubPage(TSubPage *pg);
        TSubPage *getSubPage(int pageID);
        TSubPage *getSubPage(const std::string& name);
        TSubPage *getFirstSubPage();
        TSubPage *getNextSubPage();
        TSubPage *getPrevSubPage();
        TSubPage *getLastSubPage();
        bool removeSubPage(int ID);
        bool removeSubPage(const std::string& nm);
        int getActZOrder() { return mZOrder; }
        int getNextZOrder();
        int decZOrder();
        void resetZOrder() { mZOrder = ZORDER_INVALID; }
#ifdef _OPAQUE_SKIA_
        void registerCallback(std::function<void (ulong handle, TBitmap image, int width, int height, ulong color)> setBackground) { _setBackground = setBackground; }
#else
        void registerCallback(std::function<void (ulong handle, TBitmap image, int width, int height, ulong color, int opacity)> setBackground) { _setBackground = setBackground; }
#endif
        void registerCallbackDB(std::function<void(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top)> displayButton) { _displayButton = displayButton; }
        void regCallDropPage(std::function<void (ulong handle)> callDropPage) { _callDropPage = callDropPage; }
        void regCallDropSubPage(std::function<void (ulong handle)> callDropSubPage) { _callDropSubPage = callDropSubPage; }
        void regCallPlayVideo(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> playVideo) { _playVideo = playVideo; };

        void show();
        void drop();
        void sortSubpages();

    protected:
//        bool sortButtons();
#ifdef _SCALE_SKIA_
        void calcPosition(int im_width, int im_height, int *left, int *top, bool scale=false);
#else
        void calcPosition(int im_width, int im_height, int *left, int *top);
#endif
    private:
        void addProgress();
#ifdef _OPAQUE_SKIA_
        std::function<void (ulong handle, TBitmap image, int width, int height, ulong color)> _setBackground{nullptr};
#else
        std::function<void (ulong handle, TBitmap image, int width, int height, ulong color, int opacity)> _setBackground{nullptr};
#endif
        std::function<void (ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top)> _displayButton{nullptr};
        std::function<void (ulong handle)> _callDropPage{nullptr};
        std::function<void (ulong handle)> _callDropSubPage{nullptr};
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _playVideo{nullptr};

        std::string mPath;              // Path and name of the XML file
        PAGE_T mPage;                   // Definitions of page
        std::vector<Button::SR_T> sr;   // Background details
        std::string mFile;              // The name of the file where the page is defined.
        bool mVisible{false};           // true = Page is visible
        TPalette *mPalette{nullptr};    // The color palette

        PAGECHAIN_T *mSubPages{nullptr};// Subpages related to this page
        int mLastSubPage{0};            // Stores the number of the last subpage
        int mZOrder{ZORDER_INVALID};    // The Z-Order of the subpages
        std::vector<LIST_t> mLists;     // Lists of page
};



#endif // _TPAGE_H

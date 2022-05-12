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

#ifndef __TPAGE_H__
#define __TPAGE_H__

#include <string>
#include <vector>
#include "tvalidatefile.h"
#include "tpagelist.h"
#include "tsubpage.h"
#include "tconfig.h"
#include "terror.h"
#include "tpalette.h"
#include "tfont.h"

#define ZORDER_INVALID      -1

typedef struct PAGECHAIN_T
{
    TSubPage *subpage{nullptr}; // Ponter to subpage
    PAGECHAIN_T *next{nullptr}; // Pointer to next element
}PAGECHAIN_T;

class TPage : public TValidateFile
{
    public:
        TPage() {}
        TPage(const std::string& name);
        ~TPage();

        void initialize(const std::string& name);
        void setPalette(TPalette *pal) { mPalette = pal; }
        void setFonts(TFont *ft) { mFonts = ft; }

        int getWidth() { return width; }
        int getHeight() { return height; }
        void setName(const std::string& n) { name = n; }
        std::string& getName() { return name; }
        int getNumber() { return pageID; }
        bool isVisilble() { return mVisible; }
        bool hasButton(int id);
        Button::TButton *getButton(int id);
        std::vector<Button::TButton *> getButtons(int ap, int ad);
        std::vector<Button::TButton *> getAllButtons();
        Button::TButton *getFirstButton();
        Button::TButton *getNextButton();
        Button::TButton *getLastButton();
        Button::TButton *getPreviousButton();

        PAGECHAIN_T *addSubPage(TSubPage *pg);
        TSubPage *getSubPage(int pageID);
        TSubPage *getSubPage(const std::string& name);
        TSubPage *getFirstSubPage();
        TSubPage *getNextSubPage();
        bool removeSubPage(int ID);
        bool removeSubPage(const std::string& nm);
        int getActZOrder() { return mZOrder; }
        int getNextZOrder();
        int decZOrder();
        void resetZOrder() { mZOrder = ZORDER_INVALID; }

        Button::BUTTONS_T *addButton(Button::TButton *button);

        void registerCallback(std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> setBackground) { _setBackground = setBackground; }
        void registerCallbackDB(std::function<void(ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> displayButton) { _displayButton = displayButton; }
        void regCallDropPage(std::function<void (ulong handle)> callDropPage) { _callDropPage = callDropPage; }
        void regCallDropSubPage(std::function<void (ulong handle)> callDropSubPage) { _callDropSubPage = callDropSubPage; }
        void regCallPlayVideo(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> playVideo) { _playVideo = playVideo; };

        void show();
        void drop();
        void sortSubpages();

    protected:
        bool sortButtons();
#ifdef _SCALE_SKIA_
        void calcPosition(int im_width, int im_height, int *left, int *top, bool scale=false);
#else
        void calcPosition(int im_width, int im_height, int *left, int *top);
#endif
    private:
        void addProgress();
        bool drawText(SkBitmap *img);
        Button::POSITION_t calcImagePosition(int width, int height, Button::CENTER_CODE cc, int line);
        int calcLineHeight(const std::string& text, SkFont& font);
        int numberLines(const std::string& str);

        std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> _setBackground{nullptr};
        std::function<void (ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> _displayButton{nullptr};
        std::function<void (ulong handle)> _callDropPage{nullptr};
        std::function<void (ulong handle)> _callDropSubPage{nullptr};
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _playVideo{nullptr};

        std::string mPath;              // Path and name of the XML file
        std::string name;               // Name of the page
        int pageID{0};                  // Number of the page
        int width;                      // Width of the page
        int height;                     // height of the page
        Button::BUTTONS_T *mButtons{nullptr};    // Chain of buttons
        int mLastButton{0};             // Internal counter for iterating through button chain.
        std::vector<Button::SR_T> sr;   // Background details
        std::string mFile;              // The name of the file where the page is defined.
        bool mVisible{false};           // true = Page is visible
        TPalette *mPalette{nullptr};    // The color palette

        PAGECHAIN_T *mSubPages{nullptr};// Subpages related to this page
        int mLastSubPage{0};            // Stores the number of the last subpage
        TFont *mFonts{nullptr};         // Holds the class with the font list
        int mZOrder{ZORDER_INVALID};    // The Z-Order of the subpages
};



#endif // _TPAGE_H

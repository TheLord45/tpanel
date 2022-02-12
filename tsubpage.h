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

#ifndef __TSUBPAGE_H__
#define __TSUBPAGE_H__

#include <string>
#include <vector>
#include "tbutton.h"
#include "tvalidatefile.h"
#include "tpalette.h"
#include "tfont.h"

enum SHOWEFFECT
{
    SE_NONE,
    SE_FADE,
    SE_SLIDE_LEFT,
    SE_SLIDE_RIGHT,
    SE_SLIDE_TOP,
    SE_SLIDE_BOTTOM,
    SE_SLIDE_LEFT_FADE,
    SE_SLIDE_RIGHT_FADE,
    SE_SLIDE_TOP_FADE,
    SE_SLIDE_BOTTOM_FADE
};

typedef SHOWEFFECT SHOWEFFECT_t;

typedef struct ANIMATION_t
{
    SHOWEFFECT_t showEffect{SE_NONE};
    int showTime{0};
    SHOWEFFECT_t hideEffect{SE_NONE};
    int hideTime{0};
}ANIMATION_t;

typedef struct SUBPAGE_T
{
    std::string popupType;                  // The type of the popup
    int pageID{0};                          // Unique ID of popup/page
    std::string name;                       // The name of the popup/page
    int left{0};                            // Left position of popup
    int top{0};                             // Top position of popup
    int width{0};                           // Width of popup
    int height{0};                          // Height of popup
    int modal{0};                           // 0 = Popup/Page = non modal
    std::string group;                      // Name of the group the popup belongs
    int timeout{0};                         // Time after the popup hides in 1/10 seconds
    SHOWEFFECT showEffect{SE_NONE};         // The effect when the popup is shown
    int showTime{0};                        // The time reserved for the show effect
    int showX{0};                           // End of show effect position (by default "left+width");
    int showY{0};                           // End of show effect position (by default "top+height");
    SHOWEFFECT hideEffect{SE_NONE};         // The effect when the popup hides
    int hideTime{0};                        // The time reserved for the hide effect
    int hideX{0};                           // End of hide effect position (by default "left");
    int hideY{0};                           // End of hide effect position (by default "top");
    std::vector<Button::SR_T> sr;           // Page/Popup description
}SUBPAGE_T;

typedef struct RECT_T
{
    int left{0};
    int top{0};
    int width{0};
    int height{0};
}RECT_T;

class TSubPage : public TValidateFile
{
    public:
        TSubPage(const std::string& name);
        ~TSubPage();

        void setPalette(TPalette *pal) { mPalette = pal; }
        void setFonts(TFont *ft) { mFonts = ft; }

        int getNumber() { return mSubpage.pageID; }
        std::string& getName() { return mSubpage.name; }
        SUBPAGE_T& getSubPage() { return mSubpage; }
        std::string& getGroupName() { return mSubpage.group; }
        int getLeft() { return mSubpage.left; }
        void setLeft(int l) { mSubpage.left = l; }
        int getTop() { return mSubpage.top; }
        void setTop(int t) { mSubpage.top = t; }
        int getWidth() { return mSubpage.width; }
        void setWidth(int w) { mSubpage.width = w; }
        int getHeight() { return mSubpage.height; }
        void setHeight(int h) { mSubpage.height = h; }
        RECT_T getRegion();
        int getZOrder() { if (mVisible) return mZOrder; else return -1; }
        void setZOrder(int z) { mZOrder = z; }
        void setGroup(const std::string& group) { mSubpage.group = group; }
        void setModal(int m) { mSubpage.modal = m; }
        bool isModal() { return (mSubpage.modal != 0); }
        SHOWEFFECT getShowEffect() { return mSubpage.showEffect; }
        void setShowEffect(SHOWEFFECT se) { mSubpage.showEffect = se; }
        int getShowTime() { return mSubpage.showTime; }
        void setShowTime(int t) { mSubpage.showTime = t; }
        void setShowEndPosition(int x, int y) { mSubpage.showX = x; mSubpage.showY = y; }
        SHOWEFFECT getHideEffect() { return mSubpage.hideEffect; }
        void setHideEffect(SHOWEFFECT he) { mSubpage.hideEffect = he; }
        void setHideEndPosition(int x, int y) { mSubpage.hideX = x; mSubpage.hideY = y; }
        int getHideTime() { return mSubpage.hideTime; }
        void setHideTime(int t) { mSubpage.hideTime = t; }
        int getTimeout() { return mSubpage.timeout; }
        void setTimeout(int t) { mSubpage.timeout = t; }
        bool isVisible() { return mVisible; }
        bool hasButton(int id);
        Button::TButton *getButton(int id);
        std::vector<Button::TButton *> getButtons(int ap, int ad);
        std::vector<Button::TButton *> getAllButtons();
        void show();
        void drop();
        void doClick(int x, int y, bool pressed);
        void startTimer();
        void stopTimer() { mTimerRunning = false; }
        void registerCallback(std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> setBackground) { _setBackground = setBackground; }
        void registerCallbackDB(std::function<void(ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> displayButton) { _displayButton = displayButton; }
        void regCallDropSubPage(std::function<void (ulong handle)> callDropSubPage) { _callDropSubPage = callDropSubPage; }
        void regCallPlayVideo(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> playVideo) { _playVideo = playVideo; };

    protected:
        void initialize();
        Button::BUTTONS_T *addButton(Button::TButton* button);
        void runTimer();
        bool sortButtons();
#ifdef  _SCALE_SKIA_
        void calcPosition(int im_width, int im_height, int *left, int *top, bool scale = false);
#else
        void calcPosition(int im_width, int im_height, int *left, int *top);
#endif
    private:
        std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> _setBackground{nullptr};
        std::function<void (ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> _displayButton{nullptr};
        std::function<void (ulong handle)> _callDropSubPage{nullptr};
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _playVideo{nullptr};

        bool drawText(SkBitmap *img);
        int numberLines(const std::string& str);
        int calcLineHeight(std::string text, SkFont& font);
        Button::POSITION_t calcImagePosition(int width, int height, Button::CENTER_CODE cc, int line);

        bool mVisible{false};                   // TRUE = subpage is visible
        std::string mFName;                     // The file name of the page
        std::string mFile;                      // The path and file name of the page
        TPalette *mPalette{nullptr};            // The color palette
        SUBPAGE_T mSubpage;                     // Parameters of the subpage
        Button::BUTTONS_T *mButtons{nullptr};   // The elements of the subpage
        TFont *mFonts{nullptr};                 // The font management
        int mZOrder{-1};                        // The Z-Order of the subpage if it is visible
        std::atomic<bool>mTimerRunning{false};  // TRUE= timer is running
        std::thread mThreadTimer;               // The thread started if a timeout is defined.
};

#endif

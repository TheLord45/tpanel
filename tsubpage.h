/*
 * Copyright (C) 2020 to 2022 by Andreas Theofilu <andreas@theosys.at>
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
#include "tpageinterface.h"

typedef struct RECT_T
{
    int left{0};
    int top{0};
    int width{0};
    int height{0};
}RECT_T;

class TSubPage : public TValidateFile, public TPageInterface
{
    public:
        TSubPage(const std::string& name);
        ~TSubPage();

        void setPalette(TPalette *pal) { mPalette = pal; }

        int getNumber() { return mSubpage.pageID; }
        std::string& getName() { return mSubpage.name; }
        PAGE_T& getSubPage() { return mSubpage; }
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
        ulong getHandle() { return ((mSubpage.pageID << 16) & 0xffff0000); }
        void show();
        void drop();
        void doClick(int x, int y, bool pressed);
        void startTimer();
        void stopTimer() { mTimerRunning = false; }
#ifdef _OPAQUE_SKIA_
        void registerCallback(std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> setBackground) { _setBackground = setBackground; }
#else
        void registerCallback(std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color, int opacity)> setBackground) { _setBackground = setBackground; }
#endif
        void registerCallbackDB(std::function<void(ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> displayButton) { _displayButton = displayButton; }
        void regCallDropSubPage(std::function<void (ulong handle)> callDropSubPage) { _callDropSubPage = callDropSubPage; }
        void regCallPlayVideo(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> playVideo) { _playVideo = playVideo; };

    protected:
        void initialize();
        void runTimer();
#ifdef  _SCALE_SKIA_
        void calcPosition(int im_width, int im_height, int *left, int *top, bool scale = false);
#else
        void calcPosition(int im_width, int im_height, int *left, int *top);
#endif
    private:
#ifdef _OPAQUE_SKIA_
        std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> _setBackground{nullptr};
#else
        std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color, int opacity)> _setBackground{nullptr};
#endif
        std::function<void (ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> _displayButton{nullptr};
        std::function<void (ulong handle)> _callDropSubPage{nullptr};
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _playVideo{nullptr};

        bool mVisible{false};                   // TRUE = subpage is visible
        std::string mFName;                     // The file name of the page
        std::string mFile;                      // The path and file name of the page
        TPalette *mPalette{nullptr};            // The color palette
        PAGE_T mSubpage;                        // Parameters of the subpage
        int mZOrder{-1};                        // The Z-Order of the subpage if it is visible
        std::atomic<bool>mTimerRunning{false};  // TRUE= timer is running
        std::thread mThreadTimer;               // The thread started if a timeout is defined.
        std::vector<LIST_t> mLists;             // Lists of subpage
};

#endif

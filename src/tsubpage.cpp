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

#include <exception>

#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>

#include "tresources.h"
#include "texpat++.h"
#include "tpagemanager.h"
#include "tsubpage.h"
#include "tdrawimage.h"
#include "tconfig.h"
#include "terror.h"
#if TESTMODE == 1
#include "testmode.h"
#endif

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
using namespace Button;
using namespace Expat;

extern TPageManager *gPageManager;

TSubPage::TSubPage(const string& name)
    : mFile(name)
{
    DECL_TRACER("TSubPage::TSubPage(const string& path)");
    TError::clear();

    string projectPath = TConfig::getProjectPath();

    if (!fs::exists(projectPath + "/prj.xma"))
    {
        MSG_ERROR("Directory " << projectPath << " doesn't exist!");
        return;
    }

    string path = makeFileName(projectPath, name);

    if (isValidFile())
        mFName = getFileName();
    else
    {
        MSG_ERROR("Either the path \"" << projectPath << "\" or the file name \"" << name << "\" is invalid!");
        TError::setError();
        return;
    }

    if (gPageManager)
    {
        if (!_displayButton)
            _displayButton = gPageManager->getCallbackDB();

        if (!_setBackground)
            _setBackground = gPageManager->getCallbackBG();

        if (!_playVideo)
            _playVideo = gPageManager->getCallbackPV();

        if (!_callDropSubPage)
            _callDropSubPage = gPageManager->getCallDropSubPage();
    }

    initialize();
}

TSubPage::~TSubPage()
{
    DECL_TRACER("TSubPage::~TSubPage()");

    if (mSubpage.name.empty())
    {
        MSG_WARNING("Invalid page found!");
        return;
    }

    MSG_DEBUG("Destroing subpage " << mSubpage.pageID << ": " << mSubpage.name);

    BUTTONS_T *b = TPageInterface::getButtons();
    BUTTONS_T *next = nullptr;

    while (b)
    {
        next = b->next;

        if (b->button)
        {
            // If this is a system button, we must remove it first from the list
            dropButton(b->button);
            delete b->button;
        }

        delete b;
        b = next;
    }

    setButtons(nullptr);
}

void TSubPage::initialize()
{
    DECL_TRACER("TSubPage::initialize()");

    if (mFName.empty())
        return;

    TError::clear();
    TExpat xml(mFName);
    xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return;

    int depth = 0;
    size_t index = 0;
    size_t oldIndex = 0;
    string ename, content;
    vector<ATTRIBUTE_t> attrs;

    if ((index = xml.getElementIndex("page", &depth)) == TExpat::npos)
    {
        MSG_ERROR("Element \"page\" with attribute \"type\" was not found! Invalid XML file!");
        TError::setError();
        return;
    }

    attrs = xml.getAttributes();
    string stype = xml.getAttribute("type", attrs);

    if (stype.compare("subpage") != 0)
    {
        MSG_ERROR("The type " << stype << " is invalid for a subpage!");
        TError::setError();
        return;
    }

    mSubpage.popupType = xml.getAttribute("popupType", attrs);  // popup or subpage

    while ((index = xml.getNextElementFromIndex(index, &ename, &content, &attrs)) != TExpat::npos)
    {
        if (ename.compare("pageID") == 0)
            mSubpage.pageID = xml.convertElementToInt(content);
        else if (ename.compare("name") == 0)
            mSubpage.name = content;
        else if (ename.compare("left") == 0)
        {
            mSubpage.left = xml.convertElementToInt(content);
            mSubpage.leftOrig = mSubpage.left;
        }
        else if (ename.compare("top") == 0)
        {
            mSubpage.top = xml.convertElementToInt(content);
            mSubpage.topOrig = mSubpage.top;
        }
        else if (ename.compare("width") == 0)
        {
            mSubpage.width = xml.convertElementToInt(content);
            mSubpage.widthOrig = mSubpage.width;
        }
        else if (ename.compare("height") == 0)
        {
            mSubpage.height = xml.convertElementToInt(content);
            mSubpage.heightOrig = mSubpage.height;
        }
        else if (ename.compare("group") == 0)
            mSubpage.group = content;
        else if (ename.compare("modal") == 0)
            mSubpage.modal = xml.convertElementToInt(content);
        else if (ename.compare("showEffect") == 0)
            mSubpage.showEffect = (SHOWEFFECT)xml.convertElementToInt(content);
        else if (ename.compare("showTime") == 0)
            mSubpage.showTime = xml.convertElementToInt(content);
        else if (ename.compare("hideTime") == 0)
            mSubpage.hideTime = xml.convertElementToInt(content);
        else if (ename.compare("hideEffect") == 0)
            mSubpage.hideEffect = (SHOWEFFECT)xml.convertElementToInt(content);
        else if (ename.compare("timeout") == 0)
            mSubpage.timeout = xml.convertElementToInt(content);
        else if (ename.compare("resetPos") == 0)
            mSubpage.resetPos = xml.convertElementToInt(content);
        else if (ename.compare("button") == 0)      // Read a button
        {
            try
            {
                TError::clear();
                TButton *button = new TButton();
                TPageInterface::registerListCallback<TSubPage>(button, this);
                button->setPalette(mPalette);
                button->setFonts(getFonts());
                index = button->initialize(&xml, index);
                button->setParentSize(mSubpage.width, mSubpage.height);
                button->registerCallback(_displayButton);
                button->regCallPlayVideo(_playVideo);

                if (TError::isError())
                {
                    MSG_ERROR("Dropping button because of previous errors!");
                    delete button;
                    return;
                }

                button->setHandle(((mSubpage.pageID << 16) & 0xffff0000) | button->getButtonIndex());
                button->createButtons();
                addButton(button);
                index++;    // Jump over the end tag of the button.
            }
            catch (std::exception& e)
            {
                MSG_ERROR("Memory exception: " << e.what());
                TError::setError();
                return;
            }
        }
        else if (ename.compare("sr") == 0)
        {
            SR_T sr;
            sr.number = xml.getAttributeInt("number", attrs);

            while ((index = xml.getNextElementFromIndex(index, &ename, &content, &attrs)) != TExpat::npos)
            {
                if (ename.compare("bs") == 0)
                    sr.bs = content;
                else if (ename.compare("cb") == 0)
                    sr.cb = content;
                else if (ename.compare("cf") == 0)
                    sr.cf = content;
                else if (ename.compare("ct") == 0)
                    sr.ct = content;
                else if (ename.compare("ec") == 0)
                    sr.ec = content;
                else if (ename.compare("bm") == 0)
                    sr.bm = content;
                else if (ename.compare("mi") == 0)
                    sr.mi = content;
                else if (ename.compare("ji") == 0)
                    sr.ji = xml.convertElementToInt(content);
                else if (ename.compare("jb") == 0)
                    sr.jb = xml.convertElementToInt(content);
                else if (ename.compare("fi") == 0)
                    sr.fi = xml.convertElementToInt(content);
                else if (ename.compare("ii") == 0)
                    sr.ii = xml.convertElementToInt(content);
                else if (ename.compare("ix") == 0)
                    sr.ix = xml.convertElementToInt(content);
                else if (ename.compare("iy") == 0)
                    sr.iy = xml.convertElementToInt(content);
                else if (ename.compare("oo") == 0)
                    sr.oo = xml.convertElementToInt(content);
                else if (ename.compare("te") == 0)
                    sr.te = content;
                else if (ename.compare("tx") == 0)
                    sr.tx = xml.convertElementToInt(content);
                else if (ename.compare("ty") == 0)
                    sr.ty = xml.convertElementToInt(content);
                else if (ename.compare("et") == 0)
                    sr.et = xml.convertElementToInt(content);
                else if (ename.compare("ww") == 0)
                    sr.ww = xml.convertElementToInt(content);
                else if (ename.compare("jt") == 0)
                    sr.jt = (Button::ORIENTATION)xml.convertElementToInt(content);

                oldIndex = index;
            }

            mSubpage.sr.push_back(sr);
        }

        if (index == TExpat::npos)
            index = oldIndex + 1;
    }

    setSR(mSubpage.sr);
/*
    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        MSG_DEBUG("PageID: " << mSubpage.pageID);
        MSG_DEBUG("Name  : " << mSubpage.name);
        MSG_DEBUG("Left  : " << mSubpage.left);
        MSG_DEBUG("Top   : " << mSubpage.top);
        MSG_DEBUG("Width : " << mSubpage.width);
        MSG_DEBUG("Height: " << mSubpage.height);

        vector<SR_T>::iterator iter;
        size_t pos = 1;

        for (iter = mSubpage.sr.begin(); iter != mSubpage.sr.end(); ++iter)
        {
            MSG_DEBUG("   " << pos << ": id: " << iter->number);
            MSG_DEBUG("   " << pos << ": bs: " << iter->bs);
            MSG_DEBUG("   " << pos << ": cb: " << iter->cb);
            MSG_DEBUG("   " << pos << ": cf: " << iter->cf);
            MSG_DEBUG("   " << pos << ": ct: " << iter->ct);
            MSG_DEBUG("   " << pos << ": ec: " << iter->ec);
            MSG_DEBUG("   " << pos << ": bm: " << iter->bm);
            MSG_DEBUG("   " << pos << ": mi: " << iter->mi);
            MSG_DEBUG("   " << pos << ": fi: " << iter->fi);
            pos++;
        }
    }
*/
    // Here the sort function could be called. But it's not necessary because
    // the buttons are stored in ascending Z order. Therefor the following
    // method call is commented out.
    // sortButtons();
}

void TSubPage::show()
{
    DECL_TRACER("TSubPage::show()");

    if (mSubpage.sr.empty())
    {
        MSG_ERROR("No page elements found for page " << mSubpage.name << "!");
        return;
    }

    if (!_setBackground)
    {
        if (gPageManager && gPageManager->getCallbackBG())
            _setBackground = gPageManager->getCallbackBG();
        else
        {
            MSG_WARNING("No callback \"setBackground\" was set!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }
    }

    bool haveImage = false;
    ulong handle = (mSubpage.pageID << 16) & 0xffff0000;
    MSG_DEBUG("Processing subpage " << mSubpage.pageID << ": " << mSubpage.name);
    SkBitmap target;

    if (!allocPixels(mSubpage.width, mSubpage.height, &target))
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (mSubpage.resetPos != 0)
    {
        mSubpage.left = mSubpage.leftOrig;
        mSubpage.top = mSubpage.topOrig;
        mSubpage.width = mSubpage.widthOrig;
        mSubpage.height = mSubpage.heightOrig;
    }

    target.eraseColor(TColor::getSkiaColor(mSubpage.sr[0].cf));
    // Draw the background, if any
    if (mSubpage.sr.size() > 0 && (!mSubpage.sr[0].bm.empty() || !mSubpage.sr[0].mi.empty()))
    {
        TDrawImage dImage;
        dImage.setWidth(mSubpage.width);
        dImage.setHeight(mSubpage.height);

        if (!mSubpage.sr[0].bm.empty())
        {
            MSG_DEBUG("Loading image " << mSubpage.sr[0].bm);
            sk_sp<SkData> rawImage = readImage(mSubpage.sr[0].bm);
            SkBitmap bm;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image BM ...");

                if (!DecodeDataToBitmap(rawImage, &bm))
                {
                    MSG_WARNING("Problem while decoding image " << mSubpage.sr[0].bm);
                }
                else if (!bm.empty())
                {
                    dImage.setImageBm(bm);
                    SkImageInfo info = bm.info();
                    mSubpage.sr[0].bm_width = info.width();
                    mSubpage.sr[0].bm_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("BM image " << mSubpage.sr[0].bm << " seems to be empty!");
                }
            }
        }

        if (!mSubpage.sr[0].mi.empty())
        {
            MSG_DEBUG("Loading image " << mSubpage.sr[0].mi);
            sk_sp<SkData> rawImage = readImage(mSubpage.sr[0].mi);
            SkBitmap mi;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image MI ...");

                if (!DecodeDataToBitmap(rawImage, &mi))
                {
                    MSG_WARNING("Problem while decoding image " << mSubpage.sr[0].mi);
                }
                else if (!mi.empty())
                {
                    dImage.setImageMi(mi);
                    SkImageInfo info = mi.info();
                    mSubpage.sr[0].mi_width = info.width();
                    mSubpage.sr[0].mi_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("MI image " << mSubpage.sr[0].mi << " seems to be empty!");
                }
            }
        }

        if (haveImage)
        {
            dImage.setSr(mSubpage.sr);

            if (!dImage.drawImage(&target))
                return;

            if (!mSubpage.sr[0].te.empty())
            {
                if (!drawText(mSubpage, &target))
                    return;
            }
#ifdef _OPAQUE_SKIA_
            if (mSubpage.sr[0].oo < 255 && mSubpage.sr[0].te.empty() && mSubpage.sr[0].bs.empty())
                setOpacity(&target, mSubpage.sr[0].oo);
#endif

#ifdef _SCALE_SKIA_
            if (gPageManager && gPageManager->getScaleFactor() != 1.0)
            {
                SkPaint paint;
                int left, top;
                SkImageInfo info = target.info();

                paint.setBlendMode(SkBlendMode::kSrc);
                paint.setFilterQuality(kHigh_SkFilterQuality);
                // Calculate new dimension
                double scaleFactor = gPageManager->getScaleFactor();
                MSG_DEBUG("Using scale factor " << scaleFactor);
                int lwidth = (int)((double)info.width() * scaleFactor);
                int lheight = (int)((double)info.height() * scaleFactor);
                int twidth = (int)((double)mSubpage.width * scaleFactor);
                int theight = (int)((double)mSubpage.height * scaleFactor);
                calcPosition(lwidth, lheight, &left, &top);
                // Create a canvas and draw new image
                sk_sp<SkImage> im = SkImage::MakeFromBitmap(target);

                if (!allocPixels(twidth, theight, &target))
                {
#if TESTMODE == 1
                    setScreenDone();
#endif
                    return;
                }

                target.eraseColor(TColor::getSkiaColor(mSubpage.sr[0].cf));
                SkCanvas can(target, SkSurfaceProps());
                SkRect rect = SkRect::MakeXYWH(left, top, lwidth, lheight);
                can.drawImageRect(im, rect, &paint);
                MSG_DEBUG("Scaled size of background image: " << left << ", " << top << ", " << lwidth << ", " << lheight);
            }
#endif
#ifdef _OPAQUE_SKIA_
            if (mSubpage.sr[0].te.empty() && mSubpage.sr[0].bs.empty())
            {
                TBitmap image((unsigned char *)target.getPixels(), target.info().width(), target.info().height());
                _setBackground(handle, image, target.info().width(), target.info().height(), TColor::getColor(mSubpage.sr[0].cf));
            }
#else
            if (mSubpage.sr[0].te.empty() && mSubpage.sr[0].bs.empty())
            {
                TBitmap image((unsigned char *)target.getPixels(), target.info().width(), target.info().height());
                _setBackground(handle, image, target.info().width(), target.info().height(), TColor::getColor(mSubpage.sr[0].cf), mSubpage.sr[0].oo);
            }
#endif
        }
    }

    if (mSubpage.sr.size() > 0 && !mSubpage.sr[0].te.empty())
    {
        MSG_DEBUG("Drawing a text only on background image ...");

        if (drawText(mSubpage, &target))
            haveImage = true;
    }

    // Check for a frame and draw it if there is one.
    if (!mSubpage.sr[0].bs.empty())
    {
        if (drawFrame(mSubpage, &target))
            haveImage = true;
    }

    if (haveImage)
    {
#ifdef _OPAQUE_SKIA_
        if (mSubpage.sr[0].oo < 255)
            setOpacity(&target, mSubpage.sr[0].oo);
#endif
        TBitmap image((unsigned char *)target.getPixels(), target.info().width(), target.info().height());
#ifdef _OPAQUE_SKIA_
        _setBackground(handle, image, target.info().width(), target.info().height(), TColor::getColor(mSubpage.sr[0].cf));
#else
        _setBackground(handle, image, target.info().width(), target.info().height(), TColor::getColor(mSubpage.sr[0].cf), mSubpage.sr[0].oo);
#endif
    }
    else if (mSubpage.sr.size() > 0 && !haveImage)
    {
        MSG_DEBUG("Calling \"setBackground\" with no image ...");
#ifdef _OPAQUE_SKIA_
        _setBackground(handle, TBitmap(), 0, 0, TColor::getColor(mSubpage.sr[0].cf));
#else
        _setBackground(handle, TBitmap(), 0, 0, TColor::getColor(mSubpage.sr[0].cf), mSubpage.sr[0].oo);
#endif
    }

    // Draw the buttons
    BUTTONS_T *button = TPageInterface::getButtons();

    while (button)
    {
        if (button->button)
        {
            MSG_DEBUG("Drawing button " << handleToString(button->button->getHandle()) << ": " << button->button->getButtonIndex() << " --> " << button->button->getButtonName());
            TPageInterface::registerListCallback<TSubPage>(button->button, this);
            button->button->registerCallback(_displayButton);
            button->button->regCallPlayVideo(_playVideo);
            button->button->setFonts(getFonts());
            button->button->setPalette(mPalette);
            button->button->createButtons();

            if (mSubpage.sr.size() > 0)
                button->button->setGlobalOpacity(mSubpage.sr[0].oo);

            if (mSubpage.resetPos != 0)
                button->button->resetButton();

            button->button->show();
        }

        button = button->next;
    }

    // Mark page as visible
    mVisible = true;

    if (gPageManager && gPageManager->getPageFinished())
        gPageManager->getPageFinished()(handle);
}

SkBitmap& TSubPage::getBgImage()
{
    DECL_TRACER("TSubPage::getBgImage()");

    if (!mBgImage.empty())
        return mBgImage;

    bool haveImage = false;
    MSG_DEBUG("Creating image for subpage " << mSubpage.pageID << ": " << mSubpage.name);
    SkBitmap target;

    if (!allocPixels(mSubpage.width, mSubpage.height, &target))
        return mBgImage;

    target.eraseColor(TColor::getSkiaColor(mSubpage.sr[0].cf));
    // Draw the background, if any
    if (mSubpage.sr.size() > 0 && (!mSubpage.sr[0].bm.empty() || !mSubpage.sr[0].mi.empty()))
    {
        TDrawImage dImage;
        dImage.setWidth(mSubpage.width);
        dImage.setHeight(mSubpage.height);

        if (!mSubpage.sr[0].bm.empty())
        {
            MSG_DEBUG("Loading image " << mSubpage.sr[0].bm);
            sk_sp<SkData> rawImage = readImage(mSubpage.sr[0].bm);
            SkBitmap bm;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image BM ...");

                if (!DecodeDataToBitmap(rawImage, &bm))
                {
                    MSG_WARNING("Problem while decoding image " << mSubpage.sr[0].bm);
                }
                else if (!bm.empty())
                {
                    dImage.setImageBm(bm);
                    SkImageInfo info = bm.info();
                    mSubpage.sr[0].bm_width = info.width();
                    mSubpage.sr[0].bm_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("BM image " << mSubpage.sr[0].bm << " seems to be empty!");
                }
            }
        }

        if (!mSubpage.sr[0].mi.empty())
        {
            MSG_DEBUG("Loading image " << mSubpage.sr[0].mi);
            sk_sp<SkData> rawImage = readImage(mSubpage.sr[0].mi);
            SkBitmap mi;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image MI ...");

                if (!DecodeDataToBitmap(rawImage, &mi))
                {
                    MSG_WARNING("Problem while decoding image " << mSubpage.sr[0].mi);
                }
                else if (!mi.empty())
                {
                    dImage.setImageMi(mi);
                    SkImageInfo info = mi.info();
                    mSubpage.sr[0].mi_width = info.width();
                    mSubpage.sr[0].mi_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("MI image " << mSubpage.sr[0].mi << " seems to be empty!");
                }
            }
        }

        if (haveImage)
        {
            dImage.setSr(mSubpage.sr);

            if (!dImage.drawImage(&target))
                return mBgImage;

            if (!mSubpage.sr[0].te.empty())
            {
                if (!drawText(mSubpage, &target))
                    return mBgImage;
            }

            if (mSubpage.sr[0].oo < 255 && mSubpage.sr[0].te.empty() && mSubpage.sr[0].bs.empty())
                setOpacity(&target, mSubpage.sr[0].oo);

#ifdef _SCALE_SKIA_
            size_t rowBytes = info.minRowBytes();
            size_t size = info.computeByteSize(rowBytes);
            SkImageInfo info = target.info();

            if (gPageManager && gPageManager->getScaleFactor() != 1.0)
            {
                SkPaint paint;
                int left, top;

                paint.setBlendMode(SkBlendMode::kSrc);
                paint.setFilterQuality(kHigh_SkFilterQuality);
                // Calculate new dimension
                double scaleFactor = gPageManager->getScaleFactor();
                MSG_DEBUG("Using scale factor " << scaleFactor);
                int lwidth = (int)((double)info.width() * scaleFactor);
                int lheight = (int)((double)info.height() * scaleFactor);
                int twidth = (int)((double)mSubpage.width * scaleFactor);
                int theight = (int)((double)mSubpage.height * scaleFactor);
                calcPosition(lwidth, lheight, &left, &top);
                // Create a canvas and draw new image
                sk_sp<SkImage> im = SkImage::MakeFromBitmap(target);

                if (!allocPixels(twidth, theight, &target))
                    return;

                target.eraseColor(TColor::getSkiaColor(mSubpage.sr[0].cf));
                SkCanvas can(target, SkSurfaceProps());
                SkRect rect = SkRect::MakeXYWH(left, top, lwidth, lheight);
                can.drawImageRect(im, rect, &paint);
                MSG_DEBUG("Scaled size of background image: " << left << ", " << top << ", " << lwidth << ", " << lheight);
            }
#endif
        }
    }

    if (mSubpage.sr.size() > 0 && !mSubpage.sr[0].te.empty())
    {
        MSG_DEBUG("Drawing a text only on background image ...");

        if (drawText(mSubpage, &target))
            haveImage = true;
    }

    // Check for a frame and draw it if there is one.
    if (!mSubpage.sr[0].bs.empty())
    {
        if (!drawBorder(&target, mSubpage.sr[0].bs, mSubpage.width, mSubpage.height, mSubpage.sr[0].cb))
        {
            if (drawFrame(mSubpage, &target))
                haveImage = true;
        }
    }

    if (haveImage)
    {
        if (mSubpage.sr[0].oo < 255)
            setOpacity(&target, mSubpage.sr[0].oo);

        mBgImage = target;
    }

    return mBgImage;
}

void TSubPage::drop()
{
    DECL_TRACER("TSubPage::drop()");

    stopTimer();

    if (mVisible && _callDropSubPage)
        _callDropSubPage((mSubpage.pageID << 16) & 0xffff0000, mParent);
#if TESTMODE == 1
    else
    {
        __success = true;
        setScreenDone();
    }
#endif

    // Set all elements of subpage invisible
    BUTTONS_T *bt = TPageInterface::getButtons();

    while (bt)
    {
        bt->button->hide();
        bt = bt->next;
    }

    mZOrder = -1;
    mVisible = false;
}

void TSubPage::startTimer()
{
    DECL_TRACER("TSubPage::startTimer()");

    if (mSubpage.timeout <= 0 || mTimerRunning)
        return;

    try
    {
        mThreadTimer = std::thread([=] { runTimer(); });
        mThreadTimer.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting a timeout thread: " << e.what());
        return;
    }
}

void TSubPage::runTimer()
{
    DECL_TRACER("TSubPage::runTimer()");

    if (mTimerRunning)
        return;

    mTimerRunning = true;
    ulong tm = mSubpage.timeout * 100;
    ulong unit = 100;
    ulong total = 0;

    while (mTimerRunning && !prg_stopped && total < tm)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(unit));
        total += unit;
    }

    drop();
    mTimerRunning = false;
}

#ifdef _SCALE_SKIA_
void TSubPage::calcPosition(int im_width, int im_height, int *left, int *top, bool scale)
#else
void TSubPage::calcPosition(int im_width, int im_height, int *left, int *top)
#endif
{
    DECL_TRACER("TSubPage::calcPosition(int im_width, int im_height, int *left, int *top)");

    int nw = mSubpage.width;
    int nh = mSubpage.height;
#ifdef _SCALE_SKIA_
    if (scale && gPageManager && gPageManager->getScaleFactor() != 1.0)
    {
        nw = (int)((double)mSubpage.width * gPageManager->getScaleFactor());
        nh = (int)((double)mSubpage.height * gPageManager->getScaleFactor());
    }
#endif
    switch (mSubpage.sr[0].jb)
    {
        case 0: // absolute position
            *left = mSubpage.sr[0].bx;
            *top = mSubpage.sr[0].by;
#ifdef _SCALE_SKIA_
            if (scale && gPageManager && gPageManager->getScaleFactor() != 1.0)
            {
                *left = (int)((double)mSubpage.sr[0].bx * gPageManager->getScaleFactor());
                *left = (int)((double)mSubpage.sr[0].by * gPageManager->getScaleFactor());
            }
#endif
        break;

        case 1: // top, left
            *left = 0;
            *top = 0;
        break;

        case 2: // center, top
            *left = (nw - im_width) / 2;
            *top = 0;
        break;

        case 3: // right, top
            *left = nw - im_width;
            *top = 0;
        break;

        case 4: // left, middle
            *left = 0;
            *top = (nh - im_height) / 2;
        break;

        case 6: // right, middle
            *left = nw - im_width;
            *top = (nh - im_height) / 2;
        break;

        case 7: // left, bottom
            *left = 0;
            *top = nh - im_height;
        break;

        case 8: // center, bottom
            *left = (nw - im_width) / 2;
            *top = nh - im_height;
        break;

        case 9: // right, bottom
            *left = nw - im_width;
            *top = nh - im_height;
        break;

        default:    // center middle
            *left = (nw - im_width) / 2;
            *top = (nh - im_height) / 2;
    }

    if (*left < 0)
        *left = 0;

    if (*top < 0)
        *top = 0;
}

RECT_T TSubPage::getRegion()
{
    DECL_TRACER("TSubPage::getRegion()");
    return {mSubpage.left, mSubpage.top, mSubpage.width, mSubpage.height};
}

/**
 * This method is called indirectly from the GUI after a mouse click. If This
 * subpage matches the clicked coordinates, than the elements are tested. If
 * an element is found that matches the coordinates it gets the click. It
 * depends on the kind of element what happens.
 */
void TSubPage::doClick(int x, int y, bool pressed)
{
    DECL_TRACER("TSubPage::doClick(int x, int y)");

    BUTTONS_T *button = TPageInterface::getButtons();

    if (!button)
        return;

    // Find last button
    while (button && button->next)
        button = button->next;

    // Scan in reverse order
    while (button)
    {
        TButton *but = button->button;
        bool clickable = but->isClickable();
        MSG_DEBUG("Testing button " << but->getButtonIndex() << " (" << but->getButtonName() << "): " << (clickable ? "CLICKABLE" : "NOT CLICKABLE"));

        if (clickable && x > but->getLeftPosition() && x < (but->getLeftPosition() + but->getWidth()) &&
            y > but->getTopPosition() && y < (but->getTopPosition() + but->getHeight()))
        {
            MSG_DEBUG("Clicking button " << but->getButtonIndex() << ": " << but->getButtonName() << " to state " << (pressed ? "PRESS" : "RELEASE"));
            int btX = x - but->getLeftPosition();
            int btY = y - but->getTopPosition();

            if (but->doClick(btX, btY, pressed))
                break;
        }

        button = button->previous;
    }
}

void TSubPage::moveMouse(int x, int y)
{
    DECL_TRACER("TSubPage::moveMouse(int x, int y)");

    BUTTONS_T *button = TPageInterface::getButtons();

    if (!button)
        return;

    // Find last button
    while (button && button->next)
        button = button->next;

    // Scan in reverse order
    while (button)
    {
        TButton *but = button->button;

        if (but->getButtonType() != BARGRAPH && but->getButtonType() != JOYSTICK)
        {
            button = button->previous;
            continue;
        }

        bool clickable = but->isClickable();

        if (clickable && x > but->getLeftPosition() && x < (but->getLeftPosition() + but->getWidth()) &&
            y > but->getTopPosition() && y < (but->getTopPosition() + but->getHeight()))
        {
            int btX = x - but->getLeftPosition();
            int btY = y - but->getTopPosition();

            if (but->getButtonType() == BARGRAPH)
                but->moveBargraphLevel(btX, btY);
            else if (but->getButtonType() == JOYSTICK && !but->getLevelFuction().empty())
            {
                but->drawJoystick(btX, btY);
                but->sendJoystickLevels();
            }

            break;
        }

        button = button->previous;
    }
}

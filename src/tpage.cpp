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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>

#include "tlock.h"
#include "tresources.h"
#include "tpagemanager.h"
#include "tpage.h"
#include "tdrawimage.h"
#include "texpat++.h"
#include "tconfig.h"
#include "terror.h"
#include "ttpinit.h"
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
using std::map;
using std::pair;
using namespace Button;
using namespace Expat;

extern TPageManager *gPageManager;

TPage::TPage(const string& name)
{
    DECL_TRACER("TPage::TPage(const string& name)");
    TError::clear();

    if (gPageManager)
    {
        if (!_setBackground)
            _setBackground = gPageManager->getCallbackBG();

        if (!_displayButton)
            _displayButton = gPageManager->getCallbackDB();

        if (!_callDropPage)
            _callDropPage = gPageManager->getCallDropPage();

        if (!_callDropSubPage)
            _callDropSubPage = gPageManager->getCallDropSubPage();

        if (!_playVideo)
            _playVideo = gPageManager->getCallbackPV();
    }

    if (name.compare("_progress") == 0)
    {
        addProgress();
        return ;
    }

    initialize(name);
}

TPage::~TPage()
{
    DECL_TRACER("TPage::~TPage()");

    MSG_DEBUG("Destroing page " << mPage.pageID << ": " << mPage.name);
    BUTTONS_T *p = getButtons();
    BUTTONS_T *next = nullptr;

    while (p)
    {
        next = p->next;
        delete p->button;
        delete p;
        p = next;
    }

    setButtons(nullptr);
    mSubPages.clear();
}

void TPage::initialize(const string& nm)
{
    DECL_TRACER("TPage::initialize(const string& name)");

    string projectPath = TConfig::getProjectPath();

    if (!fs::exists(projectPath + "/prj.xma"))
    {
        MSG_ERROR("Directory " << projectPath << " doesn't exist!");
        return;
    }

    makeFileName(projectPath, nm);

    MSG_DEBUG("Using path: " << projectPath << " and file: " << nm);

    if (isValidFile())
        mPath = getFileName();

    TExpat xml(mPath);

    if (!TTPInit::isTP5())
        xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return;

    int depth = 0;
    size_t index = 0;
    size_t oldIndex = 0;
    int bmIndex = 0;
    vector<ATTRIBUTE_t> attrs;

    if ((index = xml.getElementIndex("page", &depth)) == TExpat::npos)
    {
        MSG_ERROR("Element \"page\" with attribute \"type\" was not found!");
        TError::SetError();
        return;
    }

    attrs = xml.getAttributes();
    string type = xml.getAttribute("type", attrs);

    if (type.compare("page") != 0)
    {
        MSG_ERROR("Invalid page type \"" << type << "\"!");
        TError::SetError();
        return;
    }

    TError::clear();
    depth++;
    string ename, content;

    while ((index = xml.getNextElementFromIndex(index, &ename, &content, &attrs)) != TExpat::npos)
    {
        string e = ename;
        MSG_DEBUG("Element: " << ename << " at index " << index);

        if (e.compare("pageID") == 0)
            mPage.pageID = xml.convertElementToInt(content);
        else if (e.compare("name") == 0)
            mPage.name = content;
        else if (e.compare("width") == 0)
            mPage.width = xml.convertElementToInt(content);
        else if (e.compare("height") == 0)
            mPage.height = xml.convertElementToInt(content);
        else if (e.compare("button") == 0)
        {
            TButton *button = new TButton();
            TPageInterface::registerListCallback<TPage>(button, this);

            button->setPalette(mPalette);
            button->setFonts(getFonts());
            button->registerCallback(_displayButton);
            button->regCallPlayVideo(_playVideo);
            index = button->initialize(&xml, index);
            button->setParentSize(mPage.width, mPage.height);

            if (TError::isError())
            {
                PRINT_LAST_ERROR();
                MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error: " << TError::getErrorMsg());
                delete button;
                return;
            }

            button->setHandle(((mPage.pageID << 16) & 0xffff0000) | button->getButtonIndex());
            button->createButtons();
            addButton(button);
            string sType1 = xml.getElementTypeStr(index);
            string sType2 = xml.getElementTypeStr(index+1);
            MSG_DEBUG("Element type 1: " << sType1);
            MSG_DEBUG("Element type 2: " << sType2);

            if (xml.isElementTypeEnd(index+1))
                index++;        // Jump over the end tag of the button.
        }
        else if (e.compare("sr") == 0)
        {
            SR_T bsr;
            bsr.number = xml.getAttributeInt("number", attrs);
            MSG_DEBUG("Page " << mPage.name << " at State " << bsr.number);
            index++;
            bmIndex = 0;

            while ((index = xml.getNextElementFromIndex(index, &ename, &content, &attrs)) != TExpat::npos)
            {
                if (ename.compare("bs") == 0)
                    bsr.bs = content;
                else if (ename.compare("cb") == 0)
                    bsr.cb = content;
                else if (ename.compare("cf") == 0)
                    bsr.cf = content;
                else if (ename.compare("ct") == 0)
                    bsr.ct = content;
                else if (ename.compare("ec") == 0)
                    bsr.ec = content;
                else if (ename.compare("bm") == 0)
                    bsr.bm = content;
                else if (ename.compare("mi") == 0)
                    bsr.mi = content;
                else if (ename.compare("fi") == 0)
                    bsr.fi = xml.convertElementToInt(content);
                else if (ename.compare("te") == 0)
                    bsr.te = content;
                else if (ename.compare("tx") == 0)
                    bsr.tx = xml.convertElementToInt(content);
                else if (ename.compare("ty") == 0)
                    bsr.ty = xml.convertElementToInt(content);
                else if (ename.compare("et") == 0)
                    bsr.et = xml.convertElementToInt(content);
                else if (ename.compare("ww") == 0)
                    bsr.ww = xml.convertElementToInt(content);
                else if (ename.compare("jt") == 0)
                    bsr.jt = (Button::ORIENTATION)xml.convertElementToInt(content);
                else if (ename.compare("jb") == 0)
                    bsr.jb = (Button::ORIENTATION)xml.convertElementToInt(content);
                else if (ename.compare("bitmapEntry") == 0)
                {
                    string fname;
                    BITMAPS_t bitmapEntry;
                    MSG_DEBUG("Section: " << ename);

                    while ((index = xml.getNextElementFromIndex(index, &fname, &content, &attrs)) != TExpat::npos)
                    {
                        if (fname.compare("fileName") == 0)
                            bsr.bitmaps[bmIndex].fileName = content;
                        else if (fname.compare("justification") == 0)
                            bsr.bitmaps[bmIndex].justification = static_cast<ORIENTATION>(xml.convertElementToInt(content));
                        else if (fname.compare("offsetX") == 0)
                            bsr.bitmaps[bmIndex].offsetX = xml.convertElementToInt(content);
                        else if (fname.compare("offsetY") == 0)
                            bsr.bitmaps[bmIndex].offsetY = xml.convertElementToInt(content);

                        oldIndex = index;
                    }

                    bmIndex++;
                    MSG_DEBUG("Found image: " << bsr.bitmaps[bmIndex].fileName << ", justification: " << bsr.bitmaps[bmIndex].justification << ", Offset: " << bsr.bitmaps[bmIndex].offsetX << "x" << bsr.bitmaps[bmIndex].offsetY);

                    if (index == TExpat::npos)
                        index = oldIndex + 1;
                }

                oldIndex = index;
            }

            sr.push_back(bsr);

            if (index == TExpat::npos)
                index = oldIndex + 1;
        }
    }

    MSG_DEBUG("Setting SR with " << sr.size() << " elements");
    setSR(sr);

    if (TPageInterface::getButtons())
        sortButtons();
}

void TPage::addProgress()
{
    DECL_TRACER("TPage::addProgress()");

    if (!gPageManager)
    {
        MSG_WARNING("The page manager is still not initialized!");
        return;
    }

    Button::SR_T bsr;
    mPage.pageID = 300;
    mPage.name = "_progress";
    mPage.width = gPageManager->getSettings()->getWidth();
    mPage.height = gPageManager->getSettings()->getHeight();
    double unit = (double)mPage.height / 10.0;
    MSG_DEBUG("One unit is " << unit);
    // Background of page
    bsr.number = 1;
    bsr.cf = "#106010ff";
    bsr.ct = "#ffffffff";
    bsr.cb = "#009000ff";
    bsr.ec = "#ffffffff";
    bsr.fi = 21;
    sr.push_back(bsr);
    // Text field 1 to show status messages
    Button::EXTBUTTON_t bt;
    bt.type = GENERAL;
    bt.bi = 1;
    bt.na = "Line1";
    bt.tp = (int)(unit * 2.0);
    bt.lt = (int)(((double)mPage.width - ((double)mPage.width / 100.0 * 80.0)) / 2.0);
    bt.wt = (int)((double)mPage.width / 100.0 * 80.0);    // Line take 80% of available width
    bt.ht = (int)(unit / 100.0 * 80.0);
    MSG_DEBUG("Dimensions button 1: lt: " << bt.lt << ", tp: " << bt.tp << ", wt: " << bt.wt << ", ht: " << bt.ht);
    bt.zo = 1;
    bt.ap = 0;
    bt.ad = 160;
    bsr.cf = "#000000ff";
    bt.sr.push_back(bsr);
    bsr.number = 2;
    bt.sr.push_back(bsr);
    TButton *button = new TButton();
    button->setPalette(mPalette);
    button->setFonts(getFonts());
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(mPage.width, mPage.height);

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((mPage.pageID << 16) & 0xffff0000) | bt.bi);
    button->createButtons();
    addButton(button);
    // Text field 2 to show status messages
    bt.bi = 2;
    bt.na = "Line2";
    bt.tp = (int)(unit * 7.0);
    MSG_DEBUG("Dimensions button 2: lt: " << bt.lt << ", tp: " << bt.tp << ", wt: " << bt.wt << ", ht: " << bt.ht);
    bt.zo = 2;
    bt.ad = 161;
    button = new TButton();
    button->setPalette(mPalette);
    button->setFonts(getFonts());
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(mPage.width, mPage.height);

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((mPage.pageID << 16) & 0xffff0000) | bt.bi);
    button->createButtons();
    addButton(button);
    // Progress bar 1 (overall status)
    bt.type = BARGRAPH;
    bt.bi = 3;
    bt.na = "Bar1";
    bt.tp = (int)(unit * 3.0);
    bt.lt = (int)(((double)mPage.width - ((double)mPage.width / 100.0 * 80.0)) / 2.0);
    bt.wt = (int)((double)mPage.width / 100.0 * 80.0);    // Line take 80% of available width
    bt.ht = (int)unit;
    MSG_DEBUG("Dimensions bargraph 1: lt: " << bt.lt << ", tp: " << bt.tp << ", wt: " << bt.wt << ", ht: " << bt.ht);
    bt.zo = 3;
    bt.ap = 0;
    bt.ad = 162;
    bt.lp = 0;
    bt.lv = 162;
    bt.rl = 1;
    bt.rh = 100;
    bt.sc = "#ffffffff";
    bt.dr = "horizontal";
    bsr.number = 1;
    bsr.cf = "#0e0e0eff";
    bsr.ct = "#ffffffff";
    bsr.cb = "#009000ff";
    bt.sr.clear();
    bt.sr.push_back(bsr);
    bsr.number = 2;
    bsr.cf = "#ffffffff";
    bt.sr.push_back(bsr);
    button = new TButton();
    button->setPalette(mPalette);
    button->setFonts(getFonts());
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(mPage.width, mPage.height);

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((mPage.pageID << 16) & 0xffff0000) | bt.bi);
    button->createButtons();
    addButton(button);
    // Progress bar 2 (details)
    bt.bi = 4;
    bt.na = "Bar2";
    bt.tp = (int)(unit * 5.0);
    MSG_DEBUG("Dimensions bargraph 2: lt: " << bt.lt << ", tp: " << bt.tp << ", wt: " << bt.wt << ", ht: " << bt.ht);
    bt.zo = 4;
    bt.ad = 163;
    bt.lv = 163;
    button = new TButton();
    button->setPalette(mPalette);
    button->setFonts(getFonts());
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(mPage.width, mPage.height);

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((mPage.pageID << 16) & 0xffff0000) | bt.bi);
    button->createButtons();
    addButton(button);
}

SkBitmap& TPage::getBgImage()
{
    DECL_TRACER("TPage::getBgImage()");

    if (!mBgImage.empty())
        return mBgImage;

    bool haveImage = false;
    MSG_DEBUG("Creating image for page " << mPage.pageID << ": " << mPage.name);
    SkBitmap target;

    if (!allocPixels(mPage.width, mPage.height, &target))
        return mBgImage;

    target.eraseColor(TColor::getSkiaColor(mPage.sr[0].cf));
    // Draw the background, if any
    if (mPage.sr.size() > 0 && (!mPage.sr[0].bm.empty() || !mPage.sr[0].mi.empty()))
    {
        TDrawImage dImage;
        dImage.setWidth(mPage.width);
        dImage.setHeight(mPage.height);
        dImage.setSr(mPage.sr);

        if (!mPage.sr[0].bm.empty())
        {
            MSG_DEBUG("Loading image " << mPage.sr[0].bm);
            sk_sp<SkData> rawImage = readImage(mPage.sr[0].bm);
            SkBitmap bm;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image BM ...");

                if (!DecodeDataToBitmap(rawImage, &bm))
                {
                    MSG_WARNING("Problem while decoding image " << mPage.sr[0].bm);
                }
                else if (!bm.empty())
                {
                    dImage.setImageBm(bm);
                    SkImageInfo info = bm.info();
                    mPage.sr[0].bm_width = info.width();
                    mPage.sr[0].bm_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("BM image " << mPage.sr[0].bm << " seems to be empty!");
                }
            }
        }

        if (!mPage.sr[0].mi.empty())
        {
            MSG_DEBUG("Loading image " << mPage.sr[0].mi);
            sk_sp<SkData> rawImage = readImage(mPage.sr[0].mi);
            SkBitmap mi;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image MI ...");

                if (!DecodeDataToBitmap(rawImage, &mi))
                {
                    MSG_WARNING("Problem while decoding image " << mPage.sr[0].mi);
                }
                else if (!mi.empty())
                {
                    dImage.setImageMi(mi);
                    SkImageInfo info = mi.info();
                    mPage.sr[0].mi_width = info.width();
                    mPage.sr[0].mi_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("MI image " << mPage.sr[0].mi << " seems to be empty!");
                }
            }
        }

        if (haveImage)
        {
            dImage.setSr(mPage.sr);

            if (!dImage.drawImage(&target))
                return mBgImage;

            if (!mPage.sr[0].te.empty())
            {
                if (!drawText(mPage, &target))
                    return mBgImage;
            }

            if (mPage.sr[0].oo < 255 && mPage.sr[0].te.empty() && mPage.sr[0].bs.empty())
                setOpacity(&target, mPage.sr[0].oo);

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
                int twidth = (int)((double)mPage.width * scaleFactor);
                int theight = (int)((double)mPage.height * scaleFactor);
                calcPosition(lwidth, lheight, &left, &top);
                // Create a canvas and draw new image
                sk_sp<SkImage> im = SkImage::MakeFromBitmap(target);

                if (!allocPixels(twidth, theight, &target))
                    return;

                target.eraseColor(TColor::getSkiaColor(mPage.sr[0].cf));
                SkCanvas can(target, SkSurfaceProps());
                SkRect rect = SkRect::MakeXYWH(left, top, lwidth, lheight);
                can.drawImageRect(im, rect, &paint);
                MSG_DEBUG("Scaled size of background image: " << left << ", " << top << ", " << lwidth << ", " << lheight);
            }
#endif
        }
    }

    if (mPage.sr.size() > 0 && !mPage.sr[0].te.empty())
    {
        MSG_DEBUG("Drawing a text only on background image ...");

        if (drawText(mPage, &target))
            haveImage = true;
    }

    // Check for a frame and draw it if there is one.
    if (!mPage.sr[0].bs.empty())
    {
        if (drawFrame(mPage, &target))
            haveImage = true;
    }

    if (haveImage)
    {
        if (mPage.sr[0].oo < 255)
            setOpacity(&target, mPage.sr[0].oo);

        mBgImage = target;
    }

    return mBgImage;
}

void TPage::show()
{
    DECL_TRACER("TPage::show()");

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

    bool isImage = false;
    ulong handle = (mPage.pageID << 16) & 0xffff0000;
    MSG_DEBUG("Processing page " << mPage.pageID);
    SkBitmap target;

    if (!allocPixels(mPage.width, mPage.height, &target))
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (sr.empty())
    {
        MSG_WARNING("Page " << mPage.name << " (" << mPage.pageID << "): The SR is empty!");
    }

    if (sr.size() > 0)
        target.eraseColor(TColor::getSkiaColor(sr[0].cf));
    else
        target.eraseColor(SK_ColorTRANSPARENT);

    // Draw the background, if any
    if (sr.size() > 0 && (!sr[0].bm.empty() || !sr[0].mi.empty() || haveImage(sr[0])))
    {
        TDrawImage dImage;
        dImage.setWidth(mPage.width);
        dImage.setHeight(mPage.height);
        dImage.setSr(sr);

        if (!TTPInit::isTP5() && !sr[0].bm.empty())
        {
            MSG_DEBUG("Loading image " << sr[0].bm);
            sk_sp<SkData> rawImage = readImage(sr[0].bm);
            SkBitmap bm;

            if (rawImage && !rawImage->isEmpty())
            {
                MSG_DEBUG("Decoding image BM ...");

                if (!DecodeDataToBitmap(rawImage, &bm))
                {
                    MSG_WARNING("Problem while decoding image " << sr[0].bm);
                }
                else if (!bm.isNull() && !bm.empty())
                {
                    dImage.setImageBm(bm);
                    SkImageInfo info = bm.info();
                    sr[0].bm_width = info.width();
                    sr[0].bm_height = info.height();
                    isImage = true;
                    MSG_DEBUG("Image " << sr[0].bm << " has dimension " << sr[0].bm_width << " x " << sr[0].bm_height);
                }
                else
                {
                    MSG_WARNING("BM image " << sr[0].bm << " seems to be empty!");
                }
            }
        }
        else if (TTPInit::isTP5() && haveImage(sr[0]))
        {
            SkBitmap image;

            for (int i = 0; i < MAX_IMAGES; ++i)
            {
                if (sr[0].bitmaps[i].fileName.empty())
                    continue;

                MSG_DEBUG("Loading TP5 image " << sr[0].bitmaps[i].fileName);
                sk_sp<SkData> rawImage = readImage(sr[0].bitmaps[i].fileName);
                SkBitmap bm;

                if (rawImage && !rawImage->isEmpty())
                {
                    MSG_DEBUG("Decoding image BM ...");

                    if (!DecodeDataToBitmap(rawImage, &bm))
                    {
                        MSG_WARNING("Problem while decoding image " << sr[0].bitmaps[i].fileName);
                    }
                    else if (!bm.isNull() && !bm.empty())
                    {
                        dImage.setImageBm(bm);
                        SkImageInfo info = bm.info();
                        isImage = true;
                        MSG_DEBUG("Image " << sr[0].bitmaps[i].fileName << " has dimension " << bm.width() << " x " << bm.height());
                    }
                    else
                    {
                        MSG_WARNING("BM image " << sr[0].bm << " seems to be empty!");
                    }
                }
            }
        }

        MSG_DEBUG("haveImage: " << (isImage ? "TRUE" : "FALSE"));

        if (!sr[0].mi.empty())
        {
            MSG_DEBUG("Loading image " << sr[0].mi);
            sk_sp<SkData> rawImage = readImage(sr[0].mi);
            SkBitmap mi;

            if (rawImage && !rawImage->isEmpty())
            {
                MSG_DEBUG("Decoding image MI ...");

                if (!DecodeDataToBitmap(rawImage, &mi))
                {
                    MSG_WARNING("Problem while decoding image " << sr[0].mi);
                }
                else if (!mi.isNull() && !mi.empty())
                {
                    dImage.setImageMi(mi);
                    SkImageInfo info = mi.info();
                    sr[0].mi_width = info.width();
                    sr[0].mi_height = info.height();
                    isImage = true;
                }
                else
                {
                    MSG_WARNING("MI image " << sr[0].mi << " seems to be empty!");
                }
            }
        }

        if (isImage)
        {
            dImage.setSr(sr);

            if (!dImage.drawImage(&target))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return;
            }

            if (!sr[0].te.empty())
            {
                if (!drawText(mPage, &target))
                {
#if TESTMODE == 1
                    setScreenDone();
#endif
                    return;
                }
            }

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
                int twidth = (int)((double)width * scaleFactor);
                int theight = (int)((double)height * scaleFactor);
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

                target.eraseColor(TColor::getSkiaColor(sr[0].cf));
                SkCanvas can(target, SkSurfaceProps());
                SkRect rect = SkRect::MakeXYWH(left, top, lwidth, lheight);
                can.drawImageRect(im, rect, &paint);
                MSG_DEBUG("Scaled size of background image: " << left << ", " << top << ", " << lwidth << ", " << lheight);
            }
#endif
        }
    }

    if (sr.size() > 0 && !sr[0].te.empty())
    {
        MSG_DEBUG("Drawing text on background image ...");

        if (drawText(mPage, &target))
            isImage = true;
    }

    // Check for a frame and draw it if there is one.
    if (sr.size() > 0 && !sr[0].bs.empty())
    {
        if (drawFrame(mPage, &target))
            isImage = true;
    }

    if (isImage)
    {
        SkImageInfo info = target.info();
        TBitmap image((unsigned char *)target.getPixels(), info.width(), info.height());

        if (sr.size() > 0)
        {
#ifdef _OPAQUE_SKIA_
            _setBackground(handle, image, target.info().width(), target.info().height(), TColor::getColor(sr[0].cf));
#else
            _setBackground(handle, image, target.info().width(), target.info().height(), TColor::getColor(sr[0].cf), sr[0].oo);
#endif
        }
    }
    else if (sr.size() > 0 && !isImage)
    {
        MSG_DEBUG("Calling \"setBackground\" with no image ...");
#ifdef _OPAQUE_SKIA_
        _setBackground(handle, TBitmap(), 0, 0, TColor::getColor(sr[0].cf));
#else
        _setBackground(handle, TBitmap(), 0, 0, TColor::getColor(sr[0].cf), sr[0].oo);
#endif
    }

    // Draw the buttons
    BUTTONS_T *button = TPageInterface::getButtons();

    while (button)
    {
        if (button->button)
        {
            MSG_DEBUG("Drawing button " << button->button->getButtonIndex() << ": " << button->button->getButtonName());
            button->button->registerCallback(_displayButton);
            button->button->regCallPlayVideo(_playVideo);
            TPageInterface::registerListCallback<TPage>(button->button, this);
            button->button->setFonts(getFonts());
            button->button->setPalette(mPalette);
            button->button->createButtons();

            if (sr.size() > 0)
                button->button->setGlobalOpacity(sr[0].oo);

            button->button->show();
        }

        button = button->next;
    }

    // Mark page as visible
    mVisible = true;

    if (gPageManager && gPageManager->getPageFinished())
        gPageManager->getPageFinished()(handle);
}

bool TPage::addSubPage(TSubPage* pg)
{
    DECL_TRACER("TPage::addSubPage(TSubPage* pg)");

    if (!pg)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::SetError();
        return false;
    }

    if (mSubPages.empty())
        mZOrder = 0;

#if __cplusplus < 201703L
    map<int, TSubPage *>::iterator iter = mSubPages.find(pg->getNumber());

    if (iter != mSubPages.end() && iter->second != pg)
        iter->second = pg;
    else
        mSubPages.insert(pair<int, TSubPage *>(pg->getNumber(), pg));
#else
    mSubPages.insert_or_assign(pg->getNumber(), pg);
#endif
    mLastSubPage = 0;
    return true;
}

bool TPage::removeSubPage(int ID)
{
    DECL_TRACER("TPage::removeSubPage(int ID)");

    map<int, TSubPage *>::iterator iter = mSubPages.find(ID);

    if (iter != mSubPages.end())
    {
        mSubPages.erase(iter);
        return true;
    }

    return false;
}

bool TPage::removeSubPage(const std::string& nm)
{
    DECL_TRACER("TPage::removeSubPage(const std::string& nm)");

    if (mSubPages.empty())
        return false;

    map<int, TSubPage *>::iterator iter;

    for (iter = mSubPages.begin(); iter != mSubPages.end(); ++iter)
    {
        if (iter->second->getName() == nm)
        {
            mSubPages.erase(iter);
            return true;
        }
    }

    return false;
}

TSubPage *TPage::getSubPage(int pageID)
{
    DECL_TRACER("TPage::getSubPage(int pageID)");

    map<int, TSubPage *>::iterator iter = mSubPages.find(pageID);

    if (iter != mSubPages.end())
        return iter->second;

    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPage::getSubPage(const std::string& name)
{
    DECL_TRACER("TPage::getSubPage(const std::string& name)");

    if (mSubPages.empty())
        return nullptr;

    map<int, TSubPage *>::iterator iter;

    for (iter = mSubPages.begin(); iter != mSubPages.end(); ++iter)
    {
        if (iter->second->getName() == name)
            return iter->second;
    }

    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPage::getFirstSubPage()
{
    DECL_TRACER("TPage::getFirstSubPage()");

    if (mSubPages.empty())
    {
        MSG_DEBUG("No subpages in chain.");
        mLastSubPage = 0;
        return nullptr;
    }

    TSubPage *pg = mSubPages.begin()->second;

    if (!pg)
    {
        MSG_ERROR("The pointer to the subpage " << mSubPages.begin()->first << " is NULL!");
        return nullptr;
    }

    mLastSubPage = pg->getNumber();
    MSG_DEBUG("Subpage (Z: " << pg->getZOrder() << "): " << pg->getNumber() << ". " << pg->getName());
    return pg;
}

TSubPage *TPage::getNextSubPage()
{
    DECL_TRACER("TPage::getNextSubPage()");

    if (mSubPages.empty())
    {
        MSG_DEBUG("No subpages in chain.");
        mLastSubPage = 0;
        return nullptr;
    }

    if (mLastSubPage <= 0)
        mLastSubPage = mSubPages.begin()->second->getNumber();

    map<int, TSubPage *>::iterator iter = mSubPages.find(mLastSubPage);

    if (iter != mSubPages.end())
        iter++;
    else
    {
        MSG_DEBUG("No more subpages in chain.");
        mLastSubPage = 0;
        return nullptr;
    }

    if (iter != mSubPages.end())
    {
        TSubPage *page = iter->second;
        mLastSubPage = page->getNumber();
        MSG_DEBUG("Subpage (Z: " << page->getZOrder() << "): " << page->getNumber() << ". " << page->getName());
        return page;
    }

    MSG_DEBUG("No more subpages in chain.");
    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPage::getPrevSubPage()
{
    DECL_TRACER("TPage::getPrevSubPage()");

    if (mSubPages.empty())
    {
        MSG_DEBUG("No last subpage or no subpages at all!");
        mLastSubPage = 0;
        return nullptr;
    }

    if (mLastSubPage < MAX_PAGE_ID)
    {
        map<int, TSubPage *>::iterator iter = mSubPages.end();
        iter--;
        mLastSubPage = iter->first;
    }

    map<int, TSubPage *>::iterator iter = mSubPages.find(mLastSubPage);

    if (iter != mSubPages.end() && iter != mSubPages.begin())
        iter--;
    else
    {
        MSG_DEBUG("No more subpages in chain.");
        mLastSubPage = 0;
        return nullptr;
    }

    TSubPage *page = iter->second;
    mLastSubPage = page->getNumber();
    MSG_DEBUG("Subpage (Z: " << page->getZOrder() << "): " << page->getNumber() << ". " << page->getName());
    return page;
}

TSubPage *TPage::getLastSubPage()
{
    DECL_TRACER("TPage::getLastSubPage()");

    if (mSubPages.empty())
    {
        mLastSubPage = 0;
        MSG_DEBUG("No subpages in cache!");
        return nullptr;
    }

    map<int, TSubPage *>::iterator iter = mSubPages.end();
    iter--;
    TSubPage *pg = iter->second;
    mLastSubPage = pg->getNumber();
    MSG_DEBUG("Subpage (Z: " << pg->getZOrder() << "): " << pg->getNumber() << ". " << pg->getName());
    return pg;
}

void TPage::drop()
{
    DECL_TRACER("TPage::drop()");

    // remove all subpages, if there are any
#if TESTMODE == 1
    _block_screen = true;
#endif
    if (!mSubPages.empty())
    {
        map<int, TSubPage *>::iterator iter;

        for (iter = mSubPages.begin(); iter != mSubPages.end(); ++iter)
        {
            if (iter->second)
                iter->second->drop();
        }
    }
#if TESTMODE == 1
    _block_screen = false;
#endif

    // remove all buttons, if there are any
    BUTTONS_T *bt = TPageInterface::getButtons();

    while (bt)
    {
        MSG_DEBUG("Dropping button " << handleToString(bt->button->getHandle()));
        bt->button->invalidate();
        bt = bt->next;
    }

    if (gPageManager && gPageManager->getCallDropPage())
    {
        ulong handle = (mPage.pageID << 16) & 0xffff0000;
        gPageManager->getCallDropPage()(handle);
    }

    mZOrder = ZORDER_INVALID;
    mVisible = false;
}
#ifdef _SCALE_SKIA_
void TPage::calcPosition(int im_width, int im_height, int *left, int *top, bool scale)
#else
void TPage::calcPosition(int im_width, int im_height, int *left, int *top)
#endif
{
    DECL_TRACER("TPage::calcPosition(int im_width, int im_height, int *left, int *top)");

    int nw = mPage.width;
    int nh = mPage.height;
#ifdef _SCALE_SKIA_
    if (scale && gPageManager && gPageManager->getScaleFactor() != 1.0)
    {
        nw = (int)((double)width * gPageManager->getScaleFactor());
        nh = (int)((double)height * gPageManager->getScaleFactor());
    }
#endif
    switch (sr[0].jb)
    {
        case 0: // absolute position
            *left = sr[0].bx;
            *top = sr[0].by;
#ifdef _SCALE_SKIA_
            if (scale && gPageManager && gPageManager->getScaleFactor() != 1.0)
            {
                *left = (int)((double)sr[0].bx * gPageManager->getScaleFactor());
                *left = (int)((double)sr[0].by * gPageManager->getScaleFactor());
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

void TPage::sortSubpages()
{
    DECL_TRACER("TPage::sortSubpage()");

    mSubPagesSorted.clear();

    if (mSubPages.empty())
        return;

    map<int, TSubPage *>::iterator iter;

    for (iter = mSubPages.begin(); iter != mSubPages.end(); ++iter)
    {
        if (iter->second->getZOrder() >= 0)
        {
            mSubPagesSorted.insert(pair<int, TSubPage *>(iter->second->getZOrder(), iter->second));
            MSG_DEBUG("Page " << iter->second->getNumber() << " (" << iter->second->getName() << "): sorted in with Z-Order " << iter->second->getZOrder());
        }
    }
}

map<int, TSubPage *>& TPage::getSortedSubpages(bool force)
{
    DECL_TRACER("TPage::getSortedSubpages(bool force)");

    if (mSubPagesSorted.empty() || force)
        sortSubpages();

    return mSubPagesSorted;
}

int TPage::getNextZOrder()
{
    DECL_TRACER("TPage::getNextZOrder()");

    // Find highest z-order number
    int cnt = 0;
    map<int, TSubPage *>::iterator iter;

    for (iter = mSubPages.begin(); iter != mSubPages.end(); ++iter)
    {
        int zo = iter->second->getZOrder();

        if (iter->second && zo != ZORDER_INVALID)
        {
            if (cnt < zo)
                cnt = zo;
        }
    }

    mZOrder = cnt + 1;
    MSG_DEBUG("New Z-order: " << mZOrder);
    return mZOrder;
}

int TPage::decZOrder()
{
    DECL_TRACER("TPage::decZOrder()");

    // Find highest z-order number
    int cnt = 0;
    map<int, TSubPage *>::iterator iter;

    for (iter = mSubPages.begin(); iter != mSubPages.end(); ++iter)
    {
        int zo = iter->second->getZOrder();

        if (iter->second && zo != ZORDER_INVALID)
        {
            if (cnt < zo)
                cnt = zo;
        }
    }

    mZOrder = cnt;
    return mZOrder;
}

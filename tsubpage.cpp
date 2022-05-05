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
    string path = makeFileName(TConfig::getProjectPath(), name);

    if (isValidFile())
        mFName = getFileName();
    else
    {
        MSG_ERROR("Either the path \"" << TConfig::getProjectPath() << "\" or the file name \"" << name << "\" is invalid!");
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

    BUTTONS_T *b = mButtons;
    BUTTONS_T *next = nullptr;

    while (b)
    {
        next = b->next;

        if (b->button)
            delete b->button;

        delete b;
        b = next;
    }

    mButtons = nullptr;
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

    mSubpage.popupType = xml.getAttribute("popupType", attrs);  // ??

    while ((index = xml.getNextElementFromIndex(index, &ename, &content, &attrs)) != TExpat::npos)
    {
        if (ename.compare("pageID") == 0)
            mSubpage.pageID = xml.convertElementToInt(content);
        else if (ename.compare("name") == 0)
            mSubpage.name = content;
        else if (ename.compare("left") == 0)
            mSubpage.left = xml.convertElementToInt(content);
        else if (ename.compare("top") == 0)
            mSubpage.top = xml.convertElementToInt(content);
        else if (ename.compare("width") == 0)
            mSubpage.width = xml.convertElementToInt(content);
        else if (ename.compare("height") == 0)
            mSubpage.height = xml.convertElementToInt(content);
        else if (ename.compare("group") == 0)
            mSubpage.group = content;
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
        else if (ename.compare("button") == 0)      // Read a button
        {
            try
            {
                TButton *button = new TButton();
                button->setPalette(mPalette);
                button->setFonts(mFonts);
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
                    sr.jt = (Button::TEXT_ORIENTATION)xml.convertElementToInt(content);

                oldIndex = index;
            }

            mSubpage.sr.push_back(sr);
        }

        if (index == TExpat::npos)
            index = oldIndex + 1;
    }

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

    // Here the sort function could be called. But it's not necessary because
    // the buttons are stored in ascending Z order. Therefor the following
    // method call is commented out.
    // sortButtons();
}

void TSubPage::show()
{
    DECL_TRACER("TSubPage::show()");

    if (!_setBackground)
    {
        if (gPageManager && gPageManager->getCallbackBG())
            _setBackground = gPageManager->getCallbackBG();
        else
        {
            MSG_WARNING("No callback \"setBackground\" was set!");
            return;
        }
    }

    bool haveImage = false;
    ulong handle = (mSubpage.pageID << 16) & 0xffff0000;
    MSG_DEBUG("Processing subpage " << mSubpage.pageID << ": " << mSubpage.name);
    SkBitmap target;
    target.allocN32Pixels(mSubpage.width, mSubpage.height);
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
                if (!drawText(&target))
                    return;
            }

            SkImageInfo info = target.info();
            size_t rowBytes = info.minRowBytes();
            size_t size = info.computeByteSize(rowBytes);

#ifdef _SCALE_SKIA_
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
                target.allocN32Pixels(twidth, theight);
                target.eraseColor(TColor::getSkiaColor(mSubpage.sr[0].cf));
                SkCanvas can(target, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
                SkRect rect = SkRect::MakeXYWH(left, top, lwidth, lheight);
                can.drawImageRect(im, rect, &paint);
                rowBytes = target.info().minRowBytes();
                size = target.info().computeByteSize(rowBytes);
                MSG_DEBUG("Scaled size of background image: " << left << ", " << top << ", " << lwidth << ", " << lheight);
            }
#endif
            if (mSubpage.sr[0].te.empty())
                _setBackground(handle, (unsigned char *)target.getPixels(), size, rowBytes, target.info().width(), target.info().height(), TColor::getColor(mSubpage.sr[0].cf));
        }
    }

    if (mSubpage.sr.size() > 0 && !mSubpage.sr[0].te.empty())
    {
        MSG_DEBUG("Drawing a text only on background image ...");

        if (!drawText(&target))
            return;

        SkImageInfo info = target.info();
        size_t rowBytes = info.minRowBytes();
        size_t size = info.computeByteSize(rowBytes);
        rowBytes = target.info().minRowBytes();
        size = target.info().computeByteSize(rowBytes);
        _setBackground(handle, (unsigned char *)target.getPixels(), size, rowBytes, target.info().width(), target.info().height(), TColor::getColor(mSubpage.sr[0].cf));
        haveImage = true;
    }

    if (mSubpage.sr.size() > 0 && !haveImage)
    {
        MSG_DEBUG("Calling \"setBackground\" with no image ...");
        _setBackground(handle, nullptr, 0, 0, 0, 0, TColor::getColor(mSubpage.sr[0].cf));
    }

    // Draw the buttons
    BUTTONS_T *button = mButtons;

    while (button)
    {
        if (button->button)
        {
            MSG_DEBUG("Drawing button " << button->button->getButtonIndex() << ": " << button->button->getButtonName());
            button->button->registerCallback(_displayButton);
            button->button->regCallPlayVideo(_playVideo);
            button->button->setFonts(mFonts);
            button->button->setPalette(mPalette);
            button->button->createButtons();

            if (mSubpage.sr.size() > 0)
                button->button->setGlobalOpacity(mSubpage.sr[0].oo);

            button->button->show();
        }

        button = button->next;
    }

    // Mark page as visible
    mVisible = true;
}

void TSubPage::drop()
{
    DECL_TRACER("TSubPage::drop()");

    if (mVisible && _callDropSubPage)
        _callDropSubPage((mSubpage.pageID << 16) & 0xffff0000);

    stopTimer();
    // Set all elements of subpage invisible
    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        bt->button->hide();
        bt = bt->next;
    }

    mZOrder = -1;
    mVisible = false;
}

BUTTONS_T *TSubPage::addButton(TButton* button)
{
    DECL_TRACER("*TSubPage::addButton(TButton* button)");

    if (!button)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::setError();
        return nullptr;
    }

    try
    {
        BUTTONS_T *chain = new BUTTONS_T;
        chain->button = button;
        chain->next = nullptr;
        chain->previous = nullptr;
        BUTTONS_T *bts = mButtons;

        if (bts)
        {
            BUTTONS_T *p = bts;

            while (p && p->next)
                p = p->next;

            p->next = chain;
            chain->previous = p;
        }
        else
            mButtons = chain;

        return chain;
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Memory error: " << e.what());
        TError::setError();
    }

    return nullptr;
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

bool TSubPage::hasButton(int id)
{
    DECL_TRACER("TSubPage::hasButton(int id)");

    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button && bt->button->getButtonIndex() == id)
            return true;

        bt = bt->next;
    }

    return false;
}

TButton *TSubPage::getButton(int id)
{
    DECL_TRACER("TSubPage::getButton(int id)");

    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button && bt->button->getButtonIndex() == id)
            return bt->button;

        bt = bt->next;
    }

    return nullptr;
}

vector<TButton *> TSubPage::getButtons(int ap, int ad)
{
    DECL_TRACER("TSubPage::getButtons(int ap, int ad)");

    vector<TButton *> list;
    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button->getAddressPort() == ap && bt->button->getAddressChannel() == ad)
            list.push_back(bt->button);

        bt = bt->next;
    }

    return list;
}

std::vector<TButton *> TSubPage::getAllButtons()
{
    DECL_TRACER("TSubPage::getAllButtons()");

    vector<TButton *> list;
    BUTTONS_T *bt = mButtons;

    while(bt)
    {
        list.push_back(bt->button);
        bt = bt->next;
    }

    return list;
}

/*
 * Sort the button according to their Z-order.
 * The button with the highest Z-order will be the last button in the chain.
 * The algorithm is a bubble sort algorithm.
 */
bool TSubPage::sortButtons()
{
    DECL_TRACER("TSubPage::sortButtons()");

    bool turned = true;

    while (turned)
    {
        BUTTONS_T *button = mButtons;
        turned = false;

        while (button)
        {
            int zo = button->button->getZOrder();

            if (button->previous)
            {
                if (zo < button->previous->button->getZOrder())
                {
                    BUTTONS_T *pprev = button->previous->previous;
                    BUTTONS_T *prev = button->previous;
                    BUTTONS_T *next = button->next;

                    if (pprev)
                        pprev->next = button;

                    prev->next = next;
                    prev->previous = button;
                    button->next = prev;
                    button->previous = pprev;

                    if (!pprev)
                        mButtons = button;

                    button = next;

                    if (next)
                        next->previous = prev;

                    turned = true;
                    continue;
                }
            }

            button = button->next;
        }
    }

    return true;
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

int TSubPage::numberLines(const string& str)
{
    DECL_TRACER("TButton::numberLines(const string& str)");

    int lines = 1;

    for (size_t i = 0; i < str.length(); i++)
    {
        if (str.at(i) == '\n')
            lines++;
    }

    return lines;
}

int TSubPage::calcLineHeight(string text, SkFont& font)
{
    DECL_TRACER("TButton::calcLineHeight(string text, SkFont& font)");

    sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(text.c_str(), font);
    SkRect rect = blob.get()->bounds();
    return rect.height();
}

Button::POSITION_t TSubPage::calcImagePosition(int width, int height, Button::CENTER_CODE cc, int line)
{
    DECL_TRACER("TButton::calcImagePosition(int with, int height, CENTER_CODE code, int number)");

    SR_T act_sr;
    POSITION_t position;
    int ix, iy;

    if (mSubpage.sr.size() == 0)
        return position;

    act_sr = mSubpage.sr.at(0);
//    int border_size = getBorderSize(act_sr.bs);
    int border_size = 0;
    int code, border = border_size;
    string dbgCC;
    int rwt = 0, rht = 0;

    switch (cc)
    {
        case SC_ICON:
            code = act_sr.ji;
            ix = act_sr.ix;
            iy = act_sr.iy;
            border = border_size = 0;
            dbgCC = "ICON";
            rwt = width;
            rht = height;
            break;

        case SC_BITMAP:
            code = act_sr.jb;
            ix = act_sr.bx;
            iy = act_sr.by;
            dbgCC = "BITMAP";
            rwt = std::min(mSubpage.width - border * 2, width);
            rht = std::min(mSubpage.height - border_size * 2, height);
            break;

        case SC_TEXT:
            code = act_sr.jt;
            ix = act_sr.tx;
            iy = act_sr.ty;
            dbgCC = "TEXT";
            border += 4;
            rwt = std::min(mSubpage.width - border * 2, width);
            rht = std::min(mSubpage.height - border_size * 2, height);
            break;
    }

    if (width > rwt || height > rht)
        position.overflow = true;

    switch (code)
    {
        case 0: // absolute position
            position.left = ix;

            if (cc == SC_TEXT)
                position.top = iy + height * line;
            else
                position.top = iy;

            if (cc == SC_BITMAP && ix < 0 && rwt < width)
                position.left *= -1;

            if (cc == SC_BITMAP && iy < 0 && rht < height)
                position.top += -1;

            position.width = rwt;
            position.height = rht;
            break;

        case 1: // top, left
            if (cc == SC_TEXT)
            {
                position.left = border;
                position.top = height * line;
            }

            position.width = rwt;
            position.height = rht;
            break;

        case 2: // center, top
            if (cc == SC_TEXT)
                position.top = height * line;

            position.left = (mSubpage.width - rwt) / 2;
            position.height = rht;
            position.width = rwt;
            break;

        case 3: // right, top
            position.left = mSubpage.width - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = height * line;
            }

            position.width = rwt;
            position.height = rht;
            break;

        case 4: // left, middle
            if (cc == SC_TEXT)
            {
                position.left = border;
                position.top = ((mSubpage.height - rht) / 2) + (height / 2 * line);
            }
            else
                position.top = (mSubpage.height - rht) / 2;

            position.width = rwt;
            position.height = rht;
            break;

        case 6: // right, middle
            position.left = mSubpage.width - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = ((mSubpage.height - rht) / 2) + (height / 2 * line);
            }
            else
                position.top = (mSubpage.height - rht) / 2;

            position.width = rwt;
            position.height = rht;
            break;

        case 7: // left, bottom
            if (cc == SC_TEXT)
            {
                position.left = border_size;
                position.top = (mSubpage.height - rht) - height * line;
            }
            else
                position.top = mSubpage.height - rht;

            position.width = rwt;
            position.height = rht;
            break;

        case 8: // center, bottom
            position.left = (mSubpage.width - rwt) / 2;

            if (cc == SC_TEXT)
                position.top = (mSubpage.height - rht) - height * line;
            else
                position.top = mSubpage.height - rht;

            position.width = rwt;
            position.height = rht;
            break;

        case 9: // right, bottom
            position.left = mSubpage.width - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = (mSubpage.height - rht) - height * line;
            }
            else
                position.top = mSubpage.height - rht;
            break;

        default: // center, middle
            position.left = (mSubpage.width - rwt) / 2;

            if (cc == SC_TEXT)
                position.top = ((mSubpage.height - rht) / 2) + (height / 2 * line);
            else
                position.top = (mSubpage.height - rht) / 2;

            position.width = rwt;
            position.height = rht;
    }

    MSG_DEBUG("Type: " << dbgCC << ", PosType=" << code << ", Position: x=" << position.left << ", y=" << position.top << ", w=" << position.width << ", h=" << position.height << ", Overflow: " << (position.overflow ? "YES" : "NO"));
    position.valid = true;
    return position;
}

bool TSubPage::drawText(SkBitmap *img)
{
    MSG_TRACE("TSubPage::drawText(SkImage& img)");

    if (mSubpage.sr[0].te.empty())
        return true;

    MSG_DEBUG("Searching for font number " << mSubpage.sr[0].fi << " with text " << mSubpage.sr[0].te);
    FONT_T font = mFonts->getFont(mSubpage.sr[0].fi);

    if (!font.file.empty())
    {
        SkCanvas canvas(*img, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
        sk_sp<SkTypeface> typeFace = mFonts->getTypeFace(mSubpage.sr[0].fi);

        if (!typeFace)
        {
            MSG_ERROR("Error creating type face " << font.fullName);
            TError::setError();
            return false;
        }

        SkScalar fontSizePt = ((SkScalar)font.size * 1.322);
        SkFont skFont(typeFace, fontSizePt);

        SkPaint paint;
        paint.setAntiAlias(true);
        SkColor color = TColor::getSkiaColor(mSubpage.sr[0].ct);
        paint.setColor(color);
        paint.setStyle(SkPaint::kFill_Style);

        SkFontMetrics metrics;
        skFont.getMetrics(&metrics);
        int lines = numberLines(mSubpage.sr[0].te);

        if (lines > 1 || mSubpage.sr[0].ww)
        {
            vector<string> textLines;

            if (!mSubpage.sr[0].ww)
                textLines = ::splitLine(mSubpage.sr[0].te);
            else
            {
                textLines = splitLine(mSubpage.sr[0].te, mSubpage.width, mSubpage.height, skFont, paint);
                lines = textLines.size();
            }

            int lineHeight = calcLineHeight(mSubpage.sr[0].te, skFont);
            int totalHeight = lineHeight * lines;

            if (totalHeight > mSubpage.height)
            {
                lines = mSubpage.height / lineHeight;
                totalHeight = lineHeight * lines;
            }

            MSG_DEBUG("Line height: " << lineHeight);
            POSITION_t position = calcImagePosition(mSubpage.width, totalHeight, SC_TEXT, 1);
            MSG_DEBUG("Position frame: l: " << position.left << ", t: " << position.top << ", w: " << position.width << ", h: " << position.height);

            if (!position.valid)
            {
                MSG_ERROR("Error calculating the text position!");
                TError::setError();
                return false;
            }

            vector<string>::iterator iter;
            int line = 0;

            for (iter = textLines.begin(); iter != textLines.end(); iter++)
            {
                sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(iter->c_str(), skFont);
                SkRect rect;
                skFont.measureText(iter->c_str(), iter->length(), SkTextEncoding::kUTF8, &rect, &paint);
                POSITION_t pos = calcImagePosition(rect.width(), lineHeight, SC_TEXT, 1);

                if (!pos.valid)
                {
                    MSG_ERROR("Error calculating the text position!");
                    TError::setError();
                    return false;
                }
                MSG_DEBUG("Triing to print line: " << *iter);

                SkScalar startX = (SkScalar)pos.left;
                SkScalar startY = (SkScalar)position.top + lineHeight * line;
                MSG_DEBUG("x=" << startX << ", y=" << startY);
                canvas.drawTextBlob(blob, startX, startY + lineHeight / 2 + 4, paint);
                line++;

                if (line > lines)
                    break;
            }
        }
        else    // single line
        {
            sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(mSubpage.sr[0].te.c_str(), skFont);
            SkRect rect;
            skFont.measureText(mSubpage.sr[0].te.c_str(), mSubpage.sr[0].te.length(), SkTextEncoding::kUTF8, &rect, &paint);
            POSITION_t position = calcImagePosition(rect.width(), (rect.height() * (float)lines), SC_TEXT, 0);

            if (!position.valid)
            {
                MSG_ERROR("Error calculating the text position!");
                TError::setError();
                return false;
            }

            MSG_DEBUG("Printing line " << mSubpage.sr[0].te);
            SkScalar startX = (SkScalar)position.left;
            SkScalar startY = (SkScalar)position.top + metrics.fCapHeight; // + metrics.fLeading; // (metrics.fAscent * -1.0);
            canvas.drawTextBlob(blob, startX, startY, paint);
        }
    }
    else
    {
        MSG_WARNING("No font file name found for font " << mSubpage.sr[0].fi);
    }

    return true;
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

    BUTTONS_T *button = mButtons;
    // Find last button
    while (button && button->next)
        button = button->next;

    // Scan in reverse order
    while (button)
    {
        TButton *but = button->button;
        bool clickable = but->isClickable();
        MSG_DEBUG("Testing button " << but->getButtonIndex() << " (" << but->getButtonName() << "): " << (clickable ? "CLICKABLE" : "NOT CLICKABLE"));

        if (x > but->getLeftPosition() && x < (but->getLeftPosition() + but->getWidth()) &&
            y > but->getTopPosition() && y < (but->getTopPosition() + but->getHeight()))
        {
            MSG_DEBUG("Clicking button " << but->getButtonIndex() << ": " << but->getButtonName() << " to state " << (pressed ? "PRESS" : "RELEASE"));
            int btX = x - but->getLeftPosition();
            int btY = y - but->getTopPosition();

            if (but->doClick(btX, btY, pressed))
                break;
        }
        else if (clickable)
        {
            MSG_DEBUG("Button out of position: L " << but->getLeftPosition() << ", R " << (but->getLeftPosition() + but->getWidth()) << ", T " << but->getTopPosition() << ", B " << (but->getTopPosition() + but->getHeight()) << " --> " << x << ":" << y);
        }

        button = button->previous;
    }
}

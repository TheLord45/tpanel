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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>

#include "tresources.h"
#include "tpagemanager.h"
#include "tpage.h"
#include "tdrawimage.h"
#include "texpat++.h"
#include "tobject.h"

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

    MSG_DEBUG("Destroing page " << pageID << ": " << name);
    BUTTONS_T *p = mButtons;
    BUTTONS_T *next = nullptr;

    while (p)
    {
        next = p->next;
        delete p->button;
        delete p;
        p = next;
    }

    mButtons = nullptr;

    PAGECHAIN_T *pc = mSubPages;
    PAGECHAIN_T *pc_next = nullptr;

    // We're not allowd to delete the subpages here, because they're managed
    // by the TPageManager.
    while (pc)
    {
        pc_next = pc->next;
        delete pc;
        pc = pc_next;
    }

    mSubPages = nullptr;
}

void TPage::initialize(const string& nm)
{
    DECL_TRACER("TPage::initialize(const string& name)");
    makeFileName(TConfig::getProjectPath(), nm);

    if (isValidFile())
        mPath = getFileName();

    TExpat xml(mPath);
    xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return;

    int depth = 0;
    size_t index = 0;
    size_t oldIndex = 0;
    vector<ATTRIBUTE_t> attrs;

    if ((index = xml.getElementIndex("page", &depth)) == TExpat::npos)
    {
        MSG_ERROR("Element \"page\" with attribute \"type\" was not found!");
        TError::setError();
        return;
    }

    attrs = xml.getAttributes();
    string type = xml.getAttribute("type", attrs);

    if (type.compare("page") != 0)
    {
        MSG_ERROR("Invalid page type \"" << type << "\"!");
        TError::setError();
        return;
    }

    depth++;
    string ename, content;

    while ((index = xml.getNextElementFromIndex(index, &ename, &content, &attrs)) != TExpat::npos)
    {
        string e = ename;

        if (e.compare("pageID") == 0)
            pageID = xml.convertElementToInt(content);
        else if (e.compare("name") == 0)
            name = content;
        else if (e.compare("width") == 0)
            width = xml.convertElementToInt(content);
        else if (e.compare("height") == 0)
            height = xml.convertElementToInt(content);
        else if (e.compare("button") == 0)
        {
            TButton *button = new TButton();
            button->setPalette(mPalette);
            button->setFonts(mFonts);
            button->registerCallback(_displayButton);
            button->regCallPlayVideo(_playVideo);
            index = button->initialize(&xml, index);
            button->setParentSize(width, height);

            if (TError::isError())
            {
                MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
                delete button;
                return;
            }

            button->setHandle(((pageID << 16) & 0xffff0000) | button->getButtonIndex());
            button->createButtons();
            addButton(button);
            index++;        // Jump over the end tag of the button.
        }
        else if (e.compare("sr") == 0)
        {
            SR_T bsr;
            bsr.number = xml.getAttributeInt("number", attrs);
            index++;

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
                    bsr.jt = (Button::TEXT_ORIENTATION)xml.convertElementToInt(content);
                else if (ename.compare("jb") == 0)
                    bsr.jb = (Button::TEXT_ORIENTATION)xml.convertElementToInt(content);

                oldIndex = index;
            }

            sr.push_back(bsr);

            if (index == TExpat::npos)
                index = oldIndex + 1;
        }
    }

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        MSG_DEBUG("PageID: " << pageID);
        MSG_DEBUG("Name  : " << name);
        MSG_DEBUG("Width : " << width);
        MSG_DEBUG("Height: " << height);

        vector<SR_T>::iterator iter;
        size_t pos = 1;

        for (iter = sr.begin(); iter != sr.end(); ++iter)
        {
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

    if (mButtons)
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
    pageID = 300;
    name = "_progress";
    width = gPageManager->getSettings()->getWith();
    height = gPageManager->getSettings()->getHeight();
    double unit = (double)height / 10.0;
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
    bt.type = Button::GENERAL;
    bt.bi = 1;
    bt.na = "Line1";
    bt.tp = (int)(unit * 2.0);
    bt.lt = (int)(((double)width - ((double)width / 100.0 * 80.0)) / 2.0);
    bt.wt = (int)((double)width / 100.0 * 80.0);    // Line take 80% of available width
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
    button->setFonts(mFonts);
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(width, height);

    if (TError::isError())
    {
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((pageID << 16) & 0xffff0000) | bt.bi);
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
    button->setFonts(mFonts);
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(width, height);

    if (TError::isError())
    {
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((pageID << 16) & 0xffff0000) | bt.bi);
    button->createButtons();
    addButton(button);
    // Progress bar 1 (overall status)
    bt.type = Button::BARGRAPH;
    bt.bi = 3;
    bt.na = "Bar1";
    bt.tp = (int)(unit * 3.0);
    bt.lt = (int)(((double)width - ((double)width / 100.0 * 80.0)) / 2.0);
    bt.wt = (int)((double)width / 100.0 * 80.0);    // Line take 80% of available width
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
    button->setFonts(mFonts);
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(width, height);

    if (TError::isError())
    {
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((pageID << 16) & 0xffff0000) | bt.bi);
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
    button->setFonts(mFonts);
    button->registerCallback(_displayButton);
    button->regCallPlayVideo(_playVideo);
    button->createSoftButton(bt);
    button->setParentSize(width, height);

    if (TError::isError())
    {
        MSG_WARNING("Button \"" << button->getButtonName() << "\" deleted because of an error!");
        delete button;
        return;
    }

    button->setHandle(((pageID << 16) & 0xffff0000) | bt.bi);
    button->createButtons();
    addButton(button);
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
            return;
        }
    }

    bool haveImage;
    ulong handle = (pageID << 16) & 0xffff0000;
    MSG_DEBUG("Processing page " << pageID);
    SkBitmap target;
    target.allocN32Pixels(width, height);
    target.eraseColor(TColor::getSkiaColor(sr[0].cf));
    // Draw the background, if any
    if (sr.size() > 0 && (!sr[0].bm.empty() || !sr[0].mi.empty()))
    {
        TDrawImage dImage;
        dImage.setWidth(width);
        dImage.setHeight(height);

        if (!sr[0].bm.empty())
        {
            MSG_DEBUG("Loading image " << sr[0].bm);
            sk_sp<SkData> rawImage = readImage(sr[0].bm);
            SkBitmap bm;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image BM ...");

                if (!DecodeDataToBitmap(rawImage, &bm))
                {
                    MSG_WARNING("Problem while decoding image " << sr[0].bm);
                }
                else if (!bm.empty())
                {
                    dImage.setImageBm(bm);
                    SkImageInfo info = bm.info();
                    sr[0].bm_width = info.width();
                    sr[0].bm_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("BM image " << sr[0].bm << " seems to be empty!");
                }
            }
        }

        if (!sr[0].mi.empty())
        {
            MSG_DEBUG("Loading image " << sr[0].mi);
            sk_sp<SkData> rawImage = readImage(sr[0].mi);
            SkBitmap mi;

            if (rawImage)
            {
                MSG_DEBUG("Decoding image MI ...");

                if (!DecodeDataToBitmap(rawImage, &mi))
                {
                    MSG_WARNING("Problem while decoding image " << sr[0].mi);
                }
                else if (!mi.empty())
                {
                    dImage.setImageMi(mi);
                    SkImageInfo info = mi.info();
                    sr[0].mi_width = info.width();
                    sr[0].mi_height = info.height();
                    haveImage = true;
                }
                else
                {
                    MSG_WARNING("MI image " << sr[0].mi << " seems to be empty!");
                }
            }
        }

        if (haveImage)
        {
            dImage.setSr(sr);

            if (!dImage.drawImage(&target))
                return;

            if (!sr[0].te.empty())
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
                int twidth = (int)((double)width * scaleFactor);
                int theight = (int)((double)height * scaleFactor);
                calcPosition(lwidth, lheight, &left, &top);
                // Create a canvas and draw new image
                sk_sp<SkImage> im = SkImage::MakeFromBitmap(target);
                target.allocN32Pixels(twidth, theight);
                target.eraseColor(TColor::getSkiaColor(sr[0].cf));
                SkCanvas can(target, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
                SkRect rect = SkRect::MakeXYWH(left, top, lwidth, lheight);
                can.drawImageRect(im, rect, &paint);
                rowBytes = target.info().minRowBytes();
                size = target.info().computeByteSize(rowBytes);
                MSG_DEBUG("Scaled size of background image: " << left << ", " << top << ", " << lwidth << ", " << lheight);
            }
#endif
            if (sr[0].te.empty())
                _setBackground(handle, (unsigned char *)target.getPixels(), size, rowBytes, target.info().width(), target.info().height(), TColor::getColor(sr[0].cf));
        }
    }

    if (sr.size() > 0 && !sr[0].te.empty())
    {
        MSG_DEBUG("Drawing text on background image ...");

        if (!drawText(&target))
            return;

        SkImageInfo info = target.info();
        size_t rowBytes = info.minRowBytes();
        size_t size = info.computeByteSize(rowBytes);
        rowBytes = target.info().minRowBytes();
        size = target.info().computeByteSize(rowBytes);
        _setBackground(handle, (unsigned char *)target.getPixels(), size, rowBytes, target.info().width(), target.info().height(), TColor::getColor(sr[0].cf));
        haveImage = true;
    }

    if (sr.size() > 0 && !haveImage)
    {
        MSG_DEBUG("Calling \"setBackground\" with no image ...");
        _setBackground(handle, nullptr, 0, 0, 0, 0, TColor::getColor(sr[0].cf));
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

            if (sr.size() > 0)
                button->button->setGlobalOpacity(sr[0].oo);

            button->button->show();
        }

        button = button->next;
    }

    // Mark page as visible
    mVisible = true;
}

int TPage::numberLines(const string& str)
{
    DECL_TRACER("TPage::numberLines(const string& str)");

    int lines = 1;

    for (size_t i = 0; i < str.length(); i++)
    {
        if (str.at(i) == '\n')
            lines++;
    }

    return lines;
}

int TPage::calcLineHeight(const string& text, SkFont& font)
{
    DECL_TRACER("TButton::calcLineHeight(const string& text, SkFont& font)");

    sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(text.c_str(), font);
    SkRect rect = blob.get()->bounds();
    return rect.height();
}

Button::POSITION_t TPage::calcImagePosition(int width, int height, Button::CENTER_CODE cc, int line)
{
    DECL_TRACER("TButton::calcImagePosition(int with, int height, CENTER_CODE code, int number)");

    SR_T act_sr;
    POSITION_t position;
    int ix, iy;

    if (sr.size() == 0)
        return position;

    act_sr = sr.at(0);
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
            rwt = std::min(this->width - border * 2, width);
            rht = std::min(this->height - border_size * 2, height);
            break;

        case SC_TEXT:
            code = act_sr.jt;
            ix = act_sr.tx;
            iy = act_sr.ty;
            dbgCC = "TEXT";
            border += 4;
            rwt = std::min(this->width - border * 2, width);
            rht = std::min(this->height - border_size * 2, height);
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

            position.left = (this->width - rwt) / 2;
            position.height = rht;
            position.width = rwt;
            break;

        case 3: // right, top
            position.left = this->width - rwt;

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
                position.top = ((this->height - rht) / 2) + (height / 2 * line);
            }
            else
                position.top = (this->height - rht) / 2;

            position.width = rwt;
            position.height = rht;
            break;

        case 6: // right, middle
            position.left = this->width - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = ((this->height - rht) / 2) + (height / 2 * line);
            }
            else
                position.top = (this->height - rht) / 2;

            position.width = rwt;
            position.height = rht;
            break;

        case 7: // left, bottom
            if (cc == SC_TEXT)
            {
                position.left = border_size;
                position.top = (this->height - rht) - height * line;
            }
            else
                position.top = this->height - rht;

            position.width = rwt;
            position.height = rht;
            break;

        case 8: // center, bottom
            position.left = (this->width - rwt) / 2;

            if (cc == SC_TEXT)
                position.top = (this->height - rht) - height * line;
            else
                position.top = this->height - rht;

            position.width = rwt;
            position.height = rht;
            break;

        case 9: // right, bottom
            position.left = this->width - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = (this->height - rht) - height * line;
            }
            else
                position.top = this->height - rht;
            break;

        default: // center, middle
            position.left = (this->width - rwt) / 2;

            if (cc == SC_TEXT)
                position.top = ((this->height - rht) / 2) + (height / 2 * line);
            else
                position.top = (this->height - rht) / 2;

            position.width = rwt;
            position.height = rht;
    }

    MSG_DEBUG("Type: " << dbgCC << ", PosType=" << code << ", Position: x=" << position.left << ", y=" << position.top << ", w=" << position.width << ", h=" << position.height << ", Overflow: " << (position.overflow ? "YES" : "NO"));
    position.valid = true;
    return position;
}

bool TPage::drawText(SkBitmap *img)
{
    MSG_TRACE("TSubPage::drawText(SkImage& img)");

    if (sr[0].te.empty())
        return true;

    MSG_DEBUG("Searching for font number " << sr[0].fi << " with text " << sr[0].te);
    FONT_T font = mFonts->getFont(sr[0].fi);

    if (!font.file.empty())
    {
        SkCanvas canvas(*img, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
        sk_sp<SkTypeface> typeFace = mFonts->getTypeFace(sr[0].fi);

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
        SkColor color = TColor::getSkiaColor(sr[0].ct);
        paint.setColor(color);
        paint.setStyle(SkPaint::kFill_Style);

        SkFontMetrics metrics;
        skFont.getMetrics(&metrics);
        int lines = numberLines(sr[0].te);

        if (lines > 1 || sr[0].ww)
        {
            vector<string> textLines;

            if (!sr[0].ww)
                textLines = ::splitLine(sr[0].te);
            else
            {
                textLines = splitLine(sr[0].te, width, height, skFont, paint);
                lines = textLines.size();
            }

            int lineHeight = calcLineHeight(sr[0].te, skFont);
            int totalHeight = lineHeight * lines;

            if (totalHeight > height)
            {
                lines = height / lineHeight;
                totalHeight = lineHeight * lines;
            }

            MSG_DEBUG("Line height: " << lineHeight);
            POSITION_t position = calcImagePosition(width, totalHeight, SC_TEXT, 1);
            MSG_DEBUG("Position frame: l: " << position.left << ", t: " << position.top << ", w: " << position.width << ", h: " << position.height);

            if (!position.valid)
            {
                MSG_ERROR("Error calculating the text position!");
                TError::setError();
                return false;
            }

            vector<string>::iterator iter;
            int line = 0;

            for (iter = textLines.begin(); iter != textLines.end(); ++iter)
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
            sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(sr[0].te.c_str(), skFont);
            SkRect rect;
            skFont.measureText(sr[0].te.c_str(), sr[0].te.length(), SkTextEncoding::kUTF8, &rect, &paint);
            POSITION_t position = calcImagePosition(rect.width(), (rect.height() * (float)lines), SC_TEXT, 0);

            if (!position.valid)
            {
                MSG_ERROR("Error calculating the text position!");
                TError::setError();
                return false;
            }

            MSG_DEBUG("Printing line " << sr[0].te);
            SkScalar startX = (SkScalar)position.left;
            SkScalar startY = (SkScalar)position.top + metrics.fCapHeight; // + metrics.fLeading; // (metrics.fAscent * -1.0);
            canvas.drawTextBlob(blob, startX, startY, paint);
        }
    }
    else
    {
        MSG_WARNING("No font file name found for font " << sr[0].fi);
    }

    return true;
}

PAGECHAIN_T *TPage::addSubPage(TSubPage* pg)
{
    DECL_TRACER("TPage::addSubPage(TSubPage* pg)");

    if (!pg)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::setError();
        return nullptr;
    }

    PAGECHAIN_T *chain = new PAGECHAIN_T;
    chain->subpage = pg;
    chain->next = nullptr;
    PAGECHAIN_T *spg = mSubPages;

    if (spg)
    {
        // First make sure that the new page is not already in the chain.
        PAGECHAIN_T *p = spg;

        while (p)
        {
            if (p->subpage->getNumber() == pg->getNumber())
            {
                MSG_TRACE("Page " << pg->getNumber() << " is already in chain. Don't add it again.");
                delete chain;
                return p;
            }

            p = p->next;
        }

        // The subpage is not in chain. So we add it now.
        p = spg;
        // Find the last element in chain
        while (p && p->next)
            p = p->next;

        p->next = chain;
    }
    else
    {
        mZOrder = 0;
        mSubPages = chain;
    }

    mLastSubPage = 0;
    return chain;
}

bool TPage::removeSubPage(int ID)
{
    DECL_TRACER("TPage::removeSubPage(int ID)");

    PAGECHAIN_T *p = mSubPages;
    PAGECHAIN_T *prev = nullptr;

    while (p)
    {
        if (p->subpage->getNumber() == ID)
        {
            PAGECHAIN_T *next = p->next;

            if (prev)
                prev->next = next;
            else
                mSubPages = next;

            delete p;
            mLastSubPage = 0;
            return true;
        }

        prev = p;
        p = p->next;
    }

    return false;
}

bool TPage::removeSubPage(const std::string& nm)
{
    DECL_TRACER("TPage::removeSubPage(const std::string& nm)");

    PAGECHAIN_T *p = mSubPages;
    PAGECHAIN_T *prev = nullptr;

    while (p)
    {
        if (p->subpage->getName().compare(nm) == 0)
        {
            PAGECHAIN_T *next = p->next;

            if (prev)
                prev->next = next;
            else
                mSubPages = next;

            delete p;
            mLastSubPage = 0;
            return true;
        }

        prev = p;
        p = p->next;
    }

    return false;
}

TSubPage *TPage::getSubPage(int pageID)
{
    DECL_TRACER("TPage::getSubPage(int pageID)");

    PAGECHAIN_T *pg = mSubPages;

    while (pg)
    {
        if (pg->subpage->getNumber() == pageID)
        {
            mLastSubPage = pageID;
            return pg->subpage;
        }

        pg = pg->next;
    }

    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPage::getSubPage(const std::string& name)
{
    DECL_TRACER("TPage::getSubPage(const std::string& name)");

    PAGECHAIN_T *pg = mSubPages;

    while (pg)
    {
        if (pg->subpage->getName().compare(name) == 0)
        {
            mLastSubPage = pg->subpage->getNumber();
            return pg->subpage;
        }

        pg = pg->next;
    }

    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPage::getFirstSubPage()
{
    DECL_TRACER("TPage::getFirstSubPage()");

    PAGECHAIN_T *pg = mSubPages;

    if (pg)
    {
        if (pg->subpage)
        {
            mLastSubPage = pg->subpage->getNumber();
            MSG_DEBUG("Subpage (Z: " << pg->subpage->getZOrder() << "): " << pg->subpage->getNumber() << ". " << pg->subpage->getName());
            return pg->subpage;
        }
    }

    MSG_DEBUG("No subpages in chain.");
    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPage::getNextSubPage()
{
    DECL_TRACER("TPage::getNextSubPage()");

    if (mLastSubPage > 0)
    {
        PAGECHAIN_T *p = mSubPages;

        while (p)
        {
            if (p->subpage->getNumber() == mLastSubPage)
            {
                if (p->next && p->next->subpage)
                {
                    TSubPage *page = p->next->subpage;
                    mLastSubPage = page->getNumber();
                    MSG_DEBUG("Subpage (Z: " << page->getZOrder() << "): " << page->getNumber() << ". " << page->getName());
                    return page;
                }
            }

            p = p->next;
        }
    }

    MSG_DEBUG("No more subpages in chain.");
    mLastSubPage = 0;
    return nullptr;
}

BUTTONS_T *TPage::addButton(TButton* button)
{
    DECL_TRACER("*TPage::addButton(TButton* button)");

    if (!button)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::setError();
        return nullptr;
    }

    BUTTONS_T *chain = new BUTTONS_T;
    BUTTONS_T *bts = mButtons;
    chain->button = button;
    chain->next = nullptr;
    chain->previous = nullptr;

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

bool TPage::hasButton(int id)
{
    DECL_TRACER("TPage::hasButton(int id)");

    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button && bt->button->getButtonIndex() == id)
            return true;

        bt = bt->next;
    }

    return false;
}

TButton *TPage::getButton(int id)
{
    DECL_TRACER("TPage::getButton(int id)");

    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button && bt->button->getButtonIndex() == id)
            return bt->button;

        bt = bt->next;
    }

    return nullptr;
}

std::vector<TButton *> TPage::getButtons(int ap, int ad)
{
    DECL_TRACER("TSubPage::getButtons(int ap, int ad)");

    std::vector<TButton *> list;
    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button->getAddressPort() == ap && bt->button->getAddressChannel() == ad)
            list.push_back(bt->button);

        bt = bt->next;
    }

    return list;
}

std::vector<TButton *> TPage::getAllButtons()
{
    DECL_TRACER("TPage::getAllButtons()");

    std::vector<TButton *> list;
    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        list.push_back(bt->button);
        bt = bt->next;
    }

    return list;
}

TButton *TPage::getFirstButton()
{
    DECL_TRACER("TButton::getFirstButton()");

    mLastButton = 0;

    if (mButtons)
        return mButtons->button;

    return nullptr;
}

TButton *TPage::getNextButton()
{
    DECL_TRACER("TPage::getNextButton()");

    BUTTONS_T *but = mButtons;
    int count = 0;
    mLastButton++;

    while (but)
    {
        if (but->button && count == mLastButton)
            return but->button;

        but = but->next;
        count++;
    }

    return nullptr;
}

TButton *TPage::getLastButton()
{
    DECL_TRACER("TPage::getLastButton()");

    BUTTONS_T *but = mButtons;
    mLastButton = 0;

    while (but && but->next)
    {
        mLastButton++;
        but = but->next;
    }

    return but->button;
}

TButton *TPage::getPreviousButton()
{
    DECL_TRACER("TPage::getPreviousButton()");

    BUTTONS_T *but = mButtons;
    int count = 0;

    if (mLastButton)
        mLastButton--;
    else
        return nullptr;

    while (but)
    {
        if (but->button && count == mLastButton)
            return but->button;

        but = but->next;
        count++;
    }

    return nullptr;
}

void TPage::drop()
{
    DECL_TRACER("TPage::drop()");

    PAGECHAIN_T *pc = mSubPages;

    // remove all subpages, if there are any
    while (pc)
    {
        MSG_DEBUG("Dropping popup " << pc->subpage->getNumber() << ": " << pc->subpage->getName());
        pc->subpage->drop();
        pc = pc->next;
    }

    // remove all buttons, if there are any
    BUTTONS_T *bt = mButtons;

    while (bt)
    {
        MSG_DEBUG("Dropping button " << TObject::handleToString(bt->button->getHandle()));
        bt->button->hide(true);
        bt = bt->next;
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

    int nw = width;
    int nh = height;
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

    map<int, TSubPage *> spages;
    PAGECHAIN_T *pages = mSubPages;
    PAGECHAIN_T *pgs = nullptr;

    while (pages)
    {
        spages.insert(pair<int, TSubPage *>(pages->subpage->getZOrder(), pages->subpage));
        pages = pages->next;
    }

    // Drop chain
    pages = mSubPages;

    while (pages)
    {
        pgs = pages->next;
        delete pages;
        pages = pgs;
    }

    mSubPages = nullptr;
    // Create the chain newly (ordered by Z-Order
    map<int, TSubPage *>::iterator iter;
    pgs = nullptr;

    for (iter = spages.begin(); iter != spages.end(); ++iter)
        addSubPage(iter->second);
}

/*
 * Sort the button according to their Z-order.
 * The button with the highest Z-order will be the last button in the chain.
 * The algorithm is a bubble sort algorithm.
 */
bool TPage::sortButtons()
{
    DECL_TRACER("TPage::sortButtons()");

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

int TPage::getNextZOrder()
{
    DECL_TRACER("TPage::getNextZOrder()");

    // Find highest z-order number
    PAGECHAIN_T *pages = mSubPages;
    int z = 0;

    while(pages)
    {
        int zo = pages->subpage->getZOrder();

        if (zo > z)
            z = zo;

        pages = pages->next;
    }

    mZOrder = z + 1;
    MSG_DEBUG("New Z-order: " << mZOrder);
    return mZOrder;
}

int TPage::decZOrder()
{
    DECL_TRACER("TPage::decZOrder()");

    // Find highest z-order number
    PAGECHAIN_T *pages = mSubPages;
    int z = 0;

    while(pages)
    {
        int zo = pages->subpage->getZOrder();

        if (zo > z)
            z = zo;

        pages = pages->next;
    }

    mZOrder = z;
    return mZOrder;
}

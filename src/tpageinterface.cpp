/*
 * Copyright (C) 2022 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

#include <include/core/SkFont.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkRegion.h>
#include <include/core/SkImageFilter.h>
#include <include/effects/SkImageFilters.h>

#include "tpageinterface.h"
#include "tsystemsound.h"
#include "ttpinit.h"
#include "tconfig.h"
#include "tresources.h"
#include "tpagemanager.h"
#include "tintborder.h"
#include "timgcache.h"
#include "terror.h"

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
using std::min;
using std::max;

bool TPageInterface::drawText(PAGE_T& pinfo, SkBitmap *img)
{
    MSG_TRACE("TPageInterface::drawText(PAGE_T& pinfo, SkImage& img)");

    if (pinfo.sr.empty() || pinfo.sr[0].te.empty())
        return true;

    MSG_DEBUG("Searching for font number " << pinfo.sr[0].fi << " with text " << pinfo.sr[0].te);
    FONT_T font = mFonts->getFont(pinfo.sr[0].fi);

    if (font.file.empty())
    {
        MSG_WARNING("No font file name found for font " << pinfo.sr[0].fi);
        return false;
    }

    SkCanvas canvas(*img, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
    sk_sp<SkTypeface> typeFace = mFonts->getTypeFace(pinfo.sr[0].fi);

    if (!typeFace)
    {
        MSG_ERROR("Error creating type face " << font.fullName);
        TError::SetError();
        return false;
    }

    SkScalar fontSizePt = ((SkScalar)font.size * 1.322);    // Calculate points from pixels (close up)
    SkFont skFont(typeFace, fontSizePt);                    // Skia require the font size in points

    SkPaint paint;
    paint.setAntiAlias(true);
    SkColor color = TColor::getSkiaColor(pinfo.sr[0].ct);
    paint.setColor(color);
    paint.setStyle(SkPaint::kFill_Style);

    SkFontMetrics metrics;
    skFont.getMetrics(&metrics);
    int lines = numberLines(pinfo.sr[0].te);

    if (lines > 1 || pinfo.sr[0].ww)
    {
        vector<string> textLines;

        if (!pinfo.sr[0].ww)
            textLines = splitLine(pinfo.sr[0].te);
        else
        {
            textLines = splitLine(pinfo.sr[0].te, pinfo.width, pinfo.height, skFont, paint);
            lines = (int)textLines.size();
        }

        MSG_DEBUG("Calculated number of lines: " << textLines.size());
        int lineHeight = calcLineHeight(pinfo.sr[0].te, skFont);
        int totalHeight = lineHeight * lines;

        if (totalHeight > pinfo.height)
        {
            lines = pinfo.height / lineHeight;
            totalHeight = lineHeight * lines;
        }

        MSG_DEBUG("Line height: " << lineHeight << ", total height: " << totalHeight);
        Button::POSITION_t position = calcImagePosition(&pinfo, pinfo.width, totalHeight, Button::SC_TEXT, 0);
        MSG_DEBUG("Position frame: l: " << position.left << ", t: " << position.top << ", w: " << position.width << ", h: " << position.height);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the text position!");
            TError::SetError();
            return false;
        }

        vector<string>::iterator iter;
        int line = 0;

        for (iter = textLines.begin(); iter != textLines.end(); iter++)
        {
            sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(iter->c_str(), skFont);
            SkRect rect;
            skFont.measureText(iter->c_str(), iter->length(), SkTextEncoding::kUTF8, &rect, &paint);
            Button::POSITION_t pos = calcImagePosition(&pinfo, rect.width(), lineHeight, Button::SC_TEXT, 1);

            if (!pos.valid)
            {
                MSG_ERROR("Error calculating the text position!");
                TError::SetError();
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
        sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(pinfo.sr[0].te.c_str(), skFont);
        SkRect rect;
        skFont.measureText(pinfo.sr[0].te.c_str(), pinfo.sr[0].te.length(), SkTextEncoding::kUTF8, &rect, &paint);
        Button::POSITION_t position = calcImagePosition(&pinfo, rect.width(), (rect.height() * (float)lines), Button::SC_TEXT, 0);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the text position!");
            TError::SetError();
            return false;
        }

        MSG_DEBUG("Printing line " << pinfo.sr[0].te);
        SkScalar startX = (SkScalar)position.left;
        SkScalar startY = (SkScalar)position.top + metrics.fCapHeight; // + metrics.fLeading; // (metrics.fAscent * -1.0);
        canvas.drawTextBlob(blob, startX, startY, paint);
    }

    return true;
}

bool TPageInterface::drawFrame(PAGE_T& pinfo, SkBitmap* bm)
{
    DECL_TRACER("TPageInterface::drawFrame(PAGE_T& pinfo, SkBitmap* bm)");

    int instance = 0;

    if (pinfo.sr[instance].bs.empty())
    {
        MSG_DEBUG("No border defined.");
        return false;
    }

    // First we look into our internal border table
    Border::TIntBorder *intBorder = new Border::TIntBorder;

    if (intBorder && intBorder->drawBorder(bm, pinfo.sr[instance].bs, pinfo.width, pinfo.height, pinfo.sr[instance].cb))
    {
        delete intBorder;
        return true;
    }

    if (intBorder)
    {
        delete intBorder;
        intBorder = nullptr;
    }

    // Try to find the border in the system table
    BORDER_t bd;
    bool classExist = (gPageManager && gPageManager->getSystemDraw());

    if (!classExist)
        return false;

    string borderName = pinfo.sr[0].bs;

    if (!gPageManager->getSystemDraw()->getBorder(borderName, TSystemDraw::LT_OFF, &bd, borderName))
        return false;

    MSG_DEBUG("System border \"" << borderName << "\" found.");
    SkColor color = TColor::getSkiaColor(pinfo.sr[instance].cb);      // border color
    MSG_DEBUG("Button color: #" << std::setw(6) << std::setfill('0') << std::hex << color << std::dec);
    // Load images
    SkBitmap imgB, imgBR, imgR, imgTR, imgT, imgTL, imgL, imgBL;

    if (!getBorderFragment(bd.b, bd.b_alpha, &imgB, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.b << "\" and \"" << bd.b_alpha << "\" with size " << imgB.info().width() << " x " << imgB.info().height());
    if (!getBorderFragment(bd.br, bd.br_alpha, &imgBR, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.br << "\" and \"" << bd.br_alpha << "\" with size " << imgBR.info().width() << " x " << imgBR.info().height());
    if (!getBorderFragment(bd.r, bd.r_alpha, &imgR, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.r << "\" and \"" << bd.r_alpha << "\" with size " << imgR.info().width() << " x " << imgR.info().height());
    if (!getBorderFragment(bd.tr, bd.tr_alpha, &imgTR, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.tr << "\" and \"" << bd.tr_alpha << "\" with size " << imgTR.info().width() << " x " << imgTR.info().height());
    if (getBorderFragment(bd.t, bd.t_alpha, &imgT, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.t << "\" and \"" << bd.t_alpha << "\" with size " << imgT.info().width() << " x " << imgT.info().height());
    if (!getBorderFragment(bd.tl, bd.tl_alpha, &imgTL, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.tl << "\" and \"" << bd.tl_alpha << "\" with size " << imgTL.info().width() << " x " << imgTL.info().height());
    if (!getBorderFragment(bd.l, bd.l_alpha, &imgL, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.l << "\" and \"" << bd.l_alpha << "\" with size " << imgL.info().width() << " x " << imgL.info().height());
    if (!getBorderFragment(bd.bl, bd.bl_alpha, &imgBL, color))
        return false;

    MSG_DEBUG("Got images \"" << bd.bl << "\" and \"" << bd.bl_alpha << "\" with size " << imgBL.info().width() << " x " << imgBL.info().height());
    MSG_DEBUG("Button image size: " << (imgTL.info().width() + imgT.info().width() + imgTR.info().width()) << " x " << (imgTL.info().height() + imgL.info().height() + imgBL.info().height()));
    MSG_DEBUG("Total size: " << pinfo.width << " x " << pinfo.height);
    stretchImageWidth(&imgB, pinfo.width - imgBL.info().width() - imgBR.info().width());
    stretchImageWidth(&imgT, pinfo.width - imgTL.info().width() - imgTR.info().width());
    stretchImageHeight(&imgL, pinfo.height - imgTL.info().height() - imgBL.info().height());
    stretchImageHeight(&imgR, pinfo.height - imgTR.info().height() - imgBR.info().height());
    MSG_DEBUG("Stretched button image size: " << (imgTL.info().width() + imgT.info().width() + imgTR.info().width()) << " x " << (imgTL.info().height() + imgL.info().height() + imgBL.info().height()));
    // Draw the frame
    SkBitmap frame;
    allocPixels(bm->info().width(), bm->info().height(), &frame);
    frame.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas target(*bm, SkSurfaceProps());
    SkCanvas canvas(frame, SkSurfaceProps());
    SkPaint paint;

    paint.setBlendMode(SkBlendMode::kSrcOver);
    paint.setAntiAlias(true);
    sk_sp<SkImage> _image = SkImages::RasterFromBitmap(imgB);   // bottom
    canvas.drawImage(_image, imgBL.info().width(), pinfo.height - imgB.info().height(), SkSamplingOptions(), &paint);
    _image = SkImages::RasterFromBitmap(imgT);                  // top
    canvas.drawImage(_image, imgTL.info().width(), 0, SkSamplingOptions(), &paint);
    _image = SkImages::RasterFromBitmap(imgBR);                 // bottom right
    canvas.drawImage(_image, pinfo.width - imgBR.info().width(), pinfo.height - imgBR.info().height(), SkSamplingOptions(), &paint);
    _image = SkImages::RasterFromBitmap(imgTR);                 // top right
    canvas.drawImage(_image, pinfo.width - imgTR.info().width(), 0, SkSamplingOptions(), &paint);
    _image = SkImages::RasterFromBitmap(imgTL);                 // top left
    canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    _image = SkImages::RasterFromBitmap(imgBL);                 // bottom left
    canvas.drawImage(_image, 0, pinfo.height - imgBL.info().height(), SkSamplingOptions(), &paint);
    _image = SkImages::RasterFromBitmap(imgL);                  // left
    canvas.drawImage(_image, 0, imgTL.info().height(), SkSamplingOptions(), &paint);
    _image = SkImages::RasterFromBitmap(imgR);                  // right
    canvas.drawImage(_image, pinfo.width - imgR.info().width(), imgTR.info().height(), SkSamplingOptions(), &paint);

    Border::TIntBorder iborder;
    iborder.erasePart(bm, frame, Border::ERASE_OUTSIDE, imgL.info().width());
    _image = SkImages::RasterFromBitmap(frame);
    paint.setBlendMode(SkBlendMode::kSrcATop);
    target.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    return true;
}

Button::POSITION_t TPageInterface::calcImagePosition(PAGE_T *page, int width, int height, Button::CENTER_CODE cc, int line)
{
    DECL_TRACER("TPageInterface::calcImagePosition(PAGE_T *page, int with, int height, CENTER_CODE code, int number)");

    if (!page)
        return Button::POSITION_t();

    Button::SR_T act_sr;
    Button::POSITION_t position;
    int ix, iy;

    if (page->sr.size() == 0)
    {
        if (sr.size() == 0)
            return position;

        act_sr = sr.at(0);
    }
    else
        act_sr = page->sr.at(0);

    //    int border_size = getBorderSize(act_sr.bs);
    int border_size = 0;
    int code, border = border_size;
    string dbgCC;
    int rwt = 0, rht = 0;

    switch (cc)
    {
        case Button::SC_ICON:
            code = act_sr.ji;
            ix = act_sr.ix;
            iy = act_sr.iy;
            border = border_size = 0;
            dbgCC = "ICON";
            rwt = width;
            rht = height;
            break;

        case Button::SC_BITMAP:
            code = act_sr.jb;
            ix = act_sr.bx;
            iy = act_sr.by;
            dbgCC = "BITMAP";
            rwt = std::min(page->width - border * 2, width);
            rht = std::min(page->height - border_size * 2, height);
            break;

        case Button::SC_TEXT:
            code = act_sr.jt;
            ix = act_sr.tx;
            iy = act_sr.ty;
            dbgCC = "TEXT";
            border += 4;
            rwt = std::min(page->width - border * 2, width);
            rht = std::min(page->height - border_size * 2, height);
            break;
    }

    if (width > rwt || height > rht)
        position.overflow = true;

    switch (code)
    {
        case 0: // absolute position
            position.left = ix;

            if (cc == Button::SC_TEXT && line > 0)
                position.top = iy + height * line;
            else
                position.top = iy;

            if (cc == Button::SC_BITMAP && ix < 0 && rwt < width)
                position.left *= -1;

            if (cc == Button::SC_BITMAP && iy < 0 && rht < height)
                position.top += -1;

            position.width = rwt;
            position.height = rht;
        break;

        case 1: // top, left
            if (cc == Button::SC_TEXT)
            {
                position.left = border;

                if (line > 0)
                    position.top = height * line;
                else
                    position.top = border;
            }

            position.width = rwt;
            position.height = rht;
        break;

        case 2: // center, top
            if (cc == Button::SC_TEXT)
            {
                if (line > 0)
                    position.top = height * line;
                else
                    position.top = border;
            }

            position.left = (page->width - rwt) / 2;
            position.height = rht;
            position.width = rwt;
        break;

        case 3: // right, top
            position.left = page->width - rwt;

            if (cc == Button::SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);

                if (line > 0)
                    position.top = height * line;
                else
                    position.top = border;
            }

            position.width = rwt;
            position.height = rht;
        break;

        case 4: // left, middle
            if (cc == Button::SC_TEXT)
            {
                position.left = border;

                if (line > 0)
                    position.top = ((page->height - rht) / 2) + (height / 2 * line);
                else
                    position.top = (page->height - rht) / 2;
            }
            else
                position.top = (page->height - rht) / 2;

            position.width = rwt;
            position.height = rht;
        break;

        case 6: // right, middle
            position.left = page->width - rwt;

            if (cc == Button::SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);

                if (line > 0)
                    position.top = ((page->height - rht) / 2) + (height / 2 * line);
                else
                    position.top = (page->height - rht) / 2;
            }
            else
                position.top = (page->height - rht) / 2;

            position.width = rwt;
            position.height = rht;
        break;

        case 7: // left, bottom
            if (cc == Button::SC_TEXT)
            {
                position.left = border_size;

                if (line > 0)
                    position.top = (page->height - rht) - height * line;
                else
                    position.top = page->height - rht;
            }
            else
                position.top = page->height - rht;

            position.width = rwt;
            position.height = rht;
        break;

        case 8: // center, bottom
            position.left = (page->width - rwt) / 2;

            if (cc == Button::SC_TEXT)
            {
                if (line > 0)
                    position.top = (page->height - rht) - height * line;
                else
                    position.top = page->height - rht;
            }
            else
                position.top = page->height - rht;

            position.width = rwt;
            position.height = rht;
        break;

        case 9: // right, bottom
            position.left = page->width - rwt;

            if (cc == Button::SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);

                if (line > 0)
                    position.top = (page->height - rht) - height * line;
                else
                    position.top = page->height - rht;
            }
            else
                position.top = page->height - rht;
        break;

        default: // center, middle
            position.left = (page->width - rwt) / 2;

            if (cc == Button::SC_TEXT)
            {
                if (line > 0)
                    position.top = ((page->height - rht) / 2) + (height / 2 * line);
                else
                    position.top = (page->height - rht) / 2;
            }
            else
                position.top = (page->height - rht) / 2;

            position.width = rwt;
            position.height = rht;
    }

    MSG_DEBUG("Type: " << dbgCC << ", PosType=" << code << ", Position: x=" << position.left << ", y=" << position.top << ", w=" << position.width << ", h=" << position.height << ", Overflow: " << (position.overflow ? "YES" : "NO"));
    position.valid = true;
    return position;
}

int TPageInterface::calcLineHeight(const string& text, SkFont& font)
{
    DECL_TRACER("TPageInterface::calcLineHeight(const string& text, SkFont& font)");

    sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(text.c_str(), font);
    SkRect rect = blob.get()->bounds();
    return rect.height();
}

int TPageInterface::numberLines(const string& str)
{
    DECL_TRACER("TPageInterface::numberLines(const string& str)");

    int lines = 1;

    for (size_t i = 0; i < str.length(); i++)
    {
        if (str.at(i) == '\n')
            lines++;
    }

    MSG_DEBUG("Detected " << lines << " lines.");
    return lines;
}

Button::BUTTONS_T *TPageInterface::addButton(Button::TButton* button)
{
    DECL_TRACER("*TPageInterface::addButton(TButton* button)");

    if (!button)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::SetError();
        return nullptr;
    }

    // We try to add this button to the list of system buttons which will
    // succeed only if it is one of the supported system buttons.
    addSysButton(button);

    try
    {
        Button::BUTTONS_T *chain = new Button::BUTTONS_T;
        chain->button = button;
        chain->next = nullptr;
        chain->previous = nullptr;
        Button::BUTTONS_T *bts = mButtons;

        if (bts)
        {
            Button::BUTTONS_T *p = bts;

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
        TError::SetError();
    }

    return nullptr;
}

bool TPageInterface::hasButton(int id)
{
    DECL_TRACER("TPageInterface::hasButton(int id)");

    Button::BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button && bt->button->getButtonIndex() == id)
            return true;

        bt = bt->next;
    }

    return false;
}

Button::TButton *TPageInterface::getButton(int id)
{
    DECL_TRACER("TPageInterface::getButton(int id)");

    Button::BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button && bt->button->getButtonIndex() == id)
            return bt->button;

        bt = bt->next;
    }

    return nullptr;
}

vector<Button::TButton *> TPageInterface::getButtons(int ap, int ad)
{
    DECL_TRACER("TPageInterface::getButtons(int ap, int ad)");

    vector<Button::TButton *> list;
    Button::BUTTONS_T *bt = mButtons;

    while (bt)
    {
        if (bt->button->getAddressPort() == ap && bt->button->getAddressChannel() == ad)
            list.push_back(bt->button);

        bt = bt->next;
    }

    return list;
}

vector<Button::TButton *> TPageInterface::getAllButtons()
{
    DECL_TRACER("TPageInterface::getAllButtons()");

    vector<Button::TButton *> list;
    Button::BUTTONS_T *bt = mButtons;

    while(bt)
    {
        list.push_back(bt->button);
        bt = bt->next;
    }

    return list;
}

Button::TButton *TPageInterface::getFirstButton()
{
    DECL_TRACER("TPageInterface::getFirstButton()");

    mLastButton = 0;

    if (mButtons)
        return mButtons->button;

    return nullptr;
}

Button::TButton *TPageInterface::getNextButton()
{
    DECL_TRACER("TPageInterface::getNextButton()");

    Button::BUTTONS_T *but = mButtons;
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

Button::TButton *TPageInterface::getLastButton()
{
    DECL_TRACER("TPageInterface::getLastButton()");

    Button::BUTTONS_T *but = mButtons;
    mLastButton = 0;

    while (but && but->next)
    {
        mLastButton++;
        but = but->next;
    }

    if (!but)
        return nullptr;

    return but->button;
}

Button::TButton *TPageInterface::getPreviousButton()
{
    DECL_TRACER("TPageInterface::getPreviousButton()");

    Button::BUTTONS_T *but = mButtons;
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

/*
 * Sort the button according to their Z-order.
 * The button with the highest Z-order will be the last button in the chain.
 * The algorithm is a bubble sort algorithm.
 */
bool TPageInterface::sortButtons()
{
    DECL_TRACER("TPageInterface::sortButtons()");

    bool turned = true;

    while (turned)
    {
        Button::BUTTONS_T *button = mButtons;
        turned = false;

        while (button)
        {
            int zo = button->button->getZOrder();

            if (button->previous)
            {
                if (zo < button->previous->button->getZOrder())
                {
                    Button::BUTTONS_T *pprev = button->previous->previous;
                    Button::BUTTONS_T *prev = button->previous;
                    Button::BUTTONS_T *next = button->next;

                    if (pprev)
                        pprev->next = button;

                    prev->next = next;
                    prev->previous = button;
                    button->next = prev;
                    button->previous = pprev;

                    if (!pprev)
                        setButtons(button);

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

void TPageInterface::setFonts(TFont *font)
{
    DECL_TRACER("TPageInterface::setFonts(TFont *font)");

    if (!font)
        return;

    mFonts = font;

    Button::BUTTONS_T *button = mButtons;

    while (button)
    {
        button->button->setFonts(font);
        button = button->next;
    }
}

vector<string> TPageInterface::getListContent(ulong handle, int ap, int ta, int ti, int rows, int columns)
{
    DECL_TRACER("TPageInterface::getListContent(ulong handle, int ap, int ta, int ti, int rows, int columns)");

    if (ap == 0 && ta == 0 && ti == 0)
    {
        vector<LIST_t>::iterator iter;

        for (iter = mLists.begin(); iter != mLists.end(); ++iter)
        {
            if (iter->handle == handle)
            {
                return iter->list;
            }
        }

        return vector<string>();
    }

    if (ap == 0 && (ta == SYSTEM_LIST_SYSTEMSOUND || ta == SYSTEM_LIST_SINGLEBEEP)) // System listbox: system sounds and system single beeps
    {
        vector<LIST_t>::iterator iter;

        for (iter = mLists.begin(); iter != mLists.end(); ++iter)
        {
            if (iter->handle == handle)
            {
                iter->ap = ap;
                iter->ta = ta;
                iter->ti = ti;
                iter->rows = rows;
                iter->columns = columns;

                if (iter->selected < 0 && !iter->list.empty())
                {
                    int row = getSystemSelection(ta, iter->list);

                    if (row > 0)
                        iter->selected = row;
                }

                return iter->list;
            }
        }

        TSystemSound sysSound(TConfig::getSystemProjectPath() + "/graphics/sounds");
        vector<string> tmpFiles = sysSound.getAllSingleBeep();
        LIST_t list;
        list.handle = handle;
        list.ap = ap;
        list.ta = ta;
        list.ti = ti;
        list.rows = rows;
        list.columns = columns;
        list.list = tmpFiles;
        list.selected = getSystemSelection(ta, tmpFiles);
        mLists.push_back(list);
        return tmpFiles;
    }
    else if (ap == 0 && ta == SYSTEM_LIST_DOUBLEBEEP)   // System listbox: double beeps
    {
        vector<LIST_t>::iterator iter;

        for (iter = mLists.begin(); iter != mLists.end(); ++iter)
        {
            if (iter->handle == handle)
            {
                iter->ap = ap;
                iter->ta = ta;
                iter->ti = ti;
                iter->rows = rows;
                iter->columns = columns;

                if (iter->selected < 0 && !iter->list.empty())
                {
                    int row = getSystemSelection(ta, iter->list);

                    if (row > 0)
                        iter->selected = row;
                }

                return iter->list;
            }
        }

        TSystemSound sysSound(TConfig::getSystemProjectPath() + "/graphics/sounds");
        vector<string> tmpFiles = sysSound.getAllDoubleBeep();
        LIST_t list;
        list.handle = handle;
        list.ap = ap;
        list.ta = ta;
        list.ti = ti;
        list.rows = rows;
        list.columns = columns;
        list.list = tmpFiles;
        list.selected = getSystemSelection(ta, tmpFiles);
        mLists.push_back(list);
        return tmpFiles;
    }
    else if (ap == 0 && ta == SYSTEM_LIST_SURFACE)  // System listbox: TP4 file (surface file)
    {
        vector<LIST_t>::iterator iter;

        for (iter = mLists.begin(); iter != mLists.end(); ++iter)
        {
            if (iter->handle == handle)
            {
                iter->ap = ap;
                iter->ta = ta;
                iter->ti = ti;
                iter->rows = rows;
                iter->columns = columns;

                if (iter->selected < 0 && !iter->list.empty())
                {
                    int row = getSystemSelection(ta, iter->list);

                    if (row > 0)
                        iter->selected = row;
                }

                return iter->list;
            }
        }

        // Load surface file names from NetLinx over FTP
        TTPInit tt;
        vector<TTPInit::FILELIST_t> fileList = tt.getFileList(".tp4");
        vector<string> tmpFiles;

        if (!fileList.empty())
        {
            vector<TTPInit::FILELIST_t>::iterator iter;

            if (gPageManager)
                gPageManager->clearFtpSurface();

            for (iter = fileList.begin(); iter != fileList.end(); ++iter)
            {
                tmpFiles.push_back(iter->fname);

                if (gPageManager)
                    gPageManager->addFtpSurface(iter->fname, iter->size);
            }
        }

        LIST_t list;
        list.handle = handle;
        list.ap = ap;
        list.ta = ta;
        list.ti = ti;
        list.rows = rows;
        list.columns = columns;
        list.list = tmpFiles;
        list.selected = getSystemSelection(ta, tmpFiles);
        mLists.push_back(list);
        return tmpFiles;
    }

    return vector<string>();
}

int TPageInterface::getSystemSelection(int ta, vector<string>& list)
{
    DECL_TRACER("TPageInterface::setSystemSelection(int ta, vector<string>* list)");

    vector<string>::iterator iterSel;
    string sel;

    if (ta == SYSTEM_LIST_SURFACE)
        sel = TConfig::getFtpSurface();
    if (ta == SYSTEM_LIST_SYSTEMSOUND)
        sel = TConfig::getSystemSound();
    else if (ta == SYSTEM_LIST_SINGLEBEEP)
        sel = TConfig::getSingleBeepSound();
    else if (ta == SYSTEM_LIST_DOUBLEBEEP)
        sel = TConfig::getDoubleBeepSound();
    else
        return -1;

    int row = 1;

    for (iterSel = list.begin(); iterSel != list.end(); ++iterSel)
    {
        if (iterSel->compare(sel) == 0)
            return row;

        row++;
    }

    return -1;
}

string TPageInterface::getListRow(int ti, int row)
{
    DECL_TRACER("TPageInterface::getListRow(ulong handle, int ti, int row)");

    vector<LIST_t>::iterator iter;

    for (iter = mLists.begin(); iter != mLists.end(); ++iter)
    {
        if (iter->ti == ti)
        {
            if (row < 1 || (size_t)row > iter->list.size())
                return string();

            return iter->list[row-1];
        }
    }

    return string();
}

void TPageInterface::setGlobalSettings(Button::TButton* button)
{
    DECL_TRACER("TPageInterface::setGlobalSettings(TButton* button)");

    if (!button)
        return;

    button->setFontOnly(sr[0].fi, 0);
    button->setTextColorOnly(sr[0].ct, 0);
    button->setTextEffectColorOnly(sr[0].ec, 0);

    if (button->getListAp() == 0 && button->getListTi() >= SYSTEM_PAGE_START)
        button->setTextJustificationOnly(4, 0, 0, 0);
}

void TPageInterface::setSelectedRow(ulong handle, int row)
{
    DECL_TRACER("TPageInterface::setSelectedRow(ulong handle, int row)");

    if (row < 1)
        return;

    vector<LIST_t>::iterator iter;

    for (iter = mLists.begin(); iter != mLists.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            if ((size_t)row <= iter->list.size())
                iter->selected = row;

            MSG_DEBUG("Row was set to " << row << " for item " << handleToString(handle));
            return;
        }
    }
}

int TPageInterface::getSelectedRow(ulong handle)
{
    DECL_TRACER("TPageInterface::getSelectedRow(ulong handle)");

    vector<LIST_t>::iterator iter;

    for (iter = mLists.begin(); iter != mLists.end(); ++iter)
    {
        if (iter->handle == handle)
            return iter->selected;
    }

    return -1;
}

string TPageInterface::getSelectedItem(ulong handle)
{
    DECL_TRACER("TPageInterface::getSelectedItem(ulong handle)");

    vector<LIST_t>::iterator iter;

    for (iter = mLists.begin(); iter != mLists.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            if (iter->selected > 0 && (size_t)iter->selected <= iter->list.size())
                return iter->list[iter->selected-1];

            ulong nPage = (handle >> 16) & 0x0000ffff;
            ulong nButt = handle & 0x0000ffff;
            string sel;

            if (nPage == SYSTEM_SUBPAGE_SURFACE && nButt == 1)
                sel = TConfig::getFtpSurface();
            if (nPage == SYSTEM_SUBPAGE_SYSTEMSOUND && nButt == 1)
                sel = TConfig::getSystemSound();
            else if (nPage == SYSTEM_SUBPAGE_SINGLEBEEP && nButt == 1)
                sel = TConfig::getSingleBeepSound();
            else if (nPage == SYSTEM_SUBPAGE_DOUBLEBEEP && nButt == 1)
                sel = TConfig::getDoubleBeepSound();
            else
                return string();

            if (iter->list.empty())
                return string();

            vector<string>::iterator iterSel;
            int row = 1;

            for (iterSel = iter->list.begin(); iterSel != iter->list.end(); ++iterSel)
            {
                if (iterSel->compare(sel) == 0)
                {
                    iter->selected = row;
                    return sel;
                }

                row++;
            }
        }
    }

    return string();
}

bool TPageInterface::haveImage(const Button::SR_T& sr)
{
    DECL_TRACER("TPageInterface::haveImage(const Button::SR_T& sr)");

    for (int i = 0; i < MAX_IMAGES; ++i)
    {
        if (!sr.bitmaps[i].fileName.empty())
            return true;
    }

    return false;
}

/**
 * @brief G5: Put all images together
 * The method takes all defined images, scales them and put one over the other.
 * The result could be combinated with a chameleon image, if there is one.
 *
 * @param sr        The section containing the image credentials
 * @oaram ignFirst  Default: FALSE; If TRUE the first image is ignored.
 * @return TRUE on success
 */
bool TPageInterface::tp5Image(SkBitmap *bm, Button::SR_T& sr, int wt, int ht, bool ignFirst)
{
    DECL_TRACER("TPageInterface::tp5Image(SkBitmap *bm, Button::SR_T& sr, int wt, int ht, bool ignFirst)");

    if (!bm)
    {
        MSG_ERROR("Have no valid image pointer!");
        return false;
    }

    if (!haveImage(sr))
        return true;

    bool first = true;

    for (int i = 0; i < MAX_IMAGES; ++i)
    {
        if (sr.bitmaps[i].fileName.empty())
            continue;

        if (ignFirst && first)
        {
            first = false;
            continue;
        }

        SkBitmap bmBm;
        int width, height;

        if (!TImgCache::getBitmap(sr.bitmaps[i].fileName, &bmBm, _BMTYPE_BITMAP, &width, &height))
        {
            sk_sp<SkData> data = readImage(sr.bitmaps[i].fileName);
            bool loaded = false;

            if (data)
            {
                DecodeDataToBitmap(data, &bmBm);

                if (!bmBm.empty())
                {
                    TImgCache::addImage(sr.bitmaps[i].fileName, bmBm, _BMTYPE_BITMAP);
                    loaded = true;
                }
            }

            if (!loaded)
            {
                MSG_ERROR("Missing image " << sr.bitmaps[i].fileName << "!");
                SET_ERROR();
                return false;
            }

            sr.bitmaps[i].index = i;
            sr.bitmaps[i].width = bmBm.info().width();
            sr.bitmaps[i].height = bmBm.info().height();
        }

        if (!bmBm.empty())
        {
            width = bmBm.info().width();
            height = bmBm.info().height();
            // If the target image was not allocated, we do this now
            if (bm->empty())
            {
                if (!allocPixels(wt, ht, bm))
                {
                    SET_ERROR_MSG("Allocation for image failed!");
                    return false;
                }
            }
            // Map bitmap
            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrcOver);
            SkCanvas can(*bm, SkSurfaceProps());
            // Scale bitmap
            if (sr.bitmaps[i].justification == Button::ORI_SCALE_FIT || sr.bitmaps[i].justification == Button::ORI_SCALE_ASPECT)
            {
                SkBitmap scaled;
                MSG_DEBUG("Scaling image " << sr.bitmaps[i].fileName << " ...");
                MSG_DEBUG("Size of bitmap: " << width << "x" << height);
                MSG_DEBUG("Size of button: " << wt << "x" << ht);
                MSG_DEBUG("Will scale to " << (sr.bitmaps[i].justification == Button::ORI_SCALE_FIT ? "scale to fit" : "keep aspect"));

                if (!allocPixels(wt, ht, &scaled))
                {
                    MSG_ERROR("Error allocating space for bitmap " << sr.bitmaps[i].fileName << "!");
                    return false;
                }

                SkIRect r;
                r.setSize(scaled.info().dimensions());      // Set the dimensions
                scaled.erase(SK_ColorTRANSPARENT, r);       // Initialize all pixels to transparent
                SkCanvas canvas(scaled, SkSurfaceProps());  // Create a canvas
                SkRect rect;

                if (sr.bitmaps[i].justification == Button::ORI_SCALE_FIT)   // Scale to fit
                    rect = SkRect::MakeXYWH(0, 0, wt, ht);
                else                                        // Scale but keep aspect ratio
                {
                    double factor = 0.0;

                    if (width > height)
                        factor = static_cast<double>(min(wt, width)) / static_cast<double>(max(wt, width));
                    else
                        factor = static_cast<double>(min(ht, height)) / static_cast<double>(max(ht, height));

                    int w = static_cast<int>(static_cast<double>(width) * factor);
                    int h = static_cast<int>(static_cast<double>(height) * factor);
                    // Calculate center of image
                    int x, y;
                    x = (wt - w) / 2;
                    y = (ht - h) / 2;
                    // initialize the rectangle
                    rect = SkRect::MakeXYWH(x, y, w, h);
                }

                MSG_DEBUG("Using rect to scale: " << rect.x() << ", " << rect.y() << ", " << rect.width() << ", " << rect.height());
                sk_sp<SkImage> im = SkImages::RasterFromBitmap(bmBm);
                canvas.drawImageRect(im, rect, SkSamplingOptions(), &paint);
                bmBm = scaled;
                width = bmBm.info().width();
                height = bmBm.info().height();
                MSG_DEBUG("Scaled image " << sr.bitmaps[i].fileName << " has dimensions " << width << " x " << height);
            }

            // Justify bitmap
            SkRect rect = justifyBitmap5(sr, wt, ht, i, width, height, 0);
            sk_sp<SkImage> im = SkImages::RasterFromBitmap(bmBm);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            MSG_DEBUG("Bitmap " << sr.bitmaps[i].fileName << " at index " << i << " was mapped to position " << rect.x() << ", " << rect.y() << ", " << rect.width() << ", " << rect.height());
        }
        else
        {
            MSG_WARNING("No or invalid bitmap!");
            return false;
        }
    }

    return true;
}

SkRect TPageInterface::justifyBitmap5(Button::SR_T& sr, int wt, int ht, int index, int width, int height, int border_size)
{
    DECL_TRACER("TPageInterface::justifyBitmap5(Button::SR_T& sr, int wt, int ht, int index, int width, int height, int border_size)");

    int x, y;
    int bwt = wt - border_size;
    int bht = ht - border_size;

    switch(sr.bitmaps[index].justification)
    {
        case Button::ORI_ABSOLUT:
            x = sr.bitmaps[index].offsetX;
            y = sr.bitmaps[index].offsetY;
            break;

        case Button::ORI_BOTTOM_LEFT:
            x = border_size;
            y = bht - height;
            break;

        case Button::ORI_BOTTOM_MIDDLE:
            x = (wt - width) / 2;
            y = bht - height;
            break;

        case Button::ORI_BOTTOM_RIGHT:
            x = bwt - width;
            y = bht -height;
            break;

        case Button::ORI_CENTER_LEFT:
            x = border_size;
            y = (bht - height) / 2;
            break;

        case Button::ORI_CENTER_MIDDLE:
            x = (wt - width) / 2;
            y = (ht - height) / 2;
            break;

        case Button::ORI_CENTER_RIGHT:
            x = bwt - width;
            y = (ht - height) / 2;
            break;

        case Button::ORI_TOP_LEFT:
            x = border_size;
            y = border_size;
            break;

        case Button::ORI_TOP_MIDDLE:
            x = (wt - width) / 2;
            y = border_size;
            break;

        case Button::ORI_TOP_RIGHT:
            x = bwt - width;
            y = border_size;
            break;

        default:
            x = border_size;
            y = border_size;
    }

    return SkRect::MakeXYWH(x + border_size, y + border_size, width, height);
}

/**
 * @brief getBorderFragment - get part of border
 * The method reads a border image fragment from the disk and converts it to
 * the border color. If there is a base image and an alpha mask image, the
 * pixels of the alpha mask are converted to the border color and then the base
 * image is layed over the mask image.
 * In case there is no base image, an image with the same size as the mask image
 * is created and filled transparaent.
 *
 * @param path      The path and file name of the base image.
 * @param pathAlpha The path and file name of the alpha mask image.
 * @param image     A pointer to an empty bitmap.
 * @param color     The border color
 *
 * @return In case the images exists and were loaded successfully, TRUE is
 * returned.
 */
bool TPageInterface::getBorderFragment(const string& path, const string& pathAlpha, SkBitmap* image, SkColor color)
{
    DECL_TRACER("TPageInterface::getBorderFragment(const string& path, const string& pathAlpha, SkBitmap* image, SkColor color)");

    if (!image)
    {
        MSG_ERROR("Invalid pointer to image!");
        return false;
    }

    sk_sp<SkData> im;
    SkBitmap bm;
    bool haveBaseImage = false;

    // If the path ends with "alpha.png" then it is a mask image. This not what
    // we want first unless this is the only image available.
    if (!endsWith(path, "alpha.png") || pathAlpha.empty())
    {
        if (!path.empty() && retrieveImage(path, image))
        {
            haveBaseImage = true;
            // Underly the pixels with the border color
            if (pathAlpha.empty() || !fs::exists(pathAlpha) || path == pathAlpha)
            {
                SkImageInfo info = image->info();
                SkBitmap b;
                allocPixels(info.width(), info.height(), &b);
                b.eraseColor(SK_ColorTRANSPARENT);

                for (int x = 0; x < info.width(); ++x)
                {
                    for (int y = 0; y < info.height(); ++y)
                    {
                        SkColor alpha = SkColorGetA(image->getColor(x, y));
                        uint32_t *pix = b.getAddr32(x, y);

                        if (alpha > 0)
                            *pix = color;
                    }
                }

                SkPaint paint;
                paint.setAntiAlias(true);
                paint.setBlendMode(SkBlendMode::kDstATop);
                SkCanvas can(*image);
                sk_sp<SkImage> _image = SkImages::RasterFromBitmap(b);
                can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
            }
        }
    }

    // If there is no valid path return.
    if (pathAlpha.empty())
        return haveBaseImage;

    // On error retrieving the image, return.
    if (!retrieveImage(pathAlpha, &bm))
        return haveBaseImage;

    // If there was no base image loaded, allocate the space for an image
    // filled transparent. Make it the same size as the mask image.
    if (!haveBaseImage)
    {
        allocPixels(bm.info().width(), bm.info().height(), image);
        image->eraseColor(SK_ColorTRANSPARENT);
    }

    // Only if the base image and the mask image have the same size, which
    // should be the case, then the visible pixels of the mask image are
    // colored by the border color.
    if (image->info().dimensions() == bm.info().dimensions())
    {
        for (int y = 0; y < image->info().height(); ++y)
        {
            for (int x = 0; x < image->info().width(); ++x)
            {
                SkColor col = bm.getColor(x, y);
                SkColor alpha = SkColorGetA(col);
                uint32_t *pix = bm.getAddr32(x, y);

                if (alpha == 0)
                    *pix = SK_ColorTRANSPARENT;
                else
                    *pix = SkColorSetA(color, alpha);
            }
        }
    }

    // Here we draw the border fragment over the base image.
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setBlendMode(SkBlendMode::kDstATop);
    SkCanvas can(*image);
    sk_sp<SkImage> _image = SkImages::RasterFromBitmap(bm);
    can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);

    return true;
}

SkBitmap TPageInterface::retrieveBorderImage(const string& pa, const string& pb, SkColor color, SkColor bgColor)
{
    DECL_TRACER("TPageInterface::retrieveBorderImage(const string& pa, const string& pb, SkColor color, SkColor bgColor)");

    SkBitmap bm, bma;

    if (!pa.empty() && !retrieveImage(pa, &bm))
        return SkBitmap();

    if (!pb.empty() && !retrieveImage(pb, &bma))
        return SkBitmap();

    return colorImage(bm, bma, color, bgColor, false);
}

bool TPageInterface::retrieveImage(const string& path, SkBitmap* image)
{
    DECL_TRACER("TPageInterface::retrieveImage(const string& path, SkBitmap* image)");

    if (path.empty() || !image)
    {
        MSG_WARNING("One or all of the parameters are invalid!");
        return false;
    }

    sk_sp<SkData> im;

    if (!(im = readImage(path)))
        return false;

    DecodeDataToBitmap(im, image);

    if (image->empty())
    {
        MSG_WARNING("Could not create the image " << path);
        return false;
    }

    return true;
}

SkBitmap TPageInterface::colorImage(SkBitmap& base, SkBitmap& alpha, SkColor col, SkColor bg, bool useBG)
{
    DECL_TRACER("TPageInterface::colorImage(SkBitmap *img, int width, int height, SkColor col, SkColor bg, bool useBG)");

    int width = base.info().width();
    int height = base.info().height();

    if (width <= 0 || height <= 0)
    {
        MSG_WARNING("Got invalid width or height! (width: " << width << ", height: " << height << ")");
        return SkBitmap();
    }

    if (!alpha.empty())
    {
        if (width != alpha.info().width() || height != alpha.info().height())
        {
            MSG_ERROR("Base and alpha masks have different size!");
            return SkBitmap();
        }
    }

    SkBitmap maskBm;

    if (!allocPixels(width, height, &maskBm))
        return SkBitmap();

    maskBm.eraseColor(SK_ColorTRANSPARENT);

    for (int ix = 0; ix < width; ix++)
    {
        for (int iy = 0; iy < height; iy++)
        {
            SkColor pixelAlpha = 0;

            if (!alpha.empty())
                pixelAlpha = alpha.getColor(ix, iy);
            else
                pixelAlpha = base.getColor(ix, iy);

            uint32_t *wpix = maskBm.getAddr32(ix, iy);

            if (!wpix)
            {
                MSG_ERROR("No pixel buffer!");
                break;
            }

            uint32_t ala = SkColorGetA(pixelAlpha);

            if (ala == 0 && !useBG)
                pixelAlpha = col;
            else if (ala == 0)
                pixelAlpha = bg;
            else
            {
                // We've to change the red and the blue color channel because
                // of an error in the Skia library.
                uint32_t red = SkColorGetR(col);
                uint32_t green = SkColorGetG(col);
                uint32_t blue = SkColorGetB(col);

                if (alpha.empty())
                {
                    uint32_t pred = SkColorGetR(pixelAlpha);
                    uint32_t pgreen = SkColorGetG(pixelAlpha);
                    uint32_t pblue = SkColorGetB(pixelAlpha);
                    uint32_t maxChan = SkColorGetG(SK_ColorWHITE);

                    red   = ((pred == maxChan) ? pred : red);
                    green = ((pgreen == maxChan) ? pgreen : green);
                    blue  = ((pblue == maxChan) ? pblue : blue);
                }
                else if (ala == 0)
                    red = green = blue = 0;

                pixelAlpha = SkColorSetARGB(ala, red, green, blue);
            }

            *wpix = pixelAlpha;
        }
    }

    if (!alpha.empty())
    {
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        SkCanvas can(maskBm);
        sk_sp<SkImage> _image = SkImages::RasterFromBitmap(base);
        can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    }

    return maskBm;
}

bool TPageInterface::stretchImageWidth(SkBitmap *bm, int width)
{
    DECL_TRACER("TPageInterface::stretchImageWidth(SkBitmap *bm, int width)");

    if (!bm)
        return false;

    int rwidth = width;
    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);

    SkImageInfo info = bm->info();
    sk_sp<SkImage> im = SkImages::RasterFromBitmap(*bm);

    if (width <= 0)
        rwidth = info.width() + width;

    if (rwidth <= 0)
        rwidth = 1;

    MSG_DEBUG("Width: " << rwidth << ", Height: " << info.height());

    if (!allocPixels(rwidth, info.height(), bm))
        return false;

    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps());
    SkRect rect = SkRect::MakeXYWH(0, 0, rwidth, info.height());
    can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
    return true;
}

bool TPageInterface::stretchImageHeight(SkBitmap *bm, int height)
{
    DECL_TRACER("TPageInterface::stretchImageHeight(SkBitmap *bm, int height)");

    if (!bm)
        return false;

    int rheight = height;
    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);

    SkImageInfo info = bm->info();

    if (height <= 0)
        rheight = info.height() + height;

    if (rheight <= 0)
        rheight = 1;

    sk_sp<SkImage> im = SkImages::RasterFromBitmap(*bm);
    MSG_DEBUG("Width: " << info.width() << ", Height: " << rheight);

    if (!allocPixels(info.width(), rheight, bm))
        return false;

    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps());
    SkRect rect = SkRect::MakeXYWH(0, 0, info.width(), rheight);
    can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
    return true;
}

#ifdef _OPAQUE_SKIA_
bool TPageInterface::setOpacity(SkBitmap *bm, int oo)
{
    DECL_TRACER("TPageInterface::setOpacity(SkBitmap *bm, int oo)");

    if (oo < 0 || oo > 255 || !bm)
        return false;

    SkBitmap ooButton;
    int w = bm->info().width();
    int h = bm->info().height();

    if (!allocPixels(w, h, &ooButton))
        return false;

    SkCanvas canvas(ooButton);
    SkIRect irect = SkIRect::MakeXYWH(0, 0, w, h);
    SkRegion region;
    region.setRect(irect);
    SkScalar opaque = (SkScalar)oo;

    SkScalar alpha = 1.0 / 255.0 * opaque;
    MSG_DEBUG("Calculated alpha value: " << alpha << " (oo=" << oo << ")");
    SkPaint paint;
    paint.setAlphaf(alpha);
    sk_sp<SkImage> _image = SkImages::RasterFromBitmap(*bm);
    canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    bm->erase(SK_ColorTRANSPARENT, {0, 0, w, h});
    *bm = ooButton;
    return true;
}
#endif

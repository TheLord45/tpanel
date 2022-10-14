/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include "tpageinterface.h"
#include "tsystemsound.h"
#include "tobject.h"
#include "ttpinit.h"
#include "tconfig.h"
#include "tresources.h"
#include "tpagemanager.h"
#include "terror.h"

using std::string;
using std::vector;

bool TPageInterface::drawText(PAGE_T& pinfo, SkBitmap *img)
{
    MSG_TRACE("TPageInterface::drawText(PAGE_T& pinfo, SkImage& img)");

    if (pinfo.sr[0].te.empty())
        return true;

    MSG_DEBUG("Searching for font number " << pinfo.sr[0].fi << " with text " << pinfo.sr[0].te);
    FONT_T font = mFonts->getFont(pinfo.sr[0].fi);

    if (!font.file.empty())
    {
        SkCanvas canvas(*img, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
        sk_sp<SkTypeface> typeFace = mFonts->getTypeFace(pinfo.sr[0].fi);

        if (!typeFace)
        {
            MSG_ERROR("Error creating type face " << font.fullName);
            TError::setError();
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
                textLines = ::splitLine(pinfo.sr[0].te);
            else
            {
                textLines = splitLine(pinfo.sr[0].te, pinfo.width, pinfo.height, skFont, paint);
                lines = textLines.size();
            }

            int lineHeight = calcLineHeight(pinfo.sr[0].te, skFont);
            int totalHeight = lineHeight * lines;

            if (totalHeight > pinfo.height)
            {
                lines = pinfo.height / lineHeight;
                totalHeight = lineHeight * lines;
            }

            MSG_DEBUG("Line height: " << lineHeight);
            Button::POSITION_t position = calcImagePosition(&pinfo, pinfo.width, totalHeight, Button::SC_TEXT, 1);
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
                Button::POSITION_t pos = calcImagePosition(&pinfo, rect.width(), lineHeight, Button::SC_TEXT, 1);

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
            sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(pinfo.sr[0].te.c_str(), skFont);
            SkRect rect;
            skFont.measureText(pinfo.sr[0].te.c_str(), pinfo.sr[0].te.length(), SkTextEncoding::kUTF8, &rect, &paint);
            Button::POSITION_t position = calcImagePosition(&pinfo, rect.width(), (rect.height() * (float)lines), Button::SC_TEXT, 0);

            if (!position.valid)
            {
                MSG_ERROR("Error calculating the text position!");
                TError::setError();
                return false;
            }

            MSG_DEBUG("Printing line " << pinfo.sr[0].te);
            SkScalar startX = (SkScalar)position.left;
            SkScalar startY = (SkScalar)position.top + metrics.fCapHeight; // + metrics.fLeading; // (metrics.fAscent * -1.0);
            canvas.drawTextBlob(blob, startX, startY, paint);
        }
    }
    else
    {
        MSG_WARNING("No font file name found for font " << pinfo.sr[0].fi);
    }

    return true;
}

Button::POSITION_t TPageInterface::calcImagePosition(PAGE_T *page, int width, int height, Button::CENTER_CODE cc, int line)
{
    DECL_TRACER("TPageInterface::calcImagePosition(PAGE_T *page, int with, int height, CENTER_CODE code, int number)");

    Button::SR_T act_sr;
    Button::POSITION_t position;
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

            if (cc == Button::SC_TEXT)
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
                position.top = height * line;
            }

            position.width = rwt;
            position.height = rht;
            break;

        case 2: // center, top
            if (cc == Button::SC_TEXT)
                position.top = height * line;

        position.left = (page->width - rwt) / 2;
        position.height = rht;
        position.width = rwt;
        break;

        case 3: // right, top
            position.left = page->width - rwt;

            if (cc == Button::SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = height * line;
            }

            position.width = rwt;
            position.height = rht;
            break;

        case 4: // left, middle
            if (cc == Button::SC_TEXT)
            {
                position.left = border;
                position.top = ((page->height - rht) / 2) + (height / 2 * line);
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
                position.top = ((page->height - rht) / 2) + (height / 2 * line);
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
                position.top = (page->height - rht) - height * line;
            }
            else
                position.top = page->height - rht;

        position.width = rwt;
        position.height = rht;
        break;

        case 8: // center, bottom
            position.left = (page->width - rwt) / 2;

            if (cc == Button::SC_TEXT)
                position.top = (page->height - rht) - height * line;
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
                position.top = (page->height - rht) - height * line;
            }
            else
                position.top = page->height - rht;
        break;

        default: // center, middle
            position.left = (page->width - rwt) / 2;

            if (cc == Button::SC_TEXT)
                position.top = ((page->height - rht) / 2) + (height / 2 * line);
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

    return lines;
}

Button::BUTTONS_T *TPageInterface::addButton(Button::TButton* button)
{
    DECL_TRACER("*TPageInterface::addButton(TButton* button)");

    if (!button)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::setError();
        return nullptr;
    }

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
        TError::setError();
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

            MSG_DEBUG("Row was set to " << row << " for item " << TObject::handleToString(handle));
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

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

#include <string>
#include <memory>
#include <algorithm>
#include <codecvt>
#include <fstream>

#include <include/core/SkPixmap.h>
#include <include/core/SkSize.h>
#include <include/core/SkColor.h>
#include <include/core/SkFont.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkRegion.h>
#include <include/core/SkImageFilter.h>
#include <include/effects/SkImageFilters.h>
#include <include/core/SkPath.h>
#include <include/core/SkSurfaceProps.h>
#ifndef __MACH__
#include <include/core/SkFilterQuality.h>
#endif
#include <include/core/SkMaskFilter.h>
#include <include/core/SkImageEncoder.h>
#include <include/core/SkRRect.h>

//#ifdef __ANDROID__
//#include <QtAndroidExtras/QAndroidJniObject>
//#include <QtAndroid>
//#endif

#include "tbutton.h"
#include "thttpclient.h"
#include "terror.h"
#include "tconfig.h"
#include "tresources.h"
#include "ticons.h"
#include "tamxnet.h"
#include "tpagemanager.h"
#include "tsystemsound.h"
#include "timgcache.h"
#include "turl.h"
#include "tlock.h"
#if TESTMODE == 1
#include "testmode.h"
#endif

using std::exception;
using std::string;
using std::vector;
using std::unique_ptr;
using std::map;
using std::pair;
using std::thread;
using std::atomic;
using std::mutex;
using std::bind;
using namespace Button;
using namespace Expat;

#define MAX_BUFFER          65536

#define RLOG_INFO           0x00fe
#define RLOG_WARNING        0x00fd
#define RLOG_ERROR          0x00fb
#define RLOG_TRACE          0x00f7
#define RLOG_DEBUG          0x00ef
#define RLOG_PROTOCOL       0x00f8
#define RLOG_ALL            0x00e0

extern TIcons *gIcons;
extern amx::TAmxNet *gAmxNet;
extern TPageManager *gPageManager;

THR_REFRESH_t *TButton::mThrRefresh = nullptr;
vector<BITMAP_CACHE> nBitmapCache;     // Holds the images who are delayed because they are external

SYSTEF_t sysTefs[] = {
    {  1, "Outline-S" },
    {  2, "Outline-M" },
    {  3, "Outline-L" },
    {  4, "Outline-X" },
    {  5, "Glow-S" },
    {  6, "Glow-M" },
    {  7, "Glow-L" },
    {  8, "Glow-X" },
    {  9, "Soft Drop Shadow 1" },
    { 10, "Soft Drop Shadow 2" },
    { 11, "Soft Drop Shadow 3" },
    { 12, "Soft Drop Shadow 4" },
    { 13, "Soft Drop Shadow 5" },
    { 14, "Soft Drop Shadow 6" },
    { 15, "Soft Drop Shadow 7" },
    { 16, "Soft Drop Shadow 8" },
    { 17, "Medium Drop Shadow 1" },
    { 18, "Medium Drop Shadow 2" },
    { 19, "Medium Drop Shadow 3" },
    { 20, "Medium Drop Shadow 4" },
    { 21, "Medium Drop Shadow 5" },
    { 22, "Medium Drop Shadow 6" },
    { 23, "Medium Drop Shadow 7" },
    { 24, "Medium Drop Shadow 8" },
    { 25, "Hard Drop Shadow 1" },
    { 26, "Hard Drop Shadow 2" },
    { 27, "Hard Drop Shadow 3" },
    { 28, "Hard Drop Shadow 4" },
    { 29, "Hard Drop Shadow 5" },
    { 30, "Hard Drop Shadow 6" },
    { 31, "Hard Drop Shadow 7" },
    { 32, "Hard Drop Shadow 8" },
    { 33, "Soft Drop Shadow 1 with outline" },
    { 34, "Soft Drop Shadow 2 with outline" },
    { 35, "Soft Drop Shadow 3 with outline" },
    { 36, "Soft Drop Shadow 4 with outline" },
    { 37, "Soft Drop Shadow 5 with outline" },
    { 38, "Soft Drop Shadow 6 with outline" },
    { 39, "Soft Drop Shadow 7 with outline" },
    { 40, "Soft Drop Shadow 8 with outline" },
    { 41, "Medium Drop Shadow 1 with outline" },
    { 42, "Medium Drop Shadow 2 with outline" },
    { 43, "Medium Drop Shadow 3 with outline" },
    { 44, "Medium Drop Shadow 4 with outline" },
    { 45, "Medium Drop Shadow 5 with outline" },
    { 46, "Medium Drop Shadow 6 with outline" },
    { 47, "Medium Drop Shadow 7 with outline" },
    { 48, "Medium Drop Shadow 8 with outline" },
    { 49, "Hard Drop Shadow 1 with outline" },
    { 50, "Hard Drop Shadow 2 with outline" },
    { 51, "Hard Drop Shadow 3 with outline" },
    { 52, "Hard Drop Shadow 4 with outline" },
    { 53, "Hard Drop Shadow 5 with outline" },
    { 54, "Hard Drop Shadow 6 with outline" },
    { 55, "Hard Drop Shadow 7 with outline" },
    { 56, "Hard Drop Shadow 8 with outline" },
    { 0,  "\0" }
};

TButton::TButton()
{
    DECL_TRACER("TButton::TButton()");

    mAniRunning = false;
    mLastBlink.clear();
}

TButton::~TButton()
{
    DECL_TRACER("TButton::~TButton()");

    if (ap == 0 && ad == 8)
    {
        if (gAmxNet)
            gAmxNet->deregNetworkState(mHandle);
    }

    if (ap == 0 && ((ad >= 141 && ad <= 143) || (ad >= 151 && ad <= 158)))
    {
        if (gAmxNet)
            gAmxNet->deregTimer(mHandle);
    }

    if (ap == 0 && ad == 81)    // Network state multi bargraph
    {
        if (gPageManager)
            gPageManager->unregCallbackNetState(mHandle);
    }

    if (mTimer)
    {
        mTimer->stop();

        while (mTimer->isRunning())
            usleep(50);

        delete mTimer;
    }

    if (mAniRunning)
    {
        ulong tm = nu * ru + nd * rd;
        mAniStop = true;

        while (mAniRunning)
            std::this_thread::sleep_for(std::chrono::milliseconds(tm * 100));
    }

    THR_REFRESH_t *next, *p = mThrRefresh;

    while (p)
    {
        if (p->mImageRefresh)
        {
            p->mImageRefresh->stop();
            int counter = 0;

            while (counter < 1000 && p->mImageRefresh->isRunning())
            {
                usleep(50);
                counter++;
            }

            delete p->mImageRefresh;
            p->mImageRefresh = nullptr;
        }

        next = p->next;
        delete p;
        p = next;
    }

    mThrRefresh = nullptr;
}

size_t TButton::initialize(TExpat *xml, size_t index)
{
    DECL_TRACER("TButton::initialize(TExpat *xml, size_t index)");

    if (!xml || index == TExpat::npos)
    {
        MSG_ERROR("Invalid NULL parameter passed!");
        TError::setError();
        return TExpat::npos;
    }

    mChanged = true;
    vector<ATTRIBUTE_t> attrs = xml->getAttributes(index);
    string stype = xml->getAttribute("type", attrs);
    type = getButtonType(stype);
    MSG_DEBUG("Button type: " << stype << " --> " << type);
    string ename, content;
    size_t oldIndex = index;

    while((index = xml->getNextElementFromIndex(index, &ename, &content, &attrs)) != TExpat::npos)
    {
        if (ename.compare("bi") == 0)
        {
            bi = xml->convertElementToInt(content);
            MSG_DEBUG("Processing button index: " << bi);
        }
        else if (ename.compare("na") == 0)          // Name
            na = content;
        else if (ename.compare("bd") == 0)          // Description
            bd = content;
        else if (ename.compare("lt") == 0)          // Left
            lt = xml->convertElementToInt(content);
        else if (ename.compare("tp") == 0)          // Top
            tp = xml->convertElementToInt(content);
        else if (ename.compare("wt") == 0)          // Width
            wt = xml->convertElementToInt(content);
        else if (ename.compare("ht") == 0)          // Height
            ht = xml->convertElementToInt(content);
        else if (ename.compare("zo") == 0)          // Z-Order
            zo = xml->convertElementToInt(content);
        else if (ename.compare("hs") == 0)          // bounding
            hs = content;
        else if (ename.compare("bs") == 0)          // border style
            bs = content;
        else if (ename.compare("fb") == 0)          // feedback type
            fb = getButtonFeedback(content);
        else if (ename.compare("ap") == 0)          // address port
            ap = xml->convertElementToInt(content);
        else if (ename.compare("ad") == 0)          // address code
            ad = xml->convertElementToInt(content);
        else if (ename.compare("ch") == 0)          // channel code
            ch = xml->convertElementToInt(content);
        else if (ename.compare("cp") == 0)          // channel port
            cp = xml->convertElementToInt(content);
        else if (ename.compare("lp") == 0)          // level port
            lp = xml->convertElementToInt(content);
        else if (ename.compare("lv") == 0)          // level code
            lv = xml->convertElementToInt(content);
        else if (ename.compare("dr") == 0)          // level direction
            dr = content;
        else if (ename.compare("co") == 0)          // command port
            co = xml->convertElementToInt(content);
        else if (ename.compare("cm") == 0)          // commands to send on button hit
            cm.push_back(content);
        else if (ename.compare("va") == 0)          // ?
            va = xml->convertElementToInt(content);
        else if (ename.compare("rm") == 0)          // State count
            rm = xml->convertElementToInt(content);
        else if (ename.compare("nu") == 0)          // Animate time up
            nu = xml->convertElementToInt(content);
        else if (ename.compare("nd") == 0)          // Animate time down
            nd = xml->convertElementToInt(content);
        else if (ename.compare("ar") == 0)          // Auto repeat state
            ar = xml->convertElementToInt(content);
        else if (ename.compare("ru") == 0)          // Animate time up (bargraph on click)
            ru = xml->convertElementToInt(content);
        else if (ename.compare("rd") == 0)          // Animate time down (bargraph on click)
            rd = xml->convertElementToInt(content);
        else if (ename.compare("lu") == 0)          // Animate time up (bargraph)
            lu = xml->convertElementToInt(content);
        else if (ename.compare("ld") == 0)          // Animate time down (bargraph)
            ld = xml->convertElementToInt(content);
        else if (ename.compare("rv") == 0)          // ?
            rv = xml->convertElementToInt(content);
        else if (ename.compare("rl") == 0)          // Bargraph range low
            rl = xml->convertElementToInt(content);
        else if (ename.compare("rh") == 0)          // Bargraph range high
            rh = xml->convertElementToInt(content);
        else if (ename.compare("ri") == 0)          // Bargraph inverted (0 = normal, 1 = inverted)
            ri = xml->convertElementToInt(content);
        else if (ename.compare("rn") == 0)          // Bargraph: Range drag increment
            rn = xml->convertElementToInt(content);
        else if (ename.compare("lf") == 0)          // Bargraph function
            lf = content;
        else if (ename.compare("sd") == 0)          // Name/Type of slider for a bargraph
            sd = content;
        else if (ename.compare("sc") == 0)          // Color of slider (for bargraph)
            sc = content;
        else if (ename.compare("mt") == 0)          // Length of text area
            mt = xml->convertElementToInt(content);
        else if (ename.compare("dt") == 0)          // Textarea multiple/single line
            dt = content;
        else if (ename.compare("im") == 0)          // Input mask of a text area
            im = content;
        else if (ename.compare("so") == 0)          // String output port
            so = xml->convertElementToInt(content);
        else if (ename.compare("op") == 0)          // String the button send
            op = content;
        else if (ename.compare("pc") == 0)          // Password character for text area (single line)
            pc = content;
        else if (ename.compare("ta") == 0)          // Listbox table channel
            ta = xml->convertElementToInt(content);
        else if (ename.compare("ti") == 0)          // Listbox table address channel of rows
            ti = xml->convertElementToInt(content);
        else if (ename.compare("tr") == 0)          // Listbox number of rows
            tr = xml->convertElementToInt(content);
        else if (ename.compare("tc") == 0)          // Listbox number of columns
            tc = xml->convertElementToInt(content);
        else if (ename.compare("tj") == 0)          // Listbox row height
            tj = xml->convertElementToInt(content);
        else if (ename.compare("tk") == 0)          // Listbox preferred row height
            tk = xml->convertElementToInt(content);
        else if (ename.compare("of") == 0)          // Listbox list offset: 0=disabled/1=enabled
            of = xml->convertElementToInt(content);
        else if (ename.compare("tg") == 0)          // Listbox managed: 0=no/1=yes
            tg = xml->convertElementToInt(content);
        else if (ename.compare("st") == 0)          // SubPageView index
            st = xml->convertElementToInt(content);
        else if (ename.compare("ws") == 0)          // SubPageView: Wrap subpages; 1 = YES
            ws = xml->convertElementToInt(content);
        else if (ename.compare("sa") == 0)          // SubPageView: Percent of space between items in list
            sa = xml->convertElementToInt(content);
        else if (ename.compare("dy") == 0)          // SubPageView: Allow dynamic reordering; 1 = YES
            dy = xml->convertElementToInt(content);
        else if (ename.compare("rs") == 0)          // SubPageView: Reset view on show; 1 = YES
            rs = xml->convertElementToInt(content);
        else if (ename.compare("on") == 0)          // SubPageView direction
            on = content;
        else if (ename.compare("ba") == 0)          // SubPageView scrollbar
            ba = xml->convertElementToInt(content);
        else if (ename.compare("bo") == 0)          // SubPageView scrollbar offset
            bo = xml->convertElementToInt(content);
        else if (ename.compare("we") == 0)          // SubViewPage Anchor position
            we = content;
        else if (ename.compare("hd") == 0)          // 1 = Hidden, 0 = Normal visible
            hd = xml->convertElementToInt(content);
        else if (ename.compare("da") == 0)          // 1 = Disabled, 0 = Normal active
            da = xml->convertElementToInt(content);
        else if (ename.compare("ac") == 0)          // Direction of text (guess)
        {
            ac_di = xml->getAttributeInt("di", attrs);  // 0 = left to right; 1 = right to left
        }
        else if (ename.compare("pf") == 0)          // Function call
        {
            PUSH_FUNC_T pf;
            pf.pfName = content;
            pf.pfType = xml->getAttribute("type", attrs);
            pushFunc.push_back(pf);
        }
        else if (ename.compare("sr") == 0)          // Section state resources
        {
            SR_T bsr;
            bsr.number = xml->getAttributeInt("number", attrs); // State number
            string e;

            while ((index = xml->getNextElementFromIndex(index, &e, &content, &attrs)) != TExpat::npos)
            {
                if (e.compare("do") == 0)           // Draw order
                    bsr._do = content;
                else if (e.compare("bs") == 0)      // Frame type
                    bsr.bs = content;
                else if (e.compare("mi") == 0)      // Chameleon image
                    bsr.mi = content;
                else if (e.compare("cb") == 0)      // Border color
                    bsr.cb = content;
                else if (e.compare("cf") == 0)      // Fill color
                    bsr.cf = content;
                else if (e.compare("ct") == 0)      // Text color
                    bsr.ct = content;
                else if (e.compare("ec") == 0)      // Text effect color
                    bsr.ec = content;
                else if (e.compare("bm") == 0)      // Bitmap image
                {
                    bsr.bm = content;
                    bsr.dynamic = ((xml->getAttributeInt("dynamic", attrs) == 1) ? true : false);
                }
                else if (e.compare("sd") == 0)      // Sound file
                    bsr.sd = content;
                else if (e.compare("sb") == 0)      // index external graphic
                    bsr.sb = xml->convertElementToInt(content);
                else if (e.compare("ii") == 0)      // Icon index
                    bsr.ii = xml->convertElementToInt(content);
                else if (e.compare("ji") == 0)      // Icon/bitmap orientation
                    bsr.ji = xml->convertElementToInt(content);
                else if (e.compare("jb") == 0)      // Bitmap orientation
                    bsr.jb = xml->convertElementToInt(content);
                else if (e.compare("bx") == 0)      // Absolute image position X
                    bsr.bx = xml->convertElementToInt(content);
                else if (e.compare("by") == 0)      // Absolute image position Y
                    bsr.by = xml->convertElementToInt(content);
                else if (e.compare("ix") == 0)      // Absolute Icon position X
                    bsr.ix = xml->convertElementToInt(content);
                else if (e.compare("iy") == 0)      // Absolute Icon position Y
                    bsr.iy = xml->convertElementToInt(content);
                else if (e.compare("fi") == 0)      // Font index
                    bsr.fi = xml->convertElementToInt(content);
                else if (e.compare("te") == 0)      // Text
                    bsr.te = content;
                else if (e.compare("jt") == 0)      // Text orientation
                    bsr.jt = (TEXT_ORIENTATION)xml->convertElementToInt(content);
                else if (e.compare("tx") == 0)      // Absolute text position X
                    bsr.tx = xml->convertElementToInt(content);
                else if (e.compare("ty") == 0)      // Absolute text position Y
                    bsr.ty = xml->convertElementToInt(content);
                else if (e.compare("ww") == 0)      // Word wrap
                    bsr.ww = xml->convertElementToInt(content);
                else if (e.compare("et") == 0)      // Text effects
                    bsr.et = xml->convertElementToInt(content);
                else if (e.compare("oo") == 0)      // Opacity
                    bsr.oo = xml->convertElementToInt(content);

                oldIndex = index;
            }

            sr.push_back(bsr);
        }

        if (index == TExpat::npos)
            index = oldIndex + 1;
    }

    visible = !hd;  // set the initial visibility
/*
    if (sr.size() > 0 && TStreamError::checkFilter(HLOG_DEBUG))
    {
        MSG_DEBUG("bi  : " << bi);
        MSG_DEBUG("na  : " << na);
        MSG_DEBUG("type: " << type);
        MSG_DEBUG("lt  : " << lt);
        MSG_DEBUG("tp  : " << tp);
        MSG_DEBUG("wt  : " << wt);
        MSG_DEBUG("ht  : " << ht);

        vector<SR_T>::iterator iter;
        size_t pos = 1;

        for (iter = sr.begin(); iter != sr.end(); ++iter)
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
            MSG_DEBUG("   " << pos << ": te: " << iter->te);
            pos++;
        }
    }
*/
    MSG_DEBUG("Added button " << bi << " --> " << na);

    if (index == TExpat::npos)
        return oldIndex + 1;

    return index;
}

bool TButton::createSoftButton(const EXTBUTTON_t& bt)
{
    DECL_TRACER("TButton::createSoftButton(const EXTBUTTON_t& bt)");

    if (bt.sr.size() < 2)
    {
        MSG_ERROR("Button " << bt.bi << ": " << bt.na << " has less than 2 states!");
        return false;
    }

    MSG_DEBUG("Adding soft button " << bt.bi << ": " << bt.na);
    type = bt.type;
    bi = bt.bi;
    na = bt.na;
    lt = bt.lt;
    tp = bt.tp;
    wt = bt.wt;
    ht = bt.ht;
    zo = bt.zo;
    hs = bt.hs;
    bs = bt.bs;
    fb = bt.fb;
    ap = bt.ap;
    ad = bt.ad;
    lp = bt.lp;
    lv = bt.lv;
    dr = bt.dr;
    lu = bt.lu;
    ld = bt.ld;
    rl = bt.rl;
    rh = bt.rh;
    rn = bt.rn;
    sc = bt.sc;
    sr = bt.sr;
    mChanged = true;
    return true;
}

BITMAP_t TButton::getLastImage()
{
    DECL_TRACER("TButton::getLastImage()");

    if (mLastImage.empty())
    {
        makeElement(mActInstance);

        if (mLastImage.empty())
            return BITMAP_t();
    }

    BITMAP_t image;
    image.buffer = (unsigned char *)mLastImage.getPixels();
    image.rowBytes = mLastImage.info().minRowBytes();
    image.width = mLastImage.info().width();
    image.height = mLastImage.info().height();
    return image;
}

TBitmap TButton::getLastBitmap()
{
    DECL_TRACER("TButton::getLastBitmap()");

    if (mLastImage.empty())
        makeElement(mActInstance);

    TBitmap bitmap((unsigned char *)mLastImage.getPixels(), mLastImage.info().width(), mLastImage.info().height());
    return bitmap;
}

FONT_T TButton::getFont()
{
    DECL_TRACER("TButton::getFont()");

    if (!mFonts)
    {
        MSG_ERROR("No fonts available!");
        return FONT_T();
    }

    if (type == LISTBOX && _getGlobalSettings)
    {
        _getGlobalSettings(this);
        mActInstance = 0;
    }

    return mFonts->getFont(sr[mActInstance].fi);
}

FONT_STYLE TButton::getFontStyle()
{
    DECL_TRACER("TButton::getFontStyle()");

    if (!mFonts)
    {
        MSG_ERROR("No fonts available!");
        return FONT_NONE;
    }

    return mFonts->getStyle(sr[mActInstance].fi);
}

void TButton::setBargraphLevel(int level)
{
    DECL_TRACER("TButton::setBargraphLevel(int level)");

    if (type != BARGRAPH && type != MULTISTATE_BARGRAPH && type != MULTISTATE_GENERAL)
        return;

    if (((type == BARGRAPH || type == MULTISTATE_BARGRAPH) && (level < rl || level > rh)) ||
        (type == MULTISTATE_GENERAL && (level < 0 || (size_t)level >= sr.size())))
    {
        MSG_WARNING("Level for bargraph " << na << " is out of range! (" << rl << " to " << rh << " or size " << sr.size() << ")");
        return;
    }

    if (((type == BARGRAPH || type == MULTISTATE_BARGRAPH) && mLastLevel != level) ||
        (type == MULTISTATE_BARGRAPH && mActInstance != level))
        mChanged = true;

    if (!mChanged)
        return;

    if (type == BARGRAPH)
    {
        mLastLevel = level;
        drawBargraph(mActInstance, level);
    }
    else if (type == MULTISTATE_BARGRAPH)
    {
        mLastLevel = level;
        mActInstance = level;
        drawMultistateBargraph(level);
    }
    else
        setActive(level);
}

bool TButton::invalidate()
{
    DECL_TRACER("TButton::invalidate()");

    if (prg_stopped)
        return true;

    ulong parent = mHandle & 0xffff0000;
    THR_REFRESH_t *tr = _findResource(mHandle, parent, bi);

    if (tr && tr->mImageRefresh)
    {
        if (tr->mImageRefresh->isRunning())
            tr->mImageRefresh->stop();
    }

    if (type == TEXT_INPUT)
    {
        if (gPageManager && gPageManager->getCallDropButton())
            gPageManager->getCallDropButton()(mHandle);
    }

    visible = false;
    return true;
}

string& TButton::getDrawOrder(int instance)
{
    DECL_TRACER("TButton::getDrawOrder(int instance)");

    if (instance < 0 || (size_t)instance > sr.size())
    {
        MSG_ERROR("Instance is out of range!");
        return dummy;
    }

    return sr[instance]._do;
}

BUTTONTYPE TButton::getButtonType(const string& bt)
{
    DECL_TRACER("TButton::getButtonType(const string& bt)");

    if (strCaseCompare(bt, "general") == 0)
        return GENERAL;
    else if (strCaseCompare(bt, "multi-state general") == 0 || strCaseCompare(bt, "multiGeneral") == 0)
        return MULTISTATE_GENERAL;
    else if (strCaseCompare(bt, "bargraph") == 0)
        return BARGRAPH;
    else if (strCaseCompare(bt, "multi-state bargraph") == 0 || strCaseCompare(bt, "multiBargraph") == 0)
        return MULTISTATE_BARGRAPH;
    else if (strCaseCompare(bt, "joistick") == 0)
        return JOISTICK;
    else if (strCaseCompare(bt, "text input") == 0 || strCaseCompare(bt, "textArea") == 0)
        return TEXT_INPUT;
    else if (strCaseCompare(bt, "computer control") == 0)
        return COMPUTER_CONTROL;
    else if (strCaseCompare(bt, "take note") == 0)
        return TAKE_NOTE;
    else if (strCaseCompare(bt, "sub-page view") == 0 || strCaseCompare(bt, "subPageView") == 0)
        return SUBPAGE_VIEW;
    else if (strCaseCompare(bt, "listBox") == 0)
        return LISTBOX;

    return NONE;
}

string TButton::buttonTypeToString()
{
    switch(type)
    {
        case NONE:                  return "NONE";
        case GENERAL:               return "GENERAL";
        case MULTISTATE_GENERAL:    return "MULTISTAE GENERAL";
        case BARGRAPH:              return "BARGRAPH";
        case MULTISTATE_BARGRAPH:   return "MULTISTATE BARGRAPH";
        case JOISTICK:              return "JOISTICK";
        case TEXT_INPUT:            return "TEXT INPUT";
        case COMPUTER_CONTROL:      return "COMPUTER CONTROL";
        case TAKE_NOTE:             return "TAKE NOTE";
        case SUBPAGE_VIEW:          return "SUBPAGE VIEW";
        case LISTBOX:               return "LISTBOX";
    }

    return "";
}

FEEDBACK TButton::getButtonFeedback(const string& fb)
{
    DECL_TRACER("TButton::getButtonFeedback(const string& fb)");

    if (fb.compare("channel") == 0)
        return FB_CHANNEL;
    else if (fb.compare("inverted channel") == 0)
        return FB_INV_CHANNEL;
    else if (fb.compare("always on") == 0)
        return FB_ALWAYS_ON;
    else if (fb.compare("momentary") == 0)
        return FB_MOMENTARY;
    else if (fb.compare("blink") == 0)
        return FB_BLINK;

    return FB_NONE;
}

bool TButton::createButtons(bool force)
{
    DECL_TRACER("TButton::createButtons(bool force)");

    if (prg_stopped)
        return false;

    if (force)
    {
        mChanged = true;
        MSG_TRACE("Creating of image is forced!");
    }

    // Get the images, if there any
    vector<SR_T>::iterator srIter;

    for (srIter = sr.begin(); srIter != sr.end(); ++srIter)
    {
        int number = srIter->number;

        if (srIter->sb > 0)
            continue;

        bool bmExistMi = false;
        bool bmExistBm = false;
        bool reload = false;

        if (!srIter->mi.empty())
        {
            if ((bmExistMi = TImgCache::existBitmap(srIter->mi, _BMTYPE_CHAMELEON)) == false)
            {
                mChanged = true;
                reload = true;
            }
        }

        if (!srIter->bm.empty())
        {
            if ((bmExistBm = TImgCache::existBitmap(srIter->bm, _BMTYPE_BITMAP)) == false)
            {
                mChanged = true;
                reload = true;
            }
        }

        if (!force)
        {
            if (!reload)   // If the image already exist, do not load it again.
                continue;
        }

        if (!bmExistMi && !srIter->mi.empty())        // Do we have a chameleon image?
        {
            sk_sp<SkData> image;
            SkBitmap bm;

            if (!(image = readImage(srIter->mi)))
                return false;

            DecodeDataToBitmap(image, &bm);

            if (bm.empty())
            {
                MSG_WARNING("Could not create a picture for element " << number << " on button " << bi << " (" << na << ")");
                return false;
            }

            TImgCache::addImage(srIter->mi, bm, _BMTYPE_CHAMELEON);
            srIter->mi_width = bm.info().width();
            srIter->mi_height = bm.info().height();
            mChanged = true;
        }

        if (!bmExistBm && !srIter->bm.empty())        // Do we have a bitmap?
        {
            sk_sp<SkData> image;
            SkBitmap bm;

            if (!(image = readImage(srIter->bm)))
                return false;

            DecodeDataToBitmap(image, &bm);

            if (bm.empty())
            {
                MSG_WARNING("Could not create a picture for element " << number << " on button " << bi << " (" << na << ")");
                return false;
            }

            TImgCache::addImage(srIter->bm, bm, _BMTYPE_BITMAP);
            srIter->bm_width = bm.info().width();
            srIter->bm_height = bm.info().height();
            mChanged = true;
        }
    }

    return true;
}

void TButton::refresh()
{
    DECL_TRACER("TButton::refresh()");

    mChanged = true;
    makeElement();
}

bool TButton::makeElement(int instance)
{
    DECL_TRACER("TButton::makeElement(int instance)");

    if (prg_stopped)
        return false;

    int inst = mActInstance;

    if (instance >= 0 && (size_t)instance < sr.size())
    {
        if (mActInstance != instance)
            mChanged = true;

        inst = instance;
    }

    bool isSystem = isSystemButton();

    if (type == MULTISTATE_GENERAL && ar == 1)
        return drawButtonMultistateAni();
    else if (type == BARGRAPH && isSystem && lv == 9)   // System volume button
        return drawBargraph(inst, TConfig::getSystemVolume());
    else if (type == BARGRAPH)
        return drawBargraph(inst, mLastLevel);
    else if (type == MULTISTATE_BARGRAPH)
        return drawMultistateBargraph(mLastLevel, true);
    else if (type == TEXT_INPUT)
    {
        if (isSystem && !mSystemReg)
        {
            registerSystemButton();
            mChanged = true;
        }

        drawTextArea(inst);
        mActInstance = inst;
    }
    else if (type == LISTBOX)
    {
        if (_getListContent && !mSystemReg)
        {
            mListContent = _getListContent(mHandle, ap, ta, ti, tr, tc);
            mChanged = true;
        }

        if (isSystem)
            mSystemReg = true;

        drawList();
    }
    else if (isSystem && type == GENERAL)
    {
        TConfig::setTemporary(true);

        if (isSystemCheckBox(ch))
        {
            int in = getButtonInstance(0, ch);

            if (in >= 0)
            {
                inst = mActInstance = in;
#ifndef __ANDROID__
                if (ch == SYSTEM_ITEM_VIEWSCALEFIT && sr[0].oo < 0) // scale to fit disabled
                {
                    sr[0].oo = 128;
                    mChanged = true;
                }
#else
                if (ch == SYSTEM_ITEM_VIEWBANNER && sr[0].oo < 0)   // show banner disabled
                {
                    sr[0].oo = 128;
                    mChanged = true;
                }
#endif
                if (ch == SYSTEM_ITEM_VIEWTOOLBAR)  // Force toolbar is only available if toolbar is not suppressed
                {
                    if (TConfig::getToolbarSuppress() && sr[0].oo < 0)
                    {
                        sr[0].oo = 128;
                        mChanged = true;
                    }
                    else if (!TConfig::getToolbarSuppress() && sr[0].oo > 0)
                    {
                        sr[0].oo = -1;
                        mChanged = true;
                    }
                }
            }
        }
        else if (isSystemTextLine(ad) && ad != SYSTEM_ITEM_FTPSURFACE)
        {
            sr[0].te = sr[1].te = fillButtonText(ad, 0);
            mChanged = true;
        }

        TConfig::setTemporary(false);

        if (mLastImage.empty())
            mChanged = true;

        MSG_DEBUG("Drawing system button " << ch << " with instance " << inst);
        return drawButton(inst);
    }
    else
    {
        if (mLastImage.empty())
            mChanged = true;

        return drawButton(inst);
    }

    return false;
}

bool TButton::setActive(int instance)
{
    DECL_TRACER("TButton::setActive(int instance)");

    if (mAniRunning)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return true;
    }

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " is out of range from 0 to " << sr.size() << "!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (instance == mActInstance && !mLastImage.empty())
    {
#if TESTMODE == 1
        __success = true;
        setScreenDone();
#endif
        return true;
    }

    mActInstance = instance;
    mChanged = true;
    makeElement(instance);

    return true;
}

bool TButton::setIcon(int id, int instance)
{
    DECL_TRACER("TButton::setIcon(int id, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].ii != id)
            mChanged = true;

        sr[inst].ii = id;
        inst++;
    }

    return makeElement(instance);
}

bool TButton::setIcon(const string& icon, int instance)
{
    DECL_TRACER("TButton::setIcon(const string& icon, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (!gIcons)
    {
        gIcons = new TIcons();

        if (TError::isError())
        {
            MSG_ERROR("Error initializing icons!");
            return false;
        }
    }

    int id = gIcons->getNumber(icon);

    if (id == -1)
    {
        MSG_WARNING("Icon " << icon << " not found!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].ii == id)
        {
            inst++;
            continue;
        }

        if (sr[inst].ii != id)
            mChanged = true;

        sr[inst].ii = id;
        inst++;
    }

    return makeElement(instance);
}

bool TButton::revokeIcon(int instance)
{
    DECL_TRACER("TButton::revokeIcon(int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].ii == 0)
        {
            inst++;
            continue;
        }

        if (sr[inst].ii != 0)
            mChanged = true;

        sr[inst].ii = 0;
        inst++;
    }

    return makeElement(instance);
}

bool TButton::setText(const string& txt, int instance)
{
    DECL_TRACER("TButton::setText(const string& txt, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
#if TESTMODE == 1
        setAllDone();
#endif
        return false;
    }

    if (!setTextOnly(txt, instance))
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return false;
    }

    if (!mChanged)      // Do not try to redraw the button if nothing changed
    {
#if TESTMODE == 1
        MSG_INFO("Nothing changed!");
        __success = true;
        setScreenDone();
#endif
        return true;
    }

    return makeElement(instance);
}

bool TButton::setTextOnly(const string& txt, int instance)
{
    DECL_TRACER("TButton::setTextOnly(const string& txt, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); ++i)
        {
            if (sr[i].te != txt && (int)i == mActInstance)
                mChanged = true;

            sr[i].te = txt;
        }
    }
    else
    {
        if (sr[instance].te != txt && (int)instance == mActInstance)
            mChanged = true;

        sr[instance].te = txt;
    }

    if (instance <= 0 && isSystemButton())
    {
        bool temp = TConfig::setTemporary(true);
        // If we've an input line or the text line of a "combobox" then we'll
        // save the changed value here.
        switch(ad)
        {
            case SYSTEM_ITEM_NETLINX_IP:        TConfig::saveController(txt); break;
            case SYSTEM_ITEM_NETLINX_CHANNEL:   TConfig::saveChannel(atoi(txt.c_str())); break;
            case SYSTEM_ITEM_NETLINX_PORT:      TConfig::savePort(atoi(txt.c_str())); break;
            case SYSTEM_ITEM_NETLINX_PTYPE:     TConfig::savePanelType(txt); break;

            case SYSTEM_ITEM_SYSTEMSOUND:       TConfig::saveSystemSoundFile(txt); break;
            case SYSTEM_ITEM_SINGLEBEEP:        TConfig::saveSingleBeepFile(txt); break;
            case SYSTEM_ITEM_DOUBLEBEEP:        TConfig::saveDoubleBeepFile(txt); break;

            case SYSTEM_ITEM_SIPPROXY:          TConfig::setSIPproxy(txt); break;
            case SYSTEM_ITEM_SIPPORT:           TConfig::setSIPport(atoi(txt.c_str())); break;
            case SYSTEM_ITEM_SIPSTUN:           TConfig::setSIPstun(txt); break;
            case SYSTEM_ITEM_SIPDOMAIN:         TConfig::setSIPdomain(txt); break;
            case SYSTEM_ITEM_SIPUSER:           TConfig::setSIPuser(txt); break;
            case SYSTEM_ITEM_SIPPASSWORD:       TConfig::setSIPpassword(txt); break;

            case SYSTEM_ITEM_LOGLOGFILE:        TConfig::saveLogFile(txt); break;

            case SYSTEM_ITEM_FTPUSER:           TConfig::saveFtpUser(txt); break;
            case SYSTEM_ITEM_FTPPASSWORD:       TConfig::saveFtpPassword(txt); break;
            case SYSTEM_ITEM_FTPSURFACE:        TConfig::saveFtpSurface(txt); break;
        }

        TConfig::setTemporary(temp);
    }

    return true;
}

bool TButton::appendText(const string &txt, int instance)
{
    DECL_TRACER("TButton::appendText(const string &txt, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (txt.empty())
    {
#if TESTMODE == 1
        __success = true;
        __done = true;
#endif
        return true;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); ++i)
            sr[i].te.append(txt);
    }
    else
        sr[instance].te.append(txt);

    mChanged = true;
    return makeElement(instance);
}

void TButton::setTextCursorPosition(int oldPos, int newPos)
{
    DECL_TRACER("TButton::setTextCursorPosition(int oldPos, int newPos)");

    if (type != TEXT_INPUT)
        return;

    if (oldPos == newPos && newPos == mCursorPosition)
        return;

    mCursorPosition = newPos;
}

void TButton::setTextFocus(bool in)
{
    DECL_TRACER("TButton::setTextFocus(bool in)");

    if (type != TEXT_INPUT)
        return;

    mHasFocus = in;

    if (mHasFocus && mActInstance != STATE_ON)
        makeElement(STATE_ON);
    else if (!mHasFocus && mActInstance != STATE_OFF)
        makeElement(STATE_OFF);
}

bool TButton::setBorderColor(const string &color, int instance)
{
    DECL_TRACER("TButton::setBorderColor(const string &color, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].cb.compare(color) == 0)
        {
            inst++;
            continue;
        }

        sr[inst].cb = color;
        mChanged = true;
        inst++;
    }

    return makeElement(instance);
}

string TButton::getBorderColor(int instance)
{
    DECL_TRACER("TButton::getBorderColor(int instance)");

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return string();
    }

    return sr[instance].cb;
}

bool TButton::setFillColor(const string& color, int instance)
{
    DECL_TRACER("TButton::setFillColor(const string& color, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].cf.compare(color) == 0)
        {
            inst++;
            continue;
        }

        sr[inst].cf = color;
        mChanged = true;
        inst++;
    }

    return makeElement(instance);
}

bool TButton::setTextColor(const string& color, int instance)
{
    DECL_TRACER("TButton::setTextColor(const string& color, int instance)");

    if (!setTextColorOnly(color, instance))
        return false;

    return makeElement(instance);
}

bool TButton::setTextColorOnly(const string& color, int instance)
{
    DECL_TRACER("TButton::setTextColorOnly(const string& color, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].ct.compare(color) == 0)
        {
            inst++;
            continue;
        }

        sr[inst].ct = color;
        inst++;
        mChanged = true;
    }

    return true;
}

bool TButton::setDrawOrder(const string& order, int instance)
{
    DECL_TRACER("TButton::setDrawOrder(const string& order, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst]._do.compare(order) == 0)
        {
            inst++;
            continue;
        }

        sr[inst]._do = order;
        inst++;
        mChanged = true;
    }

    return makeElement(instance);
}

FEEDBACK TButton::getFeedback()
{
    DECL_TRACER("TButton::getFeedback()");

    if (type != GENERAL)
        return FB_NONE;

    return fb;
}

bool TButton::setFeedback(FEEDBACK feedback)
{
    DECL_TRACER("TButton::setFeedback(FEEDBACK feedback)");

    if (type != GENERAL)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return false;
    }

    int oldFB = fb;
    fb = feedback;

    if (mEnabled && !hd)
    {
        if ((feedback == FB_ALWAYS_ON || feedback == FB_INV_CHANNEL) && mActInstance != 1)
        {
            mActInstance = 1;
            mChanged = true;
            makeElement(1);
        }
        else if (oldFB == FB_ALWAYS_ON && feedback != FB_ALWAYS_ON && feedback != FB_INV_CHANNEL && mActInstance == 1)
        {
            mActInstance = 0;
            mChanged = true;
            makeElement(0);
        }
    }
#if TESTMODE == 1
    if (!mChanged)
        __success = true;

    setScreenDone();
#endif
    return true;
}

bool TButton::setBorderStyle(const string& style, int instance)
{
    DECL_TRACER("TButton::setBorderStyle(const string& style, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    mChanged = true;
    MSG_DEBUG("Setting border " << style);

    if (strCaseCompare(style, "None") == 0)     // Clear the border?
    {
        if (instance < 0)
        {
            bs.clear();

            for (size_t i = 0; i < sr.size(); ++i)
                sr[i].bs.clear();
        }
        else
        {
            sr[instance].bs.clear();
            bs.clear();
        }

        if (mEnabled && !hd)
            makeElement(instance);

        return true;
    }

    // Look in the system table and try to find the border.
    if (gPageManager && gPageManager->getSystemDraw())
    {
        if (gPageManager->getSystemDraw()->existBorder(style))
        {
            if (instance < 0)
            {
                bs = style;

                for (size_t i = 0; i < sr.size(); ++i)
                    sr[i].bs = style;
            }
            else
            {
                sr[instance].bs = style;

                if (bs != style)
                    bs.clear();
            }

            if (mEnabled && !hd)
                makeElement(instance);

            return true;
        }
    }

    // Check whether it is a supported style or not. If the style is not
    // supported, it will be ignored.
    string corrName = getCorrectName(style);

    if (!style.empty())
    {
        if (instance < 0)
        {
            bs = corrName;

            for (size_t i = 0; i < sr.size(); ++i)
                sr[i].bs = corrName;
        }
        else
        {
            sr[instance].bs = corrName;

            if (bs != corrName)
                bs.clear();
        }

        if (mEnabled && !hd)
            makeElement(instance);

        return true;
    }
#if TESTMODE == 1
    __done = true;
#endif
    return false;
}

bool TButton::setBorderStyle(int style, int instance)
{
    DECL_TRACER("TButton::setBorderStyle(int style, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (style == 0)     // Clear the border?
    {
        if (instance < 0)
        {
            bs.clear();

            for (size_t i = 0; i < sr.size(); ++i)
            {
                if (!sr[i].bs.empty())
                    mChanged = true;

                sr[i].bs.clear();
            }

            if (!bs.empty())
                mChanged = true;

            bs.clear();
        }
        else
        {
            if (!sr[instance].bs.empty())
                mChanged = true;

            sr[instance].bs.clear();
            bs.clear();
        }

        if (mEnabled && !hd)
            makeElement(instance);

        return true;
    }

    string st = getBorderName(style);

    if (st.empty())
    {
        MSG_WARNING("The index " << style << " is not supported!");
#if TESTMODE == 1
        setAllDone();
#endif
        return false;
    }

    // Look in the system table and try to find the border.
    if (gPageManager && gPageManager->getSystemDraw())
    {
        if (gPageManager->getSystemDraw()->existBorder(st))
        {
            MSG_DEBUG("Found frame " << st << " and draw it ...");

            if (instance < 0)
            {
                bs = st;

                for (size_t i = 0; i < sr.size(); ++i)
                    sr[i].bs = st;
            }
            else
            {
                sr[instance].bs = st;

                if (bs != st)
                    bs.clear();
            }

            mChanged = true;

            if (mEnabled && !hd)
                makeElement(instance);

            return true;
        }
    }

    // Check whether it is a supported style or not. If the style is not
    // supported, it will be ignored.
    if (instance < 0)
    {
        bs = st;

        for (size_t i = 0; i < sr.size(); ++i)
            sr[i].bs = st;
    }
    else
    {
        sr[instance].bs = st;

        if (bs != st)
            bs.clear();
    }

    mChanged = true;

    if (mEnabled && !hd)
        makeElement(instance);

    return true;
}

string TButton::getBorderStyle(int instance)
{
    DECL_TRACER("TButton::getBorderStyle(int instance)");

    if (instance < 0 || instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << (instance + 1) << " submitted!");
        return string();
    }

    if (sr[instance].bs.empty())
        return bs;

    return sr[instance].bs;
}

bool TButton::setBargraphUpperLimit(int limit)
{
    DECL_TRACER("TButton::setBargraphUpperLimit(int limit)");

    if (limit < 1 || limit > 65535)
    {
        MSG_ERROR("Invalid upper limit " << limit);
        return false;
    }

    rh = limit;
    return true;
}

bool TButton::setBargraphLowerLimit(int limit)
{
    DECL_TRACER("TButton::setBargraphLowerLimit(int limit)");

    if (limit < 1 || limit > 65535)
    {
        MSG_ERROR("Invalid lower limit " << limit);
        return false;
    }

    rl = limit;
    return true;
}

bool TButton::setBargraphSliderColor(const string& color)
{
    DECL_TRACER("TButton::setBargraphSliderColor(const string& color, int inst)");

    if (!TColor::isValidAMXcolor(color))
    {
        MSG_PROTOCOL("Invalid color >" << color << "< ignored!");
        return false;
    }

    if (sc != color)
        mChanged = true;

    sc = color;

    if (visible)
        refresh();

    return true;
}

bool TButton::setFontFileName(const string& name, int /*size*/, int instance)
{
    DECL_TRACER("TButton::setFontFileName(const string& name, int size)");

    if (name.empty() || !mFonts)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if ((size_t)instance >= sr.size())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    int id = mFonts->getFontIDfromFile(name);

    if (id == -1)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); ++i)
        {
            if (sr[i].fi != id)
                mChanged = true;

            sr[i].fi = id;
        }
    }
    else if (sr[instance].fi != id)
    {
        mChanged = true;
        sr[instance].fi = id;
    }
#if TESTMODE == 1
    setScreenDone();
#endif
    return true;
}

bool TButton::setFontName(const string &name, int instance)
{
    DECL_TRACER("TButton::setFontName(const string &name, int instance)");

    if (name.empty() || !mFonts)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if ((size_t)instance >= sr.size())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    int id = mFonts->getFontIDfromName(name);

    if (id == -1)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); ++i)
        {
            if (sr[i].fi != id)
                mChanged = true;

            sr[i].fi = id;
        }
    }
    else if (sr[instance].fi != id)
    {
        mChanged = true;
        sr[instance].fi = id;
    }
#if TESTMODE == 1
    setScreenDone();
#endif
    return true;
}

bool TButton::setBitmap(const string& file, int instance)
{
    DECL_TRACER("TButton::setBitmap(const string& file, int instance)");

    if (instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid parameters!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].bm == file)
        {
            inst++;
            continue;
        }

        mChanged = true;
        sr[inst].bm = file;

        if (!file.empty() && !TImgCache::existBitmap(file, _BMTYPE_BITMAP))
        {
            sk_sp<SkData> image;
            SkBitmap bm;

            image = readImage(file);

            if (image)
            {
                DecodeDataToBitmap(image, &bm);

                if (!bm.empty())
                {
                    TImgCache::addImage(sr[inst].bm, bm, _BMTYPE_BITMAP);
                    sr[inst].bm_width = bm.info().width();
                    sr[inst].bm_height = bm.info().height();
                }
            }
        }

        inst++;
    }

    if (!createButtons(true))   // We're forcing the image to load
        return false;

    return makeElement(instance);
}

bool TButton::setCameleon(const string& file, int instance)
{
    DECL_TRACER("TButton::setCameleon(const string& file, int instance)");

    if (file.empty() || instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid parameters!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].mi == file)
        {
            inst++;
            continue;
        }

        mChanged = true;
        sr[inst].mi = file;

        if (!file.empty() && !TImgCache::existBitmap(file, _BMTYPE_CHAMELEON))
        {
            sk_sp<SkData> image;
            SkBitmap bm;

            image = readImage(file);

            if (image)
            {
                DecodeDataToBitmap(image, &bm);

                if (!bm.empty())
                {
                    TImgCache::addImage(sr[inst].mi, bm, _BMTYPE_CHAMELEON);
                    sr[inst].mi_width = bm.info().width();
                    sr[inst].mi_height = bm.info().height();
                }
            }
        }

        inst++;
    }

    if (!createButtons(true))   // We're forcing the image to load
        return false;

    return makeElement(instance);
}

bool TButton::setInputMask(const std::string& mask)
{
    DECL_TRACER("TButton::setInputMask(const std::string& mask)");

    vector<char> mTable = { '0', '9', '#', 'L', '?', 'A', 'a', '&', 'C',
                            '[', ']', '|', '{', '}', '<', '>', '^' };
    vector<char>::iterator iter;

    for (size_t i = 0; i < mask.length(); ++i)
    {
        bool found = false;

        for (iter = mTable.begin(); iter != mTable.end(); ++iter)
        {
            if (mask[i] == *iter)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            MSG_WARNING("The mask letter " << mask[i] << " is invalid!");
            return false;
        }
    }

    im = mask;
    return true;
}

void TButton::setActiveInstance(int inst)
{
    DECL_TRACER("TButton::setActiveInstance()");

    if (inst < 0 || (size_t)inst >= sr.size())
        return;

    if (mActInstance != inst)
        mChanged = true;

    mActInstance = inst;
}

SUBVIEW_POSITION_t TButton::getSubViewAnchor()
{
    DECL_TRACER("TButton::getSubViewAnchor()");

    if (we.empty())
        return SVP_CENTER;
    else if (strCaseCompare(we, "l/t") == 0)
        return SVP_LEFT_TOP;
    else if (strCaseCompare(we, "r/b") == 0)
        return SVP_RIGHT_BOTTOM;

    return SVP_CENTER;
}

bool TButton::getDynamic(int inst)
{
    DECL_TRACER("TButton::getDynamic(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return false;
    }

    return sr[inst].dynamic;
}

void TButton::setDynamic(int d, int inst)
{
    DECL_TRACER("TButton::setDynamic(int d, int inst)");

    if (inst >= (int)sr.size())
    {
        MSG_ERROR("Instance is out of size!");
        return;
    }

    bool dyn = (d != 0) ? true : false;

    if (inst < 0)
    {
        vector<SR_T>::iterator iter;
        int instance = 0;

        for (iter = sr.begin(); iter != sr.end(); ++iter)
        {
            bool old = iter->dynamic;
            iter->dynamic = dyn;

            if (old && old != dyn && mActInstance == instance)
            {
                THR_REFRESH_t *thref = _findResource(mHandle, getParent(), bi);

                if (thref)
                {
                    TImageRefresh *mImageRefresh = thref->mImageRefresh;

                    if (mImageRefresh)
                        mImageRefresh->stop();
                }

                mChanged = true;
                makeElement(instance);
            }

            instance++;
        }
    }
    else
    {
        bool old = sr[inst].dynamic;
        sr[inst].dynamic = dyn;

        if (old && old != dyn && mActInstance == inst)
        {
            THR_REFRESH_t *thref = _findResource(mHandle, getParent(), bi);

            if (thref)
            {
                TImageRefresh *mImageRefresh = thref->mImageRefresh;

                if (mImageRefresh)
                    mImageRefresh->stop();
            }

            mChanged = true;
            makeElement(inst);
        }
    }
}

int TButton::getOpacity(int inst)
{
    DECL_TRACER("TButoon::getOpacity(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return 0;
    }

    return sr[inst].oo;
}

bool TButton::setOpacity(int op, int instance)
{
    DECL_TRACER("TButton::setOpacity(int op, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (op < 0 || op > 255)
    {
        MSG_ERROR("Invalid opacity " << op << "!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); ++i)
        {
            if (sr[i].oo == op)
                continue;

            sr[i].oo = op;
            mChanged = true;
        }
    }
    else if (sr[instance].oo != op)
    {
        sr[instance].oo = op;
        mChanged = true;
    }

    if (!mChanged)
    {
#if TESTMODE == 1
        __success = true;
        setScreenDone();
#endif
        return true;
    }

    return makeElement(instance);
}

bool TButton::setFont(int id, int instance)
{
    DECL_TRACER("TButton::setFont(int id)");

    if (!setFontOnly(id, instance))
        return false;

    return makeElement(instance);
}

bool TButton::setFontOnly(int id, int instance)
{
    DECL_TRACER("TButton::setFontOnly(int id)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); ++i)
        {
            if (sr[i].fi != id)
            {
                mChanged = true;
                sr[i].fi = id;
            }
        }
    }
    else if (sr[instance].fi != id)
    {
        mChanged = true;
        sr[instance].fi = id;
    }

    return true;
}

void TButton::setLeft(int left)
{
    DECL_TRACER("TButton::setLeft(int left)");

    if (left < 0)
        return;

    if (lt != left)
        mChanged = true;

    lt = left;
    makeElement(mActInstance);
}

void TButton::setTop(int top)
{
    DECL_TRACER("TButton::setTop(int top)");

    if (top < 0)
        return;

    if (tp != top)
        mChanged = true;

    tp = top;
    makeElement(mActInstance);
}

void TButton::setLeftTop(int left, int top)
{
    DECL_TRACER("TButton::setLeftTop(int left, int top)");

    if (top < 0 || left < 0)
        return;

    if (lt != left || tp != top)
        mChanged = true;
    else
        return;

    lt = left;
    tp = top;
    makeElement(mActInstance);
}

void TButton::setRectangle(int left, int top, int right, int bottom)
{
    DECL_TRACER("setRectangle(int left, int top, int right, int bottom)");

    if (!gPageManager)
        return;

    int screenWidth = gPageManager->getSettings()->getWidth();
    int screenHeight = gPageManager->getSettings()->getHeight();
    int width = right - left;
    int height = bottom - top;

    if (left >= 0 && right > left && (left + width) < screenWidth)
        lt = left;

    if (top >= 0 && bottom > top && (top + height) < screenHeight)
        tp = top;

    if (left >= 0 && right > left)
        wt = width;

    if (top >= 0 && bottom > top)
        ht = height;
}

void TButton::getRectangle(int *left, int *top, int *height, int *width)
{
    DECL_TRACER("TButton::getRectangle(int *left, int *top, int *height, int *width)");

    if (left)
        *left = lt;

    if (top)
        *top = tp;

    if (height)
        *height = ht;

    if (width)
        *width = wt;

}

void TButton::setResourceName(const string& name, int instance)
{
    DECL_TRACER("TButton::setResourceName(const string& name, int instance)");

    if (instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (!sr[inst].dynamic)
        {
            inst++;
            continue;
        }

        if (sr[inst].bm != name)
            mChanged = true;

        sr[inst].bm = name;
        inst++;
    }
}

int TButton::getBitmapJustification(int* x, int* y, int instance)
{
    DECL_TRACER("TButton::getBitmapJustification(int* x, int* y, int instance)");

    if (instance < 0 || instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << (instance + 1));
        return -1;
    }

    if (x)
        *x = sr[instance].jb == 0 ? sr[instance].bx : 0;

    if (y)
        *y = sr[instance].jb == 0 ? sr[instance].by : 0;

    return sr[instance].jb;
}

void TButton::setBitmapJustification(int j, int x, int y, int instance)
{
    DECL_TRACER("TButton::setBitmapJustification(int j, int instance)");

    if (j < 0 || j > 9 || instance >= (int)sr.size())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); i++)
        {
            if (sr[i].jb != j)
                mChanged = true;

            sr[i].jb = j;

            if (j == 0)
            {
                sr[i].bx = x;
                sr[i].by = y;
            }
        }
    }
    else
    {
        if (sr[instance].jb != j)
            mChanged = true;

        sr[instance].jb = j;

        if (j == 0)
        {
            sr[instance].bx = x;
            sr[instance].by = y;
        }
    }

    makeElement();
}

int TButton::getIconJustification(int* x, int* y, int instance)
{
    DECL_TRACER("TButton::getIconJustification(int* x, int* y, int instance)");

    if (instance < 0 || instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << (instance + 1));
        return -1;
    }

    if (x)
        *x = sr[instance].ji == 0 ? sr[instance].ix : 0;

    if (y)
        *y = sr[instance].ji == 0 ? sr[instance].iy : 0;

    return sr[instance].ji;
}

void TButton::setIconJustification(int j, int x, int y, int instance)
{
    DECL_TRACER("TButton::setIconJustification(int j, int x, int y, int instance)");

    if (j < 0 || j > 9 || instance >= (int)sr.size())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); i++)
        {
            if (sr[i].ji != j)
                mChanged = true;

            sr[i].ji = j;

            if (j == 0)
            {
                sr[i].ix = x;
                sr[i].iy = y;
            }
        }
    }
    else
    {
        if (sr[instance].ji != j)
            mChanged = true;

        sr[instance].ji = j;

        if (j == 0)
        {
            sr[instance].ix = x;
            sr[instance].iy = y;
        }
    }

    makeElement();
}

int TButton::getTextJustification(int* x, int* y, int instance)
{
    DECL_TRACER("TButton::getTextJustification(int* x, int* y, int instance)");

    if (instance < 0 || instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << (instance + 1));
        return -1;
    }

    if (x)
        *x = sr[instance].jt == 0 ? sr[instance].tx : 0;

    if (y)
        *y = sr[instance].jt == 0 ? sr[instance].ty : 0;

    return sr[instance].jt;
}

void TButton::setTextJustification(int j, int x, int y, int instance)
{
    DECL_TRACER("TButton::setTextJustification(int j, int x, int y, int instance)");

    if (!setTextJustificationOnly(j, x, y, instance))
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    makeElement();
}

bool TButton::setTextJustificationOnly(int j, int x, int y, int instance)
{
    DECL_TRACER("TButton::setTextJustificationOnly(int j, int x, int y, int instance)");

    if (j < 0 || j > 9 || instance >= (int)sr.size())
        return false;

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); i++)
        {
            if (sr[i].jt != j)
                mChanged = true;

            sr[i].jt = (TEXT_ORIENTATION)j;

            if (j == 0)
            {
                sr[i].tx = x;
                sr[i].ty = y;
            }
        }
    }
    else
    {
        if (sr[instance].jt != j)
            mChanged = true;

        sr[instance].jt = (TEXT_ORIENTATION)j;

        if (j == 0)
        {
            sr[instance].tx = x;
            sr[instance].ty = y;
        }
    }

    return true;
}

string TButton::getText(int inst)
{
    DECL_TRACER("TButton::getText(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return string();
    }

    return sr[inst].te;
}

string TButton::getTextColor(int inst)
{
    DECL_TRACER("TButton::getTextColor(int const)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return string();
    }

    return sr[inst].ct;
}

string TButton::getTextEffectColor(int inst)
{
    DECL_TRACER ("TButton::getTextEffectColor(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return string();
    }

    return sr[inst].ec;
}

void TButton::setTextEffectColor(const string& ec, int instance)
{
    DECL_TRACER("TButton::setTextEffectColor(const string& ec, int inst)");

    if (!setTextEffectColorOnly(ec, instance))
        return;

    if (visible)
        makeElement();
}

bool TButton::setTextEffectColorOnly(const string& ec, int instance)
{
    DECL_TRACER("TButton::setTextEffectColorOnly(const string& ec, int inst)");

    if ((size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (!TColor::isValidAMXcolor(ec))
    {
        MSG_PROTOCOL("Invalid color >" << ec << "< ignored!");
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].ec.compare(ec) == 0)
        {
            inst++;
            continue;
        }

        sr[inst].ec = ec;
        mChanged = true;
        inst++;
    }

    return true;
}

int TButton::getTextEffect(int inst)
{
    DECL_TRACER("TButton::getTextEffect(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return 0;
    }

    return sr[inst].et;
}

void TButton::setTextEffect(int et, int inst)
{
    DECL_TRACER("TButton::setTextEffect(bool et, int inst)");

    if (inst >= (int)sr.size())
    {
        MSG_ERROR("instance " << inst << " is out of bounds!");
        return;
    }

    if (inst < 0)
    {
        for (size_t i = 0; i < sr.size(); i++)
        {
            if (sr[i].et != et)
                mChanged = true;

            sr[i].et = et;
        }
    }
    else
    {
        if (sr[inst].et != et)
            mChanged = true;

        sr[inst].et = et;
    }

    makeElement();
}

string TButton::getTextEffectName(int inst)
{
    DECL_TRACER("TButton::getTextEffectName(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
        return string();

    int idx = 0;

    while (sysTefs[idx].idx)
    {
        if (sysTefs[idx].idx == sr[inst].et)
            return sysTefs[idx].name;

        idx++;
    }

    return string();
}

void TButton::setTextEffectName(const string& name, int inst)
{
    DECL_TRACER("TButton::setTextEffectName(const string& name, int inst)");

    if (inst >= (int)sr.size())
        return;

    int idx = 0;

    while (sysTefs[idx].idx)
    {
        if (strCaseCompare(sysTefs[idx].name, name) == 0)
        {
            if (inst < 0)
            {
                for (size_t i = 0; i < sr.size(); i++)
                {
                    if (sr[i].et != sysTefs[idx].idx)
                        mChanged = true;

                    sr[i].et = sysTefs[idx].idx;
                }
            }
            else
            {
                if (sr[inst].et != sysTefs[idx].idx)
                    mChanged = true;

                sr[inst].et = sysTefs[idx].idx;
            }

            makeElement();
            break;
        }

        idx++;
    }
}

string TButton::getBitmapName(int inst)
{
    DECL_TRACER("TButton::getBitmapName(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return string();
    }

    return sr[inst].bm;
}

string TButton::getFillColor(int inst)
{
    DECL_TRACER("TButton::getFillColor(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return string();
    }

    return sr[inst].cf;
}

bool TButton::setTextWordWrap(bool state, int instance)
{
    DECL_TRACER("TButton::setWorWrap(bool state, int instance)");

    if (instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    int stt = state ? 1 : 0;

    if (instance < 0)
    {
        for (size_t i = 0; i < sr.size(); i++)
        {
            if (sr[i].ww != stt)
                mChanged = true;

            sr[i].ww = stt;
        }
    }
    else
    {
        if (sr[instance].ww != stt)
            mChanged = true;

        sr[instance].ww = stt;
    }

    return makeElement(instance);
}

bool TButton::getTextWordWrap(int inst)
{
    DECL_TRACER("TButton::getTextWordWrap(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return false;
    }

    return (sr[inst].ww == 1);
}

int TButton::getFontIndex(int inst)
{
    DECL_TRACER("TButton::getFontIndex(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return 0;
    }

    return sr[inst].fi;
}

bool TButton::setFontIndex(int fi, int instance)
{
    DECL_TRACER("TButton::setFontIndex(int fi, int inst)");

    if (instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].fi != fi)
            mChanged = true;

        sr[inst].fi = fi;
        inst++;
    }

    return makeElement(inst);
}

int TButton::getIconIndex(int inst)
{
    DECL_TRACER("TButton::getIconIndex(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return 0;
    }

    return sr[inst].ii;
}

string TButton::getSound(int inst)
{
    DECL_TRACER("TButton::getSound(int inst)");

    if (inst < 0 || inst >= (int)sr.size())
    {
        MSG_ERROR("Instance " << inst << " does not exist!");
        return string();
    }

    return sr[inst].sd;
}

void TButton::setSound(const string& sound, int inst)
{
    DECL_TRACER("TButton::setSound(const string& sound, int inst)");

    if (inst >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << inst);
        return;
    }

    if (inst < 0)
    {
        for (size_t i = 0; i < sr.size(); i++)
            sr[i].sd = sound;
    }
    else
        sr[inst].sd = sound;
#if TESTMODE == 1
    __success = true;
    setScreenDone();
#endif
}

bool TButton::startAnimation(int st, int end, int time)
{
    DECL_TRACER("TButton::startAnimation(int start, int end, int time)");

    if (st > end || st < 0 || (size_t)end > sr.size() || time < 0)
    {
        MSG_ERROR("Invalid parameter: start=" << st << ", end=" << end << ", time=" << time);
        return false;
    }

    if (time <= 1)
    {
        int inst = end - 1;

        if (inst >= 0 && (size_t)inst < sr.size())
        {
            if (mActInstance != inst)
            {
                mActInstance = inst;
                mChanged = true;
                drawButton(inst);
            }
        }

        return true;
    }

    int start = std::max(1, st);

    if (mAniRunning || mThrAni.joinable())
    {
        MSG_PROTOCOL("Animation is already running!");
        return true;
    }

    int number = end - start;
    ulong stepTime = ((ulong)time * 10L) / (ulong)number;
    mAniRunTime = (ulong)time * 10L;

    try
    {
        mAniStop = false;
        mThrAni = thread([=] { runAnimationRange(start, end, stepTime); });
        mThrAni.detach();
    }
    catch (exception& e)
    {
        MSG_ERROR("Error starting the button animation thread: " << e.what());
        return false;
    }

    return true;
}

void TButton::_TimerCallback(ulong)
{
    mLastBlink.second++;
    int months[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if ((mLastBlink.year % 4) == 0)
        months[1] = 29;

    if (mLastBlink.second > 59)
    {
        mLastBlink.minute++;
        mLastBlink.second = 0;

        if (mLastBlink.minute > 59)
        {
            mLastBlink.hour++;
            mLastBlink.minute = 0;

            if (mLastBlink.hour >= 24)
            {
                mLastBlink.hour = 0;
                mLastBlink.weekday++;
                mLastBlink.day++;

                if (mLastBlink.weekday > 7)
                    mLastBlink.weekday = 0;

                if (mLastBlink.day > months[mLastBlink.month-1])
                {
                    mLastBlink.day = 1;
                    mLastBlink.month++;

                    if (mLastBlink.month > 12)
                    {
                        mLastBlink.year++;
                        mLastBlink.month = 1;
                    }
                }
            }
        }
    }

    funcTimer(mLastBlink);
}

void TButton::_imageRefresh(const string& url)
{
    DECL_TRACER("TButton::_imageRefresh(const string& url)");

    if (prg_stopped || killed || !visible)
        return;

    if (!gPrjResources)
    {
        MSG_WARNING("No resources available!");
        return;
    }

    ulong parent = mHandle & 0xffff0000;
    getDrawOrder(sr[mActInstance]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
    {
        TError::clear();
        return;
    }

    SkBitmap imgButton;

    if (!allocPixels(wt, ht, &imgButton))
        return;

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, mActInstance))
                return;
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            RESOURCE_T resource = gPrjResources->findResource(sr[mActInstance].bm);

            if (resource.protocol.empty())
            {
                MSG_ERROR("Resource " << sr[mActInstance].bm << " not found!");
                return;
            }

            THTTPClient *WEBClient = nullptr;

            try
            {
                char *content = nullptr;
                size_t length = 0, contentlen = 0;

                WEBClient = new THTTPClient;

                if (WEBClient && (content = WEBClient->tcall(&length, url, resource.user, resource.password)) == nullptr)
                {
                    if (WEBClient)
                        delete WEBClient;

                    return;
                }

                contentlen = WEBClient->getContentSize();

                if (content == nullptr)
                {
                    MSG_ERROR("Server returned no or invalid content!");
                    delete WEBClient;
                    return;
                }

                sk_sp<SkData> data = SkData::MakeWithCopy(content, contentlen);

                if (!data)
                {
                    MSG_ERROR("Could not make an image!");
                    delete WEBClient;
                    return;
                }

                SkBitmap image;

                if (!DecodeDataToBitmap(data, &image))
                {
                    MSG_ERROR("Error creating an image!");
                    delete WEBClient;
                    return;
                }

                loadImage(&imgButton, image, mActInstance);
                delete WEBClient;
            }
            catch (std::exception& e)
            {
                if (WEBClient)
                    delete WEBClient;

                MSG_ERROR(e.what());
                return;
            }
            catch(...)
            {
                if (WEBClient)
                    delete WEBClient;

                MSG_ERROR("Unexpected exception occured. [TButton::_imageRefresh()]");
                return;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, mActInstance))
                return;
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(&imgButton, mActInstance))
                return;
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, mActInstance))
                return;
        }
    }

    if (mGlobalOO >= 0 || sr[mActInstance].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();

        if (!allocPixels(w, h, &ooButton))
            return;

        SkCanvas canvas(ooButton);
        SkIRect irect = SkIRect::MakeXYWH(0, 0, w, h);
        SkRegion region;
        region.setRect(irect);
        SkScalar oo;

        if (mGlobalOO >= 0 && sr[mActInstance].oo >= 0)
        {
            oo = std::min((SkScalar)mGlobalOO, (SkScalar)sr[mActInstance].oo);
            MSG_DEBUG("Set global overal opacity to " << oo);
        }
        else if (sr[mActInstance].oo >= 0)
        {
            oo = (SkScalar)sr[mActInstance].oo;
            MSG_DEBUG("Set overal opacity to " << oo);
        }
        else
        {
            oo = (SkScalar)mGlobalOO;
            MSG_DEBUG("Set global overal opacity to " << oo);
        }

        SkScalar alpha = 1.0 / 255.0 * oo;
        MSG_DEBUG("Calculated alpha value: " << alpha);
        SkPaint paint;
        paint.setAlphaf(alpha);
        paint.setImageFilter(SkImageFilters::AlphaThreshold(region, 0.0, alpha, nullptr));
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgButton);
        canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        imgButton.erase(SK_ColorTRANSPARENT, {0, 0, w, h});
        imgButton = ooButton;
    }

    mLastImage = imgButton;
    mChanged = false;

    if (!prg_stopped && visible && _displayButton)
    {
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());

            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrc);
            paint.setFilterQuality(kHigh_SkFilterQuality);
            // Calculate new dimension
            SkImageInfo info = imgButton.info();
            int width = (int)((double)info.width() * gPageManager->getScaleFactor());
            int height = (int)((double)info.height() * gPageManager->getScaleFactor());
            // Create a canvas and draw new image
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(imgButton);
            imgButton.allocN32Pixels(width, height);
            imgButton.eraseColor(SK_ColorTRANSPARENT);
            SkCanvas can(imgButton, SkSurfaceProps());
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            rowBytes = imgButton.info().minRowBytes();
            mLastImage = imgButton;
        }
#endif
        TBitmap image((unsigned char *)imgButton.getPixels(), imgButton.info().width(), imgButton.info().height());
        _displayButton(mHandle, parent, image, rwidth, rheight, rleft, rtop, isPassThrough());
    }
}

void TButton::registerSystemButton()
{
    DECL_TRACER("TButton::registerSystemButton()");

    if (mSystemReg)
        return;

    // If this is a special system button, register it to receive the state
    if (ap == 0 && ad == SYSTEM_ITEM_CONNSTATE)     // Connection status?
    {
        MSG_TRACE("Try to register button " << na << " as connection status ...");

        if (gAmxNet)
        {
            gAmxNet->registerNetworkState(bind(&TButton::funcNetwork, this, std::placeholders::_1), mHandle);
            mSystemReg = true;
            MSG_TRACE("Button registered");
        }
        else
            MSG_WARNING("Network class not initialized!");

    }
    else if (ap == 0 && ((ad >= SYSTEM_ITEM_STANDARDTIME && ad <= SYSTEM_ITEM_TIME24) || (ad >= SYSTEM_ITEM_DATEWEEKDAY && ad <= SYSTEM_ITEM_DATEYYYYMMDD))) // time or date
    {
        MSG_TRACE("Try to register button " << na << " as time/date ...");

        if (gAmxNet)
        {
            gAmxNet->registerTimer(bind(&TButton::funcTimer, this, std::placeholders::_1), mHandle);
            mSystemReg = true;
            MSG_TRACE("Button registered");
        }
        else
            MSG_WARNING("Network class not initialized!");

        if (ad >= SYSTEM_ITEM_STANDARDTIME && ad <= SYSTEM_ITEM_TIME24 && !mTimer)
        {
            mTimer = new TTimer;
            mTimer->setInterval(std::chrono::milliseconds(1000));   // 1 second
            mTimer->registerCallback(bind(&TButton::_TimerCallback, this, std::placeholders::_1));
            mTimer->run();
        }
    }
    else if (ap == 0 && (ad == SYSTEM_ITEM_BATTERYLEVEL || ad == SYSTEM_ITEM_BATTERYCHARGING))   // Battery status
    {
        if (gPageManager)
        {
#ifdef Q_OS_ANDROID
            gPageManager->regCallbackBatteryState(bind(&TButton::funcBattery, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), mHandle);
#endif
#ifdef Q_OS_IOS
            gPageManager->regCallbackBatteryState(bind(&TButton::funcBattery, this, std::placeholders::_1, std::placeholders::_2), mHandle);
#endif
        }

        mSystemReg = true;
    }
    else if (lp == 0 && lv == SYSTEM_ITEM_CONNSTRENGTH)       // Network connection strength
    {
        if (gPageManager)
            gPageManager->regCallbackNetState(bind(&TButton::funcNetworkState, this, std::placeholders::_1), mHandle);

        mSystemReg = true;
    }
    else if (lp == 0 && lv == SYSTEM_ITEM_SYSVOLUME)        // System volume
    {
        mLastLevel = TConfig::getSystemVolume();
        mChanged = true;
        mSystemReg = true;
    }
    else if (cp == 0 && type == GENERAL && ch > 0 && isSystemCheckBox(ch))
    {
        int inst = getButtonInstance(0, ch);

        if (inst >= 0)
        {
            mActInstance = inst;
            mChanged = true;
            mSystemReg = true;
        }
    }
    else if (ap == 0 && ad > 0 && isSystemTextLine(ad))
    {
        sr[0].te = sr[1].te = fillButtonText(ad, 0);
        mChanged = true;
        mSystemReg = true;
    }
}

void TButton::addPushFunction(string& func, string& page)
{
    DECL_TRACER("TButton::addPushFunction(string& func, string& page)");

    vector<string> allFunc = { "Stan", "Prev", "Show", "Hide", "Togg", "ClearG", "ClearP", "ClearA" };
    vector<string>::iterator iter;

    for (iter = allFunc.begin(); iter != allFunc.end(); ++iter)
    {
        if (strCaseCompare(*iter, func) == 0)
        {
            bool found = false;
            vector<PUSH_FUNC_T>::iterator iterPf;

            if (pushFunc.size() > 0)
            {
                for (iterPf = pushFunc.begin(); iterPf != pushFunc.end(); ++iterPf)
                {
                    if (strCaseCompare(iterPf->pfType, func) == 0)
                    {
                        iterPf->pfName = page;
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                PUSH_FUNC_T pf;
                pf.pfType = func;
                pf.pfName = page;
                pushFunc.push_back(pf);
            }

            break;
        }
    }
}

void TButton::clearPushFunction(const string& action)
{
    DECL_TRACER("TButton::clearPushFunction(const string& action)");

    if (pushFunc.empty())
        return;

    vector<PUSH_FUNC_T>::iterator iter;

    for (iter = pushFunc.begin(); iter != pushFunc.end(); ++iter)
    {
        if (strCaseCompare(iter->pfName, action) == 0)
        {
            pushFunc.erase(iter);
            return;
        }
    }
}

void TButton::getDrawOrder(const std::string& sdo, DRAW_ORDER *order)
{
    DECL_TRACER("TButton::getDrawOrder(const std::string& sdo, DRAW_ORDER *order)");

    if (!order)
        return;

    if (sdo.empty() || sdo.length() != 10)
    {
        *order     = ORD_ELEM_FILL;
        *(order+1) = ORD_ELEM_BITMAP;
        *(order+2) = ORD_ELEM_ICON;
        *(order+3) = ORD_ELEM_TEXT;
        *(order+4) = ORD_ELEM_BORDER;
        return;
    }

    int elems = (int)(sdo.length() / 2);

    for (int i = 0; i < elems; i++)
    {
        int e = atoi(sdo.substr(i * 2, 2).c_str());

        if (e < 1 || e > 5)
        {
            MSG_ERROR("Invalid draw order \"" << sdo << "\"!");
            TError::setError();
            return;
        }

        *(order+i) = (DRAW_ORDER)e;
    }
}

bool TButton::buttonFill(SkBitmap* bm, int instance)
{
    DECL_TRACER("TButton::buttonFill(SkBitmap* bm, int instance)");

    if (!bm)
    {
        MSG_ERROR("Invalid bitmap!");
        return false;
    }

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    SkColor color = TColor::getSkiaColor(sr[instance].cf);
    MSG_DEBUG("Fill color[" << instance << "]: " << sr[instance].cf << " (#" << std::setw(8) << std::setfill('0') << std::hex << color << ")" << std::dec << std::setfill(' ') << std::setw(1));
    // We create a new bitmap and fill it with the given fill color. Then
    // we put this image over the existing image "bm". In case this method is
    // not the first in the draw order, it prevents the button from completely
    // overwrite.
    SkImageInfo info = bm->info();
    SkBitmap bitmap;

    if (!allocPixels(info.width(), info.height(), &bitmap))
    {
        MSG_ERROR("Error allocating a bitmap with size " << info.width() << " x " << info.height() << "!");
        return false;
    }

    bitmap.eraseColor(color);                       // Fill the new bitmap with the fill color
    SkCanvas ctx(*bm, SkSurfaceProps());            // Create a canvas
    SkPaint paint;                                  // The paint "device"
    paint.setBlendMode(SkBlendMode::kSrcOver);      // We're overwriting each pixel
    sk_sp<SkImage> _image = SkImage::MakeFromBitmap(bitmap);    // Technically we need an image. So we convert our new bitmap into an image.
    ctx.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);   // Now we put the new image over the existing one.
    return true;
}

bool TButton::buttonBitmap(SkBitmap* bm, int inst)
{
    DECL_TRACER("TButton::buttonBitmap(SkBitmap* bm, int instane)");

    if (prg_stopped || !bm)
        return false;

    int instance = inst;

    if (inst < 0)
        instance = 0;
    else if ((size_t)inst >= sr.size())
        instance = (int)(sr.size() - 1);

    /*
     * Here we test if we have a cameleon image. If there is a mask (sr[].mi)
     * and no frame (sr[].bs) then we have a cameleon image. A bitmap is
     * optional. If there is one it will be used to draw with the mask.
     * Otherwise the mask may be used as an overlay for a bitmap on another
     * button below the mask.
     */
    if (!sr[instance].mi.empty() && sr[instance].bs.empty())       // Chameleon image?
    {
        MSG_DEBUG("Chameleon image consisting of mask " << sr[instance].mi << " and bitmap " << (sr[instance].bm.empty() ? "NONE" : sr[instance].bm) << " ...");

        SkBitmap bmMi;
        SkBitmap bmBm;

        if (!TImgCache::getBitmap(sr[instance].mi, &bmMi, _BMTYPE_CHAMELEON, &sr[instance].mi_width, &sr[instance].mi_height))
        {
            sk_sp<SkData> data = readImage(sr[instance].mi);
            bool loaded = false;

            if (data)
            {
                DecodeDataToBitmap(data, &bmMi);

                if (!bmMi.empty())
                {
                    TImgCache::addImage(sr[instance].mi, bmMi, _BMTYPE_CHAMELEON);
                    loaded = true;
                    sr[instance].mi_width = bmMi.info().width();
                    sr[instance].mi_height = bmMi.info().height();
                }
            }

            if(!loaded)
            {
                MSG_ERROR("Missing image " << sr[instance].mi << "!");
                TError::setError();
                return false;
            }
        }

        MSG_DEBUG("Chameleon image size: " << bmMi.info().width() << " x " << bmMi.info().height());
        SkBitmap imgRed(bmMi);
        SkBitmap imgMask;
        bool haveBothImages = true;

        if (!sr[instance].bm.empty())
        {
            if (!TImgCache::getBitmap(sr[instance].bm, &bmBm, _BMTYPE_BITMAP, &sr[instance].bm_width, &sr[instance].bm_height))
            {
                sk_sp<SkData> data = readImage(sr[instance].bm);
                bool loaded = false;

                if (data)
                {
                    DecodeDataToBitmap(data, &bmBm);

                    if (!bmMi.empty())
                    {
                        TImgCache::addImage(sr[instance].bm, bmMi, _BMTYPE_BITMAP);
                        loaded = true;
                        sr[instance].bm_width = bmBm.info().width();
                        sr[instance].bm_height = bmBm.info().height();
                    }
                }

                if (!loaded)
                {
                    MSG_ERROR("Missing image " << sr[instance].bm << "!");
                    TError::setError();
                    return false;
                }
            }

            if (!bmBm.empty())
            {
                if (!imgMask.installPixels(bmBm.pixmap()))
                {
                    MSG_ERROR("Error installing pixmap " << sr[instance].bm << " for chameleon image!");

                    if (!allocPixels(imgRed.info().width(), imgRed.info().height(), &imgMask))
                        return false;

                    imgMask.eraseColor(SK_ColorTRANSPARENT);
                    haveBothImages = false;
                }
            }
            else
            {
                MSG_WARNING("No or invalid bitmap! Ignoring bitmap for cameleon image.");

                if (!allocPixels(imgRed.info().width(), imgRed.info().height(), &imgMask))
                    return false;

                imgMask.eraseColor(SK_ColorTRANSPARENT);
                haveBothImages = false;
            }
        }
        else
            haveBothImages = false;

        MSG_DEBUG("Bitmap image size: " << bmBm.info().width() << " x " << bmBm.info().height());
        MSG_DEBUG("Bitmap mask size: " << imgMask.info().width() << " x " << imgMask.info().height());
        SkBitmap img = drawImageButton(imgRed, imgMask, sr[instance].mi_width, sr[instance].mi_height, TColor::getSkiaColor(sr[instance].cf), TColor::getSkiaColor(sr[instance].cb));

        if (img.empty())
        {
            MSG_ERROR("Error creating the cameleon image \"" << sr[instance].mi << "\" / \"" << sr[instance].bm << "\"!");
            TError::setError();
            return false;
        }

        MSG_DEBUG("Have both images: " << (haveBothImages ? "YES" : "NO"));
        SkCanvas ctx(img, SkSurfaceProps());
        SkImageInfo info = img.info();
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgMask);
        ctx.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);

        POSITION_t position = calcImagePosition(sr[instance].mi_width, sr[instance].mi_height, SC_BITMAP, instance);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the position of the image for button number " << bi << ": " << na);
            TError::setError();
            return false;
        }

        SkCanvas can(*bm, SkSurfaceProps());
        paint.setBlendMode(SkBlendMode::kSrc);

        if (sr[instance].sb == 0)
        {
            if (!haveBothImages)
            {
                sk_sp<SkImage> _image = SkImage::MakeFromBitmap(img);
                can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);

                if (!sr[instance].bm.empty())
                {
                    imgMask.installPixels(bmBm.pixmap());
                    paint.setBlendMode(SkBlendMode::kSrcOver);
                    _image = SkImage::MakeFromBitmap(imgMask);
                    can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
                }
            }
            else
            {
                sk_sp<SkImage> _image = SkImage::MakeFromBitmap(img);
                can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
            }
        }
        else    // Scale to fit
        {
            if (!haveBothImages)
            {
                SkRect rect;
                rect.setXYWH(0, 0, imgRed.info().width(), imgRed.info().height());
                sk_sp<SkImage> im = SkImage::MakeFromBitmap(img);
                can.drawImageRect(im, rect, SkSamplingOptions(), &paint);

                if (!sr[instance].bm.empty())
                {
                    imgMask.installPixels(bmBm.pixmap());
                    rect.setXYWH(position.left, position.top, position.width, position.height);
                    im = SkImage::MakeFromBitmap(imgMask);
                    paint.setBlendMode(SkBlendMode::kSrcOver);
                    can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
                }
            }
            else
            {
                SkRect rect = SkRect::MakeXYWH(position.left, position.top, position.width, position.height);
                sk_sp<SkImage> im = SkImage::MakeFromBitmap(img);
                can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            }
        }
    }
    else if (!sr[instance].bm.empty())
    {
        MSG_TRACE("Drawing normal image " << sr[instance].bm << " ...");

        SkBitmap image;

        if (!TImgCache::getBitmap(sr[instance].bm, &image, _BMTYPE_BITMAP, &sr[instance].bm_width, &sr[instance].bm_height))
        {
            sk_sp<SkData> data = readImage(sr[instance].bm);
            bool loaded = false;

            if (data)
            {
                DecodeDataToBitmap(data, &image);

                if (!image.empty())
                {
                    TImgCache::addImage(sr[instance].mi, image, _BMTYPE_BITMAP);
                    loaded = true;
                    sr[instance].bm_width = image.info().width();
                    sr[instance].bm_height = image.info().height();
                }
            }

            if (!loaded)
            {
                MSG_ERROR("Missing image " << sr[instance].bm << "!");
                return true;        // We want the button even without an image
            }
        }

        if (image.empty())
        {
            MSG_ERROR("Error creating the image \"" << sr[instance].bm << "\"!");
            TError::setError();
            return false;
        }

        IMAGE_SIZE_t isize = calcImageSize(image.info().width(), image.info().height(), instance, true);
        POSITION_t position = calcImagePosition((sr[instance].sb ? isize.width : image.info().width()), (sr[instance].sb ? isize.height : image.info().height()), SC_BITMAP, instance);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the position of the image for button number " << bi);
            TError::setError();
            return false;
        }

        MSG_DEBUG("Putting bitmap on top of image ...");
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        SkCanvas can(*bm, SkSurfaceProps());

        if (sr[instance].sb == 0)   // Scale bitmap?
        {                           // No, keep size
            if ((sr[instance].jb == 0 && sr[instance].bx >= 0 && sr[instance].by >= 0) || sr[instance].jb != 0)  // Draw the full image
            {
                sk_sp<SkImage> _image = SkImage::MakeFromBitmap(image);
                can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
            }
            else    // We need only a subset of the image
            {
                MSG_DEBUG("Create a subset of an image ...");

                // Create a new Info to have the size of the subset.
                SkImageInfo info = SkImageInfo::Make(position.width, position.height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
                size_t byteSize = info.computeMinByteSize();

                if (byteSize == 0)
                {
                    MSG_ERROR("Unable to calculate size of image!");
                    TError::setError();
                    return false;
                }

                MSG_DEBUG("Rectangle of part: x: " << position.left << ", y: " << position.top << ", w: " << position.width << ", h: " << position.height);
                SkBitmap part;      // Bitmap receiving the wanted part from the whole image
                SkIRect irect = SkIRect::MakeXYWH(position.left, position.top, position.width, position.height);
                image.extractSubset(&part, irect);  // Extract the part of the image containg the pixels we want
                sk_sp<SkImage> _image = SkImage::MakeFromBitmap(part);
                can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint); // Draw the image
            }
        }
        else    // Scale to fit
        {
            SkRect rect = SkRect::MakeXYWH(position.left, position.top, isize.width, isize.height);
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(image);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
        }
    }
    else
    {
        MSG_DEBUG("No bitmap defined.");
    }

    return true;
}

bool TButton::buttonDynamic(SkBitmap* bm, int instance, bool show, bool *state)
{
    DECL_TRACER("TButton::buttonDynamic(SkBitmap* bm, int instance, bool show, bool *state)");

    if (prg_stopped)
        return false;

    if (!gPrjResources)
    {
        MSG_ERROR("Internal error: Global resource class not initialized!");
        return false;
    }

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    if (!sr[instance].dynamic)
    {
        MSG_WARNING("Button " << bi << ": \"" << na << "\" is not for remote image!");
        return false;
    }

    if (!visible)
    {
        MSG_DEBUG("Dynamic button " << handleToString(mHandle) << " is invisible. Will not draw it.");
        return true;
    }

    MSG_DEBUG("Dynamic button " << handleToString(mHandle) << " will be drawn ...");
    size_t idx = 0;

    if ((idx = gPrjResources->getResourceIndex("image")) == TPrjResources::npos)
    {
        MSG_ERROR("There exists no image resource!");
        return false;
    }

    RESOURCE_T resource = gPrjResources->findResource((int)idx, sr[instance].bm);

    if (resource.protocol.empty())
    {
        MSG_WARNING("Resource " << sr[instance].bm << " not found!");
        return true;
    }

    string path = resource.path;

    if (!resource.file.empty())
        path += "/" + resource.file;

    string url = THTTPClient::makeURLs(toLower(resource.protocol), resource.host, 0, path);

    if (url.empty())
    {
        MSG_DEBUG("No URL, no bitmap!");
        return true;    // We have no image but the button still exists
    }

    SkBitmap image;

    if (TImgCache::getBitmap(url, &image, _BMTYPE_URL))
    {
        MSG_DEBUG("Found image \"" << url << "\" in the cache. Will reuse it.");
        IMAGE_SIZE_t isize = calcImageSize(image.info().width(), image.info().height(), instance, true);
        POSITION_t position = calcImagePosition((sr[instance].sb ? isize.width : image.info().width()), (sr[instance].sb ? isize.height : image.info().height()), SC_BITMAP, instance);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the position of the image for button number " << bi);
            TError::setError();
            return false;
        }

        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        SkCanvas can(*bm, SkSurfaceProps());

        if (sr[instance].sb == 0)   // Scale bitmap?
        {                           // No, keep size
            if ((sr[instance].jb == 0 && sr[instance].bx >= 0 && sr[instance].by >= 0) || sr[instance].jb != 0)  // Draw the full image
            {
                sk_sp<SkImage> _image = SkImage::MakeFromBitmap(image);
                can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
            }
            else    // We need only a subset of the image
            {
                MSG_DEBUG("Create a subset of an image ...");

                // Create a new Info to have the size of the subset.
                SkImageInfo info = SkImageInfo::Make(position.width, position.height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
                size_t byteSize = info.computeMinByteSize();

                if (byteSize == 0)
                {
                    MSG_ERROR("Unable to calculate size of image!");
                    TError::setError();
                    return false;
                }

                MSG_DEBUG("Rectangle of part: x: " << position.left << ", y: " << position.top << ", w: " << position.width << ", h: " << position.height);
                SkBitmap part;      // Bitmap receiving the wanted part from the whole image
                SkIRect irect = SkIRect::MakeXYWH(position.left, position.top, position.width, position.height);
                image.extractSubset(&part, irect);  // Extract the part of the image containg the pixels we want
                sk_sp<SkImage> _image = SkImage::MakeFromBitmap(part);
                can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint); // Draw the image
            }
        }
        else    // Scale to fit
        {
            SkRect rect = SkRect::MakeXYWH(position.left, position.top, isize.width, isize.height);
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(image);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
        }

        return true;
    }

    try
    {
        // First me must add the credential for the image into a bitmap cache element
        BITMAP_CACHE bc;
        bc.top = tp;
        bc.left = lt;
        bc.width = wt;
        bc.height = ht;
        bc.bi = bi;
        bc.show = show;
        bc.handle = getHandle();
        bc.parent = getParent();
        bc.bitmap = *bm;
        addToBitmapCache(bc);

        if (state)
            *state = true;  // Prevent the calling method from displaying the button

        MSG_TRACE("Starting thread for loading a dynamic image ...");
        mThrRes = std::thread([=] { this->funcResource(&resource, url, bc, instance); });
        MSG_TRACE("Thread started. Detaching ...");
        mThrRes.detach();
        MSG_TRACE("Thread is running and detached.");
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting the resource thread: " << e.what());
    }

    return true;
}

/*
 * Draws the elements of a button starting at the point where the bitmap was
 * already drawed. Everything coming afterwards acording to the draw order
 * is drawed in the desired order.
 * This method is called out of a thread to draw a button with an external
 * image coming from a WEB server.
 */
bool TButton::drawAlongOrder(SkBitmap *imgButton, int instance)
{
    DECL_TRACER("TButton::drawAlongOrder(SkBitmap *imgButton, int instance)");

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    bool cont = false;

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (!cont && mDOrder[i] == ORD_ELEM_BITMAP)
        {
            cont = true;
            continue;
        }
        else if (!cont)
            continue;

        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
    }

    return true;
}

void TButton::funcResource(const RESOURCE_T* resource, const std::string& url, BITMAP_CACHE bc, int instance)
{
    DECL_TRACER("TButton::funcResource(RESOURCE_T* resource, std::string& url, SkBitmap* bm, int instance)");

    if (prg_stopped || killed || _restart_ || !resource)
        return;

    if (resource->refresh > 0 && !resource->dynamo)      // Periodically refreshing image?
    {
        MSG_DEBUG("Retrieving periodicaly refreshed image");

        if (!bc.handle || !bc.parent || bc.bi <= 1)
        {
            MSG_ERROR("Invalid button. Can't make a dynamo image!");
            return;
        }

        THR_REFRESH_t *thref = _findResource(bc.handle, bc.parent, bc.bi);
        TImageRefresh *mImageRefresh = nullptr;

        if (!thref)
        {
            MSG_DEBUG("Creating a new refresh thread");
            mImageRefresh = new TImageRefresh();
            mImageRefresh->registerCallback(bind(&TButton::_imageRefresh, this, std::placeholders::_1));
            mImageRefresh->setInterval(std::chrono::seconds(resource->refresh));
            mImageRefresh->setUsername(resource->user);
            mImageRefresh->setPassword(resource->password);

            if (resource->preserve)
                mImageRefresh->setRunOnce();

            _addResource(mImageRefresh, bc.handle, bc.parent, bc.bi);
        }
        else
        {
            mImageRefresh = thref->mImageRefresh;

            if (!mImageRefresh)
            {
                MSG_ERROR("Error creating a new refresh class!");
                return;
            }

            mImageRefresh->setInterval(std::chrono::seconds(resource->refresh));
            mImageRefresh->setUsername(resource->user);
            mImageRefresh->setPassword(resource->password);

            if (resource->preserve)
                mImageRefresh->setRunOnce();
        }

        if (mImageRefresh->isRunning())
            mImageRefresh->stopWait();

        if (!mImageRefresh->isRunning() && !_restart_)
        {
            MSG_DEBUG("Starting a refresh thread.");
            mImageRefresh->run(url);
        }
    }
    else if (resource->refresh == 0 && !resource->dynamo)
    {
        MSG_DEBUG("Retrieving single image");

        if (bc.handle == 0)
        {
            MSG_ERROR("Invalid bitmap cache!");
            return;
        }

        if (instance < 0 || (size_t)instance >= sr.size())
        {
            MSG_ERROR("Invalid instance " << instance);
            return;
        }

        // Check whether we have this image already
        SkBitmap bitm;
        bool cached = false;

        cached = TImgCache::getBitmap(url, &bitm, _BMTYPE_URL);
        BITMAP_CACHE bmCache = getBCentryByHandle(bc.handle, bc.parent);

        if (!cached)    // If the bitmap was not in cache we must load it
        {
            MSG_DEBUG("Image not in cache. Downloading it ...");
            THTTPClient *WEBClient = nullptr;

            if (bmCache.handle == 0)
            {
                MSG_ERROR("Couldn't find the handle " << handleToString(bc.handle) << " in bitmap cache!");
                return;
            }

            char *content = nullptr;
            size_t length = 0, contentlen = 0;
            WEBClient = new THTTPClient;

            if (!WEBClient || (content = WEBClient->tcall(&length, url, resource->user, resource->password)) == nullptr)
            {
                if (WEBClient)
                    delete WEBClient;

                if (bc.show)
                {
                    setReady(bmCache.handle);
                    showBitmapCache();
                }
                else
                    setInvalid(bc.handle);

                return;
            }

            contentlen = WEBClient->getContentSize();
            MSG_DEBUG("Loaded " << contentlen << " bytes:");
            sk_sp<SkData> data = SkData::MakeWithCopy(content, contentlen);

            if (!data || _restart_)
            {
                delete WEBClient;
                MSG_ERROR("Error making image data!");

                if (bc.show)
                {
                    setReady(bmCache.handle);
                    showBitmapCache();
                }
                else
                    setInvalid(bc.handle);

                return;
            }

            SkBitmap image;

            if (!DecodeDataToBitmap(data, &image))
            {
                delete WEBClient;
                MSG_ERROR("Error creating an image!");

                if (bc.show)
                {
                    setReady(bmCache.handle);
                    showBitmapCache();
                }
                else
                    setInvalid(bc.handle);

                return;
            }

            // Put this image into the static image cache
            TImgCache::addImage(url, image, _BMTYPE_URL);
            // Make the button complete
            loadImage(&bmCache.bitmap, image, instance);
            drawAlongOrder(&bmCache.bitmap, instance);
            setBCBitmap(bmCache.handle, bmCache.bitmap);
            setReady(bmCache.handle);
            delete WEBClient;
            // Display the image
            showBitmapCache();
            return;
        }
        else
        {
            MSG_DEBUG("Found image in cache. Using it ...");

            if (instance < 0 || (size_t)instance >= sr.size())
            {
                MSG_ERROR("Invalid instance " << instance);
                return;
            }

            loadImage(&bmCache.bitmap, bitm, instance);
            setInvalid(bc.handle);

            if (bc.show && _displayButton)
            {
                TBitmap image((unsigned char *)bmCache.bitmap.getPixels(), bmCache.bitmap.info().width(), bmCache.bitmap.info().height());
                _displayButton(bc.handle, bc.parent, image, bc.width, bc.height, bc.left, bc.top, isPassThrough());
                mChanged = false;
            }
        }
    }
    else if (!_restart_)
    {
        MSG_DEBUG("Retrieving a video");

        if (_playVideo && !prg_stopped)
        {
            ulong parent = (mHandle >> 16) & 0x0000ffff;
            _playVideo(mHandle, parent, lt, tp, wt, ht, url, resource->user, resource->password);
        }
    }
}
#ifdef Q_OS_ANDROID
void TButton::funcBattery(int level, bool charging, int /* chargeType */)
{
    DECL_TRACER("TButton::funcBattery(int level, bool charging, int chargeType)");

    // Battery level is always a bargraph
    if (ap == 0 && ad == SYSTEM_ITEM_BATTERYLEVEL)       // Not charging
    {
        mEnabled = !charging;
        mChanged = true;

        if (!mEnabled && visible)
            hide(true);
        else if (mEnabled)
        {
            visible = true;
            drawBargraph(mActInstance, level, visible);
        }
    }
    else if (ap == 0 && ad == SYSTEM_ITEM_BATTERYCHARGING)  // Charging
    {
        mEnabled = charging;
        mChanged = true;

        if (!mEnabled && visible)
            hide(true);
        else if (mEnabled)
        {
            visible = true;
            drawBargraph(mActInstance, level, visible);
        }
    }
}
#endif
#ifdef Q_OS_IOS
void TButton::funcBattery(int level, int state)
{
    DECL_TRACER("TButton::funcBattery(int level, bool charging, int chargeType)");

    // Battery level is always a bargraph
    if (ap == 0 && ad == SYSTEM_ITEM_BATTERYLEVEL)       // Not charging
    {
        mEnabled = (state == 1 || state == 3);
        mChanged = true;

        if (!mEnabled && visible)
            hide(true);
        else if (mEnabled)
        {
            visible = true;
            drawBargraph(mActInstance, level, visible);
        }
    }
    else if (ap == 0 && ad == SYSTEM_ITEM_BATTERYCHARGING)  // Charging
    {
        mEnabled = (state == 2);
        mChanged = true;

        if (!mEnabled && visible)
            hide(true);
        else if (mEnabled)
        {
            visible = true;
            drawBargraph(mActInstance, level, visible);
        }
    }
}
#endif
void TButton::funcNetworkState(int level)
{
    DECL_TRACER("TButton::funcNetworkState(int level)");

    if (level >= rl && level <= rh)
    {
        mLastLevel = level;
        mChanged = true;
        drawMultistateBargraph(mLastLevel);
    }
}

bool TButton::loadImage(SkBitmap* bm, SkBitmap& image, int instance)
{
    DECL_TRACER("TButton::loadImage(SkBitmap* bm, SkBitmap& image, int instance)");

    if (!bm)
    {
        MSG_WARNING("Got no image to load!");
        return false;
    }

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    SkImageInfo info = image.info();
    IMAGE_SIZE_t isize = calcImageSize(image.info().width(), image.info().height(), instance, true);
    POSITION_t position = calcImagePosition((sr[instance].sb ? isize.width : info.width()), (sr[instance].sb ? isize.height : info.height()), SC_BITMAP, instance);
//    POSITION_t position = calcImagePosition(info.width(), info.height(), SC_BITMAP, instance);

    if (!position.valid)
    {
        MSG_ERROR("Error calculating the position of the image for button number " << bi);
        return false;
    }

    MSG_DEBUG("New image position: left=" << position.left << ", top=" << position.top << ", width=" << position.width << ", height=" << position.height);
    MSG_DEBUG("Image size : width=" << info.width() << ", height=" << info.height());
    MSG_DEBUG("Bitmap size: width=" << bm->info().width() << ", height=" << bm->info().height());
    MSG_DEBUG("Putting bitmap on top of image ...");
    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);

    SkCanvas can(*bm, SkSurfaceProps());

    if (sr[instance].sb == 0)
    {
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(image);
        can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
    }
    else    // Scale to fit
    {
//        paint.setFilterQuality(kHigh_SkFilterQuality);
        SkRect rect = SkRect::MakeXYWH(position.left, position.top, isize.width, isize.height);
        sk_sp<SkImage> im = SkImage::MakeFromBitmap(image);
        can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
    }

    return true;
}

bool TButton::barLevel(SkBitmap* bm, int, int level)
{
    DECL_TRACER("TButton::barLevel(SkBitmap* bm, int inst, int level)");

    if (!sr[0].mi.empty()  && sr[0].bs.empty() && !sr[1].bm.empty())       // Chameleon image?
    {
        MSG_TRACE("Chameleon image ...");
        SkBitmap bmMi, bmBm;

        TImgCache::getBitmap(sr[0].mi, &bmMi, _BMTYPE_CHAMELEON, &sr[0].mi_width, &sr[0].mi_height);
        TImgCache::getBitmap(sr[1].bm, &bmBm, _BMTYPE_BITMAP, &sr[1].bm_width, &sr[1].bm_height);
        SkBitmap imgRed(bmMi);
        SkBitmap imgMask(bmBm);

        SkBitmap img;
        SkPixmap pixmapRed = imgRed.pixmap();
        SkPixmap pixmapMask;

        if (!imgMask.empty())
            pixmapMask = imgMask.pixmap();

        int width = sr[0].mi_width;
        int height = sr[0].mi_height;
        int startX = 0;
        int startY = 0;
        // Calculation: width / <effective pixels> * level
        // Calculation: height / <effective pixels> * level
        if (dr.compare("horizontal") == 0)
            width = (int)((double)width / ((double)rh - (double)rl) * (double)level);
        else
            height = (int)((double)height / ((double)rh - (double)rl) * (double)level);

        if (ri && dr.compare("horizontal") == 0)
        {
            startX = sr[0].mi_width - width;
            width = sr[0].mi_width;
        }
        else if (dr.compare("horizontal") != 0)
        {
            startY = sr[0].mi_height - height;
            height = sr[0].mi_height;
        }

        if (!allocPixels(sr[0].mi_width, sr[0].mi_height, &img))
            return false;

        SkCanvas canvas(img);
        SkColor col1 = TColor::getSkiaColor(sr[1].cf);
        SkColor col2 = TColor::getSkiaColor(sr[1].cb);

        for (int ix = 0; ix < sr[0].mi_width; ix++)
        {
            for (int iy = 0; iy < sr[0].mi_height; iy++)
            {
                SkPaint paint;
                SkColor pixel;

                if (ix >= startX && ix < width && iy >= startY && iy < height)
                {
                    SkColor pixelRed = pixmapRed.getColor(ix, iy);
                    SkColor pixelMask;

                    if (!imgMask.empty())
                        pixelMask = pixmapMask.getColor(ix, iy);
                    else
                        pixelMask = SK_ColorWHITE;

                    pixel = baseColor(pixelRed, pixelMask, col1, col2);
                }
                else
                    pixel = SK_ColorTRANSPARENT;

                paint.setColor(pixel);
                canvas.drawPoint(ix, iy, paint);
            }
        }

        if (img.empty())
        {
            MSG_ERROR("Error creating the cameleon image \"" << sr[0].mi << "\" / \"" << sr[0].bm << "\"!");
            TError::setError();
            return false;
        }

        SkCanvas ctx(img, SkSurfaceProps());
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcATop);
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgMask);
        ctx.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);

        POSITION_t position = calcImagePosition(sr[0].mi_width, sr[0].mi_height, SC_BITMAP, 0);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the position of the image for button number " << bi << ": " << na);
            TError::setError();
            return false;
        }

        SkCanvas can(*bm, SkSurfaceProps());
        paint.setBlendMode(SkBlendMode::kSrc);
        _image = SkImage::MakeFromBitmap(img);
        can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
    }
    else if (!sr[0].bm.empty() && !sr[1].bm.empty())
    {
        MSG_TRACE("Drawing normal image ...");
        SkBitmap image1, image2;

        TImgCache::getBitmap(sr[0].bm, &image1, _BMTYPE_BITMAP, &sr[0].bm_width, &sr[0].bm_height);   // State when level = 0%
        TImgCache::getBitmap(sr[1].bm, &image2, _BMTYPE_BITMAP, &sr[1].bm_width, &sr[1].bm_height);   // State when level = 100%
        SkCanvas can_bm(*bm, SkSurfaceProps());


        if (image1.empty())
        {
            MSG_ERROR("Error creating the image \"" << sr[0].bm << "\"!");
            TError::setError();
            return false;
        }

        if (image2.empty())
        {
            MSG_ERROR("Error creating the image \"" << sr[1].bm << "\"!");
            TError::setError();
            return false;
        }

        int width = sr[1].bm_width;
        int height = sr[1].bm_height;
        int startX = 0;
        int startY = 0;

        // Calculation: width / <effective pixels> * level
        // Calculation: height / <effective pixels> * level
        if (dr.compare("horizontal") == 0)
            width = (int)((double)width / ((double)rh - (double)rl) * (double)level);
        else
            height = (int)((double)height / ((double)rh - (double)rl) * (double)level);

        if (ri && dr.compare("horizontal") == 0)     // range inverted?
        {
            startX = sr[0].mi_width - width;
            width = sr[0].mi_width;
        }
        else if (dr.compare("horizontal") != 0)
        {
            startY = sr[0].mi_height - height;
            height = sr[0].mi_height;
        }

        MSG_DEBUG("dr=" << dr << ", startX=" << startX << ", startY=" << startY << ", width=" << width << ", height=" << height << ", level=" << level);
        MSG_TRACE("Creating bargraph ...");
        SkBitmap img_bar;

        if (!allocPixels(sr[1].bm_width, sr[1].bm_height, &img_bar))
            return false;

        img_bar.eraseColor(SK_ColorTRANSPARENT);
        SkCanvas bar(img_bar, SkSurfaceProps());

        for (int ix = 0; ix < sr[1].bm_width; ix++)
        {
            for (int iy = 0; iy < sr[1].bm_height; iy++)
            {
                SkPaint paint;
                SkColor pixel;

                if (ix >= startX && ix < width && iy >= startY && iy < height)
                    pixel = image2.getColor(ix, iy);
                else
                    pixel = SK_ColorTRANSPARENT;

                paint.setColor(pixel);
                bar.drawPoint(ix, iy, paint);
            }
        }

        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrc);
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(image1);
        can_bm.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        paint.setBlendMode(SkBlendMode::kSrcATop);
        _image = SkImage::MakeFromBitmap(img_bar);
        can_bm.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);       // Draw the above created image over the 0% image
    }
    else if (sr[0].bm.empty() && !sr[1].bm.empty())     // Only one bitmap in the second instance
    {
        MSG_TRACE("Drawing second image " << sr[1].bm << " ...");
        SkBitmap image;
        TImgCache::getBitmap(sr[1].bm, &image, _BMTYPE_BITMAP, &sr[1].bm_width, &sr[1].bm_height);   // State when level = 100%
        SkCanvas can_bm(*bm, SkSurfaceProps());

        if (image.empty())
        {
            MSG_ERROR("Error creating the image \"" << sr[1].bm << "\"!");
            TError::setError();
            return false;
        }

        int width = sr[1].bm_width;
        int height = sr[1].bm_height;
        int startX = 0;
        int startY = 0;

        // Calculation: width / <effective pixels> * level
        // Calculation: height / <effective pixels> * level
        if (dr.compare("horizontal") == 0)
            width = (int)((double)width / ((double)rh - (double)rl) * (double)level);
        else
            height = (int)((double)height / ((double)rh - (double)rl) * (double)level);

        if (ri && dr.compare("horizontal") == 0)     // range inverted?
        {
            startX = sr[1].mi_width - width;
            width = sr[1].mi_width;
        }
        else if (dr.compare("horizontal") != 0)
        {
            startY = sr[1].mi_height - height;
            height = sr[1].mi_height;
        }

        MSG_DEBUG("dr=" << dr << ", startX=" << startX << ", startY=" << startY << ", width=" << width << ", height=" << height << ", level=" << level);
        MSG_TRACE("Creating bargraph ...");
        SkBitmap img_bar;

        if (!allocPixels(sr[1].bm_width, sr[1].bm_height, &img_bar))
            return false;

        img_bar.eraseColor(SK_ColorTRANSPARENT);
        SkCanvas bar(img_bar, SkSurfaceProps());
        SkPaint pt;

        for (int ix = 0; ix < sr[1].bm_width; ix++)
        {
            for (int iy = 0; iy < sr[1].bm_height; iy++)
            {
                SkColor pixel;

                if (ix >= startX && ix < width && iy >= startY && iy < height)
                    pixel = image.getColor(ix, iy);
                else
                    pixel = SK_ColorTRANSPARENT;

                pt.setColor(pixel);
                bar.drawPoint(ix, iy, pt);
            }
        }

        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(img_bar);
        can_bm.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);      // Draw the above created image over the 0% image
    }
    else
    {
        MSG_TRACE("No bitmap defined.");
        int width = wt;
        int height = ht;
        int startX = 0;
        int startY = 0;

        // Calculation: width / <effective pixels> * level = <level position>
        // Calculation: height / <effective pixels> * level = <level position>
        if (dr.compare("horizontal") == 0)
            width = (int)((double)width / ((double)rh - (double)rl) * (double)level);
        else
            height = (int)((double)height / ((double)rh - (double)rl) * (double)level);

        if (ri && dr.compare("horizontal") == 0)
        {
            startX = wt - width;
            width = wt;
        }
        else if (dr.compare("horizontal") != 0)
        {
            startY = ht - height;
            height = ht;
        }

        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrc);
        SkCanvas can(*bm, SkSurfaceProps());
        paint.setStyle(SkPaint::kFill_Style);
        paint.setAntiAlias(true);
        paint.setStrokeWidth(4);
        paint.setColor(TColor::getSkiaColor(sr[1].cf));
        MSG_DEBUG("Drawing rectangle: X=" << startX << ", Y=" << startY << ", W=" << width << ", H=" << height << ", level=" << level);
        SkRect dst;
        dst.setXYWH(startX, startY, width, height);
        can.drawRect(dst, paint);
        // If we have a slider button defined, we must draw it. To do it, we
        // must look into the system resources to find the credentials to draw
        // the button.
        if (!sd.empty())
        {
            MSG_DEBUG("Attempt to draw the slider button \"" << sd << "\".");
            int innerW = 0;
            int innerH = 0;

            SkBitmap slButton = drawSliderButton(sd, TColor::getSkiaColor(sc));

            if (slButton.empty())
            {
                MSG_ERROR("Error drawing the slicer button " << sd);
                return true;
            }

            double scaleW, scaleH;
            int border_size = getBorderSize(sr[0].bs);

            if (dr.compare("horizontal") != 0)
            {
                double scale;
                innerH = (int)((double)(height - border_size * 2 - slButton.info().height() / 2) / ((double)rh - (double)rl) * (double)level) + border_size + slButton.info().height() / 2;
                innerW = width;
                scale = (double)(wt - border_size * 2) / (double)slButton.info().width();
                scaleW = scale;
                scaleH = 1.0;

                if (!ri)
                    innerH = height - innerH;
            }
            else
            {
                double scale;
                scale = (double)(ht - border_size * 2) / (double)slButton.info().height();
                scaleW = 1.0;
                scaleH = scale;
                innerW = (int)((double)(width - border_size * 2 - slButton.info().width() / 2) / ((double)rh - (double)rl) * (double)level) + border_size + slButton.info().width() / 2;
                innerH = height;

                if (!ri)
                    innerW = width - innerW;
            }

            if (scaleImage(&slButton, scaleW, scaleH))
            {
                int w = slButton.info().width();
                int h = slButton.info().height();

                if (dr.compare("horizontal") == 0)
                {
                    int pos = innerW;
                    dst.setXYWH(pos - w / 2, border_size, w, h);
                }
                else
                {
                    int pos = innerH;
                    dst.setXYWH(border_size, pos - h / 2, w, h);
                }

                SkPaint pnt;
                pnt.setBlendMode(SkBlendMode::kSrcOver);
                sk_sp<SkImage> _image = SkImage::MakeFromBitmap(slButton);
                can.drawImageRect(_image, dst, SkSamplingOptions(), &pnt);
            }
        }
    }

    return true;
}

SkBitmap TButton::drawSliderButton(const string& slider, SkColor col)
{
    DECL_TRACER("TButton::drawSliderButton(const string& slider)");

    SkBitmap slButton;
    // First we look for the slider button.
    if (!gPageManager || !gPageManager->getSystemDraw()->existSlider(slider))
        return slButton;

    // There exists one with the wanted name. We grab it and create
    // the images from the files.
    SLIDER_STYLE_t sst;

    if (!gPageManager->getSystemDraw()->getSlider(slider, &sst))    // should never be true!
    {
        MSG_ERROR("No slider entry found!");
        return slButton;
    }

    int width, height;

    if (dr.compare("horizontal") != 0)
    {
        width = (sst.fixedSize / 2) * 2 + sst.fixedSize;
        height = sst.fixedSize;
    }
    else
    {
        width = sst.fixedSize;
        height = (sst.fixedSize / 2) * 2 + sst.fixedSize;
    }

    // Retrieve all available slider graphics files from the system
    vector<SLIDER_t> sltList = gPageManager->getSystemDraw()->getSliderFiles(slider);

    if (sltList.empty())
    {
        MSG_ERROR("No system slider graphics found!");
        return SkBitmap();
    }

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);

    if (!allocPixels(width, height, &slButton))
        return slButton;

    slButton.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas slCan(slButton, SkSurfaceProps());
    vector<SLIDER_t>::iterator sltIter;
    // Loop through list of slider graphic files
    for (sltIter = sltList.begin(); sltIter != sltList.end(); ++sltIter)
    {
        SkBitmap slPart;
        SkBitmap slPartAlpha;
        SkRect dst;

        if (dr.compare("horizontal") != 0 && (sltIter->type == SGR_LEFT || sltIter->type == SGR_RIGHT || sltIter->type == SGR_VERTICAL))    // vertical slider
        {
            if (!retrieveImage(sltIter->path, &slPart))     // Get the mask
            {
                MSG_ERROR("Missing slider button mask image " << sltIter->path);
                return SkBitmap();
            }

            if (!retrieveImage(sltIter->pathAlpha, &slPartAlpha))   // Get the alpha mask
            {
                MSG_ERROR("Missing slider button alpha image " << sltIter->pathAlpha);
                return SkBitmap();
            }

            SkBitmap sl = combineImages(slPart, slPartAlpha, col);

            if (sl.empty())
                return sl;

            switch (sltIter->type)
            {
                case SGR_LEFT:      dst.setXYWH(0, 0, sl.info().width(), sl.info().height()); break;

                case SGR_VERTICAL:
                    stretchImageWidth(&sl, sst.fixedSize);
                    dst.setXYWH(sst.fixedSize / 2, 0, sl.info().width(), sl.info().height());
                break;

                case SGR_RIGHT:     dst.setXYWH((sst.fixedSize / 2) + sst.fixedSize, 0, sl.info().width(), sl.info().height()); break;

                default:
                    MSG_WARNING("Invalid type " << sltIter->type << " found!");
            }

            sk_sp<SkImage> _image = SkImage::MakeFromBitmap(sl);
            slCan.drawImageRect(_image, dst, SkSamplingOptions(), &paint);
        }
        else if (dr.compare("horizontal") == 0 && (sltIter->type == SGR_TOP || sltIter->type == SGR_BOTTOM || sltIter->type == SGR_HORIZONTAL)) // horizontal slider
        {
            if (!retrieveImage(sltIter->path, &slPart))
            {
                MSG_ERROR("Missing slider button image " << sltIter->path);
                return SkBitmap();
            }

            if (!retrieveImage(sltIter->pathAlpha, &slPartAlpha))
            {
                MSG_ERROR("Missing slider button image " << sltIter->pathAlpha);
                return SkBitmap();
            }

            SkBitmap sl = combineImages(slPart, slPartAlpha, col);

            if (sl.empty())
                return sl;

            switch (sltIter->type)
            {
                case SGR_TOP:       dst.setXYWH(0, 0, sl.info().width(), sl.info().height()); break;

                case SGR_HORIZONTAL:
                    stretchImageHeight(&sl, sst.fixedSize);
                    dst.setXYWH(0, sst.fixedSize / 2, sl.info().width(), sl.info().height());
                break;

                case SGR_BOTTOM:    dst.setXYWH(0, (sst.fixedSize / 2) + sst.fixedSize, sl.info().width(), sl.info().height()); break;

                default:
                    MSG_WARNING("Invalid type " << sltIter->type << " found!");
            }

            sk_sp<SkImage> _image = SkImage::MakeFromBitmap(sl);
            slCan.drawImageRect(_image, dst, SkSamplingOptions(), &paint);
        }
    }

    return slButton;
}

bool TButton::buttonIcon(SkBitmap* bm, int instance)
{
    DECL_TRACER("TButton::buttonIcon(SkBitmap* bm, int instance)");

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    if (sr[instance].ii <= 0)
    {
        MSG_TRACE("No icon defined!");
        return true;
    }

    MSG_DEBUG("Drawing an icon ...");

    if (!gIcons)
    {
        MSG_WARNING("No icons were defined!");
        return true;
    }

    string file = gIcons->getFile(sr[instance].ii);

    if (file.empty())
    {
        MSG_WARNING("The icon " << sr[instance].ii << " was not found in table!");
        return true;
    }

    MSG_DEBUG("Loading icon file " << file);
    sk_sp<SkData> image;
    SkBitmap icon;

    if (!(image = readImage(file)))
        return true;

    DecodeDataToBitmap(image, &icon);

    if (icon.empty())
    {
        MSG_WARNING("Could not create an icon for element " << sr[instance].ii << " on button " << bi << " (" << na << ")");
        return true;
    }

    SkImageInfo info = icon.info();
    POSITION_t position = calcImagePosition(icon.width(), icon.height(), SC_ICON, instance);

    if (!position.valid)
    {
        MSG_ERROR("Error calculating the position of the image for button number " << bi);
        TError::setError();
        return false;
    }

    MSG_DEBUG("Putting Icon on top of bitmap ...");
    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);
    SkCanvas can(*bm, SkSurfaceProps());

    if (position.overflow)
    {
        SkRect irect;
        SkRect bdst;
        SkBitmap dst;
        int left = (position.left >= 0) ? 0 : position.left * -1;
        int top = (position.top >= 0) ? 0 : position.top * -1;
        int width = std::min(wt, info.width());
        int height = std::min(ht, info.height());
        irect.setXYWH(left, top, width, height);
        bm->getBounds(&bdst);
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(icon);
        can.drawImageRect(_image, irect, bdst, SkSamplingOptions(), &paint, SkCanvas::kStrict_SrcRectConstraint);
    }
    else
    {
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(icon);
        can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
    }

    return true;
}

bool TButton::buttonText(SkBitmap* bm, int inst)
{
    DECL_TRACER("TButton::buttonText(SkBitmap* bm, int inst)");

    int instance = inst;

    if ((size_t)instance >= sr.size())
        instance = (int)(sr.size() - 1);
    else if (instance < 0)
        instance = 0;

    if (sr[instance].te.empty())            // Is there a text?
    {                                       // No, then return
        MSG_DEBUG("Empty text string.");
        return true;
    }

    if (!mFonts)                            // Do we have any fonts?
    {                                       // No, warn and return
        MSG_WARNING("No fonts available to write a text!");
        return true;
    }

    MSG_DEBUG("Searching for font number " << sr[instance].fi << " with text " << sr[instance].te);
    FONT_T font = mFonts->getFont(sr[instance].fi);

    if (font.file.empty())
    {
        MSG_WARNING("No font file name found for font " << sr[instance].fi);
        return true;
    }

    sk_sp<SkTypeface> typeFace = mFonts->getTypeFace(sr[instance].fi);
    SkCanvas canvas(*bm);

    if (!typeFace)
    {
        MSG_WARNING("Error creating type face " << font.fullName);
    }

    SkScalar fontSizePt = ((SkScalar)font.size * 1.322);
    SkFont skFont;

    if (typeFace && typeFace->countTables() > 0)
        skFont.setTypeface(typeFace);

    skFont.setSize(fontSizePt);
    skFont.setEdging(SkFont::Edging::kAntiAlias);
    MSG_DEBUG("Wanted font size: " << font.size << ", this is " << fontSizePt << " pt");

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(TColor::getSkiaColor(sr[instance].ct));
    paint.setStyle(SkPaint::kFill_Style);

    SkFontMetrics metrics;
    skFont.getMetrics(&metrics);
    int lines = numberLines(sr[instance].te);
//    MSG_DEBUG("fAvgCharWidth: " << metrics.fAvgCharWidth);
//    MSG_DEBUG("fCapHeight:    " << metrics.fCapHeight);
//    MSG_DEBUG("fAscent:       " << metrics.fAscent);
//    MSG_DEBUG("fDescent:      " << metrics.fDescent);
//    MSG_DEBUG("fLeading:      " << metrics.fLeading);
//    MSG_DEBUG("fXHeight:      " << metrics.fXHeight);

    MSG_DEBUG("Found " << lines << " lines.");

    if (lines > 1 || sr[instance].ww)
    {
        vector<string> textLines;

        if (!sr[instance].ww)
            textLines = splitLine(sr[instance].te);
        else
        {
            textLines = splitLine(sr[instance].te, wt, ht, skFont, paint);
            lines = (int)textLines.size();
        }

        MSG_DEBUG("Calculated number of lines: " << lines);
        int lineHeight = (metrics.fAscent * -1) + metrics.fDescent;
        int totalHeight = lineHeight * lines;

        if (totalHeight > ht)
        {
            lines = ht / lineHeight;
            totalHeight = lineHeight * lines;
        }

        MSG_DEBUG("Line height: " << lineHeight << ", total height: " << totalHeight);
        vector<string>::iterator iter;
        int line = 0;
        int maxWidth = 0;

        if (textLines.size() > 0)
        {
            // Calculate the maximum width
            for (iter = textLines.begin(); iter != textLines.end(); ++iter)
            {
                SkRect rect;
                skFont.measureText(iter->c_str(), iter->length(), SkTextEncoding::kUTF8, &rect, &paint);

                if (rect.width() > maxWidth)
                    maxWidth = rect.width();
            }

            POSITION_t pos = calcImagePosition(maxWidth, totalHeight, SC_TEXT, instance);

            if (!pos.valid)
            {
                MSG_ERROR("Error calculating the text position!");
                TError::setError();
                return false;
            }

            SkScalar lnHt = metrics.fAscent * -1;

            for (iter = textLines.begin(); iter != textLines.end(); ++iter)
            {
                sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(iter->c_str(), skFont);
                MSG_DEBUG("Trying to print line: " << *iter);
                // We want to take care about the horizontal position.
                SkRect rect;
                skFont.measureText(iter->c_str(), iter->length(), SkTextEncoding::kUTF8, &rect, &paint);
                SkScalar horizontal = 0.0;

                switch(sr[instance].jt)
                {
                    case ORI_BOTTOM_MIDDLE:
                    case ORI_CENTER_MIDDLE:
                    case ORI_TOP_MIDDLE:
                        horizontal = (wt - rect.width()) / 2.0f;
                    break;

                    case ORI_BOTTOM_RIGHT:
                    case ORI_CENTER_RIGHT:
                    case ORI_TOP_RIGHT:
                        horizontal = wt - rect.width();
                    break;

                    default:
                        horizontal = pos.left;
                }

                SkScalar startX = horizontal;
                SkScalar startY = (SkScalar)pos.top + (SkScalar)lineHeight * (SkScalar)line;
                MSG_DEBUG("x=" << startX << ", y=" << startY);
                bool tEffect = false;
                // Text effects
                if (sr[instance].et > 0)
                    tEffect = textEffect(&canvas, blob, startX, startY + lnHt, instance);

                if (!tEffect)
                    canvas.drawTextBlob(blob.get(), startX, startY + lnHt, paint);

                line++;

                if (line > lines)
                    break;
            }
        }
    }
    else    // single line
    {
        string text = sr[instance].te;
        sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(text.data(), skFont);
        SkRect rect;
        skFont.measureText(text.data(), text.size(), SkTextEncoding::kUTF8, &rect, &paint);
        MSG_DEBUG("Calculated Skia rectangle of font: width=" << rect.width() << ", height=" << rect.height());
        POSITION_t position;

        if (metrics.fCapHeight >= 1.0)
            position = calcImagePosition(rect.width(), metrics.fCapHeight, SC_TEXT, instance);
        else
            position = calcImagePosition(rect.width(), rect.height(), SC_TEXT, instance);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the text position!");
            TError::setError();
            return false;
        }

        MSG_DEBUG("Printing line " << text);
        SkScalar startX = (SkScalar)position.left;
        SkScalar startY = (SkScalar)position.top;

        if (metrics.fCapHeight >= 1.0)
            startY += metrics.fCapHeight;   // This is the offset of the line
        else
            startY += rect.height();        // This is the offset of the line

        FONT_TYPE sym = TFont::isSymbol(typeFace);
        bool tEffect = false;
        // Text effects
        if (sr[instance].et > 0)
            tEffect = textEffect(&canvas, blob, startX, startY, instance);

        if (!tEffect && utf8Strlen(text) > 1)
            canvas.drawTextBlob(blob.get(), startX, startY, paint);
        else
        {
            int count = 0;
            uint16_t *glyphs = nullptr;

            if (sym == FT_SYM_MS)
            {
                MSG_DEBUG("Microsoft proprietary symbol font detected.");
                uint16_t *uni;
                size_t num = TFont::utf8ToUtf16(text, &uni, true);
                MSG_DEBUG("Got " << num << " unichars, first unichar: " << std::hex << std::setw(4) << std::setfill('0') << *uni << std::dec);

                if (num > 0)
                {
                    glyphs = new uint16_t[num];
                    size_t glyphSize = sizeof(uint16_t) * num;
                    count = skFont.textToGlyphs(uni, num, SkTextEncoding::kUTF16, glyphs, (int)glyphSize);

                    if (count <= 0)
                    {
                        delete[] glyphs;
                        glyphs = TFont::textToGlyphs(text, typeFace, &num);
                        count = (int)num;
                    }
                }
                else
                {
                    canvas.drawTextBlob(blob.get(), startX, startY, paint);
                    return true;
                }

                if (uni)
                    delete[] uni;
            }
            else if (tEffect)
                return true;
            else
            {
                glyphs = new uint16_t[text.size()];
                size_t glyphSize = sizeof(uint16_t) * text.size();
                count = skFont.textToGlyphs(text.data(), text.size(), SkTextEncoding::kUTF8, glyphs, (int)glyphSize);
            }

            if (glyphs && count > 0)
            {
                MSG_DEBUG("1st glyph: 0x" << std::hex << std::setw(8) << std::setfill('0') << *glyphs << ", # glyphs: " << std::dec << count);
                canvas.drawSimpleText(glyphs, sizeof(uint16_t) * count, SkTextEncoding::kGlyphID, startX, startY, skFont, paint);
            }
            else    // Try to print something
            {
                MSG_WARNING("Got no glyphs! Try to print: " << text);
                canvas.drawString(text.data(), startX, startY, skFont, paint);
            }

            if (glyphs)
                delete[] glyphs;
        }
    }

    return true;
}

int TButton::calcLineHeight(const string& text, SkFont& font)
{
    DECL_TRACER("TButton::calcLineHeight(const string& text, SkFont& font)");

    size_t pos = text.find("\n");       // Search for a line break.
    string lText = text;

    if (pos != string::npos)            // Do we have found a line break?
        lText = text.substr(0, pos - 1);// Yes, take only the text up to 1 before the line break (only 1 line).

    sk_sp<SkTextBlob> blob = SkTextBlob::MakeFromString(lText.c_str(), font);
    SkRect rect = blob.get()->bounds();
    return rect.height();
}

bool TButton::textEffect(SkCanvas *canvas, sk_sp<SkTextBlob>& blob, SkScalar startX, SkScalar startY, int instance)
{
    DECL_TRACER("TButton::textEffect(SkBitmap *bm, int instance)");

    if (!canvas)
        return false;

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Invalid instance " << instance);
        return false;
    }

    // Drop Shadow
    if (sr[instance].et >= 9 && sr[instance].et <= 32)
    {
        SkScalar gap = 0.0;
        SkScalar sigma = 0.0;
        SkScalar xDrop = 0.0;
        SkScalar yDrop = 0.0;
        uint8_t blurAlpha = 255;
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(TColor::getSkiaColor(sr[instance].ct));

        // Soft drop shadow
        if (sr[instance].et >= 9 && sr[instance].et <= 16)
        {
            gap = (SkScalar)sr[instance].et - 8.0f;
            sigma = 3.0f;
            blurAlpha = 127;
        }
        else if (sr[instance].et >= 17 && sr[instance].et <= 24) // Medium drop shadow
        {
            gap = (SkScalar)sr[instance].et - 16.0f;
            sigma = 2.0f;
            blurAlpha = 159;
        }
        else    // Hard drop shadow
        {
            gap = (SkScalar)sr[instance].et - 24.0f;
            sigma = 1.1f;
            blurAlpha = 207;
        }

        xDrop = gap;
        yDrop = gap;
        SkPaint blur(paint);
        blur.setAlpha(blurAlpha);
        blur.setColor(TColor::getSkiaColor(sr[instance].ec));
        blur.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, sigma, 0));
        canvas->drawTextBlob(blob.get(), startX + xDrop, startY + yDrop, blur);
        canvas->drawTextBlob(blob.get(), startX, startY, paint);
        return true;
    }
    else if (sr[instance].et >= 5 && sr[instance].et <= 8)  // Glow
    {
        SkScalar sigma = 0.0;

        switch(sr[instance].et)
        {
            case 5: sigma = 2.0; break;     // Glow-S
            case 6: sigma = 4.0; break;     // Glow-M
            case 7: sigma = 6.0; break;     // Glow-L
            case 8: sigma = 8.0; break;     // Glow-X
        }

        SkPaint paint, blur;
        paint.setAntiAlias(true);
        paint.setColor(TColor::getSkiaColor(sr[instance].ct));
        blur.setColor(TColor::getSkiaColor(sr[instance].ec));
        blur.setStyle(SkPaint::kStroke_Style);
        blur.setStrokeWidth(sigma / 1.5);
        blur.setMaskFilter(SkMaskFilter::MakeBlur(kOuter_SkBlurStyle, sigma));
        canvas->drawTextBlob(blob.get(), startX, startY, paint);
        canvas->drawTextBlob(blob.get(), startX, startY, blur);
        return true;
    }
    else if (sr[instance].et >= 1 && sr[instance].et <= 4)  // Outline
    {
        SkScalar sigma = 0.0;

        switch(sr[instance].et)
        {
            case 1: sigma = 1.0; break;     // Outline-S
            case 2: sigma = 2.0; break;     // Outline-M
            case 3: sigma = 4.0; break;     // Outline-L
            case 4: sigma = 6.0; break;     // Outline-X
        }

        SkPaint paint, outline;
        paint.setAntiAlias(true);
        paint.setColor(TColor::getSkiaColor(sr[instance].ct));
        outline.setAntiAlias(true);
        outline.setColor(TColor::getSkiaColor(sr[instance].ec));
        outline.setStyle(SkPaint::kStroke_Style);
        outline.setStrokeWidth(sigma);
        canvas->drawTextBlob(blob.get(), startX, startY, outline);
        canvas->drawTextBlob(blob.get(), startX, startY, paint);
        return true;
    }

    return false;
}

/**
 * @brief TButton::buttonBorder - draw a border, if any.
 * This method draws a border if there is one defined in \b sr[].bs. If there
 * is also a global border defined in \b bs then this border is limiting the
 * valid borders to it. The method does not check this, because it is subject
 * to TPDesign.
 *
 * @param bm        Bitmap to draw the border on.
 * @param inst      The instance where the border definitition should be taken from
 *
 * @return TRUE on success, otherwise FALSE.
 */
bool TButton::buttonBorder(SkBitmap* bm, int inst)
{
    DECL_TRACER("TButton::buttonBorder(SkBitmap* bm, int instance)");

    int instance = inst;

    if (instance < 0)
        instance = 0;
    else if ((size_t)instance > sr.size())
        instance = (int)sr.size() - 1;

    if (sr[instance].bs.empty())
    {
        MSG_DEBUG("No border defined.");
        return true;
    }

    string bname = sr[instance].bs;
    // Try to find the border in the system table
    if (drawBorder(bm, bname, wt, ht, sr[instance].cb))
        return true;

    // The border was not found or defined to be not drawn. Therefor we look
    // into the system directory (__system/graphics/borders). If the wanted
    // border exists there, we're drawing it.
    BORDER_t bd, bda;
    int numBorders = 0;
    bool extBorder = false;

    if (sr.size() == 2)
    {
        if (gPageManager->getSystemDraw()->getBorder(bname, TSystemDraw::LT_OFF, &bd))
            numBorders++;
    }
    else if (gPageManager->getSystemDraw()->getBorder(bname, TSystemDraw::LT_ON, &bd))
        numBorders++;

    if (numBorders)
        extBorder = true;

    if (extBorder)
    {
        SkColor color = TColor::getSkiaColor(sr[instance].cb);      // border color
        MSG_DEBUG("Button color: #" << std::setw(6) << std::setfill('0') << std::hex << color);
        // Load images
        SkBitmap imgB, imgBR, imgR, imgTR, imgT, imgTL, imgL, imgBL;

        if (!retrieveImage(bd.b, &imgB) || imgB.empty())
            return false;

        MSG_DEBUG("Got images " << bd.b << " and " << bda.b << " with size " << imgB.info().width() << " x " << imgB.info().height());
        if (!retrieveImage(bd.br, &imgBR) || imgBR.empty())
            return false;

        MSG_DEBUG("Got images " << bd.br << " and " << bda.br << " with size " << imgBR.info().width() << " x " << imgBR.info().height());
        if (!retrieveImage(bd.r, &imgR) || imgR.empty())
            return false;

        MSG_DEBUG("Got images " << bd.r << " and " << bda.r << " with size " << imgR.info().width() << " x " << imgR.info().height());
        if (!retrieveImage(bd.tr, &imgTR) || imgTR.empty())
            return false;

        MSG_DEBUG("Got images " << bd.tr << " and " << bda.tr << " with size " << imgTR.info().width() << " x " << imgTR.info().height());
        if (!retrieveImage(bd.t, &imgT) || imgT.empty())
            return false;

        MSG_DEBUG("Got images " << bd.t << " and " << bda.t << " with size " << imgT.info().width() << " x " << imgT.info().height());
        if (!retrieveImage(bd.tl, &imgTL) || imgTL.empty())
            return false;

        MSG_DEBUG("Got images " << bd.tl << " and " << bda.tl << " with size " << imgTL.info().width() << " x " << imgTL.info().height());
        if (!retrieveImage(bd.l, &imgL) || imgL.empty())
            return false;

        mBorderWidth = imgL.info().width();

        MSG_DEBUG("Got images " << bd.l << " and " << bda.l << " with size " << imgL.info().width() << " x " << imgL.info().height());
        if (!retrieveImage(bd.bl, &imgBL) || imgBL.empty())
            return false;

        MSG_DEBUG("Got images " << bd.bl << " and " << bda.bl << " with size " << imgBL.info().width() << " x " << imgBL.info().height());
        MSG_DEBUG("Button image size: " << (imgTL.info().width() + imgT.info().width() + imgTR.info().width()) << " x " << (imgTL.info().height() + imgL.info().height() + imgBL.info().height()));
        MSG_DEBUG("Total size: " << wt << " x " << ht);
        stretchImageWidth(&imgB, wt - imgBL.info().width() - imgBR.info().width());
        stretchImageWidth(&imgT, wt - imgTL.info().width() - imgTR.info().width());
        stretchImageHeight(&imgL, ht - imgTL.info().height() - imgBL.info().height());
        stretchImageHeight(&imgR, ht - imgTR.info().height() - imgBR.info().height());
        MSG_DEBUG("Stretched button image size: " << (imgTL.info().width() + imgT.info().width() + imgTR.info().width()) << " x " << (imgTL.info().height() + imgL.info().height() + imgBL.info().height()));
        // Draw the frame
        SkBitmap frame;
        allocPixels(bm->info().width(), bm->info().height(), &frame);
        frame.eraseColor(SK_ColorTRANSPARENT);
        SkCanvas target(*bm, SkSurfaceProps());
        SkCanvas canvas(frame, SkSurfaceProps());
        SkPaint paint;

        paint.setBlendMode(SkBlendMode::kSrcOver);
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgB);
        canvas.drawImage(_image, imgBL.info().width(), ht - imgB.info().height(), SkSamplingOptions(), &paint);
        _image = SkImage::MakeFromBitmap(imgBR);
        canvas.drawImage(_image, wt - imgBR.info().width(), ht - imgBR.info().height(), SkSamplingOptions(), &paint);
        _image = SkImage::MakeFromBitmap(imgR);
        canvas.drawImage(_image, wt - imgR.info().width(), imgTR.info().height(), SkSamplingOptions(), &paint);
        _image = SkImage::MakeFromBitmap(imgTR);
        canvas.drawImage(_image, wt - imgTR.info().width(), 0, SkSamplingOptions(), &paint);
        _image = SkImage::MakeFromBitmap(imgT);
        canvas.drawImage(_image, imgTL.info().width(), 0, SkSamplingOptions(), &paint);
        _image = SkImage::MakeFromBitmap(imgTL);
        canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        _image = SkImage::MakeFromBitmap(imgL);
        canvas.drawImage(_image, 0, imgTL.info().height(), SkSamplingOptions(), &paint);
        _image = SkImage::MakeFromBitmap(imgBL);
        canvas.drawImage(_image, 0, ht - imgBL.info().height(), SkSamplingOptions(), &paint);

        erasePart(bm, frame, Border::ERASE_OUTSIDE);
        backgroundFrame(bm, frame, color);
        _image = SkImage::MakeFromBitmap(frame);
        paint.setBlendMode(SkBlendMode::kSrcATop);
        target.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    }
    else    // We try to draw a frame by forcing it to draw even the not to draw marked frames.
        drawBorder(bm, bname, wt, ht, sr[instance].cb, true);

    return true;
}

int TButton::numberLines(const string& str)
{
    DECL_TRACER("TButton::numberLines(const string& str)");

    int lines = 1;

    if (str.empty())
    {
        MSG_DEBUG("Found an empty string.");
        return lines;
    }

    string::const_iterator iter;

    for (iter = str.begin(); iter != str.end(); ++iter)
    {
        if (*iter == '\n')
            lines++;
    }

    return lines;
}

SkRect TButton::calcRect(int width, int height, int pen)
{
    DECL_TRACER("TButton::calcRect(int width, int height, int pen)");
    SkRect rect;

    SkScalar left = (SkScalar)pen / 2.0;
    SkScalar top = (SkScalar)pen / 2.0;
    SkScalar w = (SkScalar)width - (SkScalar)pen;
    SkScalar h = (SkScalar)height - (SkScalar)pen;
    rect.setXYWH(left, top, w, h);
    return rect;
}

void TButton::runAnimation()
{
    DECL_TRACER("TButton::runAnimation()");

    if (mAniRunning)
        return;

    mAniRunning = true;
    int instance = 0;
    int max = (int)sr.size();
    ulong tm = nu * ru + nd * rd;

    while (mAniRunning && !mAniStop && !prg_stopped)
    {
        mActInstance = instance;
        mChanged = true;

        if (visible && !drawButton(instance))
            break;

        instance++;

        if (instance >= max)
            instance = 0;

        std::this_thread::sleep_for(std::chrono::milliseconds(tm));
    }

    mAniRunning = false;
}

void TButton::runAnimationRange(int start, int end, ulong step)
{
    DECL_TRACER("TButton::runAnimationRange(int start, int end, ulong step)");

    if (mAniRunning)
        return;

    mAniRunning = true;
    int instance = start - 1;
    int max = std::min(end, (int)sr.size());
    std::chrono::steady_clock::time_point startt = std::chrono::steady_clock::now();

    while (mAniRunning && !mAniStop && !prg_stopped)
    {
        mActInstance = instance;
        mChanged = true;

        if (visible)
            drawButton(instance);   // We ignore the state and try to draw the next instance

        instance++;

        if (instance >= max)
            instance = start - 1;

        std::this_thread::sleep_for(std::chrono::milliseconds(step));

        if (mAniRunTime > 0)
        {
            std::chrono::steady_clock::time_point current = std::chrono::steady_clock::now();
            std::chrono::nanoseconds difftime = current - startt;
            ulong duration = std::chrono::duration_cast<std::chrono::milliseconds>(difftime).count();

            if (duration >= mAniRunTime)
                break;
        }
    }

    mAniRunTime = 0;
    mAniRunning = false;
}

bool TButton::drawButtonMultistateAni()
{
    DECL_TRACER("TButton::drawButtonMultistateAni()");

    if (prg_stopped)
        return true;

    if (!visible || hd)    // Do nothing if this button is invisible
        return true;

    if (mAniRunning || mThrAni.joinable())
    {
        MSG_TRACE("Animation is already running!");
        return true;
    }

    try
    {
        mAniStop = false;
        mThrAni = thread([=] { runAnimation(); });
        mThrAni.detach();
    }
    catch (exception& e)
    {
        MSG_ERROR("Error starting the button animation thread: " << e.what());
        return false;
    }

    return true;
}

bool TButton::drawButton(int instance, bool show, bool subview)
{
    DECL_TRACER("TButton::drawButton(int instance, bool show, bool subview)");

    if (prg_stopped)
        return false;

    if (subview)
        mSubViewPart = subview;

    if ((size_t)instance >= sr.size() || instance < 0)
    {
        MSG_ERROR("Instance " << instance << " is out of bounds!");
        TError::setError();
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (!_displayButton && gPageManager)
        _displayButton = gPageManager->getCallbackDB();

    if (!visible || hd || instance != mActInstance || !_displayButton)
    {
        bool db = (_displayButton != nullptr);
        MSG_DEBUG("Button " << bi << ", \"" << na << "\" at instance " << instance << " is not to draw!");
        MSG_DEBUG("Visible: " << (visible ? "YES" : "NO") << ", Hidden: " << (hd ? "YES" : "NO") << ", Instance/actual instance: " << instance << "/" << mActInstance << ", callback: " << (db ? "PRESENT" : "N/A"));
#if TESTMODE == 1
        setScreenDone();
#endif
        return true;
    }

    MSG_DEBUG("Drawing button " << bi << ", \"" << na << "\" at instance " << instance);

    if (!mChanged && !mLastImage.empty())
    {
        if (show)
        {
            showLastButton();

            if (type == SUBPAGE_VIEW)
            {
                if (gPageManager)
                    gPageManager->showSubViewList(st, this);
            }
        }

        return true;
    }

    ulong parent = mHandle & 0xffff0000;
    getDrawOrder(sr[instance]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    SkBitmap imgButton;

    if (!allocPixels(wt, ht, &imgButton))
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    // We create an empty (transparent) image here. Later it depends on the
    // draw order of the elements. If, for example, the back ground fill is
    // not the first thing, we must be sure to not destroy already drawn
    // elemts of the button.
    imgButton.eraseColor(SkColors::kTransparent);
    bool dynState = false;

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!sr[instance].dynamic && !buttonBitmap(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
            else if (sr[instance].dynamic && !buttonDynamic(&imgButton, instance, show, &dynState))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
    }

    if (mGlobalOO >= 0 || sr[instance].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();

        if (!allocPixels(w, h, &ooButton))
        {
#if TESTMODE == 1
            setScreenDone();
#endif
            return false;
        }

        SkCanvas canvas(ooButton);
        SkIRect irect = SkIRect::MakeXYWH(0, 0, w, h);
        SkRegion region;
        region.setRect(irect);
        SkScalar oo;

        if (mGlobalOO >= 0 && sr[instance].oo >= 0)
        {
            oo = std::min((SkScalar)mGlobalOO, (SkScalar)sr[instance].oo);
            MSG_DEBUG("Set global overal opacity to " << oo);
        }
        else if (sr[instance].oo >= 0)
        {
            oo = (SkScalar)sr[instance].oo;
            MSG_DEBUG("Set overal opacity to " << oo);
        }
        else
        {
            oo = (SkScalar)mGlobalOO;
            MSG_DEBUG("Set global overal opacity to " << oo);
        }

        SkScalar alpha = 1.0 / 255.0 * oo;
        MSG_DEBUG("Calculated alpha value: " << alpha);
        SkPaint paint;
        paint.setAlphaf(alpha);
        paint.setImageFilter(SkImageFilters::AlphaThreshold(region, 0.0, alpha, nullptr));
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgButton);
        canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        imgButton.erase(SK_ColorTRANSPARENT, {0, 0, w, h});
        imgButton = ooButton;
    }

    mLastImage = imgButton;
    mChanged = false;

    if (!prg_stopped && !dynState)
    {
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());

            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrc);
            paint.setFilterQuality(kHigh_SkFilterQuality);
            // Calculate new dimension
            SkImageInfo info = imgButton.info();
            int width = (int)((double)info.width() * gPageManager->getScaleFactor());
            int height = (int)((double)info.height() * gPageManager->getScaleFactor());
            // Create a canvas and draw new image
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(imgButton);
            imgButton.allocN32Pixels(width, height);
            imgButton.eraseColor(SK_ColorTRANSPARENT);
            SkCanvas can(imgButton, SkSurfaceProps());
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            rowBytes = imgButton.info().minRowBytes();
            mLastImage = imgButton;
        }
#endif
        if (show)
        {
            MSG_DEBUG("Button type: " << buttonTypeToString());

            if (type != SUBPAGE_VIEW && !mSubViewPart)
            {
                TBitmap image((unsigned char *)imgButton.getPixels(), imgButton.info().width(), imgButton.info().height());
                _displayButton(mHandle, parent, image, rwidth, rheight, rleft, rtop, isPassThrough());
            }
            else if (type != SUBPAGE_VIEW && mSubViewPart)
            {
                if (gPageManager)
                    gPageManager->updateSubViewItem(this);
            }
        }
    }

    if (!prg_stopped && type == SUBPAGE_VIEW && show)
    {
        if (gPageManager)
            gPageManager->showSubViewList(st, this);
    }

    return true;
}

bool TButton::drawTextArea(int instance)
{
    DECL_TRACER("TButton::drawTextArea(int instance)");

    if (prg_stopped)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (!visible || hd)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return true;
    }

    if ((size_t)instance >= sr.size() || instance < 0)
    {
        MSG_ERROR("Instance " << instance << " is out of bounds!");
        TError::setError();
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (!mChanged)
    {
        showLastButton();
        return true;
    }

    getDrawOrder(sr[instance]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    SkBitmap imgButton;

    if (!allocPixels(wt, ht, &imgButton))
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!sr[instance].dynamic && !buttonBitmap(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
            else if (sr[instance].dynamic && !buttonDynamic(&imgButton, instance, false))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, instance))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                return false;
            }
        }
    }

    if (mGlobalOO >= 0 || sr[instance].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();

        if (!allocPixels(w, h, &ooButton))
        {
#if TESTMODE == 1
            setScreenDone();
#endif
            return false;
        }

        SkCanvas canvas(ooButton);
        SkIRect irect = SkIRect::MakeXYWH(0, 0, w, h);
        SkRegion region;
        region.setRect(irect);
        SkScalar oo;

        if (mGlobalOO >= 0 && sr[instance].oo >= 0)
        {
            oo = std::min((SkScalar)mGlobalOO, (SkScalar)sr[instance].oo);
            MSG_DEBUG("Set global overal opacity to " << oo);
        }
        else if (sr[instance].oo >= 0)
        {
            oo = (SkScalar)sr[instance].oo;
            MSG_DEBUG("Set overal opacity to " << oo);
        }
        else
        {
            oo = (SkScalar)mGlobalOO;
            MSG_DEBUG("Set global overal opacity to " << oo);
        }

        SkScalar alpha = 1.0 / 255.0 * oo;
        MSG_DEBUG("Calculated alpha value: " << alpha);
        SkPaint paint;
        paint.setAlphaf(alpha);
        paint.setImageFilter(SkImageFilters::AlphaThreshold(region, 0.0, alpha, nullptr));
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgButton);
        canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        imgButton.erase(SK_ColorTRANSPARENT, {0, 0, w, h});
        imgButton = ooButton;
    }

    mLastImage = imgButton;
    mChanged = false;

    if (!prg_stopped)
    {
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;
        size_t rowBytes = imgButton.info().minRowBytes();
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            size_t rowBytes = imgButton.info().minRowBytes();
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());

            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrc);
            paint.setFilterQuality(kHigh_SkFilterQuality);
            // Calculate new dimension
            SkImageInfo info = imgButton.info();
            int width = (int)((double)info.width() * gPageManager->getScaleFactor());
            int height = (int)((double)info.height() * gPageManager->getScaleFactor());
            // Create a canvas and draw new image
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(imgButton);
            imgButton.allocN32Pixels(width, height);
            imgButton.eraseColor(SK_ColorTRANSPARENT);
            SkCanvas can(imgButton, SkSurfaceProps());
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            rowBytes = imgButton.info().minRowBytes();
            mLastImage = imgButton;
        }
#endif
        if (gPageManager && gPageManager->getCallbackInputText())
        {
            BITMAP_t bm;
            bm.buffer = (unsigned char *)imgButton.getPixels();
            bm.rowBytes = rowBytes;
            bm.left = rleft;
            bm.top = rtop;
            bm.width = rwidth;
            bm.height = rheight;
            gPageManager->getCallbackInputText()(this, bm, mBorderWidth);
        }
    }

    return true;
}

bool TButton::drawMultistateBargraph(int level, bool show)
{
    mutex_button.lock();
    DECL_TRACER("TButton::drawMultistateBargraph(int level, bool show)");

    if (prg_stopped)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        mutex_button.unlock();
        return false;
    }

    if (!_displayButton && gPageManager)
        _displayButton = gPageManager->getCallbackDB();

    if (!visible || hd || !_displayButton)
    {
        bool db = (_displayButton != nullptr);
        MSG_DEBUG("Multistate bargraph " << bi << ", \"" << na << " is not to draw!");
        MSG_DEBUG("Visible: " << (visible ? "YES" : "NO") << ", callback: " << (db ? "PRESENT" : "N/A"));
#if TESTMODE == 1
        setScreenDone();
#endif
        mutex_button.unlock();
        return true;
    }

    int maxLevel = level;

    if (maxLevel > rh)
        maxLevel = rh;
    else if (maxLevel < rl)
        maxLevel = rl;
    else if (maxLevel < 0)
        maxLevel = rl;

    MSG_DEBUG("Display instance " << maxLevel);
    ulong parent = mHandle & 0xffff0000;
    getDrawOrder(sr[maxLevel]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        mutex_button.unlock();
        return false;
    }

    SkBitmap imgButton;

    if (!allocPixels(wt, ht, &imgButton))
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, maxLevel))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!buttonBitmap(&imgButton, maxLevel))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, maxLevel))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(&imgButton, maxLevel))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, maxLevel))
            {
#if TESTMODE == 1
                setScreenDone();
#endif
                mutex_button.unlock();
                return false;
            }
        }
    }

    if (mGlobalOO >= 0 || sr[maxLevel].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();

        if (!allocPixels(w, h, &ooButton))
        {
#if TESTMODE == 1
            setScreenDone();
#endif
            return false;
        }

        SkCanvas canvas(ooButton);
        SkIRect irect = SkIRect::MakeXYWH(0, 0, w, h);
        SkRegion region;
        region.setRect(irect);
        SkScalar oo;

        if (mGlobalOO >= 0 && sr[maxLevel].oo >= 0)
        {
            oo = std::min((SkScalar)mGlobalOO, (SkScalar)sr[maxLevel].oo);
            MSG_DEBUG("Set global overal opacity to " << oo);
        }
        else if (sr[maxLevel].oo >= 0)
        {
            oo = (SkScalar)sr[maxLevel].oo;
            MSG_DEBUG("Set overal opacity to " << oo);
        }
        else
        {
            oo = (SkScalar)mGlobalOO;
            MSG_DEBUG("Set global overal opacity to " << oo);
        }

        SkScalar alpha = 1.0 / 255.0 * oo;
        MSG_DEBUG("Calculated alpha value: " << alpha);
        SkPaint paint;
        paint.setAlphaf(alpha);
        paint.setImageFilter(SkImageFilters::AlphaThreshold(region, 0.0, alpha, nullptr));
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgButton);
        canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        imgButton.erase(SK_ColorTRANSPARENT, {0, 0, w, h});
        imgButton = ooButton;
    }

    mLastImage = imgButton;
    mChanged = false;

    if (!prg_stopped)
    {
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());

            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrc);
            paint.setFilterQuality(kHigh_SkFilterQuality);
            // Calculate new dimension
            SkImageInfo info = imgButton.info();
            int width = (int)((double)info.width() * gPageManager->getScaleFactor());
            int height = (int)((double)info.height() * gPageManager->getScaleFactor());
            MSG_DEBUG("Button dimension: " << width << " x " << height);
            // Create a canvas and draw new image
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(imgButton);
            imgButton.allocN32Pixels(width, height);
            imgButton.eraseColor(SK_ColorTRANSPARENT);
            SkCanvas can(imgButton, SkSurfaceProps());
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            MSG_DEBUG("Old rowBytes: " << rowBytes);
            rowBytes = imgButton.info().minRowBytes();
            MSG_DEBUG("New rowBytes: " << rowBytes);
            mLastImage = imgButton;
        }
#endif
        if (show)
        {
            TBitmap image((unsigned char *)imgButton.getPixels(), imgButton.info().width(), imgButton.info().height());
            _displayButton(mHandle, parent, image, rwidth, rheight, rleft, rtop, isPassThrough());
        }
#if TESTMODE == 1
        else
            setScreenDone();
#endif
    }

    mutex_button.unlock();
    return true;
}

bool TButton::drawList(bool show)
{
    DECL_TRACER("TButton::drawList(bool show)");

    if (!mChanged)
    {
        showLastButton();
        return true;
    }

    getDrawOrder(sr[0]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
        return false;

    SkBitmap imgButton;

    if (!allocPixels(wt, ht, &imgButton))
        return false;

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, 0))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!sr[0].dynamic && !buttonBitmap(&imgButton, 0))
                return false;
            else if (sr[0].dynamic && !buttonDynamic(&imgButton, 0, false))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, 0))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, 0))
                return false;
        }
    }

    if (mGlobalOO >= 0 || sr[0].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();

        if (!allocPixels(w, h, &ooButton))
            return false;

        SkCanvas canvas(ooButton);
        SkIRect irect = SkIRect::MakeXYWH(0, 0, w, h);
        SkRegion region;
        region.setRect(irect);
        SkScalar oo;

        if (mGlobalOO >= 0 && sr[0].oo >= 0)
        {
            oo = std::min((SkScalar)mGlobalOO, (SkScalar)sr[0].oo);
            MSG_DEBUG("Set global overal opacity to " << oo);
        }
        else if (sr[0].oo >= 0)
        {
            oo = (SkScalar)sr[0].oo;
            MSG_DEBUG("Set overal opacity to " << oo);
        }
        else
        {
            oo = (SkScalar)mGlobalOO;
            MSG_DEBUG("Set global overal opacity to " << oo);
        }

        SkScalar alpha = 1.0 / 255.0 * oo;
        MSG_DEBUG("Calculated alpha value: " << alpha);
        SkPaint paint;
        paint.setAlphaf(alpha);
        paint.setImageFilter(SkImageFilters::AlphaThreshold(region, 0.0, alpha, nullptr));
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgButton);
        canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        imgButton.erase(SK_ColorTRANSPARENT, {0, 0, w, h});
        imgButton = ooButton;
    }

    mLastImage = imgButton;
    mChanged = false;

    if (!prg_stopped)
    {
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;
        size_t rowBytes = imgButton.info().minRowBytes();
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            size_t rowBytes = imgButton.info().minRowBytes();
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());

            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrc);
            paint.setFilterQuality(kHigh_SkFilterQuality);
            // Calculate new dimension
            SkImageInfo info = imgButton.info();
            int width = (int)((double)info.width() * gPageManager->getScaleFactor());
            int height = (int)((double)info.height() * gPageManager->getScaleFactor());
            // Create a canvas and draw new image
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(imgButton);
            imgButton.allocN32Pixels(width, height);
            imgButton.eraseColor(SK_ColorTRANSPARENT);
            SkCanvas can(imgButton, SkSurfaceProps());
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            rowBytes = imgButton.info().minRowBytes();
            mLastImage = imgButton;
        }
#endif
        if (show && gPageManager && gPageManager->getCallbackListBox())
        {
            BITMAP_t bm;
            bm.buffer = (unsigned char *)imgButton.getPixels();
            bm.rowBytes = rowBytes;
            bm.left = rleft;
            bm.top = rtop;
            bm.width = rwidth;
            bm.height = rheight;
            gPageManager->getCallbackListBox()(this, bm, mBorderWidth);
        }
    }

    return true;
}

bool TButton::drawBargraph(int instance, int level, bool show)
{
    DECL_TRACER("TButton::drawBargraph(int instance, int level, bool show)");

    if ((size_t)instance >= sr.size() || instance < 0)
    {
        MSG_ERROR("Instance " << instance << " is out of bounds!");
        TError::setError();
        return false;
    }

    if (!_displayButton && gPageManager)
        _displayButton = gPageManager->getCallbackDB();

    if (!mChanged && mLastLevel == level)
    {
        showLastButton();
        return true;
    }

    TTRYLOCK(mutex_bargraph);

    if (level < rl)
        mLastLevel = rl;
    else if (level > rh)
        mLastLevel = rh;
    else
        mLastLevel = level;

    int inst = instance;

    if (!visible || hd || instance != mActInstance || !_displayButton)
    {
        bool db = (_displayButton != nullptr);
        MSG_DEBUG("Bargraph " << bi << ", \"" << na << "\" at instance " << instance << " with level " << mLastLevel << " is not to draw!");
        MSG_DEBUG("Visible: " << (visible ? "YES" : "NO") << ", Instance/actual instance: " << instance << "/" << mActInstance << ", callback: " << (db ? "PRESENT" : "N/A"));
        return true;
    }

    ulong parent = mHandle & 0xffff0000;

    if (type == BARGRAPH)
    {
        getDrawOrder(sr[1]._do, (DRAW_ORDER *)&mDOrder);
        inst = 1;
    }
    else
        getDrawOrder(sr[instance]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
        return false;

    SkBitmap imgButton;

    if (!allocPixels(wt, ht, &imgButton))
        return false;

    imgButton.eraseColor(TColor::getSkiaColor(sr[0].cf));
    bool haveFrame = false;

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL && !haveFrame)
        {
            if (!buttonFill(&imgButton, (type == BARGRAPH ? 0 : inst)))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!barLevel(&imgButton, inst, mLastLevel))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, inst))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(&imgButton, inst))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, (type == BARGRAPH ? 0 : inst)))
                return false;

            haveFrame = true;
        }
    }

    if (mGlobalOO >= 0 || sr[inst].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();

        if (!allocPixels(w, h, &ooButton))
            return false;

        SkCanvas canvas(ooButton);
        SkIRect irect = SkIRect::MakeXYWH(0, 0, w, h);
        SkRegion region;
        region.setRect(irect);
        SkScalar oo;

        if (mGlobalOO >= 0 && sr[inst].oo >= 0)
        {
            oo = std::min((SkScalar)mGlobalOO, (SkScalar)sr[inst].oo);
            MSG_DEBUG("Set global overal opacity to " << oo);
        }
        else if (sr[inst].oo >= 0)
        {
            oo = (SkScalar)sr[inst].oo;
            MSG_DEBUG("Set overal opacity to " << oo);
        }
        else
        {
            oo = (SkScalar)mGlobalOO;
            MSG_DEBUG("Set global overal opacity to " << oo);
        }

        SkScalar alpha = 1.0 / 255.0 * oo;
        MSG_DEBUG("Calculated alpha value: " << alpha);
        SkPaint paint;
        paint.setAlphaf(alpha);
        paint.setImageFilter(SkImageFilters::AlphaThreshold(region, 0.0, alpha, nullptr));
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(imgButton);
        canvas.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
        imgButton.erase(SK_ColorTRANSPARENT, {0, 0, w, h});
        imgButton = ooButton;
    }

    mLastImage = imgButton;
    mChanged = false;

    if (!prg_stopped && show && visible && instance == mActInstance && _displayButton)
    {
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());

            SkPaint paint;
            paint.setBlendMode(SkBlendMode::kSrc);
            paint.setFilterQuality(kHigh_SkFilterQuality);
            // Calculate new dimension
            SkImageInfo info = imgButton.info();
            int width = (int)((double)info.width() * gPageManager->getScaleFactor());
            int height = (int)((double)info.height() * gPageManager->getScaleFactor());
            // Create a canvas and draw new image
            sk_sp<SkImage> im = SkImage::MakeFromBitmap(imgButton);
            imgButton.allocN32Pixels(width, height);
            imgButton.eraseColor(SK_ColorTRANSPARENT);
            SkCanvas can(imgButton, SkSurfaceProps());
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            mLastImage = imgButton;
        }
#endif
        TBitmap image((unsigned char *)imgButton.getPixels(), imgButton.info().width(), imgButton.info().height());
        _displayButton(mHandle, parent, image, rwidth, rheight, rleft, rtop, isPassThrough());
    }

    return true;
}

POSITION_t TButton::calcImagePosition(int width, int height, CENTER_CODE cc, int number, int line)
{
    DECL_TRACER("TButton::calcImagePosition(int with, int height, CENTER_CODE code, int number)");

    SR_T act_sr;
    POSITION_t position;
    int ix, iy, ln;

    if (sr.size() == 0)
        return position;

    if (number <= 0)
        act_sr = sr.at(0);
    else if ((size_t)number < sr.size())
        act_sr = sr.at(number);
    else if ((size_t)number >= sr.size())
        act_sr = sr.at(sr.size() - 1);
    else
        return position;

    if (line <= 0)
        ln = 1;
    else
        ln = line;

    int border_size = getBorderSize(act_sr.bs);
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
            rwt = std::min(wt - border * 2, width);
            rht = std::min(ht - border_size * 2, height);
        break;

        case SC_TEXT:
            code = act_sr.jt;
            ix = act_sr.tx;
            iy = act_sr.ty;
            dbgCC = "TEXT";

            if (border < 4)
                border = 4;
//            border += 4;
            rwt = std::min(wt - border * 2, width);         // We've always a minimum (invisible) border of 4 pixels.
            rht = std::min(ht - border_size * 2, height);   // The height is calculated from a defined border, if any.
        break;
    }

    if (width > rwt || height > rht)
        position.overflow = true;

    switch (code)
    {
        case 0: // absolute position
            position.left = ix;
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
                position.top = ht - ((ht - rht) / 2) - height * ln;
            }

            position.width = rwt;
            position.height = rht;
        break;

        case 2: // center, top
            if (cc == SC_TEXT)
                position.top = ht - ((ht - rht) / 2) - height * ln;

            position.left = (wt - rwt) / 2;
            position.height = rht;
            position.width = rwt;
        break;

        case 3: // right, top
            position.left = wt - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = ht - (ht - rht) - height * ln;
            }

            position.width = rwt;
            position.height = rht;
        break;

        case 4: // left, middle
            if (cc == SC_TEXT)
            {
                position.left = border;
                position.top = (ht - height) / 2;
            }
            else
                position.top = (ht - rht) / 2;

            position.width = rwt;
            position.height = rht;
        break;

        case 6: // right, middle
            position.left = wt - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = (ht - height) / 2;
            }
            else
                position.top = (ht - rht) / 2;

            position.width = rwt;
            position.height = rht;
        break;

        case 7: // left, bottom
            if (cc == SC_TEXT)
            {
                position.left = border_size;
                position.top = (ht - rht) - height * ln;
            }
            else
                position.top = ht - rht;

            position.width = rwt;
            position.height = rht;
        break;

        case 8: // center, bottom
            position.left = (wt - rwt) / 2;

            if (cc == SC_TEXT)
                position.top = (ht - rht) - height * ln;
            else
                position.top = ht - rht;

            position.width = rwt;
            position.height = rht;
        break;

        case 9: // right, bottom
            position.left = wt - rwt;

            if (cc == SC_TEXT)
            {
                position.left = (((position.left - border) < 0) ? 0 : position.left - border);
                position.top = (ht - rht) - height * ln;
            }
            else
                position.top = ht - rht;
        break;

        default: // center, middle
            position.left = (wt - rwt) / 2;

            if (cc == SC_TEXT)
                position.top = (ht - height) / 2;
            else
                position.top = (ht - rht) / 2;

            position.width = rwt;
            position.height = rht;
    }

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        string format = getFormatString((TEXT_ORIENTATION)code);
        MSG_DEBUG("Type: " << dbgCC << ", format: " << format <<
            ", PosType=" << code << ", total height=" << ht << ", height object=" << height <<
            ", Position: x=" << position.left << ", y=" << position.top << ", w=" << position.width <<
            ", h=" << position.height << ", Overflow: " << (position.overflow ? "YES" : "NO"));
    }

    position.valid = true;
    return position;
}

IMAGE_SIZE_t TButton::calcImageSize(int imWidth, int imHeight, int instance, bool aspect)
{
    DECL_TRACER("TButton::calcImageSize(int imWidth, int imHeight, bool aspect)");

    int border = getBorderSize(sr[instance].bs);
    IMAGE_SIZE_t isize;

    if (!aspect)
    {
        isize.width = wt - border * 2;
        isize.height = ht - border * 2;
    }
    else
    {
        int w = wt - border * 2;
        int h = ht - border * 2;
        double scale;

        if (w < h || imWidth > imHeight)
            scale = (double)w / (double)imWidth;
        else
            scale = (double)h / (double)imHeight;

        isize.width = (int)((double)imWidth * scale);
        isize.height = (int)((double)imHeight * scale);
    }

    MSG_DEBUG("Sizing image: Original: " << imWidth << " x " << imHeight << " to " << isize.width << " x " << isize.height);
    return isize;
}

string TButton::getFormatString(TEXT_ORIENTATION to)
{
    DECL_TRACER("TButton::getFormatString(CENTER_CODE cc)");

    switch(to)
    {
        case ORI_ABSOLUT:       return "ABSOLUT";
        case ORI_BOTTOM_LEFT:   return "BOTTOM/LEFT";
        case ORI_BOTTOM_MIDDLE: return "BOTTOM/MIDDLE";
        case ORI_BOTTOM_RIGHT:  return "BOTTOM/RIGHT";
        case ORI_CENTER_LEFT:   return "CENTER/LEFT";
        case ORI_CENTER_MIDDLE: return "CENTER/MIDDLE";
        case ORI_CENTER_RIGHT:  return "CENTER/RIGHT";
        case ORI_TOP_LEFT:      return "TOP/LEFT";
        case ORI_TOP_MIDDLE:    return "TOP/MIDDLE";
        case ORI_TOP_RIGHT:     return "TOP/RIGHT";
    }

    return "UNKNOWN";   // Should not happen!
}

int TButton::getBorderSize(const std::string& name)
{
    DECL_TRACER("TButton::getBorderSize(const std::string& name)");

    int width = getBorderWidth(name);

    if (width > 0)
        return width;

    if (gPageManager && gPageManager->getSystemDraw())
    {
        if (gPageManager->getSystemDraw()->existBorder(name))
            return gPageManager->getSystemDraw()->getBorderWidth(name);
    }

    return 0;
}

void TButton::calcImageSizePercent(int imWidth, int imHeight, int btWidth, int btHeight, int btFrame, int *realX, int *realY)
{
    DECL_TRACER("TButton::clacImageSizePercent(int imWidth, int imHeight, int btWidth, int btHeight, int btFrame, int *realX, int *realY)");

    int spX = btWidth - (btFrame * 2);
    int spY = btHeight - (btFrame * 2);

    if (imWidth <= spX && imHeight <= spY)
    {
        *realX = imWidth;
        *realY = imHeight;
        return;
    }

    int oversizeX = 0, oversizeY = 0;

    if (imWidth > spX)
        oversizeX = imWidth - spX;

    if (imHeight > spY)
        oversizeY = imHeight - spY;

    double percent = 0.0;

    if (oversizeX > oversizeY)
        percent = 100.0 / (double)imWidth * (double)spX;
    else
        percent = 100.0 / (double)imHeight * (double)spY;

    *realX = (int)(percent / 100.0 * (double)imWidth);
    *realY = (int)(percent / 100.0 * (double)imHeight);
}

SkBitmap TButton::drawImageButton(SkBitmap& imgRed, SkBitmap& imgMask, int width, int height, SkColor col1, SkColor col2)
{
    DECL_TRACER("TButton::drawImageButton(SkImage& imgRed, SkImage& imgMask, int width, int height, SkColor col1, SkColor col2)");

    if (width <= 0 || height <= 0)
    {
        MSG_WARNING("Got invalid width of height! (width: " << width << ", height: " << height << ")");
        return SkBitmap();
    }

    if (imgRed.empty())
    {
        MSG_WARNING("Missing mask to draw image!");
        return SkBitmap();
    }

    SkPixmap pixmapRed = imgRed.pixmap();
    SkPixmap pixmapMask;
    bool haveBothImages = true;

    if (!imgMask.empty())
        pixmapMask = imgMask.pixmap();
    else
        haveBothImages = false;

    SkBitmap maskBm;

    if (!allocPixels(width, height, &maskBm))
        return SkBitmap();

    maskBm.eraseColor(SK_ColorTRANSPARENT);

    for (int ix = 0; ix < width; ix++)
    {
        for (int iy = 0; iy < height; iy++)
        {
            SkColor pixelRed;
            SkColor pixelMask;

            if (ix < pixmapRed.info().width() && iy < pixmapRed.info().height())
                pixelRed = pixmapRed.getColor(ix, iy);
            else
                pixelRed = 0;

            if (haveBothImages && !imgMask.empty() &&
                    ix < pixmapMask.info().width() && iy < pixmapMask.info().height())
                pixelMask = pixmapMask.getColor(ix, iy);
            else
                pixelMask = SkColorSetA(SK_ColorWHITE, 0);

            SkColor pixel = baseColor(pixelRed, pixelMask, col1, col2);
            uint32_t alpha = SkColorGetA(pixel);
            uint32_t *wpix = nullptr;

            if (ix < maskBm.info().width() && iy < maskBm.info().height())
                wpix = maskBm.getAddr32(ix, iy);

            if (!wpix)
                continue;

            if (alpha == 0)
                pixel = pixelMask;

            *wpix = pixel;
        }
    }

    return maskBm;
}

/**
 * @brief Takes 2 images and combines them to one.
 *
 * The 2 images are a solid base image defining the basic form and an identical
 * image defining the alpha channel.
 *
 * @param base  The base image containing the form as black pixels.
 * @param alpha The image containing just an alpha channel.
 * @param col   The color which should be used instead of a black pixel.
 *
 * @return On success a valid bitmap is returned containing the form.
 * On error an empty bitmap is returned.
 */
SkBitmap TButton::combineImages(SkBitmap& base, SkBitmap& alpha, SkColor col)
{
    DECL_TRACER("TButton::combineImages(SkBitmap& base, SkBitmap& alpha, SkColor col)");

    int width = base.info().width();
    int height = base.info().height();
    SkBitmap Bm;    // The new bitmap. It will be returned in the end.

    if (width != alpha.info().width() || height != alpha.info().height())
    {
        MSG_ERROR("Mask and alpha have different size! [ " << width << " x " << height << " to " << alpha.info().width() << " x " << alpha.info().height());
        return Bm;
    }

    if (!allocPixels(width, height, &Bm))
        return Bm;

    Bm.eraseColor(SK_ColorTRANSPARENT);

    for (int ix = 0; ix < width; ix++)
    {
        for (int iy = 0; iy < height; iy++)
        {
            SkColor pixelAlpha = alpha.getColor(ix, iy);
            uint32_t *bpix = Bm.getAddr32(ix, iy);

            uchar al    = SkColorGetA(pixelAlpha);
            uchar red   = SkColorGetR(col);
            uchar green = SkColorGetG(col);
            uchar blue  = SkColorGetB(col);

            if (pixelAlpha == 0)
                red = green = blue = 0;

            // Skia reads image files in the natural byte order of the CPU.
            // While on Intel CPUs the byte order is little endian it is
            // mostly big endian on other CPUs. This means that the order of
            // the colors is RGB on big endian CPUs (ARM, ...) and BGR on others.
            // To compensate this, we check the endianess of the CPU and set
            // the byte order according.

            if (isBigEndian())
                *bpix = SkColorSetARGB(al, blue, green, red);
            else
                *bpix = SkColorSetARGB(al, red, green, blue);
        }
    }

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);
    SkCanvas can(Bm);
    sk_sp<SkImage> _image = SkImage::MakeFromBitmap(base);
    can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    return Bm;
}

/**
 * @brief TButton::colorImage: Colorize frame element
 * This method colorizes a frame element. If there is, beside the base picture,
 * also a an alpha mask picture present, the elemnt is colorized by taking the
 * mask to find the pixels to colorize.
 * Otherwise the pixel is melted with the target color. This means a pseudo mask
 * is used.
 *
 * @param base      This is the base image and must be present.
 * @param alpha     This is optional alpha mask. If present it is used to
 *                  define the alpha value of the pixels.
 * @param col       This is the color to be used.
 * @param bg        This is the background color to be used on the transparent
 *                  pixels inside an element. On the transparent pixels on the
 *                  outside of the element the pixel is set to transparent.
 * @param useBG     If this is TRUE, all transparent pixels are set to the
 *                  background color \b bg.
 * @return
 * On success a new image containing the colorized element is returned.
 * Otherwise an empty image is returned.
 */
SkBitmap TButton::colorImage(SkBitmap& base, SkBitmap& alpha, SkColor col, SkColor bg, bool useBG)
{
    DECL_TRACER("TButton::colorImage(SkBitmap *img, int width, int height, SkColor col, SkColor bg, bool useBG)");

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
                pixelAlpha = SK_ColorTRANSPARENT; //col;
            else if (ala == 0)
                pixelAlpha = bg;
            else
            {
                uint32_t red   = SkColorGetR(col);
                uint32_t green = SkColorGetG(col);
                uint32_t blue  = SkColorGetB(col);
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
        sk_sp<SkImage> _image = SkImage::MakeFromBitmap(base);
        can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    }

    return maskBm;
}

SkBitmap TButton::retrieveBorderImage(const string& pa, const string& pb, SkColor color, SkColor bgColor)
{
    DECL_TRACER("TButton::retrieveBorderImage(const string& pa, const string& pb, SkColor color, SkColor bgColor)");

    SkBitmap bm, bma;

    if (!pa.empty() && !retrieveImage(pa, &bm))
        return SkBitmap();

    if (!pb.empty() && !retrieveImage(pb, &bma))
        return bm;
//        return SkBitmap();

    return colorImage(bm, bma, color, bgColor, false);
}

bool TButton::retrieveImage(const string& path, SkBitmap* image)
{
    DECL_TRACER("TButton::retrieveImage(const string& path, SkBitmap* image)");

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

void TButton::show()
{
    DECL_TRACER("TButton::show()");

    // First we detect whether we have a dynamic button or not.
    // To do this, we find out the current active instance.
    int inst = 0;

    if (mActInstance >= 0 && (size_t)mActInstance < sr.size())
        inst = mActInstance;
    // If the dynamic flag is not set and we have already an image of the
    // button, we send just the saved image to the screen.
    if (visible && !mChanged && !sr[inst].dynamic && !mLastImage.empty())
    {
        showLastButton();
        return;
    }
    // Here the button, or the active instance was never drawn or it is a
    // dynamic button. Then the button must be drawn.
    visible = true;
    makeElement();

    if (isSystemButton() && !mSystemReg)
        registerSystemButton();
}

void TButton::showLastButton()
{
    DECL_TRACER("TButton::showLastButton()");

    if (mLastImage.empty())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!_displayButton && gPageManager)
        _displayButton = gPageManager->getCallbackDB();

    if (!prg_stopped && visible)
    {
        ulong parent = mHandle & 0xffff0000;
        size_t rowBytes = mLastImage.info().minRowBytes();
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());
        }
#endif
        if (type == TEXT_INPUT)
        {
            if (gPageManager && gPageManager->getCallbackInputText())
            {
                BITMAP_t bm;
                bm.buffer = (unsigned char *)mLastImage.getPixels();
                bm.rowBytes = rowBytes;
                bm.left = rleft;
                bm.top = rtop;
                bm.width = rwidth;
                bm.height = rheight;
                gPageManager->getCallbackInputText()(this, bm, mBorderWidth);
            }
        }
        else if (type == LISTBOX)
        {
            if (gPageManager && gPageManager->getCallbackListBox())
            {
                BITMAP_t bm;
                bm.buffer = (unsigned char *)mLastImage.getPixels();
                bm.rowBytes = rowBytes;
                bm.left = rleft;
                bm.top = rtop;
                bm.width = rwidth;
                bm.height = rheight;
                gPageManager->getCallbackListBox()(this, bm, mBorderWidth);
            }
        }
        else if (type == SUBPAGE_VIEW)
        {
            if (gPageManager && gPageManager->getDisplayViewButton())
            {
                TBitmap image((unsigned char *)mLastImage.getPixels(), mLastImage.info().width(), mLastImage.info().height());
                TColor::COLOR_T bgcolor = TColor::getAMXColor(sr[mActInstance].cf);
                gPageManager->getDisplayViewButton()(mHandle, getParent(), (on.empty() ? false : true), image, wt, ht, lt, tp, sa, bgcolor);
            }
        }
        else if (_displayButton)
        {
            TBitmap image((unsigned char *)mLastImage.getPixels(), mLastImage.info().width(), mLastImage.info().height());
            _displayButton(mHandle, parent, image, rwidth, rheight, rleft, rtop, isPassThrough());
        }

        mChanged = false;
    }
}

void TButton::hide(bool total)
{
    DECL_TRACER("TButton::hide()");

//    if (type == MULTISTATE_GENERAL && ar == 1)
//        mAniStop = true;

    if (!prg_stopped && total)
    {
        int rwidth = wt;
        int rheight = ht;
        int rleft = lt;
        int rtop = tp;

        ulong parent = mHandle & 0xffff0000;
        THR_REFRESH_t *tr = _findResource(mHandle, parent, bi);

        if (tr && tr->mImageRefresh)
        {
            if (tr->mImageRefresh->isRunning())
                tr->mImageRefresh->stop();
        }
#ifdef _SCALE_SKIA_
        if (gPageManager && gPageManager->getScaleFactor() != 1.0)
        {
            rwidth = (int)((double)wt * gPageManager->getScaleFactor());
            rheight = (int)((double)ht * gPageManager->getScaleFactor());
            rleft = (int)((double)lt * gPageManager->getScaleFactor());
            rtop = (int)((double)tp * gPageManager->getScaleFactor());
        }
#endif
        if (type == TEXT_INPUT)
        {
            if (gPageManager && gPageManager->getCallDropButton())
                gPageManager->getCallDropButton()(mHandle);

            visible = false;
            return;
        }

        SkBitmap imgButton;

        if (rwidth < 0 || rheight < 0)
        {
            MSG_ERROR("Invalid size of image: " << rwidth << " x " << rheight);
            return;
        }

        try
        {
            if (!allocPixels(wt, ht, &imgButton))
                return;

            imgButton.eraseColor(SK_ColorTRANSPARENT);
        }
        catch (std::exception& e)
        {
            MSG_ERROR("Error creating image: " << e.what());
            visible = false;
            return;
        }

        if (!_displayButton && gPageManager)
            _displayButton = gPageManager->getCallbackDB();

        if (_displayButton)
        {
            TBitmap image((unsigned char *)imgButton.getPixels(), imgButton.info().width(), imgButton.info().height());
            _displayButton(mHandle, parent, image, rwidth, rheight, rleft, rtop, isPassThrough());
            mChanged = false;
        }
    }

    visible = false;
}

bool TButton::isClickable(int x, int y)
{
    DECL_TRACER("TButton::isClickable()");

    if (mEnabled && /*((cp != 0 && ch != 0) || (lp != 0 && lv != 0) || !cm.empty() || !op.empty() || !pushFunc.empty() || isSystemButton()) &&*/ hs.compare("passThru") != 0)
    {
        if (x != -1 && y != -1 && hs.empty() && !mLastImage.empty() && isPixelTransparent(x, y))
            return false;

        return true;
    }

    return false;
}

/**
 * Handling of system button "connection state". It consists of 12 states
 * indicating the network status. The states have the following meaning:
 *
 * 0      Diconnected (never was connected before since startup)
 * 1 - 6  Connected (blink may be shown with dark and light green)
 * 7, 8   Disconnected (timeout or loss of connection)
 * 9 - 11 Connection in progress
 */
void TButton::funcNetwork(int state)
{
    mutex_sysdraw.lock();
    DECL_TRACER("TButton::funcNetwork(int state)");

    mLastLevel = state;
    mActInstance = state;
    mChanged = true;

    if (visible)
        makeElement(state);

    mutex_sysdraw.unlock();
}

/**
 * Handling the timer event from the controller. This comes usualy every
 * 20th part of a second (1 second / 20)
 */
void TButton::funcTimer(const amx::ANET_BLINK& blink)
{
    mutex_sysdraw.lock();
    DECL_TRACER("TButton::funcTimer(const amx::ANET_BLINK& blink)");

    string tm;
    std::stringstream sstr;

    switch (ad)
    {
        case 141:   // Standard time
            sstr << std::setw(2) << std::setfill('0') << (int)blink.hour << ":"
                 << std::setw(2) << std::setfill('0') << (int)blink.minute << ":"
                 << std::setw(2) << std::setfill('0') << (int)blink.second;
            mLastBlink = blink;
        break;

        case 142:   // Time AM/PM
        {
            int hour = (blink.hour > 12) ? (blink.hour - 12) : blink.hour;
            sstr << std::setw(2) << std::setfill('0') << hour << ":"
                 << std::setw(2) << std::setfill('0') << (int)blink.minute << " ";

            if (blink.hour <= 12)
                sstr << "AM";
            else
                sstr << "PM";

            mLastBlink = blink;
        }
        break;

        case 143:   // Time 24 hours
            sstr << std::setw(2) << std::setfill('0') << (int)blink.hour << ":"
                 << std::setw(2) << std::setfill('0') << (int)blink.minute;
            mLastBlink = blink;
        break;

        case 151:   // Weekday
            switch (blink.weekday)
            {
                case 0: sstr << "Monday"; break;
                case 1: sstr << "Tuesday"; break;
                case 2: sstr << "Wednesday"; break;
                case 3: sstr << "Thursday"; break;
                case 4: sstr << "Friday"; break;
                case 5: sstr << "Saturday"; break;
                case 6: sstr << "Sunday"; break;
            }
        break;

        case 152:   // Date mm/dd
            sstr << (int)blink.month << "/" << (int)blink.day;
        break;

        case 153:   // Date dd/mm
            sstr << (int)blink.day << "/" << (int)blink.month;
        break;

        case 154:   // Date mm/dd/yyyy
            sstr << (int)blink.month << "/" << (int)blink.day << "/" << (int)blink.year;
        break;

        case 155:   // Date dd/mm/yyyy
            sstr << blink.day << "/" << blink.month << "/" << blink.year;
        break;

        case 156:   // Date month dd/yyyy
            switch (blink.month)
            {
                case 1:  sstr << "January"; break;
                case 2:  sstr << "February"; break;
                case 3:  sstr << "March"; break;
                case 4:  sstr << "April"; break;
                case 5:  sstr << "May"; break;
                case 6:  sstr << "June"; break;
                case 7:  sstr << "July"; break;
                case 8:  sstr << "August"; break;
                case 9:  sstr << "September"; break;
                case 10:  sstr << "October"; break;
                case 11:  sstr << "November"; break;
                case 12:  sstr << "December"; break;
            }

            sstr << " " << (int)blink.day << "/" << (int)blink.year;
        break;

        case 157:   // Date dd month yyyy
            sstr << (int)blink.day;

            switch (blink.month)
            {
                case 1:  sstr << "January"; break;
                case 2:  sstr << "February"; break;
                case 3:  sstr << "March"; break;
                case 4:  sstr << "April"; break;
                case 5:  sstr << "May"; break;
                case 6:  sstr << "June"; break;
                case 7:  sstr << "July"; break;
                case 8:  sstr << "August"; break;
                case 9:  sstr << "September"; break;
                case 10:  sstr << "October"; break;
                case 11:  sstr << "November"; break;
                case 12:  sstr << "December"; break;
            }

            sstr << " " << (int)blink.year;
        break;

        case 158:   // Date yyyy-mm-dd
            sstr << (int)blink.year << "-" << (int)blink.month << "-" << (int)blink.day;
        break;

        default:
            mutex_sysdraw.unlock();
            return;
    }

    vector<SR_T>::iterator iter;
    tm = sstr.str();

    for (iter = sr.begin(); iter != sr.end(); ++iter)
        iter->te = tm;

    mChanged = true;

    if (visible)
        makeElement(mActInstance);

    mutex_sysdraw.unlock();
}

bool TButton::isPixelTransparent(int x, int y)
{
    DECL_TRACER("TButton::isPixelTransparent(int x, int y)");

    // If there is no image we treat it as a non transpararent pixel.
    if (sr[mActInstance].mi.empty() && sr[mActInstance].bm.empty())
        return false;

    // The mLastImage must never be empty! Although this should never be true,
    // we treat it as a transparent pixel if it happen.
    if (mLastImage.empty())
    {
        MSG_ERROR("Internal error: No image for button available!");
        return true;
    }

    // Make sure the coordinates are inside the bounds. A test for a pixel
    // outside the bounds would lead in an immediate exit because of an assert
    // test in skia. Although this should not happen, we treat the pixel as
    // transparent in this case, because the coordinates didn't hit the button.
    if (x < 0 || x >= mLastImage.info().width() || y < 0 || y >= mLastImage.info().height())
    {
        MSG_ERROR("The X or Y coordinate is out of bounds!");
        MSG_ERROR("X=" << x << ", Y=" << y << ", width=" << mLastImage.info().width() << ", height=" << mLastImage.info().height());
        return true;
    }

    float alpha = mLastImage.getAlphaf(x, y);   // Get the alpha value (0.0 to 1.0)

    if (alpha != 0.0)
        return false;

    return true;
}

bool TButton::checkForSound()
{
    DECL_TRACER("TButton::checkForSound()");

    vector<SR_T>::iterator iter;

    for (iter = sr.begin(); iter != sr.end(); ++iter)
    {
        if (!iter->sd.empty())
            return true;
    }

    return false;
}

bool TButton::scaleImage(SkBitmap *bm, double scaleWidth, double scaleHeight)
{
    DECL_TRACER("TButton::scaleImage(SkBitmap *bm, double scaleWidth, double scaleHeight)");

    if (!bm)
        return false;

    if (scaleWidth == 1.0 && scaleHeight == 1.0)
        return true;

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);
//    paint.setFilterQuality(kHigh_SkFilterQuality);
    // Calculate new dimension
    SkImageInfo info = bm->info();
    int width  = std::max(1, (int)((double)info.width() * scaleWidth));
    int height = std::max(1, (int)((double)info.height() * scaleHeight));
    MSG_DEBUG("Scaling image to size " << width << " x " << height);
    // Create a canvas and draw new image
    sk_sp<SkImage> im = SkImage::MakeFromBitmap(*bm);

    if (!allocPixels(width, height, bm))
        return false;

    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps());
    SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
    can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
    return true;
}

bool TButton::stretchImageWidth(SkBitmap *bm, int width)
{
    DECL_TRACER("TButton::stretchImageWidth(SkBitmap *bm, int width)");

    if (!bm)
        return false;

    int rwidth = width;
    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);
//    paint.setFilterQuality(kHigh_SkFilterQuality);

    SkImageInfo info = bm->info();
    sk_sp<SkImage> im = SkImage::MakeFromBitmap(*bm);

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

bool TButton::stretchImageHeight(SkBitmap *bm, int height)
{
    DECL_TRACER("TButton::stretchImageHeight(SkBitmap *bm, int height)");

    if (!bm)
        return false;

    int rheight = height;
    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);
//    paint.setFilterQuality(kHigh_SkFilterQuality);

    SkImageInfo info = bm->info();

    if (height <= 0)
        rheight = info.height() + height;

    if (rheight <= 0)
        rheight = 1;

    sk_sp<SkImage> im = SkImage::MakeFromBitmap(*bm);
    MSG_DEBUG("Width: " << info.width() << ", Height: " << rheight);

    if (!allocPixels(info.width(), rheight, bm))
        return false;

    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps());
    SkRect rect = SkRect::MakeXYWH(0, 0, info.width(), rheight);
    can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
    return true;
}

bool TButton::stretchImageWH(SkBitmap *bm, int width, int height)
{
    DECL_TRACER("TButton::stretchImageWH(SkBitmap *bm, int width, int height)");

    if (!bm)
        return false;

    int rwidth = width;
    int rheight = height;
    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrc);
//    paint.setFilterQuality(kHigh_SkFilterQuality);

    SkImageInfo info = bm->info();

    if (width <= 0)
        rwidth = info.width() + width;

    if (height <= 0)
        rheight = info.height() + height;

    if (rheight <= 0)
        rheight = 1;

    if (rwidth <= 0)
        rwidth = 1;

    sk_sp<SkImage> im = SkImage::MakeFromBitmap(*bm);
    MSG_DEBUG("Width: " << rwidth << ", Height: " << rheight);

    if (!allocPixels(rwidth, rheight, bm))
        return false;

    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps());
    SkRect rect = SkRect::MakeXYWH(0, 0, rwidth, rheight);
    can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
    return true;
}

/**
 * This button got the click because it matches the coordinates of a mouse
 * click. It checkes whether it is clickable or not. If it is clickable, it
 * depends on the type of element what happens.
 */
bool TButton::doClick(int x, int y, bool pressed)
{
    DECL_TRACER("TButton::doClick(int x, int y, bool pressed)");

    if (!isClickable(x, y))
        return false;

    amx::ANET_SEND scmd;
    int instance = 0;
    int sx = x, sy = y;
    bool isSystem = isSystemButton();

    if (pressed && gPageManager && !checkForSound() && (ch > 0 || lv > 0 || !pushFunc.empty() || isSystem))
    {
        TSystemSound sysSound(TConfig::getSystemPath(TConfig::SOUNDS));

        if (gPageManager->havePlaySound() && sysSound.getSystemSoundState())
            gPageManager->getCallPlaySound()(sysSound.getTouchFeedbackSound());
    }

#ifdef _SCALE_SKIA_
    // To be able to test a button for a transparent pixel, we must scale
    // the coordinates because the test is made on the last drawn image and
    // this image is scaled (if scaling is activated).
    if (TConfig::getScale() && gPageManager && gPageManager->getScaleFactor() != 1.0)
    {
        double scaleFactor = gPageManager->getScaleFactor();
        sx = (int)((double)x * scaleFactor);
        sy = (int)((double)y * scaleFactor);
    }
#endif

    if (_buttonPress && mActInstance >= 0 && (size_t)mActInstance < sr.size() && cp == 0 && ch > 0)
        _buttonPress(ch, mHandle, pressed);

    if (type == GENERAL)
    {
        MSG_DEBUG("Button type: GENERAL; System button: " << (isSystem ? "YES" : "NO") << "; CH: " << cp << ":" << ch << "; AD: " << ap << ":" << ad);

        if (isSystem && ch == SYSTEM_ITEM_SOUNDSWITCH)   // Button sounds on/off
        {
            if (pressed)
            {
                MSG_TRACE("System button sounds are toggled ...");
                TConfig::setTemporary(false);
                bool sstate = TConfig::getSystemSoundState();

                if (sstate)
                    mActInstance = instance = 0;
                else
                    mActInstance = instance = 1;

                TConfig::saveSystemSoundState(!sstate);
                TConfig::saveSettings();
                mChanged = true;
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SETUPPAGE)  // Enter setup page
        {
            if (pressed)
            {
                if (gPageManager && gPageManager->haveSetupPage())
                    gPageManager->callSetupPage();
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SHUTDOWN)  // Shutdown program
        {
            if (pressed)
            {
                if (gPageManager && gPageManager->haveShutdown())
                    gPageManager->callShutdown();
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_VOLUMEUP)     // System volume up
        {
            TConfig::setTemporary(true);
            int vol = TConfig::getSystemVolume() + 10;

            if (vol > 100)
                vol = 100;

            if (pressed)
                TConfig::saveSystemVolume(vol);

            if (pressed)
                mActInstance = instance = 1;
            else
                mActInstance = instance = 0;

            mChanged = true;
            drawButton(mActInstance, true);

            if (pressed && gPageManager)
            {
                int channel = TConfig::getChannel();
                int system = TConfig::getSystem();

                amx::ANET_COMMAND cmd;
                cmd.MC = 0x000a;
                cmd.device1 = channel;
                cmd.port1 = 0;
                cmd.system = system;
                cmd.data.message_value.system = system;
                cmd.data.message_value.value = 9;   // System volume
                cmd.data.message_value.content.integer = vol;
                cmd.data.message_value.device = channel;
                cmd.data.message_value.port = 0;    // Must be the address port of button
                cmd.data.message_value.type = 0x20; // Unsigned int
                gPageManager->doCommand(cmd);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_VOLUMEDOWN)     // System volume down
        {
            TConfig::setTemporary(true);
            int vol = TConfig::getSystemVolume() - 10;

            if (vol < 0)
                vol = 0;

            if (pressed)
                TConfig::saveSystemVolume(vol);

            if (pressed)
                mActInstance = instance = 1;
            else
                mActInstance = instance = 0;

            mChanged = true;
            drawButton(mActInstance, true);

            if (pressed && gPageManager)
            {
                int channel = TConfig::getChannel();
                int system = TConfig::getSystem();

                amx::ANET_COMMAND cmd;
                cmd.MC = 0x000a;
                cmd.device1 = channel;
                cmd.port1 = 0;
                cmd.system = system;
                cmd.data.message_value.system = system;
                cmd.data.message_value.value = 9;   // System volume
                cmd.data.message_value.content.integer = vol;
                cmd.data.message_value.device = channel;
                cmd.data.message_value.port = 0;    // Must be the address port of button
                cmd.data.message_value.type = 0x20; // Unsigned int
                gPageManager->doCommand(cmd);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_VOLUMEMUTE)     // System mute
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool mute = TConfig::getMuteState();

                if (mute)
                    mActInstance = instance = 0;
                else
                    mActInstance = instance = 1;

                TConfig::setMuteState(!mute);

                if (gPageManager && gPageManager->getCallMuteSound())
                    gPageManager->getCallMuteSound()(!mute);

                mChanged = true;
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_BTSAVESETTINGS)     // System button OK: Save settings
        {
            if (pressed)
            {
                mActInstance = instance = 1;
                TConfig::setTemporary(true);
                TConfig::saveSettings();
                drawButton(mActInstance, true);

                if (gPageManager && gPageManager->getSettings() != gPageManager->getSystemSettings())
                    gPageManager->hideSetup();
                else if (gPageManager && gPageManager->getDisplayMessage())
                    gPageManager->getDisplayMessage()("Settings were saved!", "Info");
                else
                    MSG_INFO("Settings were saved.");
            }
            else
            {
                mActInstance = instance = 0;
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_BTCANCELSETTINGS)     // System button Cancel: Cancel settings changes
        {
            if (pressed)
            {
                mActInstance = instance = 1;
                TConfig::reset();
                drawButton(mActInstance, true);

                if (gPageManager && gPageManager->getSettings() != gPageManager->getSystemSettings())
                    gPageManager->hideSetup();
            }
            else
            {
                mActInstance = instance = 0;
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SIPENABLE)     // SIP: enabled/disabled
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getSIPstatus();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::setSIPstatus(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGINFO)    // Debug info
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                uint ll = TConfig::getLogLevelBits();
                bool st = (ll & HLOG_INFO) ? true : false;
                mActInstance = instance = (st ? 0 : 1);
                ll = (st ? (ll &= RLOG_INFO) : (ll |= HLOG_INFO));
                mChanged = true;
                TConfig::saveLogLevel(ll);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGWARNING)    // Debug warning
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                uint ll = TConfig::getLogLevelBits();
                bool st = (ll & HLOG_WARNING) ? true : false;
                mActInstance = instance = (st ? 0 : 1);
                ll = (st ? (ll &= RLOG_WARNING) : (ll |= HLOG_WARNING));
                mChanged = true;
                TConfig::saveLogLevel(ll);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGERROR)    // Debug error
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                uint ll = TConfig::getLogLevelBits();
                bool st = (ll & HLOG_ERROR) ? true : false;
                mActInstance = instance = (st ? 0 : 1);
                ll = (st ? (ll &= RLOG_ERROR) : (ll |= HLOG_ERROR));
                mChanged = true;
                TConfig::saveLogLevel(ll);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGTRACE)    // Debug trace
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                uint ll = TConfig::getLogLevelBits();
                bool st = (ll & HLOG_TRACE) ? true : false;
                mActInstance = instance = (st ? 0 : 1);
                ll = (st ? (ll &= RLOG_TRACE) : (ll |= HLOG_TRACE));
                mChanged = true;
                TConfig::saveLogLevel(ll);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGDEBUG)    // Debug debug
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                uint ll = TConfig::getLogLevelBits();
                bool st = (ll & HLOG_DEBUG) ? true : false;
                mActInstance = instance = (st ? 0 : 1);
                ll = (st ? (ll &= RLOG_DEBUG) : (ll |= HLOG_DEBUG));
                mChanged = true;
                TConfig::saveLogLevel(ll);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGPROTOCOL)    // Debug protocol
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                uint ll = TConfig::getLogLevelBits();
                bool st = (ll & HLOG_PROTOCOL) == HLOG_PROTOCOL ? true : false;
                mActInstance = instance = (st ? 0 : 1);
                ll = (st ? (ll &= RLOG_PROTOCOL) : (ll |= HLOG_PROTOCOL));
                mChanged = true;
                TConfig::saveLogLevel(ll);
                drawButton(mActInstance, true);

                if (gPageManager)
                    gPageManager->updateActualPage();
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGALL)    // Debug all
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                uint ll = TConfig::getLogLevelBits();
                bool st = (ll & HLOG_ALL) == HLOG_ALL ? true : false;
                mActInstance = instance = (st ? 0 : 1);
                ll = (st ? (ll &= RLOG_ALL) : (ll |= HLOG_ALL));
                mChanged = true;
                TConfig::saveLogLevel(ll);
                drawButton(mActInstance, true);

                if (gPageManager)
                    gPageManager->updateActualPage();
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGPROFILE)    // Log profiling
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getProfiling();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::saveProfiling(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_DEBUGLONG)    // Log long format
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::isLongFormat();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::saveFormat(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_LOGRESET)    // Log reset path
        {
            if (pressed)
            {
                char *HOME = getenv("HOME");
                string logFile = TConfig::getLogFile();

                if (HOME)
                {
                    logFile = HOME;
                    logFile += "/tpanel/tpanel.log";
                }

                ulong handle = (SYSTEM_PAGE_LOGGING << 16) | SYSTEM_PAGE_LOG_TXLOGFILE;
                TConfig::setTemporary(true);
                TConfig::saveLogFile(logFile);
                MSG_DEBUG("Setting text \"" << logFile << "\" to button " << handleToString(handle));

                if (gPageManager)
                    gPageManager->setTextToButton(handle, logFile, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_LOGFILEOPEN) // Log file dialog
        {
            if (pressed && gPageManager && gPageManager->getFileDialogFunction())
            {
                TConfig::setTemporary(true);
                ulong handle = (SYSTEM_PAGE_LOGGING << 16) | SYSTEM_PAGE_LOG_TXLOGFILE;
                string currFile = TConfig::getLogFile();
                gPageManager->getFileDialogFunction()(handle, currFile, "*.log *.txt", "log");
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_FTPDOWNLOAD)    // FTP download surface button
        {
            if (pressed)
            {
                TConfig::setTemporary(false);
                string surfaceOld = TConfig::getFtpSurface();
                TConfig::setTemporary(true);
                string surfaceNew = TConfig::getFtpSurface();

                MSG_DEBUG("Surface difference: Old: " << surfaceOld << ", New: " << surfaceNew);

                if (gPageManager && gPageManager->getDownloadSurface())
                {
                    size_t size = gPageManager->getFtpSurfaceSize(surfaceNew);
                    gPageManager->getDownloadSurface()(surfaceNew, size);
                }
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_FTPPASSIVE)    // FTP passive mode
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getFtpPassive();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::saveFtpPassive(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SOUNDPLAYSYSSOUND)    // Play system sound
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                string sound = TConfig::getProjectPath() + "/__system/graphics/sounds/" + TConfig::getSystemSound();

                if (!sound.empty() && gPageManager && gPageManager->getCallPlaySound())
                    gPageManager->getCallPlaySound()(sound);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SOUNDPLAYBEEP)    // Play single beep
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                string sound = TConfig::getProjectPath() + "/__system/graphics/sounds/" + TConfig::getSingleBeepSound();

                if (!sound.empty() && gPageManager && gPageManager->getCallPlaySound())
                    gPageManager->getCallPlaySound()(sound);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SOUNDPLAYDBEEP)    // Play double beep
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                string sound = TConfig::getProjectPath() + "/__system/graphics/sounds/" + TConfig::getDoubleBeepSound();

                if (!sound.empty() && gPageManager && gPageManager->getCallPlaySound())
                    gPageManager->getCallPlaySound()(sound);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SOUNDPLAYTESTSOUND)    // Play test sound
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                string sound = TConfig::getProjectPath() + "/__system/graphics/sounds/audioTest.wav";

                if (gPageManager && gPageManager->getCallPlaySound())
                    gPageManager->getCallPlaySound()(sound);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SIPIPV4)    // SIP: IPv4
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getSIPnetworkIPv4();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::setSIPnetworkIPv4(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SIPIPV6)    // SIP: IPv6
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getSIPnetworkIPv6();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::setSIPnetworkIPv6(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_SIPIPHONE)    // SIP: internal phone
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getSIPiphone();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::setSIPiphone(!st);
                drawButton(mActInstance, true);
            }
        }
#ifdef __ANDROID__
        else if (isSystem && ch == SYSTEM_ITEM_VIEWSCALEFIT)    // Scale to fit
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getScale();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::saveScale(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_VIEWBANNER)    // Show banner (disabled)
        {
            if (sr[0].oo < 0)
            {
                sr[0].oo = 128;
                mChanged = true;
                mActInstance = 0;
                drawButton(mActInstance, true);
            }
        }
#else
        else if (isSystem && ch == SYSTEM_ITEM_VIEWSCALEFIT)    // Scale to fit (disabled)
        {
            if (sr[0].oo < 0)
            {
                sr[0].oo = 128;
                mChanged = true;
                mActInstance = 0;
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_VIEWBANNER)    // Show banner
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::showBanner();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::saveBanner(st);
                drawButton(mActInstance, true);
            }
        }
#endif
        else if (isSystem && ch == SYSTEM_ITEM_VIEWNOTOOLBAR)    // Suppress toolbar
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getToolbarSuppress();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::saveToolbarSuppress(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_VIEWTOOLBAR)    // Force toolbar
        {
            if (pressed)
            {
                TConfig::setTemporary(true);

                if (TConfig::getToolbarSuppress())
                {
                    if (sr[0].oo < 0)
                    {
                        sr[0].oo = 128;
                        mChanged = true;
                        mActInstance = 0;
                        drawButton(mActInstance, true);
                    }
                }
                else
                {
                    if (sr[0].oo >= 0)
                        sr[0].oo = -1;

                    bool st = TConfig::getToolbarForce();
                    mActInstance = instance = (st ? 0 : 1);
                    mChanged = true;
                    TConfig::saveToolbarForce(!st);
                    drawButton(mActInstance, true);
                }
            }
        }
        else if (isSystem && ch == SYSTEM_ITEM_VIEWROTATE)    // Lock rotation
        {
            if (pressed)
            {
                TConfig::setTemporary(true);
                bool st = TConfig::getRotationFixed();
                mActInstance = instance = (st ? 0 : 1);
                mChanged = true;
                TConfig::setRotationFixed(!st);
                drawButton(mActInstance, true);
            }
        }
        else if (fb == FB_MOMENTARY)
        {
            if (pressed)
                instance = 1;
            else
                instance = 0;

            MSG_DEBUG("Flavor FB_MOMENTARY, instance=" << instance);
            mActInstance = instance;
            mChanged = true;

            if (pushFunc.empty() || (!pushFunc.empty() && instance == 0))
                drawButton(instance);

            // If there is nothing in "hs", then it depends on the pixel of the
            // layer. Only if the pixel the coordinates point to are not
            // transparent, the button takes the click.
            if (hs.empty() && isPixelTransparent(sx, sy))
                return false;

            // Play sound, if one is defined
            if (gPageManager)
            {
                if (pressed && gPageManager->havePlaySound() && !sr[0].sd.empty() && strCaseCompare(sr[0].sd, "None") != 0)
                    gPageManager->getCallPlaySound()(TConfig::getProjectPath() + "/sounds/" + sr[0].sd);
                else if (!pressed && gPageManager->havePlaySound() && !sr[1].sd.empty() && strCaseCompare(sr[1].sd, "None") != 0)
                    gPageManager->getCallPlaySound()(TConfig::getProjectPath() + "/sounds/" + sr[1].sd);
            }

            if (pushFunc.empty())   // Don't draw the button if it has a push function defined
                showLastButton();
            else
                mActInstance = 0;
        }
        else if (fb == FB_CHANNEL || fb == FB_NONE)
        {
            if (pressed)
                instance = 1;
            else
                instance = 0;

            MSG_DEBUG("Flavor FB_CHANNEL, instance=" << instance);
            // If there is nothing in "hs", then it depends on the pixel of the
            // layer. Only if the pixel the coordinates point to are not
            // transparent, the button takes the click.
            if (hs.empty() && isPixelTransparent(sx, sy))
                return false;
        }
        else if (fb == FB_INV_CHANNEL)
        {
            if (pressed)
                instance = 0;
            else
                instance = 1;

            MSG_DEBUG("Flavor FB_INV_CHANNEL, instance=" << instance);
            // If there is nothing in "hs", then it depends on the pixel of the
            // layer. Only if the pixel the coordinates point to are not
            // transparent, the button takes the click.
            if (hs.empty() && isPixelTransparent(sx, sy))
                return false;

            // Play sound, if one is defined
            if (gPageManager)
            {
                if (pressed && gPageManager->havePlaySound() && !sr[1].sd.empty() && strCaseCompare(sr[0].sd, "None") != 0)
                    gPageManager->getCallPlaySound()(TConfig::getProjectPath() + "/sounds/" + sr[1].sd);
                else if (!pressed && gPageManager->havePlaySound() && !sr[0].sd.empty() && strCaseCompare(sr[1].sd, "None") != 0)
                    gPageManager->getCallPlaySound()(TConfig::getProjectPath() + "/sounds/" + sr[0].sd);
            }
        }
        else if (fb == FB_ALWAYS_ON)
        {
            int oldInst = mActInstance;
            instance = 1;
            mActInstance = 1;
            MSG_DEBUG("Flavor FB_ALWAYS_ON, instance=" << instance);

            if (oldInst != mActInstance)        // This should never become true!
            {
                mChanged = true;
                drawButton(instance, false);
            }

            // If there is nothing in "hs", then it depends on the pixel of the
            // layer. Only if the pixel the coordinates point to are not
            // transparent, the button takes the click.
            if (hs.empty() && isPixelTransparent(sx, sy))
                return false;

            // Play sound, if one is defined
            if (pressed && gPageManager && gPageManager->havePlaySound() && !sr[1].sd.empty() && strCaseCompare(sr[1].sd, "None") != 0)
                gPageManager->getCallPlaySound()(TConfig::getProjectPath() + "/sounds/" + sr[1].sd);
        }

        if ((cp && ch) || !op.empty())
        {
            scmd.device = TConfig::getChannel();
            scmd.port = cp;
            scmd.channel = ch;

            if (op.empty())
            {
                if (instance)
                    scmd.MC = 0x0084;
                else
                    scmd.MC = 0x0085;
            }
            else
            {
                scmd.MC = 0x008b;
                scmd.msg = op;
            }

            MSG_DEBUG("Button " << bi << ", " << na << " with handle " << handleToString(mHandle));
            MSG_DEBUG("Sending to device <" << scmd.device << ":" << scmd.port << ":0> channel " << scmd.channel << " value 0x" << std::setw(2) << std::setfill('0') << std::hex << scmd.MC << " (" << (pressed?"PUSH":"RELEASE") << ")");

            if (gAmxNet)
            {
                if (scmd.MC != 0x008b || (pressed && scmd.MC == 0x008b))
                    gAmxNet->sendCommand(scmd);
            }
            else
                MSG_WARNING("Missing global class TAmxNet. Can't send a message!");
        }
    }
    else if (type == MULTISTATE_GENERAL)
    {
        // Play sound, if one is defined
        if (pressed && gPageManager && gPageManager->havePlaySound() && !sr[mActInstance].sd.empty() && strCaseCompare(sr[mActInstance].sd, "None") != 0)
            gPageManager->getCallPlaySound()(TConfig::getProjectPath() + "/sounds/" + sr[mActInstance].sd);

        if ((cp && ch) || !op.empty())
        {
            scmd.device = TConfig::getChannel();
            scmd.port = cp;
            scmd.channel = ch;

            if (op.empty())
            {
                if (pressed || fb == FB_ALWAYS_ON)
                    scmd.MC = 0x0084;
                else
                    scmd.MC = 0x0085;
            }
            else
            {
                scmd.MC = 0x008b;
                scmd.msg = op;
            }

            MSG_DEBUG("Button " << bi << ", " << na << " with handle " << handleToString(mHandle));
            MSG_DEBUG("Sending to device <" << scmd.device << ":" << scmd.port << ":0> channel " << scmd.channel << " value 0x" << std::setw(2) << std::setfill('0') << std::hex << scmd.MC << " (" << (pressed?"PUSH":"RELEASE") << ")");

            if (gAmxNet)
            {
                if (scmd.MC != 0x008b || (pressed && scmd.MC == 0x008b))
                    gAmxNet->sendCommand(scmd);
            }
            else
                MSG_WARNING("Missing global class TAmxNet. Can't send a message!");
        }
    }
    else if (type == BARGRAPH && lf.find("active") != string::npos)
    {
        // Find the click position
        int level = 0;

        if (dr.compare("horizontal") == 0)
        {
            level = (ri ? (wt - x) : x);
            level = (int)((double)(rh - rl) / (double)wt * (double)level);
        }
        else
        {
            level = (ri ? y : (ht - y));
            level = (int)((double)(rh - rl) / (double)ht * (double)level);
        }

        // Draw the bargraph
        if (!drawBargraph(mActInstance, level, visible))
            return false;

        // Handle click
        if (isSystem && lv == 9)    // System volume button
        {
            TConfig::saveSystemVolume(level);
            TConfig::saveSettings();
        }
        else if ((cp && ch) || !op.empty())
        {
            scmd.device = TConfig::getChannel();
            scmd.port = cp;
            scmd.channel = ch;

            if (op.empty())
            {
                if (pressed || fb == FB_ALWAYS_ON)
                    scmd.MC = 0x0084;
                else
                    scmd.MC = 0x0085;
            }
            else
            {
                scmd.MC = 0x008b;
                scmd.msg = op;
            }

            if (gAmxNet)
            {
                if (scmd.MC != 0x008b || (pressed && scmd.MC == 0x008b))
                    gAmxNet->sendCommand(scmd);
            }
            else
                MSG_WARNING("Missing global class TAmxNet. Can't send a message!");
        }

        // Send the level
        if (lp && lv && gPageManager && gPageManager->getLevelSendState())
        {
            scmd.device = TConfig::getChannel();
            scmd.port = lp;
            scmd.channel = lv;
            scmd.level = lv;
            scmd.value = level;
            scmd.MC = 0x008a;

            if (gAmxNet)
                gAmxNet->sendCommand(scmd);
        }
    }
    else if (type == TEXT_INPUT)
    {
        MSG_DEBUG("Text area detected. Switching on keyboard");
        // Drawing background graphic (visible part of area)
        drawTextArea(mActInstance);
    }

    /* FIXME: Move the following to class TPageManager!
     *        To do that, the preconditions are to be implemented. It must be
     *        possible to find the button and get access to the credentials
     *        of it.
     */
    if (!pushFunc.empty() && pressed)
    {
        MSG_DEBUG("Executing a push function ...");
        vector<PUSH_FUNC_T>::iterator iter;

        for (iter = pushFunc.begin(); iter != pushFunc.end(); ++iter)
        {
            MSG_DEBUG("Testing for function \"" << iter->pfType << "\"");

            if (fb == FB_MOMENTARY || fb == FB_NONE)
                mActInstance = 0;
            else if (fb == FB_ALWAYS_ON || fb == FB_INV_CHANNEL)
                mActInstance = 1;

            if (strCaseCompare(iter->pfType, "SSHOW") == 0)            // show popup
            {
                if (gPageManager)
                    gPageManager->showSubPage(iter->pfName);
            }
            else if (strCaseCompare(iter->pfType, "SHIDE") == 0)       // hide popup
            {
                if (gPageManager)
                    gPageManager->hideSubPage(iter->pfName);
            }
            else if (strCaseCompare(iter->pfType, "SCGROUP") == 0)     // hide group
            {
                if (gPageManager)
                    gPageManager->closeGroup(iter->pfName);
            }
            else if (strCaseCompare(iter->pfType, "SCPAGE") == 0)      // flip to page
            {
                if (gPageManager && !iter->pfName.empty())
                    gPageManager->setPage(iter->pfName);
            }
            else if (strCaseCompare(iter->pfType, "STAN") == 0)        // Flip to standard page
            {
                if (gPageManager)
                {
                    if (!iter->pfName.empty())
                        gPageManager->setPage(iter->pfName);
                    else
                    {
                        TPage *page = gPageManager->getActualPage();

                        if (!page)
                        {
                            MSG_DEBUG("Internal error: No actual page found!");
                            return false;
                        }

                        TSettings *settings = gPageManager->getSettings();

                        if (settings && settings->getPowerUpPage().compare(page->getName()) != 0)
                            gPageManager->setPage(settings->getPowerUpPage());
                    }
                }
            }
            else if (strCaseCompare(iter->pfType, "FORGET") == 0)      // Flip to page and forget
            {
                if (gPageManager && !iter->pfName.empty())
                        gPageManager->setPage(iter->pfName, true);
            }
            else if (strCaseCompare(iter->pfType, "PREV") == 0)        // Flip to previous page
            {
                if (gPageManager)
                {
                    int old = gPageManager->getPreviousPageNumber();

                    if (old > 0)
                        gPageManager->setPage(old);
                }
            }
            else if (strCaseCompare(iter->pfType, "STOGGLE") == 0)     // Toggle popup state
            {
                if (!iter->pfName.empty() && gPageManager)
                {
                    TSubPage *page = gPageManager->getSubPage(iter->pfName);

                    if (!page)      // Is the page not in cache?
                    {               // No, then load it
                        gPageManager->showSubPage(iter->pfName);
                        return true;
                    }

                    if (page->isVisible())
                        gPageManager->hideSubPage(iter->pfName);
                    else
                        gPageManager->showSubPage(iter->pfName);
                }
            }
            else if (strCaseCompare(iter->pfType, "SCPANEL") == 0)   // Hide all popups
            {
                if (gPageManager)
                {
                    TSubPage *page = gPageManager->getFirstSubPage();

                    while (page)
                    {
                        page->drop();
                        page = gPageManager->getNextSubPage();
                    }
                }
            }
            else
            {
                MSG_WARNING("Unknown page flip command " << iter->pfType);
            }
        }
    }

    if (!cm.empty() && co == 0 && pressed)      // Feed command to ourself?
    {                                           // Yes, then feed it into command queue.
        MSG_DEBUG("Button has a self feed command");

        int channel = TConfig::getChannel();
        int system = TConfig::getSystem();

        if (gPageManager)
        {
            amx::ANET_COMMAND cmd;
            cmd.MC = 0x000c;
            cmd.device1 = channel;
            cmd.port1 = 1;
            cmd.system = system;
            cmd.data.message_string.device = channel;
            cmd.data.message_string.port = 1;   // Must be 1
            cmd.data.message_string.system = system;
            cmd.data.message_string.type = 1;   // 8 bit char string

            vector<string>::iterator iter;

            for (iter = cm.begin(); iter != cm.end(); ++iter)
            {
                cmd.data.message_string.length = iter->length();
                memset(&cmd.data.message_string.content, 0, sizeof(cmd.data.message_string.content));
                strncpy((char *)&cmd.data.message_string.content, iter->c_str(), sizeof(cmd.data.message_string.content)-1);
                MSG_DEBUG("Executing system command: " << *iter);
                gPageManager->doCommand(cmd);
            }
        }
    }
    else if (!cm.empty() && pressed)
    {
        MSG_DEBUG("Button sends a command on port " << co);

        if (gPageManager)
        {
            vector<string>::iterator iter;

            for (iter = cm.begin(); iter != cm.end(); ++iter)
                gPageManager->sendCommandString(co, *iter);
        }
    }

    return true;
}

/**
 * Based on the pixels in the \a basePix, the function decides whether to return
 * the value of \a col1 or \a col2. A red pixel returns the color \a col1 and
 * a green pixel returns the color \a col2. If there is no red and no green
 * pixel, a transparent pixel is returned.
 *
 * @param basePix
 * This is a pixel from a mask containing red and/or green pixels.
 *
 * @param maskPix
 * This is a pixel from a mask containing more or less tranparent pixels. If
 * the alpha channel of \a basePix is 0 (transparent) this pixel is returned.
 *
 * @param col1
 * The first color.
 *
 * @param col2
 * The second color.
 *
 * @return
 * An array containing the color for one pixel.
 */
SkColor TButton::baseColor(SkColor basePix, SkColor maskPix, SkColor col1, SkColor col2)
{
    uint alpha = SkColorGetA(basePix);
    uint green = SkColorGetG(basePix);
    uint red = 0;
//#if (defined(__x86_64__) || defined(_M_X64)) && !defined(__MACH__)
    if (isBigEndian())
        red = SkColorGetB(basePix);
    else
        red = SkColorGetR(basePix);
//#endif

    if (alpha == 0)
        return maskPix;

    if (red && green)
    {
        if (red < green)
            return col2;

        return col1;
    }

    if (red)
        return col1;

    if (green)
        return col2;

    return SK_ColorTRANSPARENT; // transparent pixel
}

TEXT_EFFECT TButton::textEffect(const std::string& effect)
{
    DECL_TRACER("TButton::textEffect(const std::string& effect)");

    if (effect == "Outline-S")
        return EFFECT_OUTLINE_S;
    else if (effect == "Outline-M")
        return EFFECT_OUTLINE_M;
    else if (effect == "Outline-L")
        return EFFECT_OUTLINE_L;
    else if (effect == "Outline-X")
        return EFFECT_OUTLINE_X;
    else if (effect == "Glow-S")
        return EFFECT_GLOW_S;
    else if (effect == "Glow-M")
        return EFFECT_GLOW_M;
    else if (effect == "Glow-L")
        return EFFECT_GLOW_L;
    else if (effect == "Glow-X")
        return EFFECT_GLOW_X;
    else if (effect == "Soft Drop Shadow 1")
        return EFFECT_SOFT_DROP_SHADOW_1;
    else if (effect == "Soft Drop Shadow 2")
        return EFFECT_SOFT_DROP_SHADOW_2;
    else if (effect == "Soft Drop Shadow 3")
        return EFFECT_SOFT_DROP_SHADOW_3;
    else if (effect == "Soft Drop Shadow 4")
        return EFFECT_SOFT_DROP_SHADOW_4;
    else if (effect == "Soft Drop Shadow 5")
        return EFFECT_SOFT_DROP_SHADOW_5;
    else if (effect == "Soft Drop Shadow 6")
        return EFFECT_SOFT_DROP_SHADOW_6;
    else if (effect == "Soft Drop Shadow 7")
        return EFFECT_SOFT_DROP_SHADOW_7;
    else if (effect == "Soft Drop Shadow 8")
        return EFFECT_SOFT_DROP_SHADOW_8;
    else if (effect == "Medium Drop Shadow 1")
        return EFFECT_MEDIUM_DROP_SHADOW_1;
    else if (effect == "Medium Drop Shadow 2")
        return EFFECT_MEDIUM_DROP_SHADOW_2;
    else if (effect == "Medium Drop Shadow 3")
        return EFFECT_MEDIUM_DROP_SHADOW_3;
    else if (effect == "Medium Drop Shadow 4")
        return EFFECT_MEDIUM_DROP_SHADOW_4;
    else if (effect == "Medium Drop Shadow 5")
        return EFFECT_MEDIUM_DROP_SHADOW_5;
    else if (effect == "Medium Drop Shadow 6")
        return EFFECT_MEDIUM_DROP_SHADOW_6;
    else if (effect == "Medium Drop Shadow 7")
        return EFFECT_MEDIUM_DROP_SHADOW_7;
    else if (effect == "Medium Drop Shadow 8")
        return EFFECT_MEDIUM_DROP_SHADOW_8;
    else if (effect == "Hard Drop Shadow 1")
        return EFFECT_HARD_DROP_SHADOW_1;
    else if (effect == "Hard Drop Shadow 2")
        return EFFECT_HARD_DROP_SHADOW_2;
    else if (effect == "Hard Drop Shadow 3")
        return EFFECT_HARD_DROP_SHADOW_3;
    else if (effect == "Hard Drop Shadow 4")
        return EFFECT_HARD_DROP_SHADOW_4;
    else if (effect == "Hard Drop Shadow 5")
        return EFFECT_HARD_DROP_SHADOW_5;
    else if (effect == "Hard Drop Shadow 6")
        return EFFECT_HARD_DROP_SHADOW_6;
    else if (effect == "Hard Drop Shadow 7")
        return EFFECT_HARD_DROP_SHADOW_7;
    else if (effect == "Hard Drop Shadow 8")
        return EFFECT_HARD_DROP_SHADOW_8;
    else if (effect == "Soft Drop Shadow 1 with outline")
        return EFFECT_SOFT_DROP_SHADOW_1_WITH_OUTLINE;
    else if (effect == "Soft Drop Shadow 2 with outline")
        return EFFECT_SOFT_DROP_SHADOW_2_WITH_OUTLINE;
    else if (effect == "Soft Drop Shadow 3 with outline")
        return EFFECT_SOFT_DROP_SHADOW_3_WITH_OUTLINE;
    else if (effect == "Soft Drop Shadow 4 with outline")
        return EFFECT_SOFT_DROP_SHADOW_4_WITH_OUTLINE;
    else if (effect == "Soft Drop Shadow 5 with outline")
        return EFFECT_SOFT_DROP_SHADOW_5_WITH_OUTLINE;
    else if (effect == "Soft Drop Shadow 6 with outline")
        return EFFECT_SOFT_DROP_SHADOW_6_WITH_OUTLINE;
    else if (effect == "Soft Drop Shadow 7 with outline")
        return EFFECT_SOFT_DROP_SHADOW_7_WITH_OUTLINE;
    else if (effect == "Soft Drop Shadow 8 with outline")
        return EFFECT_SOFT_DROP_SHADOW_8_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 1 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_1_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 2 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_2_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 3 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_3_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 4 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_4_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 5 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_5_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 6 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_6_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 7 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_7_WITH_OUTLINE;
    else if (effect == "Medium Drop Shadow 8 with outline")
        return EFFECT_MEDIUM_DROP_SHADOW_8_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 1 with outline")
        return EFFECT_HARD_DROP_SHADOW_1_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 2 with outline")
        return EFFECT_HARD_DROP_SHADOW_2_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 3 with outline")
        return EFFECT_HARD_DROP_SHADOW_3_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 4 with outline")
        return EFFECT_HARD_DROP_SHADOW_4_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 5 with outline")
        return EFFECT_HARD_DROP_SHADOW_5_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 6 with outline")
        return EFFECT_HARD_DROP_SHADOW_6_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 7 with outline")
        return EFFECT_HARD_DROP_SHADOW_7_WITH_OUTLINE;
    else if (effect == "Hard Drop Shadow 8 with outline")
        return EFFECT_HARD_DROP_SHADOW_8_WITH_OUTLINE;

    return EFFECT_NONE;
}

bool TButton::isSystemButton()
{
    DECL_TRACER("TButton::isSystemButton()");

    if (type == MULTISTATE_BARGRAPH && lp == 0 && TSystem::isSystemButton(lv))
        return true;
    else if (type == BARGRAPH && lp == 0 && TSystem::isSystemButton(lv))
        return true;
    else if (type == LISTBOX && ap == 0 && ad > 0 && ti >= SYSTEM_PAGE_START)
        return true;
    else if (ap == 0 && TSystem::isSystemButton(ad))
        return true;
    else if (cp == 0 && TSystem::isSystemButton(ch))
        return true;

    return false;
}

THR_REFRESH_t *TButton::_addResource(TImageRefresh* refr, ulong handle, ulong parent, int bi)
{
    DECL_TRACER("TButton::_addResource(TImageRefresh* refr, ulong handle, ulong parent, int bi)");

    THR_REFRESH_t *p = mThrRefresh;
    THR_REFRESH_t *r, *last = p;

    if (!refr || !handle || !parent || bi <= 0)
    {
        MSG_ERROR("Invalid parameter!");
        return nullptr;
    }

    r = new THR_REFRESH_t;
    r->mImageRefresh = refr;
    r->handle = handle;
    r->parent = parent;
    r->bi = bi;
    r->next = nullptr;

    // If the chain is empty, add the new item;
    if (!mThrRefresh)
        mThrRefresh = r;
    else    // Find the end and append the item
    {
        while (p)
        {
            last = p;

            if (p->handle == handle && p->parent == parent && p->bi == bi)
            {
                MSG_WARNING("Duplicate button found! Didn't add it again.");
                delete r;
                return p;
            }

            p = p->next;
        }

        last->next = r;
    }

    MSG_DEBUG("New dynamic button added.");
    return r;
}

THR_REFRESH_t *TButton::_findResource(ulong handle, ulong parent, int bi)
{
    DECL_TRACER("TButton::_findResource(ulong handle, ulong parent, int bi)");

    THR_REFRESH_t *p = mThrRefresh;

    while (p)
    {
        if (p->handle == handle && p->parent == parent && p->bi == bi)
            return p;

        p = p->next;
    }

    return nullptr;
}

void TButton::addToBitmapCache(BITMAP_CACHE& bc)
{
    DECL_TRACER("TButton::addToBitmapCache(BITMAP_CACHE& bc)");

    mutex_bmCache.lock();

    if (nBitmapCache.size() == 0)
    {
        nBitmapCache.push_back(bc);
        mutex_bmCache.unlock();
        return;
    }

    vector<BITMAP_CACHE>::iterator iter;

    for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
    {
        if (iter->handle == bc.handle && iter->parent == bc.parent && iter->bi == bc.bi)
        {
            nBitmapCache.erase(iter);
            nBitmapCache.push_back(bc);
            mutex_bmCache.unlock();
            return;
        }
    }

    nBitmapCache.push_back(bc);
    mutex_bmCache.unlock();
}

BITMAP_CACHE& TButton::getBCentryByHandle(ulong handle, ulong parent)
{
    DECL_TRACER("TButton::getBCentryByHandle(ulong handle, ulong parent)");

    mutex_bmCache.lock();

    if (nBitmapCache.size() == 0)
    {
        mutex_bmCache.unlock();
        return mBCDummy;
    }

    vector<BITMAP_CACHE>::iterator iter;

    for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
    {
        if (iter->handle == handle && iter->parent == parent)
        {
            mutex_bmCache.unlock();
            return *iter;
        }
    }

    mutex_bmCache.unlock();
    return mBCDummy;
}

BITMAP_CACHE& TButton::getBCentryByBI(int bIdx)
{
    DECL_TRACER("TButton::getBCentryByBI(int bIdx)");

    mutex_bmCache.lock();

    if (nBitmapCache.size() == 0)
    {
        mutex_bmCache.unlock();
        return mBCDummy;
    }

    vector<BITMAP_CACHE>::iterator iter;

    for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
    {
        if (iter->bi == bIdx)
        {
            mutex_bmCache.unlock();
            return *iter;
        }
    }

    mutex_bmCache.unlock();
    return mBCDummy;
}

void TButton::removeBCentry(std::vector<BITMAP_CACHE>::iterator *elem)
{
    DECL_TRACER("TButton::removeBCentry(std::vector<BITMAP_CACHE>::iterator *elem)");

    if (nBitmapCache.size() == 0 || !elem)
        return;

    mutex_bmCache.lock();
    vector<BITMAP_CACHE>::iterator iter;

    for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
    {
        if (iter == *elem)
        {
            nBitmapCache.erase(iter);
            mutex_bmCache.unlock();
            return;
        }
    }

    mutex_bmCache.unlock();
}

void TButton::setReady(ulong handle)
{
    DECL_TRACER("TButton::setReady(ulong handle)");

    if (nBitmapCache.size() == 0)
        return;

    mutex_bmCache.lock();
    vector<BITMAP_CACHE>::iterator iter;

    for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            iter->ready = true;
            mutex_bmCache.unlock();
            return;
        }
    }

    mutex_bmCache.unlock();
}

void TButton::setInvalid(ulong handle)
{
    DECL_TRACER("TButton::setInvalid(ulong handle)");

    if (nBitmapCache.size() == 0)
        return;

    mutex_bmCache.lock();
    vector<BITMAP_CACHE>::iterator iter;

    for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            nBitmapCache.erase(iter);
            mutex_bmCache.unlock();
            return;
        }
    }

    mutex_bmCache.unlock();
}

void TButton::setBCBitmap(ulong handle, SkBitmap& bm)
{
    DECL_TRACER("TButton::setBCBitmap(ulong handle, SkBitmap& bm)");

    if (nBitmapCache.size() == 0)
        return;

    mutex_bmCache.lock();
    vector<BITMAP_CACHE>::iterator iter;

    for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            iter->bitmap = bm;
            mutex_bmCache.unlock();
            return;
        }
    }

    mutex_bmCache.unlock();
}

void TButton::showBitmapCache()
{
    DECL_TRACER("TButton::showBitmapCache()");

    mutex_bmCache.lock();
    vector<BITMAP_CACHE>::iterator iter;
    bool found;

    while (nBitmapCache.size() > 0)
    {
        found = false;

        for (iter = nBitmapCache.begin(); iter != nBitmapCache.end(); ++iter)
        {
            if (iter->ready)
            {
                if (_displayButton)
                {
                    TBitmap image((unsigned char *)iter->bitmap.getPixels(), iter->bitmap.info().width(), iter->bitmap.info().height());
                    _displayButton(iter->handle, iter->parent, image, iter->width, iter->height, iter->left, iter->top, isPassThrough());
                    mChanged = false;
                }

                nBitmapCache.erase(iter);
                found = true;
                break;
            }
        }

        if (!found)
            break;
    }

    mutex_bmCache.unlock();
}

uint32_t TButton::pixelMix(uint32_t s, uint32_t d, uint32_t a, PMIX mix)
{
    DECL_TRACER("TButton::pixelMultiply(uint32_t s, uint32_t d)");

    uint32_t r = 0;

    switch(mix)
    {
        case PMIX_SRC:          r = s; break;                                                   // SRC
        case PMIX_DST:          r = d; break;                                                   // DST
        case PMIX_MULTIPLY:     r = s * (255 - (d * a)) + d * (255 - (s * a)) + s * d; break;   // Multiply
        case PMIX_PLUS:         r = std::min(s + d, (uint32_t)255); break;                      // plus
        case PMIX_XOR:          r = s * (255 - (d * a)) + d * (255 - (s * a)); break;           // XOr
        case PMIX_DSTTOP:       r = d * (s * a) + s * (255 - (d * a)); break;                   // DstATop
        case PMIX_SRCTOP:       r = s * (d * a) + d * (255 - (s * a)); break;                   // SrcATop
        case PMIX_SRCOVER:      r = s + (255 - (s * a)) * d; break;                             // SrcOver
        case PMIX_SCREEN:       r = s + d - s * d; break;                                       // Screen
    }

    return r & 0x00ff;
}

bool TButton::isPassThrough()
{
    DECL_TRACER("TButton::isPassThrough()");

    if (hs.empty())
        return false;

    if (strCaseCompare(hs, "passThru") == 0)
        return true;

    return false;
}

bool TButton::setListSource(const string &source, const vector<string>& configs)
{
    DECL_TRACER("TButton::setListSource(const string &source, const vector<string>& configs)");

    TUrl url;

    listSourceUser.clear();
    listSourcePass.clear();
    listSourceCsv = false;
    listSourceHasHeader = false;

    if (configs.size() > 0)
    {
        vector<string>::const_iterator iter;

        for (iter = configs.begin(); iter != configs.end(); ++iter)
        {
            size_t pos;

            if ((pos = iter->find("user=")) != string::npos)
                listSourceUser = iter->substr(pos + 5);
            else if ((pos = iter->find("pass=")) != string::npos)
                listSourcePass = iter->substr(pos + 5);
            else if (iter->find("csv=") != string::npos)
            {
                string str = *iter;
                string low = toLower(str);

                if (low.find("true") != string::npos || low.find("1") != string::npos)
                    listSourceCsv = true;
            }
            else if (iter->find("has_header=") != string::npos)
            {
                string str = *iter;
                string low = toLower(str);

                if (low.find("true") != string::npos || low.find("1") != string::npos)
                    listSourceHasHeader = true;
            }
        }
    }

    if (!url.setUrl(source))    // Dynamic source?
    {
        size_t idx = 0;

        if (!gPrjResources)
            return false;

        if ((idx = gPrjResources->getResourceIndex("image")) == TPrjResources::npos)
        {
            MSG_ERROR("There exists no image resource!");
            return false;
        }

        RESOURCE_T resource = gPrjResources->findResource(idx, source);

        if (resource.protocol.empty())
        {
            MSG_WARNING("Resource " << source << " not found!");
            return false;
        }

        listSource = resource.protocol + "://";

        if (!resource.user.empty() || !listSourceUser.empty())
        {
            listSource += ((listSourceUser.empty() == false) ? listSourceUser : resource.user);

            if ((!resource.password.empty() && !resource.encrypted) || !listSourcePass.empty())
                listSource += ":" + ((listSourcePass.empty() == false) ? listSourcePass : resource.password);

            listSource += "@";
        }

        listSource += resource.host;

        if (!resource.path.empty())
            listSource += "/" + resource.path;

        if (!resource.file.empty())
            listSource += "/" + resource.file;

        return true;
    }

    listSource = source;
    return true;
}

bool TButton::setListSourceFilter(const string& filter)
{
    DECL_TRACER("TButton::setListSourceFilter(const string& filter)");

    if (filter.empty())
        return false;

    listFilter = filter;
    MSG_DEBUG("listSourceFilter: " << listFilter);
    return true;
}

void TButton::setListViewColumns(int cols)
{
    DECL_TRACER("TButton::setListViewColumns(int cols)");

    if (cols <= 0)
        return;

    tc = cols;
}

void TButton::setListViewLayout(int layout)
{
    DECL_TRACER("TButton::setListViewLayout(int layout)");

    if (layout < 1 || layout > 6)
        return;

    listLayout = layout;
}

void TButton::setListViewComponent(int comp)
{
    DECL_TRACER("TButton::setListViewComponent(int comp)");

    if (comp < 0 || comp > 7)
        return;

    listComponent = comp;
}

void TButton::setListViewCellheight(int height, bool percent)
{
    DECL_TRACER("TButton::setListViewCellheight(int height, bool percent)");

    int minHeight = ht / tr;    // Total height / number of rows
    int maxHeight = (int)((double)ht / 100.0 * 95.0);

    if (!percent && (height < minHeight || height > maxHeight))
        return;

    if (percent)
    {
        int h = (int)((double)ht / 100.0 * (double)height);

        if (h >= minHeight && h <= maxHeight)
            tj = h;

        return;
    }

    tj = height;
}

void TButton::setListViewFilterHeight(int height, bool percent)
{
    DECL_TRACER("TButton::setListViewFilterHeight(int height, bool percent)");

    if (percent && (height < 5 || height > 25))
        return;

    if (!percent && height < 24)
        return;

    if (percent)
    {
        listViewColFilterHeight = (int)((double)ht / 100.0 * (double)height);
        return;
    }
    else
    {
        int maxHeight = (int)((double)ht / 100.0 * 25.0);

        if (height < maxHeight)
            listViewColFilterHeight = height;
    }
}

void TButton::setListViewP1(int p1)
{
    DECL_TRACER("TButton::setListViewP1(int p1)");

    if (p1 < 10 || p1 > 90)
        return;

    listViewP1 = p1;
}

void TButton::setListViewP2(int p2)
{
    DECL_TRACER("TButton::setListViewP2(int p2)");

    if (p2 < 10 || p2 > 90)
        return;

    listViewP2 = p2;
}

void TButton::listViewNavigate(const string &command, bool select)
{
    DECL_TRACER("TButton::listViewNavigate(const string &command, bool select)");

    string cmd = command;
    string upCmd = toUpper(cmd);

    if (upCmd != "T" && upCmd != "B" && upCmd != "D" && upCmd != "U" && !isNumeric(upCmd, true))
        return;

    // TODO: Add code to navigate a list
    MSG_WARNING("ListView navigation is not supported!" << " [" << upCmd << ", " << (select ? "TRUE" : "FALSE") << "]");
}

void TButton::listViewRefresh(int interval, bool force)
{
    DECL_TRACER("TButton::listViewRefresh(int interval, bool force)");

    Q_UNUSED(interval);
    Q_UNUSED(force);

    // TODO: Add code to load list data and display / refresh them
}

void TButton::listViewSortData(const vector<string> &columns, LIST_SORT order, const string &override)
{
    DECL_TRACER("TButton::listViewSortData(const vector<string> &columns, LIST_SORT order, const string &override)");

    Q_UNUSED(columns);
    Q_UNUSED(order);
    Q_UNUSED(override);

    // TODO: Insert code to sort the data in the list
}


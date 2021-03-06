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
#include <include/core/SkFilterQuality.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkImageEncoder.h>
#include <include/core/SkRRect.h>

#ifdef __ANDROID__
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroid>
#endif

#include "tbutton.h"
#include "thttpclient.h"
#include "terror.h"
#include "tconfig.h"
#include "tresources.h"
#include "ticons.h"
#include "tamxnet.h"
#include "tobject.h"
#include "tpagemanager.h"
#include "tsystemsound.h"
#include "timgcache.h"

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

#define MAX_BUFFER      65536

extern TIcons *gIcons;
extern amx::TAmxNet *gAmxNet;
extern TPageManager *gPageManager;

THR_REFRESH_t *TButton::mThrRefresh = nullptr;
vector<BITMAP_CACHE> nBitmapCache;     // Holds the images who are delayed because they are external

mutex mutex_button;
mutex mutex_text;
mutex mutex_bargraph;
mutex mutex_sysdraw;
mutex mutex_bmCache;

/**
 * The following table defines some of the system borders. It is mostly a
 * fallback table but defines whether a border should be calculated internaly
 * or constructed out of the images in the system border folder. The later is
 * only possible if this folder exist and contains the system images from AMX.
 * This images could be retrieved by sending a full surface with the system
 * files included from TPDesign4.
 * All borders not listed in this table are constructed out of the system
 * border images, if they exist.
 */
SYSBORDER_t sysBorders[] = {
//   ID   Name                  AMX number  Style          width radius calc
    {  1, (char *)"Single Line",         0, (char *)"solid",   1,   0, true },
    {  2, (char *)"Double Line",         0, (char *)"solid",   2,   0, true },
    {  3, (char *)"Quad Line",           0, (char *)"solid",   4,   0, true },
    {  4, (char *)"Picture Frame",       0, (char *)"double",  0,   0, false },
    {  5, (char *)"Circle 15",           8, (char *)"solid",   2,   7, true },
    {  6, (char *)"Circle 25",           9, (char *)"solid",   2,  14, true },
    {  7, (char *)"Circle 35",          10, (char *)"solid",   2,  21, true },
    {  8, (char *)"Circle 45",          11, (char *)"solid",   2,  28, true },
    {  9, (char *)"Circle 55",          12, (char *)"solid",   2,  35, true },
    { 10, (char *)"Circle 65",          13, (char *)"solid",   2,  42, true },
    { 11, (char *)"Circle 75",          14, (char *)"solid",   2,  49, true },
    { 12, (char *)"Circle 85",          15, (char *)"solid",   2,  56, true },
    { 13, (char *)"Circle 95",          16, (char *)"solid",   2,  63, true },
    { 14, (char *)"Circle 105",         17, (char *)"solid",   2,  70, true },
    { 15, (char *)"Circle 115",         18, (char *)"solid",   2,  77, true },
    { 16, (char *)"Circle 125",         19, (char *)"solid",   2,  84, true },
    { 17, (char *)"Circle 135",         20, (char *)"solid",   2,  91, true },
    { 18, (char *)"Circle 145",         21, (char *)"solid",   2,  98, true },
    { 19, (char *)"Circle 155",         22, (char *)"solid",   2, 105, true },
    { 20, (char *)"Circle 165",         23, (char *)"solid",   2, 112, true },
    { 21, (char *)"Circle 175",         24, (char *)"solid",   2, 119, true },
    { 22, (char *)"Circle 185",         25, (char *)"solid",   2, 126, true },
    { 23, (char *)"Circle 195",         26, (char *)"solid",   2, 133, true },
    { 24, (char *)"AMX Elite Inset -L",  0, (char *)"groove", 20,   0, false },
    { 25, (char *)"AMX Elite Raised -L", 0, (char *)"ridge",  20,   0, false },
    { 26, (char *)"AMX Elite Inset -M",  0, (char *)"groove", 10,   0, false },
    { 27, (char *)"AMX Elite Raised -M", 0, (char *)"ridge",  10,   0, false },
    { 28, (char *)"AMX Elite Inset -S",  0, (char *)"groove",  4,   0, false },
    { 29, (char *)"AMX Elite Raised -S", 0, (char *)"ridge",   4,   0, false },
    { 30, (char *)"Bevel Inset -L",      0, (char *)"inset",  20,   0, false },
    { 31, (char *)"Bevel Raised -L",     0, (char *)"outset", 20,   0, false },
    { 32, (char *)"Bevel Inset -M",      0, (char *)"inset",  10,   0, false },
    { 33, (char *)"Bevel Raised -M",     0, (char *)"outset", 10,   0, false },
    { 34, (char *)"Bevel Inset -S",      0, (char *)"inset",   4,   0, false },
    { 35, (char *)"Bevel Raised -S",     0, (char *)"outset",  4,   0, false },
    {  0, nullptr, 0, nullptr, 0, 0 }
};

SYSBUTTONS_t sysButtons[] = {
    {    8, MULTISTATE_BARGRAPH,  12, 0,  11 },  // Connection status
    {    9, BARGRAPH,              2, 0, 100 },  // System volume
    {   17, GENERAL,               2, 0,   0 },  // Button sounds on/off
    {   73, GENERAL,               2, 0,   0 },  // Enter setup page
    {   80, GENERAL,               2, 0,   0 },  // Shutdown program
    {   81, MULTISTATE_BARGRAPH,   6, 1,   6 },  // Network signal stength
    {  122, TEXT_INPUT,            2, 0,   0 },  // IP Address of server or domain name
    {  123, TEXT_INPUT,            2, 9,   0 },  // Channel number of panel
    {  124, TEXT_INPUT,            2, 0,   0 },  // The network port number (1319)
    {  141, GENERAL,               2, 0,   0 },  // Standard time
    {  142, GENERAL,               2, 0,   0 },  // Time AM/PM
    {  143, GENERAL,               2, 0,   0 },  // 24 hour time
    {  151, GENERAL,               2, 0,   0 },  // Date weekday
    {  152, GENERAL,               2, 0,   0 },  // Date mm/dd
    {  153, GENERAL,               2, 0,   0 },  // Date dd/mm
    {  154, GENERAL,               2, 0,   0 },  // Date mm/dd/yyyy
    {  155, GENERAL,               2, 0,   0 },  // Date dd/mm/yyyy
    {  156, GENERAL,               2, 0,   0 },  // Date month dd, yyyy
    {  157, GENERAL,               2, 0,   0 },  // Date dd month, yyyy
    {  158, GENERAL,               2, 0,   0 },  // Date yyyy-mm-dd
    {  171, GENERAL,               2, 0,   0 },  // Sytem volume up
    {  172, GENERAL,               2, 0,   0 },  // Sytem volume down
    {  173, GENERAL,               2, 0,   0 },  // Sytem mute toggle
    {  199, TEXT_INPUT,            2, 0,   0 },  // Technical name of panel
    {  234, GENERAL,               2, 0,   0 },  // Battery charging/not charging
    {  242, BARGRAPH,              2, 0, 100 },  // Battery level
    { 1101, TEXT_INPUT,            2, 0,   0 },  // Path and name of the logfile
    {    0, NONE,                  0, 0,   0 }   // Terminate
};

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
        else if (ename.compare("na") == 0)
            na = content;
        else if (ename.compare("bd") == 0)
            bd = content;
        else if (ename.compare("lt") == 0)
            lt = xml->convertElementToInt(content);
        else if (ename.compare("tp") == 0)
            tp = xml->convertElementToInt(content);
        else if (ename.compare("wt") == 0)
            wt = xml->convertElementToInt(content);
        else if (ename.compare("ht") == 0)
            ht = xml->convertElementToInt(content);
        else if (ename.compare("zo") == 0)
            zo = xml->convertElementToInt(content);
        else if (ename.compare("hs") == 0)
            hs = content;
        else if (ename.compare("bs") == 0)
            bs = content;
        else if (ename.compare("fb") == 0)
            fb = getButtonFeedback(content);
        else if (ename.compare("ap") == 0)
            ap = xml->convertElementToInt(content);
        else if (ename.compare("ad") == 0)
            ad = xml->convertElementToInt(content);
        else if (ename.compare("ch") == 0)
            ch = xml->convertElementToInt(content);
        else if (ename.compare("cp") == 0)
            cp = xml->convertElementToInt(content);
        else if (ename.compare("lp") == 0)
            lp = xml->convertElementToInt(content);
        else if (ename.compare("lv") == 0)
            lv = xml->convertElementToInt(content);
        else if (ename.compare("dr") == 0)
            dr = content;
        else if (ename.compare("co") == 0)
            co = xml->convertElementToInt(content);
        else if (ename.compare("cm") == 0)
            cm.push_back(content);
        else if (ename.compare("va") == 0)
            va = xml->convertElementToInt(content);
        else if (ename.compare("rm") == 0)
            rm = xml->convertElementToInt(content);
        else if (ename.compare("nu") == 0)
            nu = xml->convertElementToInt(content);
        else if (ename.compare("nd") == 0)
            nd = xml->convertElementToInt(content);
        else if (ename.compare("ar") == 0)
            ar = xml->convertElementToInt(content);
        else if (ename.compare("ru") == 0)
            ru = xml->convertElementToInt(content);
        else if (ename.compare("rd") == 0)
            rd = xml->convertElementToInt(content);
        else if (ename.compare("lu") == 0)
            lu = xml->convertElementToInt(content);
        else if (ename.compare("ld") == 0)
            ld = xml->convertElementToInt(content);
        else if (ename.compare("rv") == 0)
            rv = xml->convertElementToInt(content);
        else if (ename.compare("rl") == 0)
            rl = xml->convertElementToInt(content);
        else if (ename.compare("rh") == 0)
            rh = xml->convertElementToInt(content);
        else if (ename.compare("ri") == 0)
            ri = xml->convertElementToInt(content);
        else if (ename.compare("rn") == 0)
            rn = xml->convertElementToInt(content);
        else if (ename.compare("lf") == 0)
            lf = content;
        else if (ename.compare("sd") == 0)
            sd = content;
        else if (ename.compare("sc") == 0)
            sc = content;
        else if (ename.compare("mt") == 0)
            mt = xml->convertElementToInt(content);
        else if (ename.compare("dt") == 0)
            dt = content;
        else if (ename.compare("im") == 0)
            im = content;
        else if (ename.compare("op") == 0)
            op = content;
        else if (ename.compare("pc") == 0)
            pc = content;
        else if (ename.compare("hd") == 0)
            hd = xml->convertElementToInt(content);
        else if (ename.compare("da") == 0)
            da = xml->convertElementToInt(content);
        else if (ename.compare("ac") == 0)
        {
            ac_di = xml->getAttributeInt("di", attrs);
        }
        else if (ename.compare("pf") == 0)
        {
            PUSH_FUNC_T pf;
            pf.pfName = content;
            pf.pfType = xml->getAttribute("type", attrs);
            pushFunc.push_back(pf);
        }
        else if (ename.compare("sr") == 0)
        {
            SR_T bsr;
            bsr.number = xml->getAttributeInt("number", attrs);
            string e;

            while ((index = xml->getNextElementFromIndex(index, &e, &content, &attrs)) != TExpat::npos)
            {
                if (e.compare("do") == 0)
                    bsr._do = content;
                else if (e.compare("bs") == 0)
                    bsr.bs = content;
                else if (e.compare("mi") == 0)
                    bsr.mi = content;
                else if (e.compare("cb") == 0)
                    bsr.cb = content;
                else if (e.compare("cf") == 0)
                    bsr.cf = content;
                else if (e.compare("ct") == 0)
                    bsr.ct = content;
                else if (e.compare("ec") == 0)
                    bsr.ec = content;
                else if (e.compare("bm") == 0)
                {
                    bsr.bm = content;
                    bsr.dynamic = ((xml->getAttributeInt("dynamic", attrs) == 1) ? true : false);
                }
                else if (e.compare("sd") == 0)
                    bsr.sd = content;
                else if (e.compare("sb") == 0)
                    bsr.sb = xml->convertElementToInt(content);
                else if (e.compare("ii") == 0)
                    bsr.ii = xml->convertElementToInt(content);
                else if (e.compare("ji") == 0)
                    bsr.ji = xml->convertElementToInt(content);
                else if (e.compare("jb") == 0)
                    bsr.jb = xml->convertElementToInt(content);
                else if (e.compare("bx") == 0)
                    bsr.bx = xml->convertElementToInt(content);
                else if (e.compare("by") == 0)
                    bsr.by = xml->convertElementToInt(content);
                else if (e.compare("ix") == 0)
                    bsr.ix = xml->convertElementToInt(content);
                else if (e.compare("iy") == 0)
                    bsr.iy = xml->convertElementToInt(content);
                else if (e.compare("fi") == 0)
                    bsr.fi = xml->convertElementToInt(content);
                else if (e.compare("te") == 0)
                    bsr.te = content;
                else if (e.compare("jt") == 0)
                    bsr.jt = (TEXT_ORIENTATION)xml->convertElementToInt(content);
                else if (e.compare("tx") == 0)
                    bsr.tx = xml->convertElementToInt(content);
                else if (e.compare("ty") == 0)
                    bsr.ty = xml->convertElementToInt(content);
                else if (e.compare("ww") == 0)
                    bsr.ww = xml->convertElementToInt(content);
                else if (e.compare("et") == 0)
                    bsr.et = xml->convertElementToInt(content);
                else if (e.compare("oo") == 0)
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

    BITMAP_t image;
    image.buffer = (unsigned char *)mLastImage.getPixels();
    image.rowBytes = mLastImage.info().minRowBytes();
    image.width = mLastImage.info().width();
    image.height = mLastImage.info().height();
    return image;
}

FONT_T TButton::getFont()
{
    DECL_TRACER("TButton::getFont()");

    if (!mFonts)
    {
        MSG_ERROR("No fonts available!");
        return FONT_T();
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

BUTTONTYPE TButton::getButtonType(const string& bt)
{
    DECL_TRACER("TButton::getButtonType(const string& bt)");

    if (bt.compare("general") == 0)
        return GENERAL;
    else if (bt.compare("multi-state general") == 0 || bt.compare("multiGeneral") == 0)
        return MULTISTATE_GENERAL;
    else if (bt.compare("bargraph") == 0)
        return BARGRAPH;
    else if (bt.compare("multi-state bargraph") == 0 || bt.compare("multiBargraph") == 0)
        return MULTISTATE_BARGRAPH;
    else if (bt.compare("joistick") == 0)
        return JOISTICK;
    else if (bt.compare("text input") == 0 || bt.compare("textArea") == 0)
        return TEXT_INPUT;
    else if (bt.compare("computer control") == 0)
        return COMPUTER_CONTROL;
    else if (bt.compare("take note") == 0)
        return TAKE_NOTE;
    else if (bt.compare("sub-page view") == 0)
        return SUBPAGE_VIEW;

    return NONE;
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
    int i = 0;

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

        i++;
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
    DECL_TRACER("TButton::makeElement()");

    if (prg_stopped)
        return false;

    int inst = mActInstance;

    if (instance >= 0 && (size_t)instance < sr.size())
    {
        if (mActInstance != instance)
            mChanged = true;

        inst = instance;
    }

    if (type == MULTISTATE_GENERAL && ar == 1)
        return drawButtonMultistateAni();
    else if (type == BARGRAPH && isSystemButton() && lv == 9)   // System volume button
        return drawBargraph(inst, TConfig::getSystemVolume());
    else if (type == BARGRAPH)
        return drawBargraph(inst, mLastLevel);
    else if (type == MULTISTATE_BARGRAPH)
        return drawMultistateBargraph(mLastLevel, true);
    else if (type == TEXT_INPUT)
    {
        if (isSystemButton() && !mSystemReg)
            registerSystemButton();

        drawTextArea(mActInstance);
    }
    else if (isSystemButton() && ch == 17)  // System button sound ON/OFF
    {
        if (TConfig::getSystemSoundState())
            inst = mActInstance = 1;
        else
            inst = mActInstance = 0;

        return drawButton(inst);
    }
    else if (isSystemButton() && ch == 173) // System mute setting
    {
        if (TConfig::getMuteState())
            inst = mActInstance = 1;
        else
            inst = mActInstance = 0;

        return drawButton(inst);
    }
    else
        return drawButton(inst);

    return false;
}

bool TButton::setActive(int instance)
{
    DECL_TRACER("TButton::setActive(int instance)");

    if (mAniRunning)
        return true;

    if (instance < 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " is out of range from 0 to " << sr.size() << "!");
        return false;
    }

    if (instance == mActInstance)
        return true;

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

        sr[inst].ii = id;
        mChanged = true;
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

        sr[inst].ii = 0;
        inst++;
        mChanged = true;
    }

    return makeElement(instance);
}

bool TButton::setText(const string& txt, int instance)
{
    DECL_TRACER("TButton::setText(const string& txt, int instance)");

    if (instance >= 0 && (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (!setTextOnly(txt, instance))
        return false;

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

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].te != txt)
            mChanged = true;

        sr[inst].te = txt;
        inst++;
    }

    return true;
}

bool TButton::appendText(const string &txt, int instance)
{
    DECL_TRACER("TButton::appendText(const string &txt, int instance)");

    if (instance >= 0 || (size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return false;
    }

    if (txt.empty())
        return true;

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        sr[inst].te.append(txt);
        mChanged = true;
        inst++;
    }

    return makeElement(instance);
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

    return makeElement(instance);
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

bool TButton::setFeedback(FEEDBACK feedback)
{
    DECL_TRACER("TButton::setFeedback(FEEDBACK feedback)");

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

    if (strCaseCompare(style, "None") == 0)     // Clear the border?
    {
        if (instance < 0)
            bs.clear();
        else
            sr[instance].bs.clear();

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
                bs = style;
            else
                sr[instance].bs = style;

            if (mEnabled && !hd)
                makeElement(instance);

            return true;
        }
    }

    // Check whether it is a supported style or not. If the style is not
    // supported, it will be ignored.
    int i = 0;

    while (sysBorders[i].id > 0)
    {
        if (strCaseCompare(sysBorders[i].name, style) == 0)
        {
            if (instance < 0)
                bs = sysBorders[i].name;
            else
                sr[instance].bs = sysBorders[i].name;

            if (mEnabled && !hd)
                makeElement(instance);

            return true;
        }

        i++;
    }

    return false;
}

string TButton::getBorderStyle(int instance)
{
    DECL_TRACER("TButton::getBorderStyle(int instance)");

    if (instance < 0 || instance >= (int)sr.size())
    {
        MSG_ERROR("Invalid instance " << (instance + 1) << " submitted!");
        return string();
    }

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
        return false;

    if ((size_t)instance >= sr.size())
        return false;

    int id = mFonts->getFontIDfromFile(name);

    if (id == -1)
        return false;

    int inst = instance;
    int loop = 1;

    if (inst < 0)
    {
        loop = (int)sr.size();
        inst = 0;
    }

    for (int i = 0; i < loop; ++i)
    {
        if (sr[inst].fi != id)
            mChanged = true;

        sr[inst].fi = id;
        inst++;
    }

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

void TButton::setActiveInstance(int inst)
{
    DECL_TRACER("TButton::setActiveInstance()");

    if (inst < 0 || (size_t)inst >= sr.size())
        return;

    if (mActInstance != inst)
        mChanged = true;

    mActInstance = inst;
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
        return false;
    }

    if (op < 0 || op < 255)
    {
        MSG_ERROR("Invalid opacity " << op << "!");
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
        if (sr[inst].oo == op)
        {
            inst++;
            continue;
        }

        sr[inst].oo = op;
        mChanged = true;
        inst++;
    }

    return makeElement(instance);
}

bool TButton::setFont(int id, int instance)
{
    DECL_TRACER("TButton::setFont(int id)");

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
        if (sr[inst].fi == id)
        {
            inst++;
            continue;
        }

        sr[inst].fi = id;
        mChanged = true;
        inst++;
    }

    return makeElement(instance);
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

    lt = left;
    tp = top;
    makeElement(mActInstance);
}

void TButton::setRectangle(int left, int top, int right, int bottom)
{
    DECL_TRACER("setRectangle(int left, int top, int right, int bottom)");

    if (!gPageManager)
        return;

    int screenWidth = gPageManager->getSettings()->getWith();
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
        *x = sr[instance].bx;

    if (y)
        *y = sr[instance].by;

    return sr[instance].jb;
}

void TButton::setBitmapJustification(int j, int x, int y, int instance)
{
    DECL_TRACER("TButton::setBitmapJustification(int j, int instance)");

    if (j < 0 || j > 9 || instance >= (int)sr.size())
        return;

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
        *x = sr[instance].ix;

    if (y)
        *y = sr[instance].iy;

    return sr[instance].ji;
}

void TButton::setIconJustification(int j, int x, int y, int instance)
{
    DECL_TRACER("TButton::setIconJustification(int j, int x, int y, int instance)");

    if (j < 0 || j > 9 || instance >= (int)sr.size())
        return;

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
        *x = sr[instance].tx;

    if (y)
        *y = sr[instance].ty;

    return sr[instance].jt;
}

void TButton::setTextJustification(int j, int x, int y, int instance)
{
    DECL_TRACER("TButton::setTextJustification(int j, int x, int y, int instance)");

    if (j < 0 || j > 9 || instance >= (int)sr.size())
        return;

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

    makeElement();
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

    if ((size_t)instance >= sr.size())
    {
        MSG_ERROR("Instance " << instance << " does not exist!");
        return;
    }

    if (!TColor::isValidAMXcolor(ec))
    {
        MSG_PROTOCOL("Invalid color >" << ec << "< ignored!");
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
        if (sr[inst].ec.compare(ec) == 0)
        {
            inst++;
            continue;
        }

        sr[inst].ec = ec;
        mChanged = true;
        inst++;
    }

    if (visible)
        makeElement();
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
        {
            if (sr[i].sd != sound)
                mChanged = true;

            sr[i].sd = sound;
        }
    }
    else
    {
        if (sr[inst].sd != sound)
            mChanged = true;

        sr[inst].sd = sound;
    }
}

bool TButton::startAnimation(int start, int end, int time)
{
    DECL_TRACER("TButton::startAnimation(int start, int end, int time)");

    if (start > end || start < 0 || (size_t)end > sr.size() || time < 0)
    {
        MSG_ERROR("Invalid parameter: start=" << start << ", end=" << end << ", time=" << time);
        return false;
    }

    if (time == 0)
    {
        int inst = end - 1;

        if (inst >= 0 && (size_t)inst < sr.size())
        {
            if (mActInstance != inst)
            {
                mActInstance = inst;
                drawButton(inst);
            }
        }

        return true;
    }

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
    imgButton.allocN32Pixels(wt, ht);

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
        ooButton.allocN32Pixels(w, h, true);
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
    size_t rowBytes = imgButton.info().minRowBytes();

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
            SkCanvas can(imgButton, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            rowBytes = imgButton.info().minRowBytes();
            mLastImage = imgButton;
        }
#endif
        _displayButton(mHandle, parent, (unsigned char *)imgButton.getPixels(), rwidth, rheight, rowBytes, rleft, rtop);
    }
}

void TButton::registerSystemButton()
{
    DECL_TRACER("TButton::registerSystemButton()");

    if (mSystemReg)
        return;

    // If this is a special system button, register it to receive the state
    if (ap == 0 && ad == 8)     // Connection status?
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
    else if (ap == 0 && ((ad >= 141 && ad <= 143) || (ad >= 151 && ad <= 158))) // time or date
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

        if (ad >= 141 && ad <= 143 && !mTimer)
        {
            mTimer = new TTimer;
            mTimer->setInterval(std::chrono::milliseconds(1000));   // 1 second
            mTimer->registerCallback(bind(&TButton::_TimerCallback, this, std::placeholders::_1));
            mTimer->run();
        }
    }
    else if (ap == 0 && (ad == 242 || ad == 234))   // Battery status
    {
        if (gPageManager)
            gPageManager->regCallbackBatteryState(bind(&TButton::funcBattery, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), mHandle);
    }
    else if (lp == 0 && lv == 81)
    {
        if (gPageManager)
            gPageManager->regCallbackNetState(bind(&TButton::funcNetworkState, this, std::placeholders::_1), mHandle);
    }
    else if (ap == 0 && ad == 122)      // IP Address of server
    {
        sr[0].te = sr[1].te = TConfig::getController();
        mChanged = true;
    }
    else if (ap == 0 && ad == 123)      // Channel number of panel
    {
        sr[0].te = sr[1].te = std::to_string(TConfig::getChannel());
        mChanged = true;
    }
    else if (ap == 0 && ad == 124)      // Network port number of the controller
    {
        sr[0].te = sr[1].te = std::to_string(TConfig::getPort());
        mChanged = true;
    }
    else if (ap == 0 && ad == 199)      // Technical name of panel
    {
        sr[0].te = sr[1].te = TConfig::getPanelType();
        mChanged = true;
    }
    else if (ap == 0 && ad == 1101)     // Path and name of the logfile
    {
        sr[0].te = sr[1].te = TConfig::getLogFile();
        mChanged = true;
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

    int elems = sdo.length() / 2;

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
    MSG_DEBUG("Fill color[" << instance << "]: " << sr[instance].cf << " (#" << std::setw(8) << std::setfill('0') << std::hex << color << ")");
    bm->eraseColor(color);
    return true;
}

bool TButton::buttonBitmap(SkBitmap* bm, int inst)
{
    DECL_TRACER("TButton::buttonBitmap(SkBitmap* bm, int instane)");

    if (prg_stopped)
        return false;

    int instance = inst;

    if (inst < 0)
        instance = 0;
    else if ((size_t)inst >= sr.size())
        instance = sr.size() - 1;

    if (!sr[instance].mi.empty() && sr[instance].bs.empty())       // Chameleon image?
    {
        MSG_TRACE("Chameleon image consisting of mask " << sr[instance].mi << " and bitmap " << (sr[instance].bm.empty() ? "NONE" : sr[instance].bm) << " ...");

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

        MSG_DEBUG("Chameleon image size: " << sr[instance].mi_width << " x " << sr[instance].mi_height);
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

            if (imgRed.info().width() == bmBm.info().width() &&
                imgRed.info().height() == bmBm.info().height())
            {
                imgMask.installPixels(bmBm.pixmap());
            }
            else
            {
                MSG_WARNING("The mask has an invalid size. Will ignore the mask!");
                imgMask.allocN32Pixels(imgRed.info().width(), imgRed.info().height());
                imgMask.eraseColor(SK_ColorTRANSPARENT);
                haveBothImages = false;
            }
        }
        else
            haveBothImages = false;

        MSG_DEBUG("Bitmap image size: " << sr[instance].bm_width << " x " << sr[instance].bm_height);
        SkBitmap img = drawImageButton(imgRed, imgMask, sr[instance].mi_width, sr[instance].mi_height, TColor::getSkiaColor(sr[instance].cf), TColor::getSkiaColor(sr[instance].cb));
//        SkBitmap img = drawImageButton(imgRed, imgMask, imgRed.info().width(), imgRed.info().height(), TColor::getSkiaColor(sr[instance].cf), TColor::getSkiaColor(sr[instance].cb));

        if (img.empty())
        {
            MSG_ERROR("Error creating the cameleon image \"" << sr[instance].mi << "\" / \"" << sr[instance].bm << "\"!");
            TError::setError();
            return false;
        }

        SkCanvas ctx(img, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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

        SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
//        POSITION_t position = calcImagePosition(sr[instance].bm_width, sr[instance].bm_height, SC_BITMAP, instance);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the position of the image for button number " << bi);
            TError::setError();
            return false;
        }

        MSG_DEBUG("Putting bitmap on top of image ...");
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        SkCanvas can(*bm, SkSurfaceProps(SkSurfaceProps::kUseDeviceIndependentFonts_Flag, kUnknown_SkPixelGeometry));

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
        MSG_DEBUG("Dynamic button " << TObject::handleToString(mHandle) << " is invisible. Will not draw it.");
        return true;
    }

    MSG_DEBUG("Dynamic button " << TObject::handleToString(mHandle) << " will be drawn ...");
    size_t idx = 0;

    if ((idx = gPrjResources->getResourceIndex("image")) == TPrjResources::npos)
    {
        MSG_ERROR("There exists no image resource!");
        return false;
    }

    RESOURCE_T resource = gPrjResources->findResource(idx, sr[instance].bm);

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
        SkCanvas can(*bm, SkSurfaceProps(SkSurfaceProps::kUseDeviceIndependentFonts_Flag, kUnknown_SkPixelGeometry));

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
                MSG_ERROR("Couldn't find the handle " << TObject::handleToString(bc.handle) << " in bitmap cache!");
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
                _displayButton(bc.handle, bc.parent, (unsigned char *)bmCache.bitmap.getPixels(), bc.width, bc.height, bmCache.bitmap.info().minRowBytes(), bc.left, bc.top);
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

void TButton::funcBattery(int level, bool charging, int /* chargeType */)
{
    DECL_TRACER("TButton::funcBattery(int level, bool charging, int chargeType)");

    // Battery level is always a bargraph
    if (ap == 0 && ad == 242)       // Not charging
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
    else if (ap == 0 && ad == 234)  // Charging
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

    SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));

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

        img.allocPixels(SkImageInfo::MakeN32Premul(sr[0].mi_width, sr[0].mi_height));
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

        SkCanvas ctx(img, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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

        SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
        SkCanvas can_bm(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));


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
        img_bar.allocPixels(SkImageInfo::MakeN32Premul(sr[1].bm_width, sr[1].bm_height));
        img_bar.eraseColor(SK_ColorTRANSPARENT);
        SkCanvas bar(img_bar, SkSurfaceProps(1, kUnknown_SkPixelGeometry));

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
        SkCanvas can_bm(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));

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
        img_bar.allocPixels(SkImageInfo::MakeN32Premul(sr[1].bm_width, sr[1].bm_height));
        img_bar.eraseColor(SK_ColorTRANSPARENT);
        SkCanvas bar(img_bar, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
        SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
    slButton.allocN32Pixels(width, height);
    slButton.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas slCan(slButton, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
        gIcons = new TIcons();

        if (TError::isError())
        {
            MSG_ERROR("Error initializing icons!");
            return false;
        }
    }

    string file = gIcons->getFile(sr[instance].ii);
    MSG_DEBUG("Loading icon file " << file);
    sk_sp<SkData> image;
    SkBitmap icon;

    if (!(image = readImage(file)))
        return false;

    DecodeDataToBitmap(image, &icon);

    if (icon.empty())
    {
        MSG_WARNING("Could not create an icon for element " << sr[instance].ii << " on button " << bi << " (" << na << ")");
        return false;
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
    SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));

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
        instance = sr.size() - 1;
    else if (instance < 0)
        instance = 0;

    if (sr[instance].te.empty() || !mFonts)     // Is there a text and fonts?
    {                                           // No, then return
        MSG_DEBUG("Empty text string.");
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
    SkCanvas canvas(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));

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
            lines = textLines.size();
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
                    count = skFont.textToGlyphs(uni, num, SkTextEncoding::kUTF16, glyphs, glyphSize);

                    if (count <= 0)
                    {
                        delete[] glyphs;
                        glyphs = TFont::textToGlyphs(text, typeFace, &num);
                        count = num;
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
                count = skFont.textToGlyphs(text.data(), text.size(), SkTextEncoding::kUTF8, glyphs, glyphSize);
            }

            if (glyphs)
            {
                MSG_DEBUG("1st glyph: 0x" << std::hex << std::setw(8) << std::setfill('0') << *glyphs << ", # glyphs: " << std::dec << count);
                canvas.drawSimpleText(glyphs, sizeof(uint16_t) * count, SkTextEncoding::kGlyphID, startX, startY, skFont, paint);
            }
            else    // Try to print something
            {
                MSG_WARNING("Got no glyphs! Try to print: " << text);
                glyphs = new uint16_t[text.size()];
                size_t glyphSize = sizeof(uint16_t) * text.size();
                count = skFont.textToGlyphs(text.data(), text.size(), SkTextEncoding::kUTF8, glyphs, glyphSize);
                canvas.drawSimpleText(glyphs, sizeof(uint16_t) * count, SkTextEncoding::kGlyphID, startX, startY, skFont, paint);
            }

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

    // Try to find the border in the system table
    BORDER_t bd, bda;
    int borderIndex = -1;
    int i = 0;

    while (sysBorders[i].id)
    {
        string sysBrd = sysBorders[i].name;

        if (sysBrd == sr[instance].bs)
        {
            borderIndex = i;
            MSG_DEBUG("Found internal system border [" << i << "]: " << sysBrd);
            break;
        }

        i++;
    }

    bool classExist = (gPageManager && gPageManager->getSystemDraw());

    if ((classExist && borderIndex >= 0 && !sysBorders[borderIndex].calc) || (classExist && borderIndex < 0))
    {
        int numBorders = 0;
        bool extBorder = false;

        string borderName1 = (bs.empty() ? sr[instance].bs : bs);
        string borderName2 = (!bs.empty() && !sr[instance].bs.empty() ? sr[instance].bs : bs);

        if (bs.empty())
        {
            if (gPageManager->getSystemDraw()->getBorder(sr[instance].bs, TSystemDraw::LT_OFF, &bd))
                numBorders++;

            if (gPageManager->getSystemDraw()->getBorder(sr[instance].bs, TSystemDraw::LT_ON, &bda))
                numBorders++;

            if (numBorders == 2)
                extBorder = true;
        }
        else if (!sr[instance].bs.empty())
        {
            if (gPageManager->getSystemDraw()->getBorder(bs, (instance == 0 ? TSystemDraw::LT_OFF : TSystemDraw::LT_ON), &bd, sr[instance].bs))
            {
                numBorders++;
                extBorder = true;
            }
        }

        if (extBorder)
        {
            MSG_DEBUG("System border \"" << borderName1 << "\" and \"" << borderName2 << "\" found.");
            SkColor color = TColor::getSkiaColor(sr[instance].cb);      // border color
            SkColor bgColor = TColor::getSkiaColor(sr[instance].cf);    // fill color
            MSG_DEBUG("Button color: #" << std::setw(6) << std::setfill('0') << std::hex << color);
            // Load images
            SkBitmap imgB, imgBR, imgR, imgTR, imgT, imgTL, imgL, imgBL;

            imgB = retrieveBorderImage(bd.b, bda.b, color, bgColor);

            if (imgB.empty())
                return false;

            MSG_DEBUG("Got images " << bd.b << " and " << bda.b << " with size " << imgB.info().width() << " x " << imgB.info().height());
            imgBR = retrieveBorderImage(bd.br, bda.br, color, bgColor);

            if (imgBR.empty())
                return false;

            MSG_DEBUG("Got images " << bd.br << " and " << bda.br << " with size " << imgBR.info().width() << " x " << imgBR.info().height());
            imgR = retrieveBorderImage(bd.r, bda.r, color, bgColor);

            if (imgR.empty())
                return false;

            MSG_DEBUG("Got images " << bd.r << " and " << bda.r << " with size " << imgR.info().width() << " x " << imgR.info().height());
            imgTR = retrieveBorderImage(bd.tr, bda.tr, color, bgColor);

            if (imgTR.empty())
                return false;

            MSG_DEBUG("Got images " << bd.tr << " and " << bda.tr << " with size " << imgTR.info().width() << " x " << imgTR.info().height());
            imgT = retrieveBorderImage(bd.t, bda.t, color, bgColor);

            if (imgT.empty())
                return false;

            MSG_DEBUG("Got images " << bd.t << " and " << bda.t << " with size " << imgT.info().width() << " x " << imgT.info().height());
            imgTL = retrieveBorderImage(bd.tl, bda.tl, color, bgColor);

            if (imgTL.empty())
                return false;

            MSG_DEBUG("Got images " << bd.tl << " and " << bda.tl << " with size " << imgTL.info().width() << " x " << imgTL.info().height());
            imgL = retrieveBorderImage(bd.l, bda.l, color, bgColor);

            if (imgL.empty())
                return false;

            MSG_DEBUG("Got images " << bd.l << " and " << bda.l << " with size " << imgL.info().width() << " x " << imgL.info().height());
            imgBL = retrieveBorderImage(bd.bl, bda.bl, color, bgColor);

            if (imgBL.empty())
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
            SkCanvas canvas(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
            return true;
        }
    }

    // Here we've not found the wanted border in the system table. Reasons may
    // be a wrong name or the system images don't exist.
    // We'll try to find it in our internal table. This is a fallback which
    // provides only a limited number of system borders.

    if (borderIndex < 0)
        return true;

    bool systemBrdExist = false;

    if (classExist && gPageManager->getSystemDraw()->getBorder(sr[instance].bs, ((instance == 0) ? TSystemDraw::LT_OFF : TSystemDraw::LT_ON), &bd))
        systemBrdExist = true;

    if (sr[instance].bs.compare(sysBorders[borderIndex].name) == 0)
    {
        MSG_DEBUG("Border " << sysBorders[borderIndex].name << " found.");
        SkCanvas canvas(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
        SkPaint paint;
        SkColor color = TColor::getSkiaColor(sr[instance].cb);

        paint.setColor(color);
        paint.setBlendMode(SkBlendMode::kSrc);
        paint.setStyle(SkPaint::kStroke_Style);
        SkRRect outher, inner;
        SkScalar radius = (SkScalar)sysBorders[borderIndex].radius;
        int red, green, blue;
        SkColor borderColor, bcLight, bcDark;
        int lineWidth = 0;

        switch (sysBorders[borderIndex].id)
        {
            case 1: // Single Frame
            case 2: // Double Frame
            case 3: // Quad Frame
                paint.setStrokeWidth(sysBorders[borderIndex].width);
                canvas.drawRect(calcRect(wt, ht, sysBorders[borderIndex].width), paint);
            break;

            case 4: // Picture Frame
                {
                    paint.setStrokeWidth(2);
                    SkRect rect = SkRect::MakeXYWH(0, 0, wt, ht);
                    canvas.drawRect(rect, paint);
                    rect = SkRect::MakeXYWH(4, 4, wt - 4, ht - 4);
                    canvas.drawRect(rect, paint);
                }
            break;

            case 5: // Circle 15
            case 6: // Circle 25
            case 7: // Circle 35
            case 8: // Circle 45
            case 9: // Circle 55
            case 10: // Circle 65
            case 11: // Circle 75
            case 12: // Circle 85
            case 13: // Circle 95
            case 14: // Circle 105
            case 15: // Circle 115
            case 16: // Circle 125
            case 17: // Circle 135
            case 18: // Circle 145
            case 19: // Circle 155
            case 20: // Circle 165
            case 21: // Circle 175
            case 22: // Circle 185
            case 23: // Circle 195
                if (systemBrdExist)
                {
                    radius = (double)bd.border.idealWidth / 2.0;    // Take diameter and calculate radius
                    lineWidth = bd.border.textLeft;
                }
                else
                    lineWidth = sysBorders[borderIndex].width;

                paint.setStrokeWidth(1.0);
                paint.setStyle(SkPaint::kFill_Style);
                MSG_DEBUG("Line width: " << lineWidth << ", radius: " << radius);
                // We draw a rounded rectangle to "clip" the corners. To do this
                // in a way to not miss any pixel, we draw a rectangle followed
                // by a rounded rectangle as an inner one. The space between
                // them will be filled transparent.
                outher = SkRRect::MakeRect({0, 0, (SkScalar)wt, (SkScalar)ht});
                inner = SkRRect::MakeRectXY(calcRect(wt, ht, 1), radius, radius);
                paint.setColor(SK_ColorTRANSPARENT);
                canvas.drawDRRect(outher, inner, paint);
                // Here we draw the rounded rectangle.
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setStrokeWidth(lineWidth);
                paint.setColor(color);
                paint.setStrokeJoin(SkPaint::kRound_Join);
                canvas.drawRoundRect(calcRect(wt, ht, lineWidth), radius, radius, paint);

            break;

            case 24:    // AMX Elite Inset -L
            case 26:    // AMX Elite Inset -M
            case 28:    // AMX Elite Inset -S
                {
                    borderColor = TColor::getSkiaColor(sr[instance].cb);
                    vector<SkColor> cols = TColor::colorRange(borderColor, sysBorders[borderIndex].width, 40, TColor::DIR_LIGHT_DARK_LIGHT);
                    vector<SkColor>::iterator iter;
                    int i = 0;

                    for (iter = cols.begin(); iter != cols.end(); ++iter)
                    {
                        paint.setStrokeWidth(1);
                        paint.setColor(*iter);
                        SkRect rect = SkRect::MakeXYWH(i, i, wt - i, ht - i);
                        canvas.drawRect(rect, paint);
                        i++;
                    }
                }
            break;

            case 25:    // AMX Elite Raised -L
            case 27:    // AMX Elite Raised -M
            case 29:    // AMX Elite Raised -S
            {
                borderColor = TColor::getSkiaColor(sr[instance].cb);
                vector<SkColor> cols = TColor::colorRange(borderColor, sysBorders[borderIndex].width, 40, TColor::DIR_DARK_LIGHT_DARK);
                vector<SkColor>::iterator iter;
                int i = 0;

                for (iter = cols.begin(); iter != cols.end(); ++iter)
                {
                    paint.setStrokeWidth(1);
                    paint.setColor(*iter);
                    SkRect rect = SkRect::MakeXYWH(i, i, wt - i, ht - i);
                    canvas.drawRect(rect, paint);
                    i++;
                }
            }
            break;

            case 30:    // BevelInset -L
            case 32:    // Bevel Inset -M
            case 34:    // Bevel Inset -S
                borderColor = TColor::getSkiaColor(sr[instance].cb);
                red = std::min((int)SkColorGetR(borderColor) + 20, 255);
                green = std::min((int)SkColorGetG(borderColor) + 20, 255);
                blue = std::min((int)SkColorGetB(borderColor) + 20, 255);
                bcLight = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
                red = std::max((int)SkColorGetR(borderColor) - 20, 0);
                green = std::max((int)SkColorGetG(borderColor) - 20, 0);
                blue = std::max((int)SkColorGetB(borderColor) - 20, 0);
                bcDark = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
                paint.setStrokeWidth(1);
                paint.setColor(bcDark);
                // Lines on the left
                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int yt = i;
                    int yb = ht - i;
                    canvas.drawLine(i, yt, i, yb, paint);
                }
                // Lines on the top
                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int xl = i;
                    int xr = wt - i;
                    canvas.drawLine(xl, i, xr, i, paint);
                }
                // Lines on right side
                paint.setColor(bcLight);

                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int yt = i;
                    int yb = ht - i;
                    canvas.drawLine(wt - i, yt, wt - i, yb, paint);
                }
                // Lines on bottom
                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int xl = i;
                    int xr = wt - i;
                    canvas.drawLine(xl, ht - i, xr, ht - i, paint);
                }
            break;

            case 31:    // Bevel Raised _L
            case 33:    // Bevel Raised _M
            case 35:    // Bevel Raised _S
                borderColor = TColor::getSkiaColor(sr[instance].cb);
                red = std::min((int)SkColorGetR(borderColor) + 10, 255);
                green = std::min((int)SkColorGetG(borderColor) + 10, 255);
                blue = std::min((int)SkColorGetB(borderColor) + 10, 255);
                bcLight = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
                red = std::max((int)SkColorGetR(borderColor) - 10, 0);
                green = std::max((int)SkColorGetG(borderColor) - 10, 0);
                blue = std::max((int)SkColorGetB(borderColor) - 10, 0);
                bcDark = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
                paint.setStrokeWidth(1);
                paint.setColor(bcLight);
                // Lines on the left
                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int yt = i;
                    int yb = ht - i;
                    canvas.drawLine(i, yt, i, yb, paint);
                }
                // Lines on the top
                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int xl = i;
                    int xr = wt - i;
                    canvas.drawLine(xl, i, xr, i, paint);
                }
                // Lines on right side
                paint.setColor(bcDark);

                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int yt = i;
                    int yb = ht - i;
                    canvas.drawLine(wt - i, yt, wt - i, yb, paint);
                }
                // Lines on bottom
                for (int i = 0; i < sysBorders[borderIndex].width; i++)
                {
                    int xl = i;
                    int xr = wt - borderIndex;
                    canvas.drawLine(xl, ht - i, xr, ht - i, paint);
                }
            break;
        }
    }

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
    int instance = start;
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
            instance = start;

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

bool TButton::drawButton(int instance, bool show)
{
    mutex_button.lock();
    DECL_TRACER("TButton::drawButton(int instance, bool show)");

    if (prg_stopped)
    {
        mutex_button.unlock();
        return false;
    }

    if ((size_t)instance >= sr.size() || instance < 0)
    {
        MSG_ERROR("Instance " << instance << " is out of bounds!");
        TError::setError();
        mutex_button.unlock();
        return false;
    }

    if (!_displayButton && gPageManager)
        _displayButton = gPageManager->getCallbackDB();

    if (!visible || hd || instance != mActInstance || !_displayButton)
    {
        bool db = (_displayButton != nullptr);
        MSG_DEBUG("Button " << bi << ", \"" << na << "\" at instance " << instance << " is not to draw!");
        MSG_DEBUG("Visible: " << (visible ? "YES" : "NO") << ", Hidden: " << (hd ? "YES" : "NO") << ", Instance/actual instance: " << instance << "/" << mActInstance << ", callback: " << (db ? "PRESENT" : "N/A"));
        mutex_button.unlock();
        return true;
    }

    MSG_DEBUG("Drawing button " << bi << ", \"" << na << "\" at instance " << instance);

    if (!mChanged && !mLastImage.empty())
    {
        showLastButton();
        mutex_button.unlock();
        return true;
    }

    ulong parent = mHandle & 0xffff0000;
    getDrawOrder(sr[instance]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
    {
        mutex_button.unlock();
        return false;
    }

    SkBitmap imgButton;
    imgButton.allocN32Pixels(wt, ht);
    bool dynState = false;

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!sr[instance].dynamic && !buttonBitmap(&imgButton, instance))
            {
                MSG_DEBUG("Button reported an error. Abort drawing!");
                mutex_button.unlock();
                return false;
            }
            else if (sr[instance].dynamic && !buttonDynamic(&imgButton, instance, show, &dynState))
            {
                MSG_DEBUG("Dynamic button reported an error. Abort drawing!");
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(&imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, instance))
            {
                mutex_button.unlock();
                return false;
            }
        }
    }

    if (mGlobalOO >= 0 || sr[instance].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();
        ooButton.allocN32Pixels(w, h, true);
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
    size_t rowBytes = imgButton.info().minRowBytes();

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
            SkCanvas can(imgButton, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            rowBytes = imgButton.info().minRowBytes();
            mLastImage = imgButton;
        }
#endif
        if (show)
        {
            _displayButton(mHandle, parent, (unsigned char *)imgButton.getPixels(), rwidth, rheight, rowBytes, rleft, rtop);
        }
    }

    mutex_button.unlock();
    return true;
}

bool TButton::drawTextArea(int instance)
{
    DECL_TRACER("TButton::drawTextArea(int instance, bool show)");

    if (prg_stopped)
        return false;

    if (!visible || hd)
        return true;

    if ((size_t)instance >= sr.size() || instance < 0)
    {
        MSG_ERROR("Instance " << instance << " is out of bounds!");
        TError::setError();
        return false;
    }

    if (!mChanged)
    {
        showLastButton();
        return true;
    }

    getDrawOrder(sr[instance]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
        return false;

    SkBitmap imgButton;
    imgButton.allocN32Pixels(wt, ht);

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, instance))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!sr[instance].dynamic && !buttonBitmap(&imgButton, instance))
                return false;
            else if (sr[instance].dynamic && !buttonDynamic(&imgButton, instance, false))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, instance))
                return false;
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, instance))
                return false;
        }
    }

    if (mGlobalOO >= 0 || sr[instance].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();
        ooButton.allocN32Pixels(w, h, true);
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
            SkCanvas can(imgButton, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
            gPageManager->getCallbackInputText()(this, bm);
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
        mutex_button.unlock();
        return true;
    }

    int maxLevel = level;

    if (maxLevel >= rh)
        maxLevel = rh - 1 ;
    else if (maxLevel <= rl && rl > 0)
        maxLevel = rl - 1;
    else if (maxLevel < 0)
        maxLevel = 0;

    MSG_DEBUG("Display instance " << maxLevel);
    ulong parent = mHandle & 0xffff0000;
    getDrawOrder(sr[maxLevel]._do, (DRAW_ORDER *)&mDOrder);

    if (TError::isError())
    {
        mutex_button.unlock();
        return false;
    }

    SkBitmap imgButton;
    imgButton.allocN32Pixels(wt, ht);

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL)
        {
            if (!buttonFill(&imgButton, maxLevel))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!buttonBitmap(&imgButton, maxLevel))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, maxLevel))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(&imgButton, maxLevel))
            {
                mutex_button.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, maxLevel))
            {
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
        ooButton.allocN32Pixels(w, h, true);
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
    size_t rowBytes = imgButton.info().minRowBytes();

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
            SkCanvas can(imgButton, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            MSG_DEBUG("Old rowBytes: " << rowBytes);
            rowBytes = imgButton.info().minRowBytes();
            MSG_DEBUG("New rowBytes: " << rowBytes);
            mLastImage = imgButton;
        }
#endif
        if (show)
            _displayButton(mHandle, parent, (unsigned char *)imgButton.getPixels(), rwidth, rheight, rowBytes, rleft, rtop);
    }

    mutex_button.unlock();
    return true;
}

bool TButton::drawBargraph(int instance, int level, bool show)
{
    mutex_bargraph.lock();
    DECL_TRACER("TButton::drawBargraph(int instance, int level, bool show)");

    if ((size_t)instance >= sr.size() || instance < 0)
    {
        MSG_ERROR("Instance " << instance << " is out of bounds!");
        TError::setError();
        mutex_bargraph.unlock();
        return false;
    }

    if (!_displayButton && gPageManager)
        _displayButton = gPageManager->getCallbackDB();

    if (!mChanged && mLastLevel == level)
    {
        showLastButton();
        mutex_bargraph.unlock();
        return true;
    }

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
        mutex_bargraph.unlock();
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
    {
        mutex_bargraph.unlock();
        return false;
    }

    SkBitmap imgButton;
    imgButton.allocN32Pixels(wt, ht);
    imgButton.eraseColor(TColor::getSkiaColor(sr[0].cf));
    bool haveFrame = false;

    for (int i = 0; i < ORD_ELEM_COUNT; i++)
    {
        if (mDOrder[i] == ORD_ELEM_FILL && !haveFrame)
        {
            if (!buttonFill(&imgButton, (type == BARGRAPH ? 0 : inst)))
            {
                mutex_bargraph.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BITMAP)
        {
            if (!barLevel(&imgButton, inst, mLastLevel))
            {
                mutex_bargraph.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_ICON)
        {
            if (!buttonIcon(&imgButton, inst))
            {
                mutex_bargraph.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_TEXT)
        {
            if (!buttonText(&imgButton, inst))
            {
                mutex_bargraph.unlock();
                return false;
            }
        }
        else if (mDOrder[i] == ORD_ELEM_BORDER)
        {
            if (!buttonBorder(&imgButton, (type == BARGRAPH ? 0 : inst)))
            {
                mutex_bargraph.unlock();
                return false;
            }

            haveFrame = true;
        }
    }

    if (mGlobalOO >= 0 || sr[inst].oo >= 0) // Take overall opacity into consideration
    {
        SkBitmap ooButton;
        int w = imgButton.width();
        int h = imgButton.height();
        ooButton.allocN32Pixels(w, h, true);
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
    size_t rowBytes = imgButton.info().minRowBytes();

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
            SkCanvas can(imgButton, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
            SkRect rect = SkRect::MakeXYWH(0, 0, width, height);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
            rowBytes = imgButton.info().minRowBytes();
            mLastImage = imgButton;
        }
#endif
        _displayButton(mHandle, parent, (unsigned char *)imgButton.getPixels(), rwidth, rheight, rowBytes, rleft, rtop);
    }

    mutex_bargraph.unlock();
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

    if (gPageManager && gPageManager->getSystemDraw())
    {
        if (gPageManager->getSystemDraw()->existBorder(name))
            return gPageManager->getSystemDraw()->getBorderWidth(name);
    }

    for (int i = 0; sysBorders[i].name != nullptr; i++)
    {
        if (name.compare(sysBorders[i].name) == 0)
        {
            MSG_DEBUG("Border size: " << sysBorders[i].width);
            return sysBorders[i].width;
        }
    }

    MSG_DEBUG("Border size: 0");
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
    {
        pixmapMask = imgMask.pixmap();

        if (pixmapRed.info().width() != pixmapMask.info().width() ||
            pixmapRed.info().height() != pixmapMask.info().height())
        {
            MSG_WARNING("Image and mask have different sizes!");
            haveBothImages = false;
        }
    }
    else
        haveBothImages = false;

    SkBitmap maskBm;
    maskBm.allocPixels(SkImageInfo::MakeN32Premul(width, height));
    maskBm.eraseColor(SK_ColorTRANSPARENT);

    for (int ix = 0; ix < width; ix++)
    {
        for (int iy = 0; iy < height; iy++)
        {
            SkColor pixelRed = pixmapRed.getColor(ix, iy);
            SkColor pixelMask;

            if (haveBothImages && !imgMask.empty())
                pixelMask = pixmapMask.getColor(ix, iy);
            else
                pixelMask = SkColorSetA(SK_ColorWHITE, 0);

            SkColor pixel = baseColor(pixelRed, pixelMask, col1, col2);
            uint32_t alpha = SkColorGetA(pixel);
            uint32_t *wpix = maskBm.getAddr32(ix, iy);

            if (!wpix)
            {
                MSG_ERROR("No pixel buffer!");
                break;
            }

            if (alpha == 0)
                pixel = pixelMask;
            // Skia has a bug and has changed the red and the blue color
            // channel. Therefor we must change this 2 color channels for
            // Linux based OSs here. On Android this is not necessary.
#ifdef __ANDROID__
            *wpix = pixel;
#else   // We've to invert the pixels here to have the correct colors
            uchar red   = SkColorGetR(pixel);   // This is blue in reality
            uchar green = SkColorGetG(pixel);
            uchar blue  = SkColorGetB(pixel);   // This is red in reality
            uchar al    = SkColorGetA(pixel);
            *wpix = SkColorSetARGB(al, blue, green, red);
#endif
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

    Bm.allocN32Pixels(width, height);
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

            // Skia has a bug and has changed the red and the blue color
            // channel. Therefor we must change this 2 color channels for
            // Linux based OSs here. On Android this is not necessary.
#ifdef __ANDROID__
            *bpix = SkColorSetARGB(al, red, green, blue);
#else
            *bpix = SkColorSetARGB(al, blue, green, red);
#endif
        }
    }

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);
    SkCanvas can(Bm);
    sk_sp<SkImage> _image = SkImage::MakeFromBitmap(base);
    can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);
    return Bm;
}

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
    maskBm.allocPixels(SkImageInfo::MakeN32Premul(width, height));
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
                uint32_t blue = SkColorGetR(col);
                uint32_t green = SkColorGetG(col);
                uint32_t red = SkColorGetB(col);

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
//            MSG_DEBUG("Pixel " << ix << ", " << iy << ": #" << std::setw(8) << std::setfill('0') << std::hex << pixelAlpha);
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
        return SkBitmap();

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

    visible = true;
    makeElement();

    if (isSystemButton() && !mSystemReg)
        registerSystemButton();
}

void TButton::showLastButton()
{
    DECL_TRACER("TButton::showLastButton()");

    if (mLastImage.empty())
        return;

    if (!_displayButton && gPageManager)
        _displayButton = gPageManager->getCallbackDB();

    if (!prg_stopped && visible && _displayButton)
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
        _displayButton(mHandle, parent, (unsigned char *)mLastImage.getPixels(), rwidth, rheight, rowBytes, rleft, rtop);
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
        SkBitmap imgButton;
        size_t rowBytes = 0;

        if (rwidth < 0 || rheight < 0)
        {
            MSG_ERROR("Invalid size of image: " << rwidth << " x " << rheight);
            return;
        }

        try
        {
            imgButton.allocN32Pixels(rwidth, rheight);
            imgButton.eraseColor(SK_ColorTRANSPARENT);
            rowBytes = imgButton.info().minRowBytes();
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
            _displayButton(mHandle, parent, (unsigned char *)imgButton.getPixels(), rwidth, rheight, rowBytes, rleft, rtop);
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
    bm->allocN32Pixels(width, height);
    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
    bm->allocN32Pixels(rwidth, info.height());
    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
    bm->allocN32Pixels(info.width(), rheight);
    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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
    bm->allocN32Pixels(rwidth, rheight);
    bm->eraseColor(SK_ColorTRANSPARENT);
    SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
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

    if (gPageManager && !checkForSound() && (ch > 0 || lv > 0 || !pushFunc.empty() || isSystem))
    {
        TSystemSound sysSound(TConfig::getSystemPath(TConfig::SOUNDS));

        if (pressed && gPageManager->havePlaySound() && sysSound.getSystemSoundState())
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
    if (type == GENERAL)
    {
        MSG_DEBUG("Button type: GENERAL");

        if (isSystem && ch == 17)   // Button sounds on/off
        {
            if (pressed)
            {
                MSG_TRACE("System button sounds are toggled ...");
                bool sstate = TConfig::getSystemSoundState();

                if (sstate)
                    mActInstance = instance = 0;
                else
                    mActInstance = instance = 1;

                TConfig::saveSystemSoundState(!sstate);
                TConfig::saveSettings();
                mChanged = true;
                drawButton(mActInstance, false);
                showLastButton();
            }
        }
        else if (isSystem && ch == 73)  // Enter setup page
        {
            if (pressed)
            {
                if (gPageManager && gPageManager->haveSetupPage())
                    gPageManager->callSetupPage();
            }
        }
        else if (isSystem && ch == 80)  // Shutdown program
        {
            if (pressed)
            {
                if (gPageManager && gPageManager->haveShutdown())
                    gPageManager->callShutdown();
            }
        }
        else if (isSystem && ch == 171)     // System volume up
        {
            int vol = TConfig::getSystemVolume() + 10;

            if (vol > 100)
                vol = 100;

            TConfig::saveSystemVolume(vol);

            if (pressed)
                mActInstance = instance = 1;
            else
                mActInstance = instance = 0;

            mChanged = true;
            drawButton(mActInstance, true);
            int channel = TConfig::getChannel();
            int system = TConfig::getSystem();

            if (gPageManager)
            {
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
        else if (isSystem && ch == 172)     // System volume down
        {
            int vol = TConfig::getSystemVolume() - 10;

            if (vol < 0)
                vol = 0;

            TConfig::saveSystemVolume(vol);

            if (pressed)
                mActInstance = instance = 1;
            else
                mActInstance = instance = 0;

            mChanged = true;
            drawButton(mActInstance, true);
            int channel = TConfig::getChannel();
            int system = TConfig::getSystem();

            if (gPageManager)
            {
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
        else if (isSystem && ch == 173)     // System mute
        {
            if (pressed)
            {
                bool mute = TConfig::getMuteState();

                if (mute)
                    mActInstance = instance = 0;
                else
                    mActInstance = instance = 1;

                TConfig::setMuteState(!mute);

                if (gPageManager && gPageManager->getCallMuteSound())
                    gPageManager->getCallMuteSound()(!mute);

                mChanged = true;
                drawButton(mActInstance, false);
                showLastButton();
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

            MSG_DEBUG("Button " << bi << ", " << na << " with handle " << TObject::handleToString(mHandle));
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

            MSG_DEBUG("Button " << bi << ", " << na << " with handle " << TObject::handleToString(mHandle));
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
        if (isSystemButton() && lv == 9)    // System volume button
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
        if (lp && lv)
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

                        if (settings->getPowerUpPage().compare(page->getName()) != 0)
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
            cmd.port1 = ap;
            cmd.system = system;
            cmd.data.message_string.device = channel;
            cmd.data.message_string.port = ap;  // Must be the address port of button
            cmd.data.message_string.system = system;
            cmd.data.message_string.type = 1;   // 8 bit char string

            vector<string>::iterator iter;

            for (iter = cm.begin(); iter != cm.end(); ++iter)
            {
                cmd.data.message_string.length = iter->length();
                memset(&cmd.data.message_string.content, 0, sizeof(cmd.data.message_string.content));
                strncpy((char *)&cmd.data.message_string.content, iter->c_str(), sizeof(cmd.data.message_string.content));
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
#ifndef __ANDROID__
    uint red = SkColorGetR(basePix);
#else
    uint red = SkColorGetB(basePix);
#endif

    if (alpha == 0)
        return maskPix;

    if (red && green)
    {
        return col1;
/*        uint32_t newR = (SkColorGetR(col1) + SkColorGetR(col2)) / 2;
        uint32_t newG = (SkColorGetG(col1) + SkColorGetG(col2)) / 2;
        uint32_t newB = (SkColorGetB(col1) + SkColorGetB(col2)) / 2;
        uint32_t newA = (SkColorGetA(col1) + SkColorGetA(col2)) / 2;
        return SkColorSetARGB(newA, newR, newG, newB); */
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

    int i = 0;

    while (sysButtons[i].channel)
    {
        if (sysButtons[i].type == MULTISTATE_BARGRAPH && lp == 0 && lv == sysButtons[i].channel)
            return true;
        else if (sysButtons[i].type == BARGRAPH && lp == 0 && lv == sysButtons[i].channel)
            return true;
        else if (ap == 0 && ad == sysButtons[i].channel)
            return true;
        else if (cp == 0 && ch == sysButtons[i].channel)
            return true;

        i++;
    }

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
                    _displayButton(iter->handle, iter->parent, (unsigned char *)iter->bitmap.getPixels(), iter->width, iter->height, iter->bitmap.info().minRowBytes(), iter->left, iter->top);
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

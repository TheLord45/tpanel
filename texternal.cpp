/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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

#include "texternal.h"
#include "terror.h"
#include "tconfig.h"
#include "texpat++.h"

using std::string;
using std::vector;
using namespace Expat;

TExternal::TExternal()
{
    DECL_TRACER("TExternal::TExternal()");

    makeFileName(TConfig::getProjectPath(), "external.xma");

    if (!isValidFile())
    {
        TError::setErrorMsg(TERRERROR, "Invalid file name " + getFileName());
        return;
    }

    mFileName = getFileName();
    initialize();
}

void TExternal::setFileName(const string &fn)
{
    DECL_TRACER("TExternal::setFileName(const string &fn)");

    makeFileName(TConfig::getProjectPath(), fn);

    if (!isValidFile())
    {
        TError::setErrorMsg(TERRERROR, "Invalid file name " + getFileName());
        return;
    }

    mFileName = getFileName();
    initialize();

}

void TExternal::initialize()
{
    DECL_TRACER("TExternal::initialize()");

    TExpat xml(mFileName);
    xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return;

    int depth = 0;
    size_t index = 0;
    size_t oldIndex = 0;

    if ((index = xml.getElementIndex("externalButtons", &depth)) == TExpat::npos)
    {
        MSG_ERROR("Element \"externalButtons\" was not found!");
        TError::setError();
        return;
    }

    EXTPAGE_t page;
    depth++;

    while ((index = xml.getNextElementIndex("page", depth)) != TExpat::npos)
    {
        string name, content;
        vector<ATTRIBUTE_t> attrs;

        while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
        {
            string e = name;

            if (e.compare("pageID") == 0)
                page.pageID = xml.convertElementToInt(content);
            else if (e.compare("name") == 0)
                page.name = content;
            else if (e.compare("button") == 0)
            {
                EXTBUTTON_t button;
                string atype = xml.getAttribute("type", attrs);

                if (!atype.empty() && atype.compare("external") != 0)
                {
                    MSG_WARNING("Found unknown button type " << atype << "!");
                    continue;
                }

                while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
                {
                    if (name.compare("bi") == 0)
                        button.bi = xml.convertElementToInt(content);
                    else if (name.compare("bc") == 0)
                    {
                        button.bc = content;

                        if (button.bc.compare("cursorLeft") == 0)
                            button.type = EXT_CURSOR_LEFT;
                        else if (button.bc.compare("cursorRight") == 0)
                            button.type = EXT_CURSOR_RIGHT;
                        else if (button.bc.compare("cursorUp") == 0)
                            button.type = EXT_CURSOR_UP;
                        else if (button.bc.compare("cursorDown") == 0)
                            button.type = EXT_CURSOR_DOWN;
                        else if (button.bc.compare("cursorSel") == 0)
                            button.type = EXT_CURSOR_SELECT;
                        else if (button.bc.compare("cursorRotateRight") == 0)
                            button.type = EXT_CURSOR_ROTATE_RIGHT;
                        else if (button.bc.compare("cursorRotateLeft") == 0)
                            button.type = EXT_CURSOR_ROTATE_LEFT;
                        else if (button.bc.compare("gestureLeft") == 0)
                            button.type = EXT_GESTURE_LEFT;
                        else if (button.bc.compare("gestureRight") == 0)
                            button.type = EXT_GESTURE_RIGHT;
                        else if (button.bc.compare("gestureUp") == 0)
                            button.type = EXT_GESTURE_UP;
                        else if (button.bc.compare("gestureDown") == 0)
                            button.type = EXT_GESTURE_DOWN;
                        else if (button.bc.compare("gestureRotateLeft") == 0)
                            button.type = EXT_GESTURE_ROTATE_LEFT;
                        else if (button.bc.compare("gestureRotateRight") == 0)
                            button.type = EXT_GESTURE_ROTATE_RIGHT;
                        else if (button.bc.compare("gestureDoublePress") == 0)
                            button.type = EXT_GESTURE_DOUBLE_PRESS;
                        else if (button.bc.compare("general") == 0)
                            button.type = EXT_GENERAL;
                    }
                    else if (name.compare("na") == 0)
                        button.na = content;
                    else if (name.compare("da") == 0)
                        button.da = xml.convertElementToInt(content);
                    else if (name.compare("pp") == 0)
                        button.pp = content;
                    else if (name.compare("ap") == 0)
                        button.ap = xml.convertElementToInt(content);
                    else if (name.compare("ad") == 0)
                        button.ad = xml.convertElementToInt(content);
                    else if (name.compare("cp") == 0)
                        button.cp = xml.convertElementToInt(content);
                    else if (name.compare("ch") == 0)
                        button.ch = xml.convertElementToInt(content);
                    else if (name.compare("rt") == 0)
                        button.rt = content;
                    else if (name.compare("vt") == 0)
                        button.vt = content;
                    else if (name.compare("lp") == 0)
                        button.lp = xml.convertElementToInt(content);
                    else if (name.compare("lv") == 0)
                        button.lv = xml.convertElementToInt(content);
                    else if (name.compare("va") == 0)
                        button.va = xml.convertElementToInt(content);
                    else if (name.compare("rv") == 0)
                        button.rv = xml.convertElementToInt(content);
                    else if (name.compare("rl") == 0)
                        button.rl = xml.convertElementToInt(content);
                    else if (name.compare("rh") == 0)
                        button.rh = xml.convertElementToInt(content);
                    else if (name.compare("lu") == 0)
                        button.lu = xml.convertElementToInt(content);
                    else if (name.compare("ld") == 0)
                        button.ld = xml.convertElementToInt(content);
                    else if (name.compare("so") == 0)
                        button.so = xml.convertElementToInt(content);
                    else if (name.compare("co") == 0)
                        button.co = xml.convertElementToInt(content);
                    else if (name.compare("cm") == 0)
                        button.cm.push_back(content);
                    else if (name.compare("ac") == 0)
                    {
                        button.ac = content;
                        button.ac_de = xml.getAttributeInt("de", attrs);
                    }
                    else if (name.compare("at") == 0)
                        button.at = xml.convertElementToInt(content);

                    oldIndex = index;
                }

                page.buttons.push_back(button);
            }

            mPages.push_back(page);
            page.buttons.clear();

            if (index == TExpat::npos)
                index = oldIndex + 1;
            else
                oldIndex = index;
        }

        if (index == TExpat::npos)
            index = oldIndex + 1;
        else
            oldIndex = index;

        xml.setIndex(index);
    }
/*
    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        vector<EXTPAGE_t>::iterator iter;

        for (iter = mPages.begin(); iter != mPages.end(); ++iter)
        {
            MSG_DEBUG("Name  : " << iter->name);
            MSG_DEBUG("pageID: " << iter->pageID);

            vector<EXTBUTTON_t>::iterator ibut;

            for (ibut = iter->buttons.begin(); ibut != iter->buttons.end(); ++ibut)
            {
                MSG_DEBUG("   Button index: " << ibut->bi);
                MSG_DEBUG("   Button name : " << ibut->na);
                MSG_DEBUG("   Button bc   : " << ibut->bc);
            }
        }
    }
*/
}

EXTBUTTON_t TExternal::getButton(extButtons_t bt)
{
    DECL_TRACER("TExternal::getButton(extButtons_t bt)");

    EXTBUTTON_t empty;

    if (mPages.size() == 0)
    {
        MSG_DEBUG("No pages found.");
        return empty;
    }

    vector<EXTPAGE_t>::iterator piter;

    for (piter = mPages.begin(); piter != mPages.end(); ++piter)
    {
        MSG_DEBUG("Found page " << piter->name);

        if (piter->buttons.size() == 0)
            continue;

        vector<EXTBUTTON_t>::iterator btIter;

        for (btIter = piter->buttons.begin(); btIter != piter->buttons.end(); ++btIter)
        {
            MSG_DEBUG("Found button " << btIter->na << ", type=" << btIter->type << " (" << bt << ")");

            if (btIter->type == bt || (!mStrict && btIter->type == findCompatibel(bt)))
                return *btIter;
        }
    }

    return empty;
}

EXTBUTTON_t TExternal::getButton(int pageId, extButtons_t bt)
{
    DECL_TRACER("TExternal::getButton(int pageId, extButtons_t bt)");

    EXTBUTTON_t empty;

    if (mPages.size() == 0)
        return empty;

    vector<EXTPAGE_t>::iterator piter;

    for (piter = mPages.begin(); piter != mPages.end(); ++piter)
    {
        if (piter->pageID != pageId)
            continue;

        if (piter->buttons.size() == 0)
            continue;

        vector<EXTBUTTON_t>::iterator btIter;

        for (btIter = piter->buttons.begin(); btIter != piter->buttons.end(); ++btIter)
        {
            if (btIter->type == bt || (!mStrict && btIter->type == findCompatibel(bt)))
                return *btIter;
        }
    }

    return empty;
}

extButtons_t TExternal::findCompatibel(extButtons_t bt)
{
    DECL_TRACER("TExternal::findCompatibel(extButtons_t bt)");

    switch(bt)
    {
        case EXT_CURSOR_DOWN:           return EXT_GESTURE_DOWN;
        case EXT_CURSOR_LEFT:           return EXT_GESTURE_LEFT;
        case EXT_CURSOR_RIGHT:          return EXT_GESTURE_RIGHT;
        case EXT_CURSOR_UP:             return EXT_GESTURE_UP;
        case EXT_CURSOR_ROTATE_LEFT:    return EXT_GESTURE_ROTATE_LEFT;
        case EXT_CURSOR_ROTATE_RIGHT:   return EXT_GESTURE_ROTATE_RIGHT;
        case EXT_CURSOR_SELECT:         return EXT_GESTURE_DOUBLE_PRESS;

        case EXT_GESTURE_DOWN:          return EXT_CURSOR_DOWN;
        case EXT_GESTURE_RIGHT:         return EXT_CURSOR_RIGHT;
        case EXT_GESTURE_UP:            return EXT_CURSOR_UP;
        case EXT_GESTURE_LEFT:          return EXT_CURSOR_LEFT;
        case EXT_GESTURE_ROTATE_LEFT:   return EXT_CURSOR_ROTATE_LEFT;
        case EXT_GESTURE_ROTATE_RIGHT:  return EXT_CURSOR_ROTATE_RIGHT;
        case EXT_GESTURE_DOUBLE_PRESS:  return EXT_CURSOR_SELECT;

        default:
            return bt;
    }
}

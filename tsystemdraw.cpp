/*
 * Copyright (C) 2021, 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include <fstream>
#include <functional>

#include <expat.h>

#include "tsystemdraw.h"
#include "tdirectory.h"
#include "tresources.h"
#include "terror.h"

using std::string;
using std::vector;
using std::ifstream;

TSystemDraw::XELEMENTS_t TSystemDraw::mActData = TSystemDraw::X_NONE;
TSystemDraw::XELEMENTS_t TSystemDraw::mActFamily = TSystemDraw::X_NONE;
string TSystemDraw::mActElement;

TSystemDraw::TSystemDraw(const string &path)
    : mPath(path)
{
    DECL_TRACER("TSystemDraw::TSystemDraw(const string &path)");

    if (isValidDir(path))
        mValid = true;
    else
    {
        MSG_WARNING("No or invalid path!");
        return;
    }

    if (isValidDir(path + "/borders"))
        mHaveBorders = true;
    else
    {
        MSG_WARNING("Have no system border images");
    }

    if (isValidDir(path + "/cursors"))
        mHaveCursors = true;
    else
    {
        MSG_WARNING("Have no system cursor images");
    }

    if (isValidDir(path + "/fonts"))
        mHaveFonts = true;
    {
        MSG_WARNING("Have no system fonts");
        string perms = getPermissions(path);
        MSG_PROTOCOL("Looked for system fonts at: " << path << "/fonts -- [" << perms << "]");
    }

    if (isValidDir(path + "/images"))
        mHaveImages = true;
    else
    {
        MSG_WARNING("Have no system images");
    }

    if (isValidDir(path + "/sliders"))
        mHaveSliders = true;
    else
    {
        MSG_WARNING("Have no system slider images");
    }

    if (isValidFile(path + "/draw.xma"))
        loadConfig();
    else
    {
        MSG_WARNING("Have no system configuration file draw.xma!");
    }
}

TSystemDraw::~TSystemDraw()
{
    DECL_TRACER("TSystemDraw::~TSystemDraw()");
}

bool TSystemDraw::loadConfig()
{
    DECL_TRACER("TSystemDraw::loadConfig()");

    string buf;
    string file = mPath + "/draw.xma";
    size_t size = 0;

    // First we read the whole XML file into a buffer
    try
    {
        ifstream stream(file, std::ios::in);

        if (!stream || !stream.is_open())
            return false;

        stream.seekg(0, stream.end);    // Find the end of the file
        size = stream.tellg();          // Get the position and save it
        stream.seekg(0, stream.beg);    // rewind to the beginning of the file

        buf.resize(size, '\0');         // Initialize the buffer with zeros
        char *begin = &*buf.begin();    // Assign the plain data buffer
        stream.read(begin, size);       // Read the whole file
        stream.close();                 // Close the file
    }
    catch (std::exception& e)
    {
        MSG_ERROR("File error: " << e.what());
        return false;
    }

    // Now we parse the file and write the relevant contents into our internal
    // variables.
    // First we initialialize the parser.
    int depth = 0;
    int done = 1;   // 1 = Buffer is complete
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &depth);
    XML_SetElementHandler(parser, &TSystemDraw::startElement, &TSystemDraw::endElement);
    XML_SetCharacterDataHandler(parser, &TSystemDraw::CharacterDataHandler);
    XML_SetUserData(parser, &mDraw);

    if (XML_Parse(parser, buf.data(), size, done) == XML_STATUS_ERROR)
    {
        MSG_ERROR(XML_ErrorString(XML_GetErrorCode(parser)) << " at line " << XML_GetCurrentLineNumber(parser));
        XML_ParserFree(parser);
        return false;
    }

    XML_ParserFree(parser);
/*
    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        for (size_t i = 0; i < mDraw.borders.size(); i++)
        {
            MSG_DEBUG("Border family: " << mDraw.borders.at(i).name);

            for (size_t j = 0; j < mDraw.borders.at(i).member.size(); j++)
            {
                MSG_DEBUG("       Member: " << mDraw.borders.at(i).member.at(j));
                MSG_DEBUG("Border styles:");

                for (size_t x = 0; x < mDraw.borderStyles.size(); x++)
                {
                    if (mDraw.borderStyles.at(x).name.compare(mDraw.borders.at(i).member.at(j)) == 0)
                    {
                        MSG_DEBUG("         Name: " << mDraw.borderStyles.at(x).name);
                        MSG_DEBUG("          Off: " << mDraw.borderStyles.at(x).off);
                        MSG_DEBUG("           On: " << mDraw.borderStyles.at(x).on);
                        MSG_DEBUG("         Drag: " << mDraw.borderStyles.at(x).drag);
                        MSG_DEBUG("         Drop: " << mDraw.borderStyles.at(x).drop);

                        if (mDraw.borderStyles.at(x).g3Equiv.size() > 0)
                        {
                            for (size_t y = 0; y < mDraw.borderStyles.at(x).g3Equiv.size(); y++)
                                MSG_DEBUG("      g3Equiv: " << mDraw.borderStyles.at(x).g3Equiv.at(y));
                        }

                        vector<BORDER_DATA_t>::iterator bdIter;

                        for (bdIter = mDraw.borderData.begin(); bdIter != mDraw.borderData.end(); ++bdIter)
                        {
                            if (bdIter->name.compare(mDraw.borderStyles.at(x).name) == 0)
                            {
                                MSG_DEBUG("           Name: " << bdIter->name);
                                MSG_DEBUG("       baseFile: " << bdIter->baseFile);
                                MSG_DEBUG("     idealWidth: " << bdIter->idealWidth);
                                MSG_DEBUG("    idealHeight: " << bdIter->idealHeight);
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (size_t i = 0; i < mDraw.sliders.size(); i++)
        {
            MSG_DEBUG("Slider family: " << mDraw.sliders.at(i).name);

            for (size_t j = 0; j < mDraw.sliders.at(i).member.size(); j++)
            {
                MSG_DEBUG("       Member: " << mDraw.sliders.at(i).member.at(j));
                MSG_DEBUG("Slider styles:");

                for (size_t x = 0; x < mDraw.sliderStyles.size(); x++)
                {
                    if (mDraw.sliderStyles.at(x).name.compare(mDraw.sliders.at(i).member.at(j)) == 0)
                    {
                        MSG_DEBUG("         Name: " << mDraw.sliderStyles.at(x).name);
                        MSG_DEBUG("     baseFile: " << mDraw.sliderStyles.at(x).baseFile);
                        MSG_DEBUG("   multiColor: " << mDraw.sliderStyles.at(x).multiColor);
                        MSG_DEBUG("    incRepeat: " << mDraw.sliderStyles.at(x).incRepeat);
                        MSG_DEBUG("      minSize: " << mDraw.sliderStyles.at(x).minSize);
                        MSG_DEBUG("    fixedSize: " << mDraw.sliderStyles.at(x).fixedSize);
                    }
                }
            }
        }
    }
*/
    return true;
}

void TSystemDraw::startElement(void *, const XML_Char *name, const XML_Char **)
{
    mActElement.assign(name);

    if (strCaseCompare(name, "borderData") == 0)
        mActData = X_BORDER_DATA;

    if (strCaseCompare(name, "border") == 0)
        mActFamily = X_BORDER;

    if (strCaseCompare(name, "borderFamily") == 0)
        mActFamily = X_BORDER_FAMILY;

    if (strCaseCompare(name, "borderStyle") == 0)
        mActFamily = X_BORDER_STYLE;

    if (strCaseCompare(name, "cursorData") == 0)
        mActData = X_CURSOR_DATA;

    if (strCaseCompare(name, "cursorFamily") == 0)
        mActFamily = X_CURSOR_FAMILY;

    if (strCaseCompare(name, "cursorStyle") == 0)
        mActFamily = X_CURSOR_STYLE;

    if (strCaseCompare(name, "sliderData") == 0)
        mActData = X_SLIDER_DATA;

    if (strCaseCompare(name, "sliderFamily") == 0)
        mActFamily = X_SLIDER_FAMILY;

    if (strCaseCompare(name, "slider") == 0)
        mActFamily = X_SLIDER_STYLE;

    if (strCaseCompare(name, "effectData") == 0)
        mActData = X_EFFECT_DATA;

    if (strCaseCompare(name, "effectFamily") == 0)
        mActFamily = X_EFFECT_FAMILY;

    if (strCaseCompare(name, "effect") == 0)
        mActFamily = X_EFFECT_STYLE;

    if (strCaseCompare(name, "popupEffectData") == 0)
        mActData = X_POPUP_EFFECT_DATA;

    if (strCaseCompare(name, "popupEffect") == 0)
        mActFamily = X_POPUP_EFFECT;
}

void TSystemDraw::endElement(void *, const XML_Char *name)
{
    if (strCaseCompare(name, "borderData") == 0)
        mActData = X_NONE;

    if (strCaseCompare(name, "border") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "borderFamily") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "borderStyle") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "cursorData") == 0)
        mActData = X_NONE;

    if (strCaseCompare(name, "cursorFamily") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "cursorStyle") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "sliderData") == 0)
        mActData = X_NONE;

    if (strCaseCompare(name, "sliderFamily") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "sliderStyle") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "effectData") == 0)
        mActData = X_NONE;

    if (strCaseCompare(name, "effectFamily") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "effect") == 0)
        mActFamily = X_NONE;

    if (strCaseCompare(name, "popupEffectData") == 0)
        mActData = X_NONE;

    if (strCaseCompare(name, "popupEffect") == 0)
        mActFamily = X_NONE;
}

void TSystemDraw::CharacterDataHandler(void *userData, const XML_Char *s, int len)
{
    if (len <= 0 || !userData || !s || mActData == X_NONE || mActFamily == X_NONE)
        return;

    DRAW_t *draw = (DRAW_t *)userData;
    string content(s, len);
    content = trim(content);

    if (content.empty())
        return;

    switch(mActData)
    {
        case X_BORDER_DATA:
            switch(mActFamily)
            {
                case X_BORDER_FAMILY:
                    if (mActElement.compare("name") == 0)
                    {
                        FAMILY_t bf;
                        bf.name = content;
                        draw->borders.push_back(bf);
                    }
                    else if (mActElement.compare("member") == 0)
                        draw->borders.back().member.push_back(content);
                break;

                case X_BORDER_STYLE:
                    if (mActElement.compare("name") == 0)
                    {
                        BORDER_STYLE_t bs;
                        bs.name = content;
                        draw->borderStyles.push_back(bs);
                    }
                    else if (mActElement.compare("off") == 0)
                        draw->borderStyles.back().off = content;
                    else if (mActElement.compare("on") == 0)
                        draw->borderStyles.back().on = content;
                    else if (mActElement.compare("drag") == 0)
                        draw->borderStyles.back().drag = content;
                    else if (mActElement.compare("drop") == 0)
                        draw->borderStyles.back().drop = content;
                    else if (mActElement.compare("g3Equiv") == 0)
                        draw->borderStyles.back().g3Equiv.push_back(atoi(content.c_str()));
                break;

                case X_BORDER:
                    if (mActElement.compare("name") == 0)
                    {
                        BORDER_DATA_t bd;
                        bd.init();
                        bd.name = content;
                        draw->borderData.push_back(bd);
                    }
                    else if (mActElement.compare("baseFile") == 0)
                        draw->borderData.back().baseFile = content;
                    else if (mActElement.compare("multiColor") == 0)
                        draw->borderData.back().multiColor = atoi(content.c_str());
                    else if (mActElement.compare("fillTop") == 0)
                        draw->borderData.back().fillTop = atoi(content.c_str());
                    else if (mActElement.compare("fillLeft") == 0)
                        draw->borderData.back().fillLeft = atoi(content.c_str());
                    else if (mActElement.compare("fillBottom") == 0)
                        draw->borderData.back().fillBottom = atoi(content.c_str());
                    else if (mActElement.compare("fillRight") == 0)
                        draw->borderData.back().fillRight = atoi(content.c_str());
                    else if (mActElement.compare("textTop") == 0)
                        draw->borderData.back().textTop = atoi(content.c_str());
                    else if (mActElement.compare("textLeft") == 0)
                        draw->borderData.back().textLeft = atoi(content.c_str());
                    else if (mActElement.compare("textBottom") == 0)
                        draw->borderData.back().textBottom = atoi(content.c_str());
                    else if (mActElement.compare("textRight") == 0)
                        draw->borderData.back().textRight = atoi(content.c_str());
                    else if (mActElement.compare("idealWidth") == 0)
                        draw->borderData.back().idealWidth = atoi(content.c_str());
                    else if (mActElement.compare("idealHeight") == 0)
                        draw->borderData.back().idealHeight = atoi(content.c_str());
                    else if (mActElement.compare("minHeight") == 0)
                        draw->borderData.back().minHeight = atoi(content.c_str());
                    else if (mActElement.compare("minWidth") == 0)
                        draw->borderData.back().minWidth = atoi(content.c_str());
                    else if (mActElement.compare("incHeight") == 0)
                        draw->borderData.back().incHeight = atoi(content.c_str());
                    else if (mActElement.compare("incWidth") == 0)
                        draw->borderData.back().incWidth = atoi(content.c_str());
                break;

                default:
                    MSG_WARNING("Unknown border element \"" << mActElement << "\" with content \"" << content << "\"");
            }
        break;

        case X_CURSOR_DATA:
            switch(mActFamily)
            {
                case X_CURSOR_FAMILY:
                    if (mActElement.compare("name") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        FAMILY_t cf;
                        cf.name = content;
                        draw->cursors.push_back(cf);
                    }
                    else if (mActElement.compare("member") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->cursors.back().member.push_back(content);
                    }
                break;

                case X_CURSOR_STYLE:
                    if (mActElement.compare("name") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        CURSOR_STYLE_t cs;
                        cs.init();
                        cs.name = content;
                        draw->cursorStyles.push_back(cs);
                    }
                    else if (mActElement.compare("baseFile") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->cursorStyles.back().baseFile = content;
                    }
                    else if (mActElement.compare("multiColor") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->cursorStyles.back().multiColor = atoi(content.c_str());
                    }
                    else if (mActElement.compare("g3Equiv") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->cursorStyles.back().g3Equiv.push_back(atoi(content.c_str()));
                    }
                break;

                default:
                    MSG_WARNING("Unknown cursor element \"" << mActElement << "\" with content \"" << content << "\"");
            }
        break;

        case X_SLIDER_DATA:
            switch(mActFamily)
            {
                case X_SLIDER_FAMILY:
                    if (mActElement.compare("name") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        FAMILY_t sf;
                        sf.name = content;
                        draw->sliders.push_back(sf);
                    }
                    else if (mActElement.compare("member") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->sliders.back().member.push_back(content);
                    }
                break;

                case X_SLIDER_STYLE:
                    if (mActElement.compare("name") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        SLIDER_STYLE_t ss;
                        ss.init();
                        ss.name = content;
                        draw->sliderStyles.push_back(ss);
                    }
                    else if (mActElement.compare("baseFile") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->sliderStyles.back().baseFile = content;
                    }
                    else if (mActElement.compare("multiColor") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->sliderStyles.back().multiColor = atoi(content.c_str());
                    }
                    else if (mActElement.compare("incRepeat") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->sliderStyles.back().incRepeat = atoi(content.c_str());
                    }
                    else if (mActElement.compare("minSize") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->sliderStyles.back().minSize = atoi(content.c_str());
                    }
                    else if (mActElement.compare("fixedSize") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->sliderStyles.back().fixedSize = atoi(content.c_str());
                    }
                    else if (mActElement.compare("g3Equiv") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->sliderStyles.back().g3Equiv.push_back(atoi(content.c_str()));
                    }
                break;

                default:
                    MSG_WARNING("Unknown slider element \"" << mActElement << "\" with content \"" << content << "\"");
            }
        break;

        case X_EFFECT_DATA:
            switch(mActFamily)
            {
                case X_EFFECT_FAMILY:
                    if (mActElement.compare("name") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        FAMILY_t ef;
                        ef.name = content;
                        draw->effects.push_back(ef);
                    }
                    else if (mActElement.compare("member") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effects.back().member.push_back(content);
                    }
                break;

                case X_EFFECT_STYLE:
                    if (mActElement.compare("name") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        EFFECT_STYLE_t es;
                        es.init();
                        es.name = content;
                        draw->effectStyles.push_back(es);
                    }
                    else if (mActElement.compare("number") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effectStyles.back().number = atoi(content.c_str());
                    }
                    else if (mActElement.compare("startX") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effectStyles.back().startx = atoi(content.c_str());
                    }
                    else if (mActElement.compare("startY") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effectStyles.back().starty = atoi(content.c_str());
                    }
                    else if (mActElement.compare("height") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effectStyles.back().height = atoi(content.c_str());
                    }
                    else if (mActElement.compare("width") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effectStyles.back().width = atoi(content.c_str());
                    }
                    else if (mActElement.compare("cutout") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effectStyles.back().cutout = atoi(content.c_str());
                    }
                    else if (mActElement.compare("pixelMap") == 0)
                    {
                        DRAW_t *draw = (DRAW_t *)userData;
                        draw->effectStyles.back().pixelMap = content;
                    }
                break;

                default:
                    MSG_WARNING("Unknown effect element \"" << mActElement << "\" with content \"" << content << "\"");
            }
        break;

        case X_POPUP_EFFECT_DATA:
            if (mActFamily == X_POPUP_EFFECT)
            {
                if (mActElement.compare("name") == 0)
                {
                    DRAW_t *draw = (DRAW_t *)userData;
                    POPUP_EFFECT_t pe;
                    pe.init();
                    pe.name = content;
                    draw->popupEffects.push_back(pe);
                }
                else if (mActElement.compare("number") == 0)
                {
                    DRAW_t *draw = (DRAW_t *)userData;
                    draw->popupEffects.back().number = atoi(content.c_str());
                }
                else if (mActElement.compare("valueUsed") == 0)
                {
                    DRAW_t *draw = (DRAW_t *)userData;
                    draw->popupEffects.back().valueUsed = atoi(content.c_str());
                }
            }
        break;

        default:
            MSG_WARNING("Unknown data element \"" << mActElement << "\" with content \"" << content << "\"");
    }
}

string TSystemDraw::getDirEntry(dir::TDirectory* dir, const string& part, bool alpha)
{
    DECL_TRACER("TSystemDraw::getDirEntry(dir::TDirectory* dir, const string& part, bool precice)");

    string such = part;

    if (alpha)
        such += "_alpha";

    string dirEntry = dir->getEntryWithPart(such, true);
    such = part;

    if (dirEntry.empty())
        dirEntry = dir->getEntryWithPart(such, false);

    return dirEntry;
}

bool TSystemDraw::getBorder(const string &family, LINE_TYPE_t lt, BORDER_t *border, const string& family2)
{
    DECL_TRACER("TSystemDraw::getBorder(const string &family, LINE_TYPE_t lt, BORDER_t *border, const string& family2)");

    if (!border || family.empty() || mDraw.borders.size() == 0)
        return false;

    // Find the border details
    vector<FAMILY_t>::iterator iter;
    bool found = false;
    string fullName;

    for (iter = mDraw.borders.begin(); iter != mDraw.borders.end(); ++iter)
    {
        vector<string>::iterator strIter;

        for (strIter = iter->member.begin(); strIter != iter->member.end(); ++strIter)
        {
            /*
             * In the table we've the "family" name in the first place while in
             * the configuration file for a page/subpage we have the whole name.
             * Because of this we must look if the whole name starts with the
             * family name. If so, we've found what we're looking for.
             * Example:
             * Whole name in the XML configuration: AMX Elite Raised -M
             * Family name: AMX Elite
             * Organization:
             * The entity "borderFamily" defines the family name (AMX Elite). A
             * family has members: "AMX Elite -S", "AMX Elite -M" and "AMX Elite -L"
             * in our example.
             * The entity "borderStyle" is named like the member. With this
             * information we'll find the correct border style and with it the
             * file names containing the brorder graphics.
             */
            vector<string>parts = StrSplit(*strIter, " ", true);

            if (evaluateName(parts, family))   // Here we find the wanted member
            {
                // Find the detailed name
                vector<BORDER_STYLE_t>::iterator styIter;
                // Here we search in the style table for an element with the
                // found member name
                for (styIter = mDraw.borderStyles.begin(); styIter != mDraw.borderStyles.end(); ++styIter)
                {
                    if (styIter->name.compare(*strIter) == 0)
                    {
                        found = true;
                        border->bdStyle = *styIter;

                        switch(lt)
                        {
                            case LT_OFF:    fullName = styIter->off; break;
                            case LT_ON:     fullName = styIter->on; break;
                            case LT_DRAG:   fullName = styIter->drag; break;
                            case LT_DROP:   fullName = styIter->drop; break;
                        }


                        break;
                    }
                }
            }

            if (found)
                break;
        }

        if (found)
            break;
    }

    if (!found || fullName.empty())
    {
        MSG_WARNING("Border " << family << " not found!");
        return false;
    }

    dir::TDirectory dir(mPath + "/borders");
    dir.setStripPath(true);
    vector<BORDER_DATA_t>::iterator brdIter;
    string dataName = (family2.length() > 0 ? family2 : fullName);

    for (brdIter = mDraw.borderData.begin(); brdIter != mDraw.borderData.end(); brdIter++)
    {
        if (brdIter->name.compare(dataName) == 0)
        {
            int num = dir.scanFiles(brdIter->baseFile + "_");

            if (num < 8)
                continue;

            border->b = mPath + "/borders/" + getDirEntry(&dir, "_b", (lt == LT_ON) ? true : false);
            border->bl = mPath + "/borders/" + getDirEntry(&dir, "_bl", (lt == LT_ON) ? true : false);
            border->br = mPath + "/borders/" + getDirEntry(&dir, "_br", (lt == LT_ON) ? true : false);
            border->l = mPath + "/borders/" + getDirEntry(&dir, "_l", (lt == LT_ON) ? true : false);
            border->r = mPath + "/borders/" + getDirEntry(&dir, "_r", (lt == LT_ON) ? true : false);
            border->t = mPath + "/borders/" + getDirEntry(&dir, "_t", (lt == LT_ON) ? true : false);
            border->tl = mPath + "/borders/" + getDirEntry(&dir, "_tl", (lt == LT_ON) ? true : false);
            border->tr = mPath + "/borders/" + getDirEntry(&dir, "_tr", (lt == LT_ON) ? true : false);
            border->border = *brdIter;
            MSG_DEBUG("Bottom      : " << border->b);
            MSG_DEBUG("Top         : " << border->t);
            MSG_DEBUG("Left        : " << border->l);
            MSG_DEBUG("Right       : " << border->r);
            MSG_DEBUG("Top left    : " << border->tl);
            MSG_DEBUG("Top right   : " << border->tr);
            MSG_DEBUG("Bottom left : " << border->bl);
            MSG_DEBUG("Bottom right: " << border->br);
            return true;
        }
    }

    return false;
}

bool TSystemDraw::existBorder(const string &family)
{
    DECL_TRACER("TSystemDraw::existBorder(const string &family)");

    if (family.empty() || mDraw.borders.size() == 0)
        return false;

    // Find the border details
    vector<FAMILY_t>::iterator iter;

    for (iter = mDraw.borders.begin(); iter != mDraw.borders.end(); ++iter)
    {
        vector<string>::iterator strIter;

        for (strIter = iter->member.begin(); strIter != iter->member.end(); ++strIter)
        {
            vector<string>parts = StrSplit(*strIter, " ", true);

            if (evaluateName(parts, family))
            {
                // Find the detailed name
                vector<BORDER_STYLE_t>::iterator styIter;

                for (styIter = mDraw.borderStyles.begin(); styIter != mDraw.borderStyles.end(); ++styIter)
                {
                    if (styIter->name.compare(*strIter) == 0)
                        return true;
                }
            }
        }
    }

    return false;
}

int TSystemDraw::getBorderWidth(const string &family, LINE_TYPE_t lt)
{
    DECL_TRACER("TSystemDraw::getBorderWidth(const string &family, LINE_TYPE_t lt)");

    if (family.empty())
        return 0;

    BORDER_t bd;

    if (!getBorder(family, lt, &bd))
        return 0;

    MSG_DEBUG("Border width of \"" << family << "\" [" << lt << "]: " << bd.border.textLeft);
    return bd.border.textLeft;
}

int TSystemDraw::getBorderHeight(const string &family, LINE_TYPE_t lt)
{
    DECL_TRACER("TSystemDraw::getBorderHeight(const string &family, LINE_TYPE_t lt)");

    if (family.empty())
        return 0;

    BORDER_t bd;

    if (!getBorder(family, lt, &bd))
        return 0;

    return bd.border.textTop;
}

bool TSystemDraw::existSlider(const string& slider)
{
    DECL_TRACER("TSystemDraw::existSlider(const string& slider)");

    if (slider.empty() || mDraw.sliderStyles.size() == 0)
    {
        MSG_ERROR("Slider " << slider << " has " << mDraw.sliderStyles.size() << " entries.");
        return false;
    }

    vector<SLIDER_STYLE_t>::iterator iter;

    for (iter = mDraw.sliderStyles.begin(); iter != mDraw.sliderStyles.end(); ++iter)
    {
        if (iter->name.compare(slider) == 0)
             return true;
    }

    return false;
}

bool TSystemDraw::getSlider(const string& slider, SLIDER_STYLE_t* style)
{
    DECL_TRACER("TSystemDraw::getSlider(const string& slider, SLIDER_STYLE_t* style)");

    if (slider.empty() || mDraw.sliderStyles.size() == 0)
        return false;

    if (!style)
        return existSlider(slider);

    vector<SLIDER_STYLE_t>::iterator iter;

    for (iter = mDraw.sliderStyles.begin(); iter != mDraw.sliderStyles.end(); ++iter)
    {
        if (iter->name.compare(slider) == 0)
        {
            *style = *iter;
            return true;
        }
    }

    return false;
}

vector<SLIDER_t> TSystemDraw::getSliderFiles(const string& slider)
{
    DECL_TRACER("TSystemDraw::getSliderFiles(const string& slider)");

    vector<SLIDER_t> list;

    if (slider.empty())
        return list;

    SLIDER_STYLE_t sst;

    if (!getSlider(slider, &sst))
        return list;

    string fbase = sst.baseFile;
    string myPath = mPath + "/sliders";
    dir::TDirectory dir(myPath);
    dir.setStripPath(true);
    MSG_DEBUG("Scanning for files " << mPath << "/sliders/" << fbase << "_*");
    int num = dir.scanFiles(fbase + "_");

    if (num <= 0)
        return list;

    myPath += "/";
    SLIDER_t slid;

    slid.type = SGR_TOP;
    slid.path = myPath + dir.getEntryWithPart(fbase+"_t");
    slid.pathAlpha = myPath + dir.getEntryWithPart("_t_alpha");
    MSG_DEBUG("Adding paths \"" << slid.path << "\" and \"" << slid.pathAlpha << "\"");
    list.push_back(slid);

    slid.type = SGR_BOTTOM;
    slid.path = myPath + dir.getEntryWithPart(fbase+"_b");
    slid.pathAlpha = myPath + dir.getEntryWithPart("_b_alpha");
    MSG_DEBUG("Adding paths \"" << slid.path << "\" and \"" << slid.pathAlpha << "\"");
    list.push_back(slid);

    slid.type = SGR_LEFT;
    slid.path = myPath + dir.getEntryWithPart(fbase+"_l");
    slid.pathAlpha = myPath + dir.getEntryWithPart("_l_alpha");
    MSG_DEBUG("Adding paths \"" << slid.path << "\" and \"" << slid.pathAlpha << "\"");
    list.push_back(slid);

    slid.type = SGR_RIGHT;
    slid.path = myPath + dir.getEntryWithPart(fbase+"_r");
    slid.pathAlpha = myPath + dir.getEntryWithPart("_r_alpha");
    MSG_DEBUG("Adding paths \"" << slid.path << "\" and \"" << slid.pathAlpha << "\"");
    list.push_back(slid);

    slid.type = SGR_HORIZONTAL;
    slid.path = myPath + dir.getEntryWithPart(fbase+"_h");
    slid.pathAlpha = myPath + dir.getEntryWithPart("_h_alpha");
    MSG_DEBUG("Adding paths \"" << slid.path << "\" and \"" << slid.pathAlpha << "\"");
    list.push_back(slid);

    slid.type = SGR_VERTICAL;
    slid.path = myPath + dir.getEntryWithPart(fbase+"_v");
    slid.pathAlpha = myPath + dir.getEntryWithPart("_v_alpha");
    MSG_DEBUG("Adding paths \"" << slid.path << "\" and \"" << slid.pathAlpha << "\"");
    list.push_back(slid);

    return list;
}

/**
 * @brief TSystemDraw::evaluateName - Tests for all parts of \b parts in \b name
 * The method tests if all strings in \b parts are contained in \b name.
 *
 * @param parts     A vector array containing one or more strings.
 * @param name      A string which may contain all strings from \b parts.
 *
 * @return In case all strings from \b parts were found in \b name it returns
 * TRUE. Otherwise FALSE.
 */
bool TSystemDraw::evaluateName(const std::vector<std::string>& parts, const std::string& name)
{
    DECL_TRACER("TSystemDraw::evaluateName(const std::vector<std::string>& parts, const std::string& name)");

    if (parts.empty())
        return false;

    size_t found = 0;
//    string dbg;
    if (parts.empty())
        return false;

    vector<string>::const_iterator iter;

    for (iter = parts.begin(); iter != parts.end(); ++iter)
    {
//        dbg += "|" + *iter;

        if (StrContains(name, *iter))
            found++;
    }

    if (found == parts.size())
        return true;

    return false;
}

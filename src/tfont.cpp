/*
 * Copyright (C) 2020 to 2024 by Andreas Theofilu <andreas@theosys.at>
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

#include <codecvt>
#include <mutex>

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

#include "tresources.h"
#include "tfont.h"
#include "texpat++.h"
#include "tconfig.h"
#include "terror.h"
#include "tnameformat.h"
#include "ttpinit.h"

#include <include/core/SkFontStyle.h>
#if SKIAV >= 20250812
#include <include/core/SkFontMgr.h>
#endif

#define FTABLE_DSIG     0x44534947
#define FTABLE_EBDT     0x45424454
#define FTABLE_EBLC     0x45424c43
#define FTABLE_GDEF     0x47444546
#define FTABLE_GPOS     0x47504f53
#define FTABLE_GSUB     0x47535542
#define FTABLE_LTSH     0x4c545348
#define FTABLE_OS_2     0x4f532f32
#define FTABLE_VDMX     0x56444d58
#define FTABLE_cmap     0x636d6170
#define FTABLE_cvt      0x63767420
#define FTABLE_fpgm     0x6670676d
#define FTABLE_gasp     0x67617370
#define FTABLE_glyf     0x676c7966
#define FTABLE_head     0x68656164
#define FTABLE_hhea     0x68686561
#define FTABLE_hmtx     0x686d7478
#define FTABLE_kern     0x6b65726e
#define FTABLE_loca     0x6c6f6361
#define FTABLE_maxp     0x6d617870
#define FTABLE_name     0x6e616d65
#define FTABLE_post     0x706f7374
#define FTABLE_prep     0x70726570

#define FTABLE_PID_UNICODE          0
#define FTABLE_PID_MACINTOSH        1
#define FTABLE_PID_MICROSOFT        3

#define FTABLE_SID_UNI_VERSION1     0
#define FTABLE_SID_UNI_VERSION2     1
#define FTABLE_SID_UNI_ISO10646     2
#define FTABLE_SID_UNI_UNI2BMP      3
#define FTABLE_SID_UNI_UNI2         4
#define FTABLE_SID_UNI_UNIVS        5
#define FTABLE_SID_UNI_LASTRES      6

#define FTABLE_SID_MSC_SYMBOL       0
#define FTABLE_SID_MSC_UNICODE      1
#define FTABLE_SID_MSC_SHIFTJIS     2
#define FTABLE_SID_MSC_PRC          3
#define FTABLE_SID_MSC_BIGFIVE      4
#define FTABLE_SID_MSC_JOHAB        5
#define FTABLE_SID_MSC_UNIUCS4      10

#define MAX_TAGS        20

typedef struct FTABLE_FORMAT0_t
{
    uint16_t length;
    uint16_t language;
    unsigned char glyphIndex[256];
}FTABLE_FORMAT0_t;

typedef struct FTABLE_FORMAT4_t
{
    uint16_t length;
    uint16_t language;
    uint16_t segCountX2;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
    uint16_t *endCode;
    uint16_t reservedPad;
    uint16_t *startCode;
    uint16_t *idDelta;
    uint16_t *idRangeOffset;
    uint16_t *glyphIndexArray;
}FTABLE_FORMAT4_t;

typedef struct FTABLE_FORMATS_t
{
    uint16_t format{0};
    union fdef
    {
        FTABLE_FORMAT0_t format0;
        FTABLE_FORMAT4_t format4;
    }fdef;
}FTABLE_FORMATS_t;

typedef struct FTABLE_SUBTABLE_t
{
    uint16_t platformID;
    uint16_t platformSpecificID;
    uint32_t offset;
    FTABLE_FORMATS_t format;
}FTABLE_SUBTABLE_t;

typedef struct FTABLE_CMAP_t
{
    uint16_t version{0};
    uint16_t numSubtables{0};
    FTABLE_SUBTABLE_t *subtables;
}FTABLE_CMAP_t;

using std::string;
using std::vector;
using std::map;
using std::pair;
using std::mutex;
using namespace Expat;

mutex mutex_font;

// This is an internal used font cache
map<string, sk_sp<SkTypeface> > _tfontCache;

TFont::TFont(const string& fname, bool tp)
    : mIsG5(tp),
      mFontFile(fname)
{
    DECL_TRACER("TFont::TFont(const string& fname, bool tp)");
    initialize();
}

TFont::~TFont()
{
    DECL_TRACER("TFont::~TFont()");
}

void TFont::initialize()
{
    DECL_TRACER("TFont::initialize()");

    mutex_font.lock();

    if (mFonts.size() > 0)
        mFonts.clear();

    // System fonts first
    FONT_T font;

    if (!systemFonts())
    {
        TError::clear();
        MSG_INFO("Initializing virtual system fonts because no system files installed!");

        font.number = 1;
        font.file = "cour.ttf";
        font.faceIndex = 1;
        font.fullName = "Courier New";
        font.name = "Courier New";
        font.size = 9;
        font.subfamilyName = "normal";
        font.fileSize = 0;
        font.usageCount = 0;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 2;
        font.faceIndex = 2;
        font.size = 12;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 3;
        font.faceIndex = 3;
        font.size = 18;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 4;
        font.faceIndex = 4;
        font.size = 26;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 5;
        font.faceIndex = 5;
        font.size = 32;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 6;
        font.faceIndex = 6;
        font.size = 18;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 7;
        font.faceIndex = 7;
        font.size = 26;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 8;
        font.faceIndex = 8;
        font.size = 34;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 9;
        font.file = "Amxbold_.ttf";
        font.faceIndex = 9;
        font.fullName = "AMX Bold";
        font.name = "AMX Bold";
        font.size = 14;
        font.subfamilyName = "bold";
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 10;
        font.faceIndex = 10;
        font.size = 20;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 11;
        font.faceIndex = 11;
        font.size = 36;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 19;
        font.file = "arial.ttf";
        font.faceIndex = 19;
        font.fullName = "Arial";
        font.name = "Arial";
        font.size = 9;
        font.subfamilyName = "normal";
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 20;
        font.faceIndex = 20;
        font.size = 10;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 21;
        font.faceIndex = 21;
        font.size = 12;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 22;
        font.faceIndex = 22;
        font.size = 14;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 23;
        font.faceIndex = 23;
        font.size = 16;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 24;
        font.faceIndex = 24;
        font.size = 18;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 25;
        font.faceIndex = 25;
        font.size = 20;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 26;
        font.faceIndex = 26;
        font.size = 24;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 27;
        font.faceIndex = 27;
        font.size = 36;
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 28;
        font.file = "arialbd.ttf";
        font.faceIndex = 28;
        font.fullName = "Arial Bold";
        font.name = "Arial Bold";
        font.size = 10;
        font.subfamilyName = "bold";
        mFonts.insert(pair<int, FONT_T>(font.number, font));

        font.number = 29;
        font.faceIndex = 29;
        font.size = 8;
        mFonts.insert(pair<int, FONT_T>(font.number, font));
    }

    if (mFontFile.empty())
    {
        MSG_ERROR("Got no font file name!");
        return;
    }

    // Now the fonts for setup pages
    systemFonts(true);

    // read the individual fonts from file
    TError::clear();
    string projectPath = TConfig::getProjectPath();

    string path = makeFileName(projectPath, mFontFile);

    if (!isValidFile())
    {
        MSG_ERROR("File " << path << " doesn't exist or is not readable!");
        TError::SetError();
        mutex_font.unlock();
        return;
    }

    TExpat xml(path);

    if (!mIsG5)
        xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
    {
        mutex_font.unlock();
        return;
    }

    int depth = 0;
    size_t index = 0;
    size_t oldIndex = 0;

    if (xml.getElementIndex("fontList", &depth) == TExpat::npos)
    {
        MSG_DEBUG("File does not contain the element \"fontList\"!");
        TError::SetError();
        mutex_font.unlock();
        return;
    }

    depth++;
    int fnumber = 33;

    while ((index = xml.getNextElementIndex("font", depth)) != TExpat::npos)
    {
        string name, content;
        FONT_T ft;
        vector<ATTRIBUTE_t> attrs;

        if (!mIsG5)
        {
            attrs = xml.getAttributes(index);

            if (!attrs.empty())
                ft.number = xml.getAttributeInt("number", attrs);
            else
            {
                MSG_ERROR("Element font contains no or invalid attribute!");
                TError::SetError();
                mutex_font.unlock();
                return;
            }
        }
        else
        {
            ft.number = fnumber;
            fnumber++;
        }

        while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
        {
            string e = name;

            if (e.compare("file") == 0)
                ft.file = content;
            else if (e.compare("fileSize") == 0)
                ft.fileSize = xml.convertElementToInt(content);
            else if (e.compare("faceIndex") == 0)
                ft.faceIndex = xml.convertElementToInt(content);
            else if (e.compare("name") == 0)
                ft.name = content;
            else if (e.compare("subfamilyName") == 0)
                ft.subfamilyName = content;
            else if (e.compare("fullName") == 0)
                ft.fullName = content;
            else if (e.compare("size") == 0)
                ft.size = xml.convertElementToInt(content);
            else if (e.compare("usageCount") == 0)
                ft.usageCount = xml.convertElementToInt(content);

            oldIndex = index;
        }

        mFonts.insert(pair<int, FONT_T>(ft.number, ft));

        if (index == TExpat::npos)
            index = oldIndex + 1;

        xml.setIndex(index);
    }

    mutex_font.unlock();
}

bool TFont::systemFonts(bool setup)
{
    DECL_TRACER("TFont::systemFonts(bool setup)");

    string path;

    if (setup)
        path = makeFileName(TConfig::getSystemProjectPath(), "/" + mFontFile);
    else
        path = makeFileName(TConfig::getSystemProjectPath(), "/graphics/" + mFontFile);

    if (!isValidFile())
    {
        MSG_ERROR("File " << path << " doesn't exist or is not readable!");
        TError::SetError();
        return false;
    }

    TExpat xml(path);

    if (!mIsG5)
        xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return false;

    int depth = 0;
    size_t index = 0;
    size_t oldIndex = 0;

    if (xml.getElementIndex("fontList", &depth) == TExpat::npos)
    {
        MSG_DEBUG("File does not contain the element \"fontList\"!");
        TError::SetError();
        return false;
    }

    depth++;
    int fnumber = 1;

    while ((index = xml.getNextElementIndex("font", depth)) != TExpat::npos)
    {
        string name, content;
        FONT_T ft;
        vector<ATTRIBUTE_t> attrs;

        if (!mIsG5)
        {
            attrs = xml.getAttributes(index);

            if (!attrs.empty())
                ft.number = xml.getAttributeInt("number", attrs);
            else
            {
                MSG_ERROR("Element font contains no or invalid attribute!");
                TError::SetError();
                return false;
            }
        }
        else
        {
            ft.number = fnumber;
            fnumber++;
        }

        while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
        {
            string e = name;

            if (e.compare("file") == 0)
                ft.file = content;
            else if (e.compare("fileSize") == 0)
                ft.fileSize = xml.convertElementToInt(content);
            else if (e.compare("faceIndex") == 0)
                ft.faceIndex = xml.convertElementToInt(content);
            else if (e.compare("name") == 0)
                ft.name = content;
            else if (e.compare("subfamilyName") == 0)
                ft.subfamilyName = content;
            else if (e.compare("fullName") == 0)
                ft.fullName = content;
            else if (e.compare("size") == 0)
                ft.size = xml.convertElementToInt(content);
            else if (e.compare("usageCount") == 0)
                ft.usageCount = xml.convertElementToInt(content);

            oldIndex = index;
        }
#if __cplusplus < 201703L
        mFonts.insert(pair<int, FONT_T>(ft.number, ft));
#else
        mFonts.insert_or_assign(ft.number, ft);
#endif
        if (index == TExpat::npos)
            index = oldIndex + 1;

        xml.setIndex(index);
    }

    return true;
}

FONT_T TFont::getFont(int number)
{
    DECL_TRACER("TFont::getFont(int number)");

    if (mFonts.size() == 0)
    {
        MSG_WARNING("No fonts found!");
        return FONT_T();
    }

    map<int, FONT_T>::iterator iter = mFonts.find(number);

    if (iter == mFonts.end())
    {
        MSG_WARNING("No font with number " << number << " found!");
        return FONT_T();
    }

    return iter->second;
}

int TFont::getFontIDfromFile(const string& file)
{
    DECL_TRACER("TFont::getFontIDfromFile(const string& file)");

    if (mFonts.size() == 0)
    {
        MSG_WARNING("No fonts found!");
        return -1;
    }

    map<int, FONT_T>::iterator iter;

    for (iter = mFonts.begin(); iter != mFonts.end(); ++iter)
    {
        if (iter->second.file == file)
            return iter->first;
    }
#if TESTMODE == 1
    MSG_WARNING("There is no font file \"" << file << "\" found!");
#endif
    return -1;
}

int TFont::getFontIDfromName(const string &name)
{
    DECL_TRACER("TFont::getFontIDfromName(const string &name)");

    if (mFonts.size() == 0)
    {
        MSG_WARNING("No fonts found!");
        return -1;
    }

    map<int, FONT_T>::iterator iter;

    for (iter = mFonts.begin(); iter != mFonts.end(); ++iter)
    {
        if (iter->second.name == name)
            return iter->first;
    }
#if TESTMODE == 1
    MSG_WARNING("There is no font name \"" << name << "\" found!");
#endif
    return -1;
}

FONT_STYLE TFont::getStyle(int number)
{
    DECL_TRACER("TFont::getStyle(int number)");

    map<int, FONT_T>::iterator iter = mFonts.find(number);

    if (iter == mFonts.end())
        return FONT_NONE;

    if (iter->second.subfamilyName.compare("Regular") == 0)
        return FONT_NORMAL;
    else if (iter->second.subfamilyName.compare("Italic") == 0)
        return FONT_ITALIC;
    else if (iter->second.subfamilyName.compare("Bold") == 0)
        return FONT_BOLD;
    else if (iter->second.subfamilyName.compare("Bold Italic") == 0)
        return FONT_BOLD_ITALIC;

    return FONT_NORMAL;
}

FONT_STYLE TFont::getStyle(FONT_T& font)
{
    DECL_TRACER("TFont::getStyle(int number)");

    if (font.subfamilyName.compare("Regular") == 0)
        return FONT_NORMAL;
    else if (font.subfamilyName.compare("Italic") == 0)
        return FONT_ITALIC;
    else if (font.subfamilyName.compare("Bold") == 0)
        return FONT_BOLD;
    else if (font.subfamilyName.compare("Bold Italic") == 0)
        return FONT_BOLD_ITALIC;

    return FONT_NORMAL;
}

SkFontStyle TFont::getSkiaStyle(int number)
{
    DECL_TRACER("TFont::getSkiaStyle(int number)");

    map<int, FONT_T>::iterator iter = mFonts.find(number);

    if (iter == mFonts.end())
        return SkFontStyle(SkFontStyle::kNormal_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant);

    if (iter->second.subfamilyName.compare("Regular") == 0)
        return SkFontStyle::Normal();
    else if (iter->second.subfamilyName.compare("Italic") == 0)
        return SkFontStyle::Italic();
    else if (iter->second.subfamilyName.compare("Bold") == 0)
        return SkFontStyle::Bold();
    else if (iter->second.subfamilyName.compare("Bold Italic") == 0)
        return SkFontStyle::BoldItalic();

    return SkFontStyle::Normal();
}

#define MAX_FACES   10

sk_sp<SkTypeface> TFont::getTypeFace(const string& ff)
{
    DECL_TRACER("TFont::getTypeFace(const string& ff)");

    map<int, FONT_T>::iterator iter;

    for (iter = mFonts.begin(); iter != mFonts.end(); ++iter)
    {
        if (iter->second.file == ff)
            return getTypeFace(iter->first);
    }

    return nullptr;
}


sk_sp<SkTypeface> TFont::getTypeFace(int number)
{
    DECL_TRACER("TFont::getTypeFace(int number)");

    map<int, FONT_T>::iterator iter = mFonts.find(number);

    if (iter == mFonts.end())
    {
        MSG_ERROR("No font with index " << number << " found!");
        TError::SetError();
        return nullptr;
    }

    if (!_tfontCache.empty())
    {
        map<string, sk_sp<SkTypeface> >::iterator itFont = _tfontCache.find(iter->second.file);

        if (itFont != _tfontCache.end() && itFont->second)
        {
            MSG_DEBUG("Font " << number << ": " << iter->second.file << " was taken from cache.");
            return itFont->second;
        }
        else if (itFont != _tfontCache.end())
            _tfontCache.erase(itFont);
    }

    mutex_font.lock();
    string path;

    if (!TTPInit::isG5() && number < 32)    // System font?
    {
        path = TConfig::getProjectPath() + "/__system/graphics/fonts/" + iter->second.file;

        if (!isValidFile(path))
        {
            MSG_WARNING("Seem to miss system fonts ...");
            path = TConfig::getProjectPath() + "/fonts/" + iter->second.file;
        }
    }
    else
    {
        path = TConfig::getProjectPath() + "/fonts/" + iter->second.file;

        if (!fs::exists(path))
        {
            string pth = TConfig::getProjectPath() + "/__system/fonts/" + iter->second.file;

            if (fs::exists(pth))
                path = pth;
        }
    }

    sk_sp<SkTypeface> tf;
    MSG_DEBUG("Loading font \"" << path << "\" ...");

    if (isValidFile(path))
    {
        try
        {
            tf = MakeResourceAsTypeface(path.c_str(), iter->second.faceIndex, RESTYPE_FONT);
        }
        catch(std::exception& e)
        {
            MSG_ERROR("Error loading font: " << e.what());
        }
    }
    else
        MSG_WARNING("File " << path << " is not a valid file or does not exist!");

    if (!tf)
    {
        string perms = getPermissions(path);
        MSG_ERROR("Error loading font \"" << path << "\" [" << perms << "]");
        MSG_PROTOCOL("Trying with alternative function ...");
        TError::setError();
#if SKIAV < 20250812
        tf = SkTypeface::MakeFromName(iter->second.fullName.c_str(), getSkiaStyle(number));
#else
        sk_sp<SkFontMgr> fm = getFontManager();
        tf = fm->matchFamilyStyle(iter->second.fullName.c_str(), getSkiaStyle(number));
#endif
        if (tf)
            TError::clear();
        else
        {
            MSG_ERROR("Alternative method failed loading the font " << iter->second.fullName);
            MSG_WARNING("Will use a default font instead!");
#if SKIAV < 20250812
            tf = SkTypeface::MakeDefault();
#else
            tf = fm->makeFromData(nullptr);
#endif
            if (tf)
                TError::clear();
            else
            {
                MSG_ERROR("No default font found!");
                mutex_font.unlock();
                return tf;
            }
        }
    }
    else
    {
        if (tf->countTables() > 0)
        {
            _tfontCache.insert(pair<string, sk_sp<SkTypeface> >(iter->second.file, tf));
            MSG_DEBUG("Font \"" << path << "\" was loaded successfull.");
        }
        else
        {
            MSG_WARNING("Refused to enter invalid typeface into font cache!");
        }
    }

    SkString sname;
    tf->getFamilyName(&sname);

    if (iter->second.name.compare(sname.c_str()) != 0)
    {
        MSG_WARNING("Found font name \"" << sname.c_str() << "\" with attributes: bold=" << ((tf->isBold())?"TRUE":"FALSE") << ", italic=" << ((tf->isItalic())?"TRUE":"FALSE") << ", fixed=" << ((tf->isFixedPitch())?"TRUE":"FALSE"));
        MSG_WARNING("The loaded font \"" << sname.c_str() << "\" is not the wanted font \"" << iter->second.name << "\"!");
    }

    FONT_STYLE style = getStyle(iter->second);
    bool ret = false;

    if (style == FONT_BOLD && tf->isBold())
        ret = true;
    else if (style == FONT_ITALIC && tf->isItalic())
        ret = true;
    else if (style == FONT_BOLD_ITALIC && tf->isBold() && tf->isItalic())
        ret = true;
    else if (style == FONT_NORMAL && !tf->isBold() && !tf->isItalic())
        ret = true;

    if (ret)
    {
        mutex_font.unlock();
        return tf;
    }

    MSG_WARNING("The wanted font style " << iter->second.subfamilyName << " was not found!");
    mutex_font.unlock();
    return tf;
}

vector<string> TFont::getFontPathList()
{
    DECL_TRACER("TFont::getFontPathList()");

    vector<string> list;
    string path = TConfig::getProjectPath() + "/fonts";
    list.push_back(path);
    path = TConfig::getProjectPath() + "/__system/fonts";
    list.push_back(path);
    path = TConfig::getProjectPath() + "/__system/graphics/fonts";
    list.push_back(path);
    return list;
}

size_t _numTables;
FTABLE_CMAP_t _cmapTable;

void TFont::parseCmap(const unsigned char* cmaps)
{
    DECL_TRACER("TFont::parseCmap(const unsigned char* cmaps)");

    if (!cmaps)
        return;

    _cmapTable.version = getUint16(cmaps);
    _cmapTable.numSubtables = getUint16(cmaps+sizeof(uint16_t));
    MSG_DEBUG("Found version " << _cmapTable.version << ", found " << _cmapTable.numSubtables << " cmap tables.");
    _cmapTable.subtables = new FTABLE_SUBTABLE_t[_cmapTable.numSubtables];
    size_t pos = sizeof(uint16_t) * 2;

    for (uint16_t i = 0; i < _cmapTable.numSubtables; i++)
    {
        FTABLE_SUBTABLE_t st;
        st.platformID = getUint16(cmaps+pos);
        pos += sizeof(uint16_t);
        st.platformSpecificID = getUint16(cmaps+pos);
        pos += sizeof(uint16_t);
        st.offset = getUint32(cmaps+pos);
        pos += sizeof(uint32_t);
        memmove(&_cmapTable.subtables[i], &st, sizeof(st));
        MSG_DEBUG("Table " << (i+1) << ": platformID=" << st.platformID << ", platformSpecificID=" << st.platformSpecificID << ", offset=" << st.offset);
    }

    // Get the format and read the mapping
    for (uint16_t i = 0; i < _cmapTable.numSubtables; i++)
    {
        if (_cmapTable.subtables[i].platformID == FTABLE_PID_MACINTOSH)   // We ignore old Macintosh format
        {
            _cmapTable.subtables[i].format.format = -1;
            continue;
        }

        FTABLE_FORMATS_t format;
        format.format = getUint16(cmaps+_cmapTable.subtables[i].offset);
        pos = _cmapTable.subtables[i].offset + sizeof(uint16_t);

        switch(format.format)
        {
            case 0:
                format.fdef.format0.length = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                format.fdef.format0.language = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                memcpy(format.fdef.format0.glyphIndex, cmaps+pos, sizeof(format.fdef.format0.glyphIndex));
            break;

            case 4:
                format.fdef.format4.length = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                format.fdef.format4.language = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                format.fdef.format4.segCountX2 = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                format.fdef.format4.searchRange = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                format.fdef.format4.entrySelector = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                format.fdef.format4.rangeShift = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                uint16_t segCount = format.fdef.format4.segCountX2 / 2;
                format.fdef.format4.endCode = new uint16_t[segCount];

                for (uint16_t j = 0; j < segCount; j++)
                {
                    format.fdef.format4.endCode[j] = getUint16(cmaps+pos);
                    pos += sizeof(uint16_t);
                }

                format.fdef.format4.reservedPad = getUint16(cmaps+pos);
                pos += sizeof(uint16_t);
                format.fdef.format4.startCode = new uint16_t[segCount];

                for (uint16_t j = 0; j < segCount; j++)
                {
                    format.fdef.format4.startCode[j] = getUint16(cmaps+pos);
                    pos += sizeof(uint16_t);
                }

                format.fdef.format4.idDelta = new uint16_t[segCount];

                for (uint16_t j = 0; j < segCount; j++)
                {
                    format.fdef.format4.idDelta[j] = getUint16(cmaps+pos);
                    pos += sizeof(uint16_t);
                }

                format.fdef.format4.idRangeOffset = new uint16_t[segCount];

                for (uint16_t j = 0; j < segCount; j++)
                {
                    format.fdef.format4.idRangeOffset[j] = getUint16(cmaps+pos);
                    pos += sizeof(uint16_t);
                }

                format.fdef.format4.glyphIndexArray = (uint16_t *)(cmaps+pos);
                memcpy(&_cmapTable.subtables[i].format, &format, sizeof(FTABLE_FORMATS_t));
            break;
        }
    }
}

uint16_t TFont::getGlyphIndex(SkUnichar ch)
{
    DECL_TRACER("TFont::getGlyphIndex(SkUnichar ch)");

    uint16_t lCh = ch;
    bool symbol = false;

    for (uint16_t nTbs = 0; nTbs < _cmapTable.numSubtables; nTbs++)
    {
        if (_cmapTable.subtables[nTbs].platformID == FTABLE_PID_UNICODE ||
            _cmapTable.subtables[nTbs].platformID == FTABLE_PID_MICROSOFT)
        {
            if ((_cmapTable.subtables[nTbs].platformID == FTABLE_PID_UNICODE &&_cmapTable.subtables[nTbs].platformSpecificID == FTABLE_SID_UNI_VERSION1) ||
                (_cmapTable.subtables[nTbs].platformID == FTABLE_PID_MICROSOFT && _cmapTable.subtables[nTbs].platformSpecificID == FTABLE_SID_MSC_SYMBOL))
                symbol = true;  // Table does not have unicode table mapping (wingding)

            // Find the segment where the wanted character is in.
            if (_cmapTable.subtables[nTbs].format.format == 4)
            {
                FTABLE_FORMAT4_t form = _cmapTable.subtables[nTbs].format.fdef.format4;
                uint16_t segCount = form.segCountX2 / 2;
                uint16_t segment = 0xffff;
                MSG_DEBUG("segCountX2: " << form.segCountX2 << ", # segments: " << segCount);

                for (uint16_t sc = 0; sc < segCount; sc++)
                {
                    MSG_DEBUG("Table: " << nTbs << ": Checking range " << std::hex << std::setw(4) << std::setfill('0') << form.startCode[sc] << " to " << std::setw(4) << std::setfill('0') << form.endCode[sc] << std::dec);

                    if (symbol)
                    {
                        if (ch <= 0x00ff)
                            lCh = ch + (form.startCode[sc] & 0xff00);
                        else
                            lCh = ch + (form.startCode[sc] & 0xf000);
                    }

                    if (lCh >= form.startCode[sc] && lCh <= form.endCode[sc])
                    {
                        segment = sc;
                        break;
                    }
                }

                if (segment == 0xffff || form.startCode[segment] == 0xffff || form.endCode[segment] == 0xffff)
                {
                    MSG_WARNING("The character " << std::hex << std::setw(4) << std::setfill('0') << lCh << " is not supported by any segment!" << std::dec);
                    continue;
                }

                MSG_DEBUG("Table: " << (nTbs+1) << ": idRangeOffset: " << std::hex << std::setw(4) << std::setfill('0') << form.idRangeOffset[segment] << ", idDelta: " << std::setw(4) << std::setfill('0') << form.idDelta[segment] << std::dec);
                uint16_t glyphIndex;

                if (form.idRangeOffset[segment] == 0)
                    glyphIndex = form.idDelta[segment] + lCh;
                else
                {
                    uint16_t gArray = getUint16((unsigned char *)(form.glyphIndexArray + (form.idRangeOffset[segment] / 2)));
                    MSG_DEBUG("Value from glyphArray: " << std::hex << std::setw(4) << std::setfill('0') << gArray);

                    if (symbol && segment > 0 && gArray != 0)
                        glyphIndex = gArray + (lCh - form.startCode[segment]) - 2;
                    else
                        glyphIndex = gArray / 2 + (lCh - form.startCode[segment]);
                }

                MSG_DEBUG("Found index 0x" << std::hex << std::setw(4) << std::setfill('0') << glyphIndex << " for unichar 0x" << std::setw(4) << std::setfill('0') << lCh << std::dec);
                return glyphIndex;
            }
            else
            {
                MSG_WARNING("Ignoring table with unsupported format " << _cmapTable.subtables[nTbs].format.format);
            }
        }
    }

    return 0xffff;
}

void TFont::_freeCmap()
{
    DECL_TRACER("TFont::_freeCmap()");

    for (uint16_t nTbs = 0; nTbs < _cmapTable.numSubtables; nTbs++)
    {
        if (_cmapTable.subtables[nTbs].platformID == FTABLE_PID_UNICODE ||
            _cmapTable.subtables[nTbs].platformID == FTABLE_PID_MICROSOFT)
        {
            if (_cmapTable.subtables[nTbs].format.format == 4)
            {
                delete[] _cmapTable.subtables[nTbs].format.fdef.format4.endCode;
                delete[] _cmapTable.subtables[nTbs].format.fdef.format4.startCode;
                delete[] _cmapTable.subtables[nTbs].format.fdef.format4.idDelta;
                delete[] _cmapTable.subtables[nTbs].format.fdef.format4.idRangeOffset;
            }
        }
    }

    delete[] _cmapTable.subtables;
}

SkGlyphID *TFont::textToGlyphs(const string& str, sk_sp<SkTypeface>& typeFace, size_t *size)
{
    DECL_TRACER("TFont::textToGlyphs(const string& str, SkTypeface& typeFace)");

    *size = 0;
    int tables = typeFace->countTables();

    if (!tables)
    {
        MSG_ERROR("No tables in typeface!");
        return nullptr;
    }
#if SKIAV < 20250812
    SkFontTableTag *tbTags = new SkFontTableTag[tables];
    int tags = typeFace->getTableTags(tbTags);
#else
    SkFontTableTag tbTags[MAX_TAGS];
    int tags = std::min(typeFace->readTableTags(tbTags), MAX_TAGS);
#endif
    bool haveCmap = false;
    unsigned char *cmaps = nullptr;

    // Find the "cmap" and the "glyph" tables and get them
    for (int i = 0; i < tags; i++)
    {
        SkFontTableTag fttg = tbTags[i];

        if (fttg == FTABLE_cmap)
        {
            size_t tbSize = typeFace->getTableSize(fttg);

            if (!tbSize)
            {
                MSG_ERROR("CMAP font table size is 0!");
#if SKIAV < 20250812
                delete[] tbTags;
#endif
                return nullptr;
            }

            cmaps = new unsigned char[tbSize];
            typeFace->getTableData(fttg, 0, tbSize, cmaps);
            haveCmap = true;
            break;
        }
    }

    if (!haveCmap)
    {
        MSG_ERROR("Invalid font. Missing CMAP table!");
#if SKIAV < 20250812
        delete[] tbTags;
#endif
        return nullptr;
    }
#if SKIAV < 20250812
    delete[] tbTags;
#endif
    parseCmap(cmaps);

    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
    std::u16string dest = convert.from_bytes(str);
    SkGlyphID *gIds = new SkGlyphID[dest.length()];
    *size = dest.length();

    for (size_t i = 0; i < dest.length(); i++)
    {
        SkUnichar uniChar = (SkUnichar)dest[i];
        gIds[i] = getGlyphIndex(uniChar);
    }

    _freeCmap();
    delete[] cmaps;

    return gIds;
}

FONT_TYPE TFont::isSymbol(sk_sp<SkTypeface>& typeFace)
{
    DECL_TRACER("TFont::isSymbol(sk_sp<SkTypeface>& typeFace)");

    if (!typeFace)
    {
        MSG_ERROR("Got an empty typeface!");
        return FT_UNKNOWN;
    }

    mutex_font.lock();
    int tables = typeFace->countTables();

    if (tables <= 0)
    {
        MSG_ERROR("No tables found in typeface!");
        mutex_font.unlock();
        return FT_UNKNOWN;
    }
#if SKIAV < 20250812
    SkFontTableTag *tbTags = new SkFontTableTag[tables];
    int tags = typeFace->getTableTags(tbTags);
#else
    SkFontTableTag tbTags[MAX_TAGS];
    int tags = std::min(typeFace->readTableTags(tbTags), MAX_TAGS);
#endif
    bool haveCmap = false;
    unsigned char *cmaps = nullptr;

    // Find the "cmap" table and get it
    for (int i = 0; i < tags; i++)
    {
        SkFontTableTag fttg = tbTags[i];

        if (fttg == FTABLE_cmap)
        {
            size_t tbSize = typeFace->getTableSize(fttg);

            if (!tbSize)
            {
                MSG_ERROR("CMAP table has size of 0!");
#if SKIAV < 20250812
                delete[] tbTags;
#endif
                mutex_font.unlock();
                return FT_UNKNOWN;
            }

            cmaps = new unsigned char[tbSize];
            typeFace->getTableData(fttg, 0, tbSize, cmaps);
            haveCmap = true;
            break;
        }
    }

    if (!haveCmap)
    {
        MSG_ERROR("Invalid font. Missing CMAP table!");
#if SKIAV < 20250812
        delete[] tbTags;
#endif
        mutex_font.unlock();
        return FT_UNKNOWN;
    }
#if SKIAV < 20250812
    delete[] tbTags;
#endif
    parseCmap(cmaps);

    for (uint16_t nTbs = 0; nTbs < _cmapTable.numSubtables; nTbs++)
    {
        if ((_cmapTable.subtables[nTbs].platformID == FTABLE_PID_UNICODE && _cmapTable.subtables[nTbs].platformSpecificID == FTABLE_SID_UNI_VERSION1) ||
            (_cmapTable.subtables[nTbs].platformID == FTABLE_PID_MICROSOFT && _cmapTable.subtables[nTbs].platformSpecificID == FTABLE_SID_MSC_SYMBOL))
        {
            _freeCmap();
            delete[] cmaps;
            FONT_TYPE ft = FT_SYMBOL;

            if (_cmapTable.subtables[nTbs].platformID == FTABLE_PID_MICROSOFT && _cmapTable.subtables[nTbs].platformSpecificID == FTABLE_SID_MSC_SYMBOL)
                ft = FT_SYM_MS;

            mutex_font.unlock();
            return ft;
        }
    }

    _freeCmap();
    delete[] cmaps;
    mutex_font.unlock();
    return FT_NORMAL;
}

size_t TFont::utf8ToUtf16(const string& str, uint16_t **uni, bool toSymbol)
{
    DECL_TRACER("TFont::utf8ToUtf16(const string& str, uint16_t **uni, bool toSymbol)");

    if (str.empty() || !uni)
    {
        if (uni)
            *uni = nullptr;

        return 0;
    }

    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
    std::u16string dest = convert.from_bytes(str);

    *uni = new uint16_t[dest.length()+1];
    uint16_t *unicode = *uni;
    memset(unicode, 0, sizeof(uint16_t) * (dest.length()+1));

    for (size_t i = 0; i < dest.length(); i++)
    {
        unicode[i] = dest[i];

        if (toSymbol && unicode[i] < 0xf000)
            unicode[i] += 0xf000;
    }

    return dest.length();
}

double TFont::pixelToPoint(int dpi, int pixel)
{
    DECL_TRACER("TFont::pixelToPoint(int dpi, int pixel)");

    double size = 0.0138889 * (double)dpi * (double)pixel;
    MSG_DEBUG("Size: " << size << ", dpi: " << dpi << ", pixels: " << pixel);
    return size;
}

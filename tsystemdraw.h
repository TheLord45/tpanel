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

#ifndef __TSYSTEMDRAW_H__
#define __TSYSTEMDRAW_H__

#include <string>
#include <vector>
#include <functional>

#include <expat.h>

#include "tvalidatefile.h"
#include "tdirectory.h"

typedef enum SLIDER_GRTYPE_t
{
    SGR_TOP,
    SGR_BOTTOM,
    SGR_LEFT,
    SGR_RIGHT,
    SGR_HORIZONTAL,
    SGR_VERTICAL
}SLIDER_GRTYPE_t;

typedef struct FAMILY_t
{
    std::string name;
    std::vector<std::string> member;
}FAMILY_t;

typedef struct BORDER_STYLE_t
{
    std::string name;
    std::string off;
    std::string on;
    std::string drag;
    std::string drop;
    std::vector<int> g3Equiv;
}BORDER_STYLE_t;

typedef struct BORDER_DATA_t
{
    std::string name;
    std::string baseFile;
    int multiColor;
    int fillTop;
    int fillLeft;
    int fillBottom;
    int fillRight;
    int textTop;
    int textLeft;
    int textBottom;
    int textRight;
    int idealWidth;
    int idealHeight;
    int minHeight;
    int minWidth;
    int incHeight;
    int incWidth;

    void init()
    {
        multiColor = 0;
        fillTop = 0;
        fillLeft = 0;
        fillBottom = 0;
        fillRight = 0;
        textTop = 0;
        textLeft = 0;
        textBottom = 0;
        textRight = 0,
        idealWidth = 0;
        idealHeight = 0;
        minHeight = 0;
        minWidth = 0;
        incHeight = 0;
        incWidth = 0;
    }
}BORDER_DATA_t;

typedef struct CURSOR_STYLE_t
{
    std::string name;
    std::string baseFile;
    int multiColor;
    std::vector<int> g3Equiv;

    void init()
    {
        multiColor = 0;
    }
}CURSOR_STYLE_t;

typedef struct SLIDER_STYLE_t
{
    std::string name;
    std::string baseFile;
    int multiColor;
    int incRepeat;
    int minSize;
    int fixedSize;
    std::vector<int> g3Equiv;

    void init()
    {
        multiColor = 0;
        incRepeat = 0;
        minSize = 0;
        fixedSize = 0;
    }
}SLIDER_STYLE_t;

typedef struct EFFECT_STYLE_t
{
    std::string name;
    int number;
    int startx;
    int starty;
    int height;
    int width;
    int cutout;
    std::string pixelMap;

    void init()
    {
        number = 0;
        startx = 0;
        starty = 0;
        height = 0;
        width = 0;
        cutout = 0;
    }
}EFFECT_STYLE_t;

typedef struct POPUP_EFFECT_t
{
    std::string name;
    int number;
    int valueUsed;

    void init()
    {
        number = 0;
        valueUsed = 0;
    }
}POPUP_EFFECT_t;

typedef struct DRAW_t
{
    std::vector<FAMILY_t> borders;
    std::vector<BORDER_STYLE_t> borderStyles;
    std::vector<BORDER_DATA_t> borderData;
    std::vector<FAMILY_t> cursors;
    std::vector<CURSOR_STYLE_t> cursorStyles;
    std::vector<FAMILY_t> sliders;
    std::vector<SLIDER_STYLE_t> sliderStyles;
    std::vector<FAMILY_t> effects;
    std::vector<EFFECT_STYLE_t> effectStyles;
    std::vector<POPUP_EFFECT_t> popupEffects;
}DRAW_t;

typedef struct BORDER_t
{
    std::string bl;         // bottom left corner
    std::string b;          // bottom
    std::string br;         // bottom right corner
    std::string r;          // right
    std::string tr;         // top right corner
    std::string t;          // top
    std::string tl;         // top left corner
    std::string l;          // left
    std::string bl_alpha;   // bottom left corner
    std::string b_alpha;    // bottom
    std::string br_alpha;   // bottom right corner
    std::string r_alpha;    // right
    std::string tr_alpha;   // top right corner
    std::string t_alpha;    // top
    std::string tl_alpha;   // top left corner
    std::string l_alpha;    // left
    BORDER_DATA_t border;   // Border data
    BORDER_STYLE_t bdStyle; // The border style data
}BORDER_t;

typedef struct SLIDER_t
{
    SLIDER_GRTYPE_t type;   //<! The type of the file the path is pointing to
    std::string path;       //<! The path and file name of the graphics mask file.
    std::string pathAlpha;  //<! The path and file name of the graphics file containing the alpha part of the image.
}SLIDER_t;

typedef struct CURSOR_t
{
    std::string imageBase;  // The base image file.
    std::string imageAlpha; // The alpha image file.
}CURSOR_t;

/**
 * @brief Class to manage system resources like borders, sliders, ...
 *
 * \c TSystemDraw reads the system configuration file draw.xma. This is
 * usualy located in the system directory __system/graphics.
 */
class TSystemDraw : public TValidateFile
{
    public:
        typedef enum LINE_TYPE_t
        {
            LT_OFF,
            LT_ON,
            LT_DRAG,
            LT_DROP
        }LINE_TYPE_t;

        TSystemDraw(const std::string& path);
        ~TSystemDraw();

        bool loadConfig();

        bool getBorder(const std::string& family, LINE_TYPE_t lt, BORDER_t *border, const std::string& family2="", bool info=false);
        bool getBorderInfo(const std::string& family, LINE_TYPE_t lt, BORDER_t *border, const std::string& family2="");
        bool existBorder(const std::string& family);
        int getBorderWidth(const std::string& family, LINE_TYPE_t lt = LT_OFF);
        int getBorderHeight(const std::string& family, LINE_TYPE_t lt = LT_OFF);

        bool existSlider(const std::string& slider);
        bool getSlider(const std::string& slider, SLIDER_STYLE_t *style);
        std::vector<SLIDER_t> getSliderFiles(const std::string& slider);
        bool getCursor(const std::string& cursor, CURSOR_STYLE_t *style);
        bool existCursor(const std::string& cursor);
        CURSOR_t getCursorFiles(const CURSOR_STYLE_t& style);

    protected:
        static void startElement(void *, const XML_Char *name, const XML_Char **);
        static void XMLCALL endElement(void *userData, const XML_Char *name);
        static void XMLCALL CharacterDataHandler(void *userData, const XML_Char *s, int len);


    private:
        typedef enum XELEMENTS_t
        {
            X_NONE,
            X_BORDER,
            X_BORDER_DATA,
            X_BORDER_FAMILY,
            X_BORDER_STYLE,
            X_CURSOR_DATA,
            X_CURSOR_FAMILY,
            X_CURSOR_STYLE,
            X_SLIDER_DATA,
            X_SLIDER_FAMILY,
            X_SLIDER_STYLE,
            X_EFFECT_DATA,
            X_EFFECT_FAMILY,
            X_EFFECT_STYLE,
            X_POPUP_EFFECT_DATA,
            X_POPUP_EFFECT
        }XELEMENTS_t;

        std::string getDirEntry(dir::TDirectory *dir, const std::string& part, bool alpha = true);
        bool evaluateName(const std::vector<std::string>& parts, const std::string& name);

        std::string mPath;          // The path to the system directory tree
        bool mValid{false};         // TRUE = the path mPath is a valid directory
        bool mHaveBorders{false};   // TRUE = system directory borders exist
        bool mHaveCursors{false};   // TRUE = system directory cursors exist
        bool mHaveFonts{false};     // TRUE = system directory fonts exist
        bool mHaveImages{false};    // TRUE = system directory images exist
        bool mHaveSliders{false};   // TRUE = system directory sliders exist

        DRAW_t mDraw;

        static XELEMENTS_t mActData;
        static XELEMENTS_t mActFamily;
        static std::string mActElement;
};

#endif

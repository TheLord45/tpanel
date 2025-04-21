/*
 * Copyright (C) 2020 to 2025 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef __TBUTTON_H__
#define __TBUTTON_H__

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>

#include <include/core/SkImage.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkBitmap.h>

#include "texpat++.h"
#include "tpalette.h"
#include "tcolor.h"
#include "tfont.h"
#include "tamxnet.h"
#include "ttimer.h"
#include "timagerefresh.h"
#include "tsystem.h"
#include "tintborder.h"
#include "tsystemdraw.h"

#define ORD_ELEM_COUNT  5
#ifndef MAX_IMAGES
#define MAX_IMAGES      5
#endif

extern bool prg_stopped;
extern bool _restart_;

class SkFont;
class SkTextBlob;
class TBitmap;
class TButtonStates;

struct RESOURCE_T;

namespace Button
{
#   define STATE_BASE   0
#   define STATE_OFF    0
#   define STATE_ON     1
#   define STATE_1      0
#   define STATE_2      1
#   define STATE_3      2
#   define STATE_4      3
#   define STATE_5      4
#   define STATE_6      5
#   define STATE_7      6
#   define STATE_8      7
#   define STATE_ALL    -1

#   define HANDLE_UNDEF 0

    typedef struct SYSTEF_t     // Text effect names
    {
        int idx{0};
        std::string name;
    }SYSTEF_t;

    typedef enum ORIENTATION
    {
        ORI_ABSOLUT,
        ORI_TOP_LEFT,
        ORI_TOP_MIDDLE,
        ORI_TOP_RIGHT,
        ORI_CENTER_LEFT,
        ORI_CENTER_MIDDLE,      // default
        ORI_CENTER_RIGHT,
        ORI_BOTTOM_LEFT,
        ORI_BOTTOM_MIDDLE,
        ORI_BOTTOM_RIGHT,
        ORI_SCALE_FIT,          // G5 scale to fit
        ORI_SCALE_ASPECT        // G5 scale maintain aspect ratio
    }ORIENTATION;

    typedef enum TEXT_EFFECT
    {
        EFFECT_NONE,
        EFFECT_OUTLINE_S,
        EFFECT_OUTLINE_M,
        EFFECT_OUTLINE_L,
        EFFECT_OUTLINE_X,
        EFFECT_GLOW_S,
        EFFECT_GLOW_M,
        EFFECT_GLOW_L,
        EFFECT_GLOW_X,
        EFFECT_SOFT_DROP_SHADOW_1,
        EFFECT_SOFT_DROP_SHADOW_2,
        EFFECT_SOFT_DROP_SHADOW_3,
        EFFECT_SOFT_DROP_SHADOW_4,
        EFFECT_SOFT_DROP_SHADOW_5,
        EFFECT_SOFT_DROP_SHADOW_6,
        EFFECT_SOFT_DROP_SHADOW_7,
        EFFECT_SOFT_DROP_SHADOW_8,
        EFFECT_MEDIUM_DROP_SHADOW_1,
        EFFECT_MEDIUM_DROP_SHADOW_2,
        EFFECT_MEDIUM_DROP_SHADOW_3,
        EFFECT_MEDIUM_DROP_SHADOW_4,
        EFFECT_MEDIUM_DROP_SHADOW_5,
        EFFECT_MEDIUM_DROP_SHADOW_6,
        EFFECT_MEDIUM_DROP_SHADOW_7,
        EFFECT_MEDIUM_DROP_SHADOW_8,
        EFFECT_HARD_DROP_SHADOW_1,
        EFFECT_HARD_DROP_SHADOW_2,
        EFFECT_HARD_DROP_SHADOW_3,
        EFFECT_HARD_DROP_SHADOW_4,
        EFFECT_HARD_DROP_SHADOW_5,
        EFFECT_HARD_DROP_SHADOW_6,
        EFFECT_HARD_DROP_SHADOW_7,
        EFFECT_HARD_DROP_SHADOW_8,
        EFFECT_SOFT_DROP_SHADOW_1_WITH_OUTLINE,
        EFFECT_SOFT_DROP_SHADOW_2_WITH_OUTLINE,
        EFFECT_SOFT_DROP_SHADOW_3_WITH_OUTLINE,
        EFFECT_SOFT_DROP_SHADOW_4_WITH_OUTLINE,
        EFFECT_SOFT_DROP_SHADOW_5_WITH_OUTLINE,
        EFFECT_SOFT_DROP_SHADOW_6_WITH_OUTLINE,
        EFFECT_SOFT_DROP_SHADOW_7_WITH_OUTLINE,
        EFFECT_SOFT_DROP_SHADOW_8_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_1_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_2_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_3_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_4_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_5_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_6_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_7_WITH_OUTLINE,
        EFFECT_MEDIUM_DROP_SHADOW_8_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_1_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_2_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_3_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_4_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_5_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_6_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_7_WITH_OUTLINE,
        EFFECT_HARD_DROP_SHADOW_8_WITH_OUTLINE
    }TEXT_EFFECT;

    typedef enum DRAW_ORDER
    {
        ORD_ELEM_NONE,
        ORD_ELEM_FILL,
        ORD_ELEM_BITMAP,
        ORD_ELEM_ICON,
        ORD_ELEM_TEXT,
        ORD_ELEM_BORDER
    }DRAW_ORDER;

    typedef enum FEEDBACK
    {
        FB_NONE,
        FB_CHANNEL,
        FB_INV_CHANNEL,     // inverted channel
        FB_ALWAYS_ON,
        FB_MOMENTARY,
        FB_BLINK
    } FEEDBACK;

    typedef enum PMIX
    {
        PMIX_MULTIPLY,
        PMIX_XOR,
        PMIX_SCREEN,
        PMIX_SRC,
        PMIX_DST,
        PMIX_SRCOVER,
        PMIX_SRCTOP,
        PMIX_DSTTOP,
        PMIX_PLUS
    }PMIX;

    typedef enum SUBVIEW_POSITION_t
    {
        SVP_CENTER,
        SVP_LEFT_TOP,
        SVP_RIGHT_BOTTOM
    }SUBVIEW_POSITION_t;

    typedef enum BUTTON_ACTION_t
    {
        BT_ACTION_LAUNCH,
        BT_ACTION_PGFLIP
    }BUTTON_ACTION_t;

    /**
     * @brief TP5 button events
     * This defines the possible event types on a button.
     */
    typedef enum BUTTON_EVENT_t
    {
        EVENT_NONE,                 // No event
        EVENT_PRESS,                // Event on button press
        EVENT_RELEASE,              // Event on button release
        EVENT_GUESTURE_ANY,         // Event on any guesture
        EVENT_GUESTURE_UP,          // Event on 1 finger up guesture
        EVENT_GUESTURE_DOWN,        // Event on 1 finger down guesture
        EVENT_GUESTURE_LEFT,        // Event on 1 finger left guesture
        EVENT_GUESTURE_RIGHT,       // Event on 1 finger right guesture
        EVENT_GUESTURE_DBLTAP,      // Event on double tap
        EVENT_GUESTURE_2FUP,        // Event on 2 finger up guesture
        EVENT_GUESTURE_2FDN,        // Event on 2 finger down guesture
        EVENT_GUESTURE_2FLT,        // Event on 2 finger left guesture
        EVENT_GUESTURE_2FRT         // Event on 2 finger right guesture
    }BUTTON_EVENT_t;

    /**
     * Justification values:
     *    0 = absolut
     *    1 = top right
     *    2 = top middle
     *    3 = top right
     *    4 = center left
     *    5 = center middle (default)
     *    6 = center right
     *    7 = bottom left
     *    8 = bottom middle
     *    9 = bottom right
     *   10 = scale to fit (ignore aspect ration)
     *   11 = scale maintain aspect ratio
     */
    typedef struct BITMAPS_t    // G5 bitmap entry
    {
        std::string fileName;   // file name of the bitmap (replaces icons)
        bool dynamic{false};    // Marks an image as a dynamic image (Video, camera, ...)
        int index{-1};          // The bitmap index number
        ORIENTATION justification{ORI_CENTER_MIDDLE};   // Justification of bitmap
        int offsetX{0};         // Absolut X position (only if justification is 0)
        int offsetY{0};         // Absolut Y position (only if justification is 0)
        int width{0};           // Internal: The width of the image
        int height{0};          // Internal: The height of the image
    }BITMAPS_t;

    typedef struct SR_T
    {
        int number{0};
        std::string _do;        // Order on how to show a multistate bargraph (010203...)
        std::string bs;         // Frame type (circle, ...)
        std::string mi;         // Chameleon image
        int mi_width{0};        // Width of image
        int mi_height{0};       // Height of image
        std::string cb;         // Border color
        std::string cf;         // Fill color
        std::string ct;         // Text Color
        std::string ec;         // Text effect color
        std::string bm;         // bitmap file name
        BITMAPS_t bitmaps[5];   // G5 table of bitmaps
        std::string sd;         // Sound file to play
        int bm_width{0};        // Width of image
        int bm_height{0};       // Height of image
        bool dynamic{false};    // TRUE = moving image
        int sb{0};              // Index to external graphics download
        int ii{0};              // Icon index number
        int ix{0};              // Icon X position
        int iy{0};              // Icon Y position
        int ji{5};              // Icon style / position like "jt", default 5 = center+middle
        int jb{5};              // Image position (center, left, ...), default 5 = center+middle
        int bx{0};              // Absolute image position x
        int by{0};              // Absolute image position y
        int fi{0};              // Font index
        std::string te;         // Text
        ORIENTATION jt{ORI_CENTER_MIDDLE}; // Text orientation
        int tx{0};              // Text X position
        int ty{0};              // Text Y position
        std::string ff;         // G5 font file name
        int fs{0};              // G5 font size
        int ww{0};              // line break when 1
        int et{0};              // Text effect (^TEF)
        int oo{-1};             // Over all opacity
        int md{0};              // Marquee type: 1 = scroll left, 2 = scroll right, 3 = ping pong, 4 = scroll up, 5 = scroll down
        int mr{0};              // Marquee enabled: 1 = enabled, 0 = disabled
        int ms{1};              // Marquee speed: Range: 1 to 10 (look for command ^MSP to see of how to set this)
    } SR_T;

    typedef struct EXTBUTTON_t
    {
        BUTTONTYPE type;
        int bi{0};              // button ID
        std::string na;         // name
        int lt{0};              // pixel from left
        int tp{0};              // pixel from top
        int wt{0};              // width
        int ht{0};              // height
        int zo{0};              // Z-Order
        std::string hs;         // bounding, ...
        std::string bs;         // Border style (circle, ...)
        FEEDBACK fb{FB_NONE};   // Feedback type (momentary, ...)
        int ap{1};              // Address port (default: 1)
        int ad{0};              // Address channel
        int lp{1};              // Level port (default: 1)
        int lv{0};              // Level code
        std::string dr;         // Level "horizontal" or "vertical"
        int lu{0};              // Animate time up (Bargraph)
        int ld{0};              // Animate time down (Bargraph)
        int rl{0};              // Range low
        int rh{0};              // Range high
        int rn{0};              // Bargraph: Range drag increment
        std::string sc;         // Color of slider (for bargraph)
        std::vector<SR_T> sr;
    }EXTBUTTON_t;

    typedef struct PUSH_FUNC
    {
        int item{0};            // TP5: Item number
        std::string pfType;     // command to execute when button was pushed
        std::string pfAction;   // TP5: Action; Used for launching external apps
        std::string pfName;     // Name of popup
        BUTTON_ACTION_t action{BT_ACTION_PGFLIP};   // TP5: Button action (page flip, ...)
        int ID{0};              // TP5: An ID for launch buttons
        BUTTON_EVENT_t event{EVENT_NONE};   // TP5: Type of event
    }PUSH_FUNC_T;

    typedef enum CENTER_CODE
    {
        SC_ICON = 0,
        SC_BITMAP,
        SC_TEXT
    }CENTER_CODE;

    typedef struct POSITION_t
    {
        int width{0};
        int height{0};
        int left{1};
        int top{1};
        bool overflow{false};
        bool valid{false};
    }POSITION_t;

    typedef struct POINT_t
    {
        int x{0};
        int y{0};
    }POINT_t;

    typedef struct IMAGE_SIZE_t
    {
        int width{0};
        int height{0};
    }IMAGE_SIZE_t;

    typedef struct THR_REFRESH_t
    {
        ulong handle{0};
        ulong parent{0};
        int bi{0};
        TImageRefresh *mImageRefresh{nullptr};
        THR_REFRESH_t *next{nullptr};
    }THR_REFRESH_t;

    typedef struct BITMAP_t
    {
        unsigned char *buffer{nullptr};
        int left{0};
        int top{0};
        int width{0};
        int height{0};
        size_t rowBytes{0};
    }BITMAP_t;

    typedef struct BITMAP_CACHE
    {
        ulong handle{0};
        ulong parent{0};
        int left{0};
        int top{0};
        int width{0};
        int height{0};
        int bi{0};
        bool show{false};
        bool ready{false};
        SkBitmap bitmap;
    }BITMAP_CACHE;

#   define LIST_IMAGE_CELL     1
#   define LIST_TEXT_PRIMARY   2
#   define LIST_TEXT_SECONDARY 4

    typedef enum LIST_SORT
    {
        LIST_SORT_NONE,
        LIST_SORT_ASC,
        LIST_SORT_DESC,
        LIST_SORT_OVERRIDE
    }LIST_SORT;

    class TButton : public TSystem, public Border::TIntBorder
    {
        public:
            TButton();
            ~TButton();

            /**
             * The following function parses the parameters of a particular
             * button and creates a new button. This function is called either
             * from class TPage or TSubPage when a page or subpage is created.
             *
             * @param xml     A pointer to the XML reader
             * @param node    A pointer to the actual node in the XML tree.
             * @return On success the last index processed. On error
             * TExpat::npos is returned.
             */
            size_t initialize(Expat::TExpat *xml, size_t index);

            /**
             * Returns the button index. This is a unique number inside a page
             * or subpage.
             *
             * @return return the button index number as defined in the
             * configuration file.
             */
            int getButtonIndex() { return bi; }
            /**
             * Returns the name of the button.
             */
            std::string& getButtonName() { return na; }
            /**
             * Returns the description of the button, if there is one.
             */
            std::string& getButtonDescription() { return bd; }
            /**
             * Returns the width of the button in pixels.
             */
            int getWidth() { return wt; }
            /**
             * Returns the height of the button in pixels.
             */
            int getHeight() { return ht; }
            /**
             * Returns the left position in pixels.
             */
            int getLeftPosition() { return mPosLeft; }
            /**
             * Returns the top position of the button in pixels.
             */
            int getTopPosition() { return mPosTop; }
            /**
             * Returns the original left position.
             */
            int getLeftOriginPosition() { return lt; }
            /**
             * Returns the original top position.
             */
            int getTopOriginPosition() { return tp; }
            /**
             * Returns the Z-order. This number marks the order the buttons
             * are drawed on the screen. Inside a page or subpage the buttons
             * are always sorted.
             */
            int getZOrder() { return zo; }
            /**
             * Returns the type of the button.
             */
            BUTTONTYPE getButtonType() { return type; }
            /**
             * Set font file name and optional the size
             *
             * @param name  File name of the font
             * @param size  The size of the font in PT
             * @param inst  The instance of the button
             *
             * @return TRUE success
             */
            bool setFontFileName(const std::string& name, int size, int inst);
            /**
             * Set font name.
             *
             * @param name  The font name
             * @param inst  The instance of the button
             *
             * @return TRUE success
             */
            bool setFontName(const std::string& name, int inst);

            std::string& getName() { return na; }
            int getRangeLow() { return rl; }
            int getRangeHigh() { return rh; }
            int getLevelRangeUp() { return ru; }
            int getLevelRangeDown() { return rd; }
            int getStateCount() { return stateCount; }
            int getAddressPort() { return ap; }
            int getAddressChannel() { return ad; }
            int getChannelNumber() { return ch; }
            int getChannelPort() { return cp; }
            int getLevelPort() { return lp; }
            int getLevelChannel() { return lv; }
            bool isBargraphInverted() { return (ri != 0); }
            bool isJoystickAuxInverted() { return (ji != 0); }
            int getLevelValue();
            void setLevelValue(int level);
            int getLevelAxisX();
            int getLevelAxisY();
            uint32_t getButtonID() { return mButtonID; }
            std::string getButtonIDstr(uint32_t rid=0x1fffffff);
            std::string& getLevelFuction() { return lf; }
            std::string getText(int inst=0);
            std::string getTextColor(int inst=0);
            std::string getTextEffectColor(int inst=0);
            void setTextEffectColor(const std::string& ec, int inst=-1);
            bool setTextEffectColorOnly(const std::string& ec, int inst=-1);
            int getTextEffect(int inst=0);
            void setTextEffect(int et, int inst=-1);
            std::string getTextEffectName(int inst=0);
            void setTextEffectName(const std::string& name, int inst=-1);
            std::string getFillColor(int inst=0);
            std::string getBitmapName(int inst=0);
            bool isSingleLine() { return ( dt.compare("multiple") != 0); }
            bool isMultiLine() { return ( dt.compare("multiple") == 0); }
            int getTextMaxChars() { return mt; }
            void setTextMaxChars(int m) { mt = m; }
            bool getTextWordWrap(int inst=0);
            bool setTextWordWrap(bool ww, int inst=-1);
            void setMarqueeSpeed(int speed, int inst=-1);
            int getMarqueeSpeed(int inst=0);
            int getFontIndex(int inst=0);
            bool setFontIndex(int fi, int inst=-1);
            int getIconIndex(int inst=0);
            std::string getSound(int inst=0);
            void setSound(const std::string& sd, int inst=-1);
            bool getDynamic(int inst=0);
            void setDynamic(int d, int inst=-1);
            int getNumberInstances() { return (int)sr.size(); }
            int getActiveInstance() { return mActInstance; }
            ulong getHandle() { return mHandle; }
            ulong getParent() { return (mHandle & 0xffff0000); }
            void setActiveInstance(int inst);
            void setEnable(bool en) { mEnabled = en; }
            bool isEnabled() { return mEnabled; }
            void setHandle(ulong handle) { mHandle = handle; };
            void setPalette(TPalette *pal) { mPalette = pal; }
            void setParentWidth(int width) { mParentWidth = width; }
            void setParentHeight(int height) { mParentHeight = height; }
            void setParentSize(int width, int height) { mParentWidth = width; mParentHeight = height; }
            void setFonts(TFont *ft) { mFonts = ft; }
            void setGlobalOpacity(int oo) { if (oo >= 0 && oo <= 255) mGlobalOO = oo; }
            void setVisible(bool v) { visible = v; hd = (v ? 0 : 1); }
            bool isVisible() { return visible; }
            bool isSubViewVertical() { return on == "vert"; }
            bool haveListContent() { return _getListContent != nullptr; }
            bool haveListRow() { return _getListRow != nullptr; }
            int getSubViewID() { return st; }
            bool getSubViewScrollbar() { return (ba == 1 ? true : false); }
            int getSubViewScrollbarOffset() { return (ba > 0 ? bo : 0); }
            bool getWrapSubViewPages() { return (ws != 0 ? true : false); }
            bool isFocused() { return mHasFocus; }
            int getTextCursorPosition() { return mCursorPosition; }
            void setChanged(bool ch) { mChanged = ch; }
            SUBVIEW_POSITION_t getSubViewAnchor();
            std::function<std::vector<std::string>(ulong handle, int ap, int ta, int ti, int rows, int columns)> getCallbackListContent() { return _getListContent; }
            std::function<std::string(int ti, int row)> getCallbackListRow() { return _getListRow; }
            std::function<void (TButton *button)> getCallbackGlobalSettings() { return _getGlobalSettings; };

            /**
             * @brief setBitmap Sets a new bitmap to the button
             * If there was already a bitmap on this button and if this bitmap
             * is different from the one in \p file, then it is erased.
             * The new bitmap file name is set and it will be loaded and created.
             *
             * @param file      File name of a bitmap file.
             * @param instance  The instance where to put the new bitmap. If
             *                  this is 0, the bitmap is set on all instances.
             * @param justify   Optional; reserved for TP5: Defines the justification
             * @param x         Optional; reserved for TP5: Defines the x origin
             * @param y         Optional; reserved for TP5: Defines the y origin
             * @return TRUE if no errors occures, otherwise FALSE.
             */
            bool setBitmap(const std::string& file, int instance, int index, int justify=5, int x=0, int y=0);
            /**
             * @brief setCameleon Sets a new cameleon bitmap to the button
             * If there was already a cameleon bitmap on this button and if this
             * cameleon bitmap is different from the one in \p file, then it is
             * erased. The new cameleon bitmap file name is set and it will be
             * loaded and created.
             *
             * @param file      File name of a cameleon bitmap file.
             * @param instance  The instance where to put the new bitmap. If
             *                  this is 0, the bitmap is set on all instances.
             * @return TRUE if no errors occures, otherwise FALSE.
             */
            bool setCameleon(const std::string& file, int instance);
            /**
             * @brief setOpacity Sets the opacity of this button
             *
             * @param op        The opacity in a reange of 0 to 255.
             * @param instance  The instance where to put the new bitmap. If
             *                  this is 0, the bitmap is set on all instances.
             * @return TRUE if no errors occures, otherwise FALSE.
             */
            bool setOpacity(int op, int instance);
            int getOpacity(int inst=0);
            int getListAp() { return ap; }
            int getListTa() { return ta; }
            int getListTi() { return ti; }
            int getListNumRows() { return tr; }
            int getListNumCols() { return tc; }
            int getSubViewSpace() { return sa; }
            std::string getBounding() { return hs; }
            bool setFont(int id, int instance);
            bool setFontOnly(int id, int instance);
            void setTop(int top);
            void setLeft(int left);
            void setLeftTop(int left, int top);
            void setRectangle(int left, int top, int right, int bottom);
            void getRectangle(int *left, int *top, int *height, int *width);
            void resetButton();
            void setResourceName(const std::string& name, int instance);
            int getBitmapJustification(int *x, int *y, int instance);
            void setBitmapJustification(int j, int x, int y, int instance);
            int getIconJustification(int *x, int *y, int instance);
            void setIconJustification(int j, int x, int y, int instance);
            int getTextJustification(int *x, int *y, int instance);
            void setTextJustification(int j, int x, int y, int instance);
            bool setTextJustificationOnly(int j, int x, int y, int instance);
            bool startAnimation(int start, int end, int time);
            /**
             * @brief registerSystemButton registers the button as a system button.
             *
             * If the button is a system button, than it has special functions.
             * The action of the button depends on the kind of system button.
             */
            void registerSystemButton();
            bool isSystemButton();
            void addPushFunction(std::string& func, std::string& page);
            void clearPushFunctions() { pushFunc.clear(); }
            void clearPushFunction(const std::string& action);
            void refresh();
            /**
             * Sets a particular instance of the button active. This implies
             * a redraw of the button in case the instance is different from
             * the one already visible.
             *
             * @param instance
             * The instance of the button to be activated.
             *
             * @return
             * On error returns FALSE.
             */
            bool setActive(int instance);
            /**
             * Sets an Icon on the button. This implies a redraw of the button
             * in case the instance is different from the one already visible.
             *
             * @param id
             * The id number of the icon.
             *
             * @param instance
             * The instance where the icon should be drawed
             *
             * @return On error returns FALSE.
             */
            bool setIcon(int id, int instance);
            /**
             * Sets an Icon on the button. This implies a redraw of the button
             * in case the instance is different from the one already visible.
             *
             * @param icon
             * The file name of the icon.
             *
             * @param instance
             * The instance where the icon should be drawed
             *
             * @return
             * On error returns FALSE.
             */
            bool setIcon(const std::string& icon, int instance);
            /**
             * Removes an icon from a button. This implies a redraw of the
             * button.
             *
             * @param instance
             * The instance number involved.
             *
             * @return
             * On error returns FALSE.
             */
            bool revokeIcon(int instance);
            /**
             * Set a string to a button.
             *
             * @param txt
             * The text to write on top of  a button.
             *
             * @param instance
             * The instance number involved.
             *
             * @return
             * On error returns FALSE.
             */
            bool setText(const std::string& txt, int instance);
            /**
             * Set a string to a button. This method does not trigger a new
             * drawing of the element.
             *
             * @param txt
             * The text to write on top of  a button.
             *
             * @param instance
             * The instance number involved.
             *
             * @return
             * On error returns FALSE.
             */
            bool setTextOnly(const std::string& txt, int instance);
            /**
             * @brief appendText    Append non-unicode text.
             * @param txt
             * The text to write on top of  a button.
             * @param instance
             * The instance number involved.
             * @return
             * On error returns FALSE.
             */
            bool appendText(const std::string& txt, int instance);
            /**
             * @brief setTextCursorPosition - Set curso position
             * If the button element is of type TEXT_INPUT, this method sets
             * the cursor position. This position comes usually from an input
             * line managed by the graphical surface.
             * If the \b newPos is less then 0 the curser is set in front of the
             * first character. If the value \b newPos is grater then the number
             * of characters then the curser is set after the last character.
             *
             * @param oldPos    The old position of the cursor
             * @param newPos    The new position of the cursor
             */
            void setTextCursorPosition(int oldPos, int newPos);
            /**
             * @brief setTextFocus - Focus of input line changed
             * If the button element is of type TEXT_INPUT, this signals the
             * state of the focus. If \b in is TRUE then the input line got
             * the focus. The ON state of the button is send. If there is
             * a keyboard present the line is then related to this keyboard.
             * If \b in is FALSE the input line lost the focus. The off state
             * is displayed and no more characters are received from a
             * keyboard.
             * If this input line is a system input line, the focus is ignored.
             * It then receives always the keystrokes from a keyboard.
             *
             * @param in    TRUE: Input line got focus, FALSE: input line
             *              lost focus.
             */
            void setTextFocus(bool in);
            /**
             * @brief setBorderColor Set the border color.
             * Set the border color to the specified color. Only if the
             * specified border color is not the same as the current color.
             * Note: Color can be assigned by color name (without spaces),
             * number or R,G,B value (RRGGBB or RRGGBBAA).
             * @param color     the color
             * @param instance
             * The instance number involved.
             * @return
             * On error returns FALSE.
             */
            bool setBorderColor(const std::string& color, int instance);
            /**
             * @brief retrieves the current border color.
             * Determines the current border color of the button and returns
             * the color as a string.
             * @param instance  The instance number of the button.
             * @return
             * If everything went well it returns the color string of the button
             * instance. On error an empty string is returned.
             */
            std::string getBorderColor(int instance);
            /**
             * @brief setFillColor Set the fill color.
             * Set the fill color to the specified color. Only if the
             * specified fill color is not the same as the current color.
             * Note: Color can be assigned by color name (without spaces),
             * number or R,G,B value (RRGGBB or RRGGBBAA).
             * @param color     the color
             * @param instance
             * The instance number involved.
             * @return
             * On error returns FALSE.
             */
            bool setFillColor(const std::string& color, int instance);
            /**
             * @brief setTextColor set the text color.
             * Set the text color to the specified color. Only if the
             * specified text color is not the same as the current color. It
             * redraws the button if the color changed.
             * Note: Color can be assigned by color name (without spaces),
             * number or R,G,B value (RRGGBB or RRGGBBAA).
             * @param color     the color
             * @param instance
             * The instance number involved.
             * @return
             * On error returns FALSE.
             */
            bool setTextColor(const std::string& color, int instance);
            /**
             * @brief setTextColorOnly set the text color only.
             * Set the text color to the specified color. Only if the
             * specified text color is not the same as the current color.
             * Note: Color can be assigned by color name (without spaces),
             * number or R,G,B value (RRGGBB or RRGGBBAA).
             * @param color     the color
             * @param instance
             * The instance number involved.
             * @return
             * On error returns FALSE.
             */
            bool setTextColorOnly(const std::string& color, int instance);
            /**
             * @brief setDrawOrder - Set the button draw order.
             * Determines what order each layer of the button is drawn.
             * @param order     the draw order
             * @param instance
             * The instance number involved.
             * @return
             * On error returns FALSE.
             */
            bool setDrawOrder(const std::string& order, int instance);
            /**
             * @brief setFeedback - Set the feedback type of the button.
             * ONLY works on General-type buttons.
             * @param fb    The feedback type
             * @return On error returns FALSE.
             */
            bool setFeedback(FEEDBACK feedback);
            /**
             * @brief setFeedback - Sets the feedback type.
             * This retrieves the feedback type of a button. Only if the button
             * is a general type one, a valid value is returned.
             *
             * @return The feedback type of a button.
             */
            FEEDBACK getFeedback();
            /**
             * Set a border to a specific border style associated with a border
             * value for those buttons with a defined address range.
             * @param style     The name of the border style
             * @param instance  -1 = style for all instances
             * > 0 means the style is valid only for this instance.
             */
            bool setBorderStyle(const std::string& style, int instance=-1);
            /**
             * Set a border to a specific border style associated with a border
             * value for those buttons with a defined address range.
             * @param style     The index number of the border style
             * @param instance  -1 = style for all instances
             * > 0 means the style is valid only for this instance.
             */
            bool setBorderStyle(int style, int instance=-1);
            /**
             * Retrieves the border style, if any, of the instance \p instance
             * and returns it.
             * @param instance  The instance from where the border style is
             *                  wanted. This value must be between 1 and the
             *                  number of available instances.
             * @return The border style if there is any or an empty string.
             */
            std::string getBorderStyle(int instance=-1);
            /**
             * Set the bargraph upper limit. Range = 1 to 65535.
             * @param limit the new limit
             * @return TRUE on success.
             */
            bool setBargraphUpperLimit(int limit);
            /**
             * Set the bargraph lower limit. Range = 1 to 65535.
             * @param limit the new limit
             * @return TRUE on success.
             */
            bool setBargraphLowerLimit(int limit);
            /**
             * Change the bargraph slider color or joystick cursor color.
             * A user can also assign the color by Name and R,G,B value
             * (RRGGBB or RRGGBBAA).
             * @param color     The color value.
             * @return TRUE on success.
             */
            bool setBargraphSliderColor(const std::string& color);
            /**
             * Set the name of the bargraph slider name or the joystick cursor
             * name.
             * @param name  The name of the slider/cursor.
             * @return TRUE on success.
             */
            bool setBargraphSliderName(const std::string& name);
            /**
             * Sets the input mask for the text area. This method has no
             * effect on any non input button.
             * @param mask  The mask.
             * @return If all mask letters are valid it returns TRUE.
             */
            bool setInputMask(const std::string& mask);
            /**
             * Returns the input mask of the button.
             * @rturn The input mask of the text area, if there is any. If
             * there is no input mask present, an empty string is returned.
             */
            std::string& getInputMask() { return im; }
            /**
             * Read the images, if any, from files and create an image. If there
             * are no files, draw the image as defined.
             *
             * @param force
             * This parameter forces the function to reload the image. This is
             * necessary if the image of the button was changed by a command.
             * This parameter is optional. Defaults to FALSE.
             *
             * @return
             * On error returns FALSE.
             */
            bool createButtons(bool force = false);
            /**
             * Register a callback function to display a ready image. This
             * function is used for nearly every kind of button or bargraph.
             * It is up to the surface to bring the buttons to screen.
             */
            void registerCallback(std::function<void (ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)> displayButton)
            {
                _displayButton = displayButton;
            }
            /**
             * Register a callback function to display a video on a special
             * button. It is up to the surface to display a video. So the
             * callback passes only the size, position and the url together
             * with a user name and password, if there is one.
             */
            void regCallPlayVideo(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> playVideo) { _playVideo = playVideo; };
            /**
             * Registers a callback function to get the content of a list.
             * This function is called to get from the parent the content of
             * a list.
             */
            void regCallListContent(std::function<std::vector<std::string>(ulong handle, int ap, int ta, int ti, int rows, int columns)> getListCtnt) { _getListContent = getListCtnt; }
            /**
             * Registers a callback function to get the global settings of the
             * global page/subpage.
             */
            void regCallGlobalSettings(std::function<void (TButton *button)> getGlobalSettings) { _getGlobalSettings = getGlobalSettings; };
            /**
             * Registers a callback function to get the content of a particular
             * row of the list. This function is called for each row detected
             * in a list. If a row contains more than one column then the
             * columns are separated by a "|" symbol.
             */
            void regCallListRow(std::function<std::string(int ti, int row)> getListRow) { _getListRow = getListRow; }
            /**
             * @brief Registers a callback which will be called on every button
             * press.
             * This is used for system buttons to catch the keyboard keys.
             *
             * @param buttonPress The function pointer.
             */
            void regCallButtonPress(std::function<void(int channel, uint handle, bool pressed)> buttonPress) { _buttonPress = buttonPress; }
            /**
             * Make a pixel array and call the callback function to display the
             * image. If there is no callback function registered, nothing
             * happens.
             * This method draws a general button. It is used for most
             * specialized buttons too.
             *
             * @param instance
             * Optional. The instance of the button to draw. If the base
             * instance should be drawn, the parameter can be omitted.
             *
             * @param show
             * Optional. By defualt TRUE. If set to FALSE, the button image is
             * not send to GUI. The image is available in the variable
             * "mLastImage" immediately after ending the method.
             *
             * @param subview
             * Optional. If this is set to true, an internal marker is set to
             * mark the button as part of a subview. If it is pressed, it calls
             * another function to show the changed state.
             *
             * @return
             * On error returns FALSE.
             */
            bool drawButton(int instance=0, bool show=true, bool subview=false);
            /**
             * Creates a pixel array and holds it for access through a callback
             * function which receives this class as a whole. The callback
             * function displays a keyboard and handles input to this text area.
             * The created image can be obtained by calling the function
             * getLastImage().
             *
             * @param instance
             * Optional. The instance of the button to draw. If the base
             * instance should be drawn, the parameter can be omitted.
             *
             * @param show
             * Optional. By defualt TRUE. If set to FALSE, the button image is
             * not send to GUI. The image is available in the variable
             * "mLastImage" immediately after ending the method.
             *
             * @return
             * On error returns FALSE.
             */
            bool drawTextArea(int instance=0);
            /**
             * Method to draw a multistate animated button. It creates a thread
             * with a timer and starts an animation by drawing every instance in
             * the defined order.
             *
             * @return
             * On error returns FALSE.
             */
            bool drawButtonMultistateAni();
            /**
             * Draws a normal bargraph with an ON and OFF state. Takes care
             * about cameleon images. If registered, it calls a callback
             * function to the GUI to display the ready image.
             *
             * @param instance
             * The instance of the bargraph to draw. This value must be in the
             * range of 0 to "stateCount".
             *
             * @param level
             * This is the level to show on the bargraph. It must be a value
             * the range of "rl" (range low) and "rh" (range high). A lower
             * value than "rl" is interpreted as "rl" and a value higher than
             * "rh" is interpreted as "rh".
             *
             * @param show
             * Optional. By default TRUE. If set to FALSE, the bargraph image is
             * not send to GUI. The image is available in the variable
             * "mLastImage" immediately after ending the method.
             *
             * @return
             * On error returns FALSE.
             */
            bool drawBargraph(int instance, int level, bool show=true);
            /**
             * @brief drawMultistateBargraph draws a bargraph comparable to a button.
             * This method draws a multistate bargraph. This is like many
             * buttons in one. But they behave like a bargraph. This means, that
             * it depends on the level how many buttons are ON and which are OFF.
             *
             * @param level The level. This defines how many buttons are at ON state.
             * @param show  If this is false, the button is invisible.
             * @return If everything went well, TRUE is returned. If an error
             * occurred it returns FALSE.
             */
            bool drawMultistateBargraph(int level, bool show=true);
            /**
             * @brief Invert bargraph/joystick
             * This method sets or unsets inverting bargraphs or the axis of a
             * joystick.
             *
             * @param invert    A value between 0 and 3. If it is a bargraph
             *                  any value > 0 means to invert the axis. For
             *                  joysticks the values mean:
             *                     0 = no invert
             *                     1 = invert horizontal axis
             *                     2 = invert vertical axis
             *                     3 = invert both axis
             */
            void setBargraphInvert(int invert);
            /**
             * @brief Change ramp down time
             *
             * @param t     Time in 1/10 seconds
             */
            void setBargraphRampDownTime(int t);
            /**
             * @brief Change ramp up time
             *
             * @param t     Time in 1/10 seconds
             */
            void setBargraphRampUpTime(int t);
            /**
             * @brief Set the increment step factor
             *
             * @param inc   Step increment.
             */
            void setBargraphDragIncrement(int inc);
            /**
             * @brief Draws a joystick field.
             * The method draws a rectangular field acting as a joystick. This
             * means, that every touch of move is translated into coordinates
             * who are send to the NetLinx.
             * This works the same prinzyp as the bargraphs do, but with 2
             * dimensions.
             *
             * @param x         The x coordinate to set the curser. If this is
             *                  -1 the default position is used.
             * @param y         The y coordinate to set the cursor. If this is
             *                  -1 the default position is used.
             * @return If everything went well, TRUE is returned.
             */
            bool drawJoystick(int x, int y);
            /**
             * Draws the curser of the joystick, if there is one. The method
             * calculates the pixel position of the cursor out of the level
             * values.
             *
             * @param bm        The bitmap where to draw the curser.
             * @param x         The x coordinate to set the curser (X level).
             * @param y         The y coordinate to set the cursor (Y level).
             * @return If everything went well, TRUE is returned.
             */
            bool drawJoystickCursor(SkBitmap *bm, int x, int y);
            /**
             * Draws the background and the frame, if any, of the box. It takes
             * the number of rows in account.
             *
             * @param show  Optional: If set to false the button will not be
             * shown.
             * @return If everything went well, TRUE is returned. If an error
             * occurred it returns FALSE.
             */
            bool drawList(bool show=true);
            /**
             * Show the button with it's current state.
             */
            void show();
            /**
             * Hide the button. In case of an animated button, the animation is
             * stopped.
             *
             * @param total
             * Optional. When set to TRUE, a transparent button is displayed.
             */
            void hide(bool total=false);
            /**
             * This method sends the image together with all data to the GUI,
             * where it will be shown. The variable mLastImage must not be
             * empty for the method to succeed.
             */
            void showLastButton();
            /**
             * Handle a mouse click.
             *
             * @param x
             * The x coordinate of the mouse press
             *
             * @param y
             * The y coordinate of the mouse press
             *
             * @param pressed
             * TRUE = Button was pressed, FALSE = Button was released.
             *
             * @return
             * TRUE = Button was clickable and processed. FALSE = Button was not
             * clickable.
             */
            bool doClick(int x, int y, bool pressed);
            /**
             * Creates a button but uses the informations in the structure
             * instead of reading it from a file.
             *
             * @param bt    A structure containing all information to create a button.
             * @return On success returns TRUE, else FALSE.
             */
            bool createSoftButton(const EXTBUTTON_t& bt);
            /**
             * Returns the image in mLastImage as a pixel array with the defined
             * dimensions.
             *
             * @return A typedef IMAGE_t containing the ingredentials of the
             * image. It contains a pointer with allocated memory holding the
             * image. This buffer must NOT be freed!
             */
            BITMAP_t getLastImage();
            /**
             * Returns the image in mLastImage as a TBitmap with the defined
             * dimensions.
             *
             * @return TBitmap class
             */
            TBitmap getLastBitmap();
            /**
             * Returns the fint the button uses.
             * @return A structure containing the informations for the font
             * to load.
             */
            FONT_T getFont();
            /**
             * Returns the style of the font.
             * @return The font style.
             */
            FONT_STYLE getFontStyle();
            /**
             * Tests the button if it is clickable.
             * @return TRUE if it is clickable, FALSE otherwise.
             */
            bool isClickable(int x = -1, int y = -1);
            /**
             * Returns the password character.
             *
             * @return An integer representing the password character. If there
             * is no password character 0 is returned.
             */
            uint getPasswordChar() { return (pc.empty() ? 0 : pc[0]); }
            /**
             * Set the level of a bargraph or a multistate bargraph.
             * The method checks the level whether it is inside the defined
             * range or not.
             *
             * @param level     The new level value
             */
            void setBargraphLevel(int level);
            /**
             * @brief Move the level to the mouse position
             * The method moves the level to the mouse position while the left
             * button is pressed. The exact behavior depends on the settings for
             * the bargraph.
             *
             * @param x     The x coordinate of the mouse
             * @param y     The y coordinate of the mouse
             */
            void moveBargraphLevel(int x, int y);
            /**
             * @brief Send the levels of a joystick
             * The method sends the level of the x and y axes of a joystick to
             * the NetLinx.
             */
            void sendJoystickLevels();
            /**
             * @brief Send the level of the bargraph
             * The method sends the level of the bargraph to the NetLinx.
             */
            void sendBargraphLevel();
            /**
             * @brief invalidate - Mark a button internal as hidden.
             * This method does not call any surface methods and marks the
             * the button only internal hidden. The graphic remains.
             *
             * @return TRUE on success.
             */
            bool invalidate();
            /**
             * Returns the draw order of the elements of a button.
             *
             * @param instance  The button instance. This value must be >= 0.
             * @return The draw order as a string with a length of 10.
             */
            std::string& getDrawOrder(int instance);
            /**
             * Returns the rows of the list in case this button is a list.
             * Otherwise an empty list is returned.
             *
             * @return A vector list containing the rows of the list.
             */
            std::vector<std::string>& getListContent() { return mListContent; }
            /**
             * @brief Listview Data Source
             * This command sets the data source to drive the Listview entries.
             * Note that this command only configures the data source it does
             * not actually cause the data to be fetched. The ^LVR refresh
             * command must be issued to load the data.
             *
             * @param source    A string containing the source for list data.
             * This can be either a URL or the name of a dynamic resource data.
             * @param configs   One or more configurations defining the layout
             * of the source.
             *
             * @return If the source is valid it returns TRUE.
             */
            bool setListSource(const std::string &source, const std::vector<std::string>& configs);
            /**
             * Returns the set source for list data.
             *
             * @return A string containg the source or and empty string if no
             * valid source was set.
             */
            std::string& getListSource() { return listSource; }
            /**
             * Sets a filter for the data of a listView.
             *
             * @param filter    A string used as a filter for the data.
             *
             * @return Returns true if the string was evaluated ok.
             */
            bool setListSourceFilter(const std::string& filter);
            /**
             * Returns the filter string for a listView data source.
             *
             * @return The filter string.
             */
            std::string& getListSourceFilter() { return listFilter; }
            /**
             * Sets the listView event number. A value of 0 turns off event
             * reporting. Any other number activates the events. Currently are
             * only refresh events are reported.
             *
             * @param num   The event number.
             */
            void setListViewEventNumber(int num) { listEvNum = num; }
            /**
             * Returns the event number of the listView. By default this number
             * is 1401. But with the command ^LVE this number can be changed.
             *
             * @return The event number of the listView.
             */
            int getListViewEventNumber() { return listEvNum; };
            void setListViewColumns(int cols);
            int getListViewColumns() { return tc; }
            void setListViewLayout(int layout);
            int getListViewLayout() { return listLayout; }
            void setListViewComponent(int comp);
            int getListViewComponent() { return listComponent; }
            void setListViewCellheight(int height, bool percent=false);
            int getListViewCellheight() { return tj; }
            void setListViewP1(int p1);
            int getListViewP1() { return listViewP1; }
            void setListViewP2(int p2);
            int getListViewP2() { return listViewP2; }
            void setListViewColumnFilter(bool filter) { listViewColFiler = filter; }
            bool getListViewColumnFilter() { return listViewColFiler; }
            void setListViewFilterHeight(int height, bool percent=false);
            int getListViewFilterHeight() { return listViewColFilterHeight; }
            void setListViewAlphaScroll(bool alpha) { listAlphaScroll = alpha; }
            bool getListViewAlphaScroll() { return listAlphaScroll; }
            void setListViewFieldMap(const std::map<std::string,std::string>& map) { listFieldMap = map; }
            std::map<std::string,std::string>& getListViewFieldMap() { return listFieldMap; }
            void listViewNavigate(const std::string& command, bool select=false);
            void listViewRefresh(int interval, bool force=false);
            void listViewSortData(const std::vector<std::string>& columns, LIST_SORT order, const std::string& override);
            int getBorderSize(const std::string& name);
            void setPassword(const std::string& pw) { mPassword = pw; }
            void setUserName(const std::string& user);
            bool haveImage(const SR_T& sr);

        protected:
            BUTTONTYPE getButtonType(const std::string& bt);
            FEEDBACK getButtonFeedback(const std::string& fb);
            SkBitmap drawImageButton(SkBitmap& imgRed, SkBitmap& imgMask, int width, int height, SkColor col1, SkColor col2);
            SkBitmap combineImages(SkBitmap& base, SkBitmap& alpha, SkColor col);

            void funcTimer(const amx::ANET_BLINK& blink);
            void funcNetwork(int state);
            void funcResource(const RESOURCE_T *resource, const std::string& url, BITMAP_CACHE bc, int instance);
#ifdef __ANDROID__
            void funcBattery(int level, bool charging, int chargeType);
#endif
#if TARGET_OS_SIMULATOR || TARGET_OS_IOS
            void funcBattery(int level, int state);
#endif
            void funcNetworkState(int level);

        private:
            std::function<void (ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)> _displayButton{nullptr};
            std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _playVideo{nullptr};
            std::function<std::vector<std::string>(ulong handle, int ap, int ta, int ti, int rows, int columns)> _getListContent{nullptr};
            std::function<std::string(int ti, int row)> _getListRow{nullptr};
            std::function<void (TButton *button)> _getGlobalSettings{nullptr};
            std::function<void (int channel, uint handle, bool pressed)> _buttonPress{nullptr};

            // Mutexes used in class
            std::mutex mutex_button;
            std::mutex mutex_click;
            std::mutex mutex_text;
            std::mutex mutex_bargraph;
            std::mutex mutex_sysdraw;
            std::mutex mutex_bmCache;

            std::string buttonTypeToString();
            std::string buttonTypeToString(BUTTONTYPE t);
            POSITION_t calcImagePosition(int width, int height, CENTER_CODE cc, int number, int line = 0);
            IMAGE_SIZE_t calcImageSize(int imWidth, int imHeight, int instance, bool aspect=false);
            void calcImageSizePercent(int imWidth, int imHeight, int btWidth, int btHeight, int btFrame, int *realX, int *realY);
            SkColor baseColor(SkColor basePix, SkColor maskPix, SkColor col1, SkColor col2);
            TEXT_EFFECT textEffect(const std::string& effect);
            int numberLines(const std::string& str);
            SkRect calcRect(int width, int height, int pen);
            void runAnimation();    // Method started as thread for button animation
            void runAnimationRange(int start, int end, ulong step);     // Method started as thread for a limited time
            bool drawAlongOrder(SkBitmap *imgButton, int instance);

            void getDrawOrder(const std::string& sdo, DRAW_ORDER *order);
            bool buttonFill(SkBitmap *bm, int instance);
            bool buttonBitmap(SkBitmap *bm, int instance);
            bool buttonBitmap5(SkBitmap *bm, int instance, bool ignFirst=false);
            bool buttonDynamic(SkBitmap *bm, int instance, bool show, bool *state=nullptr, int index=-1, bool *video=nullptr);
            bool buttonIcon(SkBitmap *bm, int instance);
            bool buttonText(SkBitmap *bm, int instance);
            bool buttonBorder(SkBitmap *bm, int instance, TSystemDraw::LINE_TYPE_t lnType=TSystemDraw::LT_OFF);
            bool isPixelTransparent(int x, int y);
            bool barLevel(SkBitmap *bm, int instance, int level);
            bool makeElement(int instance=-1);
            bool loadImage(SkBitmap *bm, SkBitmap& image, int instance);
            void _TimerCallback(ulong counter);
            void _imageRefresh(const std::string& url);
            static THR_REFRESH_t *_addResource(TImageRefresh *refr, ulong handle, ulong parent, int bi);
            static THR_REFRESH_t *_findResource(ulong handle, ulong parent, int bi);
            int calcLineHeight(const std::string& text, SkFont& font);
            bool textEffect(SkCanvas *canvas, sk_sp<SkTextBlob>& blob, SkScalar startX, SkScalar startY, int instance);
            std::string getFormatString(ORIENTATION to);
            bool checkForSound();
            bool scaleImage(SkBitmap *bm, double scaleWidth, double scaleHeight);
            bool stretchImageWidth(SkBitmap *bm, int width);
            bool stretchImageHeight(SkBitmap *bm, int height);
            bool stretchImageWH(SkBitmap *bm, int width, int height);
            SkBitmap colorImage(SkBitmap& base, SkBitmap& alpha, SkColor col, SkColor bg=0, bool useBG=false);
            bool retrieveImage(const std::string& path, SkBitmap *image);
            bool getBorderFragment(const std::string& path, const std::string& pathAlpha, SkBitmap *image, SkColor color);
            SkBitmap drawSliderButton(const std::string& slider, SkColor col);
            SkBitmap drawCursorButton(const std::string& cursor, SkColor col);
            POINT_t getImagePosition(int width, int height);

            void addToBitmapCache(BITMAP_CACHE& bc);
            BITMAP_CACHE& getBCentryByHandle(ulong handle, ulong parent);
            BITMAP_CACHE& getBCentryByBI(int bIdx);
            void removeBCentry(std::vector<BITMAP_CACHE>::iterator *elem);
            void setReady(ulong handle);
            void setInvalid(ulong handle);
            void setBCBitmap(ulong handle, SkBitmap& bm);
            void showBitmapCache();
            uint32_t pixelMix(uint32_t s, uint32_t d, uint32_t a, PMIX mix);
            bool isPassThrough();
            SkColor& flipColorLevelsRB(SkColor& color);
            void runBargraphMove(int distance, bool moveUp=false);
            void threadBargraphMove(int distance, bool moveUp);
            TButtonStates *getButtonState();
            bool isButtonEvent(const std::string& token, const std::vector<std::string>& events);               // TP5: Tests for a button event
            BUTTON_EVENT_t getButtonEvent(const std::string& token);    // TP5: Returns the button event
            std::string getBitmapNames(const SR_T& sr);
            SkRect justifyBitmap5(int instance, int index, int width, int height, int border_size=0);
            std::string getFirstImageName(const SR_T& sr);
            int getBitmapFirstIndex(const SR_T& sr);
            void moveBitmapToBm(SR_T& sr, int index=-1);
            int getDynamicBmIndex(const SR_T& sr);
            bool startVideo(const SR_T& sr);

            BUTTONTYPE type;
            int bi{0};              // button ID
            std::string na;         // name
            std::string bd;         // Description --> ignored
            int lt{0};              // pixel from left
            int tp{0};              // pixel from top
            int wt{0};              // width
            int ht{0};              // height
            int zo{0};              // Z-Order
            std::string hs;         // bounding, ...
            std::string bs;         // Border style (circle, ...)
            FEEDBACK fb{FB_NONE};   // Feedback type (momentary, ...)
            int ap{1};              // Address port (default: 1)
            int ad{0};              // Address channel
            int ch{0};              // Channel number
            int cp{1};              // Channel port (default: 1)
            int lp{1};              // Level port (default: 1)
            int lv{0};              // Level code
            int ta{0};              // Listbox table channel
            int ti{0};              // Listbox table index number of rows (all rows have this number in "cp")
            int tr{0};              // Listbox number of rows
            int tc{0};              // Listbox number of columns
            int tj{0};              // Listbox row height
            int tk{0};              // Listbox preferred row height
            int of{0};              // Listbox list offset: 0=disabled/1=enabled
            int tg{0};              // Listbox managed: 0=no/1=yes
            int so{1};              // String output port
            int co{1};              // Command port
            std::vector<std::string> cm;         // Commands to send on each button hit
            std::string dr;         // Level "horizontal" or "vertical"
            int va{0};              // Level control value
            int stateCount{0};      // State count with multistate buttons (number of states)
            int rm{0};              // State count with multistate buttons?
            int nu{2};              // Animate time up
            int nd{2};              // Animate time down
            int ar{0};              // Auto repeat (1 = true)
            int ru{2};              // Animate time up (bargraph)
            int rd{2};              // Animate time down (bargraph)
            int lu{2};              // Animate time up (Bargraph)
            int ld{2};              // Animate time down (Bargraph)
            int rv{0};              // Level control repeat time
            int rl{0};              // Range low
            int rh{0};              // Range high
            int ri{0};              // Bargraph inverted (0 = normal, 1 = inverted)
            int ji{0};              // Joystick aux inverted (0 = normal, 1 = inverted)
            int rn{0};              // Bargraph: Range drag increment
            int ac_di{0};           // (guess) Direction of text: 0 = left to right (default); 1 = right to left
            int hd{0};              // 1 = Hidden, 0 = Normal visible
            int da{0};              // 1 = Disabled, 0 = Normal active
            int pp{0};              // >0 = password protected; Range 1 to 4
            std::string lf;         // Bargraph function: empty = display only, active, active centering, drag, drag centering
            std::string sd;         // Name/Type of slider for a bargraph
            std::string vt;         // Level control type (rel = relative, abs = absolute)
            std::string cd;         // Name of cursor for a joystick
            std::string sc;         // Color of slider (for bargraph)
            std::string cc;         // Color of cursor (for joystick)
            int mt{0};              // Length of text area (0 = 2000)
            std::string dt;         // "multiple" textarea has multiple lines, else single line
            std::string im;         // Input mask of a text area
            int st{0};              // SubPageView: ID of subview list
            int ws{0};              // SubPageView: Wrap subpages; 1 = YES
            std::string on;         // SubPageView: direction: vert = vertical, if empty: horizontal which is default
            int sa{0};              // SubPageView: Percent of space between items in list
            int dy{0};              // SubPageView: Allow dynamic reordering; 1 = YES
            int rs{0};              // SubPageView: Reset view on show; 1 = YES
            int ba{0};              // SubPageView: 1 = Scrollbar is visible, 0 = No scrollbar visible
            int bo{0};              // SubPageView: Scrollbar offset in pixels; Only valid if "ba" > 0
            std::string we;         // SubPageView: Anchor position: Empty = Center, "l/t" = left/top, "r/b" = right/bottom
            std::string pc;         // Password character for text area
            std::string op;         // String the button send
            bool visible{true};     // TRUE=Button is visible
            std::vector<PUSH_FUNC_T> pushFunc;  // Push functions: This are executed on button press
            std::vector<SR_T> sr;   // The elements the button consists of
            // ListView settings (G5)
            std::string listSource; // Defines the data source for a list.
            int listEvNum{1401};       // ListView event number.
            std::string listFilter; // ListView filter string.
            int listComponent{0};   // ListView component
            int listLayout{0};      // ListView layout.
            std::map<std::string,std::string> listFieldMap; // Maps the fields from the source to the columns of the list
//            LIST_SORT listSort{LIST_SORT_NONE}; // ListView sort algorithm
            std::string listSortOverride;   // A SQL ORDER BY command like sort option. Only valid if listStort == LIST_SORT_OVERRIDE
            std::string listSourceUser;   // The user name (optional)
            std::string listSourcePass;   // The password (optional)
            bool listSourceCsv{false};  // TRUE = Source of listView is in CSV data
            bool listSourceHasHeader{false};    // TRUE = The listView data has a had line which must be ignored.
            int listViewP1{0};      // ListView layout percentage 1
            int listViewP2{0};      // ListView layout percentage 2
            bool listViewColFiler{false};   // ListView column filter state (TRUE = on)
            int listViewColFilterHeight{0}; // ListView column filter height
            bool listAlphaScroll{false};    // ListView alpha scroll state (TRUE = scrollbar visible)

            TPalette *mPalette{nullptr}; // The color palette
            // Image management
            SkBitmap mLastImage;    // The last calculated image
            ulong mHandle{0};       // internal used handle to identify button
            uint32_t mButtonID{0};  // A CRC32 checksum identifying the button.
            int mParentHeight{0};   // The height of the parent page / subpage
            int mParentWidth{0};    // The width of the parent page / subpage
            bool mEnabled{true};    // By default a button is enabled (TRUE); FALSE = Button disabled
            TFont *mFonts{nullptr}; // The font table
            int mGlobalOO{-1};      // Opacity of the whole subpage, if any
            int mActInstance{0};    // Active instance
            DRAW_ORDER mDOrder[ORD_ELEM_COUNT];  // The order to draw the elements of a button
            std::thread mThrAni;    // Thread handle for animation
            std::thread mThrRes;    // A resouce (download of a remote image/video) running in background
            std::thread mThrSlider; // Thread to move a slider (bargraph)
            std::atomic<bool> mAniRunning{false}; // TRUE = Animation is running
            std::atomic<bool> mAniStop{false};  // If TRUE, the running animation will stop
//            int mLastLevel{0};      // The last level value for a bargraph
            int mBarStartLevel{0};  // The level when a drag bargraph started.
            int mBarThreshold{0};   // The difference between start coordinate and actual coordinate (drag bargraph only)
            bool mRunBargraphMove{false}; // TRUE = The thread to move the bargraph level should run.
            bool mThreadRunMove{false}; // TRUE the thread to move the bargrapg slider is running.
//            int mLastJoyX{0};       // The last x position of the joystick curser
//            int mLastJoyY{0};       // The last y position of the joystick curser
//            int mLastSendLevelX{0}; // This is the last level send from a bargraph or the X level of a joystick
//            int mLastSendLevelY{0}; // This is the last Y level from a joystick
            bool mSystemReg{false}; // TRUE = registered as system button
            amx::ANET_BLINK mLastBlink; // This is used for the system clock buttons
            TTimer *mTimer{nullptr};    // This is for buttons displaying the time or a date. It's a thread running in background.
            static THR_REFRESH_t *mThrRefresh;  // If  we have a source to reread periodicaly, this starts a thread to do that.
            ulong mAniRunTime{0};   // The time in milliseconds an animation should run. 0 = run forever.
            BITMAP_CACHE mBCDummy;  // A dummy retuned in case no cache exists or the element was not found.
            bool mChanged{true};    // TRUE=Something changed --> button must be redrawn
            int mBorderWidth{0};    // If there is a border this is set to the pixels the border is using
            std::vector<std::string> mListContent;  // The content of a list, if this button is one
            bool mSubViewPart{false};   // TRUE = The button is part of a subview item.
            int mCursorPosition{0}; // The cursor position if this is of type TEXT_INPUT
            bool mHasFocus{false};  // If this is of type TEXT_INPUT this holds the focus state
            std::string dummy;      // dummy string used to return an empty string.
            std::string mPassword;  // Contains the last typed password (askPassword()).
            std::string mUser;      // If this contains a user name contained in the User Password list, the user is asked for a password.
            int mPosLeft{0};        // The actual left position of the button
            int mPosTop{0};         // The actual top position of the button
            int mWidthOrig{0};      // The original width
            int mHeightOrig{0};     // The original height

    };

    typedef struct BUTTONS_T
    {
        TButton *button{nullptr};
        BUTTONS_T *previous{nullptr};
        BUTTONS_T *next{nullptr};
    } BUTTONS_T;
}

#endif

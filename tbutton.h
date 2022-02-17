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

#define ORD_ELEM_COUNT  5

extern bool prg_stopped;
extern bool _restart_;

class SkFont;
class SkTextBlob;

struct RESOURCE_T;

namespace Button
{
    typedef enum BUTTONTYPE
    {
        NONE,
        GENERAL,
        MULTISTATE_GENERAL,
        BARGRAPH,
        MULTISTATE_BARGRAPH,
        JOISTICK,
        TEXT_INPUT,
        COMPUTER_CONTROL,
        TAKE_NOTE,
        SUBPAGE_VIEW
    } BUTTONTYPE;

    typedef struct SYSBORDER_t
    {
        int id{0};                  // Internal unique ID number
        char *name{nullptr};        // Name of the border
        int number{0};              // AMX number
        char *style{nullptr};       // Style to use if dynamicaly calculated
        int width{0};               // The width of the border
        int radius{0};              // Radius for rounded corners
        bool calc{false};           // TRUE = Calculated inside, FALSE = Read from images
    }SYSBORDER_t;

    typedef struct SYSBUTTONS_t
    {
        int channel{0};             // Channel number
        BUTTONTYPE type{NONE};      // Type of button
        int states{0};              // Maximum number of states
        int ll{0};                  // Level low range
        int lh{0};                  // Level high range
    }SYSBUTTONS_t;

    typedef struct SYSTEF_t         // Text effect names
    {
        int idx{0};
        std::string name;
    }SYSTEF_t;

    typedef enum TEXT_ORIENTATION
    {
        ORI_ABSOLUT,
        ORI_TOP_LEFT,
        ORI_TOP_MIDDLE,
        ORI_TOP_RIGHT,
        ORI_CENTER_LEFT,
        ORI_CENTER_MIDDLE,		// default
        ORI_CENTER_RIGHT,
        ORI_BOTTOM_LEFT,
        ORI_BOTTOM_MIDDLE,
        ORI_BOTTOM_RIGHT
    } TEXT_ORIENTATION;

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
        TEXT_ORIENTATION jt{ORI_CENTER_MIDDLE}; // Text orientation
        int tx{0};              // Text X position
        int ty{0};              // Text Y position
        int ww{0};              // line break when 1
        int et{0};              // Text effect (^TEF)
        int oo{-1};             // Over all opacity
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
        std::string pfType; // command to execute when button was pushed
        std::string pfName; // Name of popup
    } PUSH_FUNC_T;

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

    class TButton
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
            int getLeftPosition() { return lt; }
            /**
             * Returns the top position of the button in pixels.
             */
            int getTopPosition() { return tp; }
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

            int getRangeLow() { return rl; }
            int getRangeHigh() { return rh; }
            int getStateCount() { return stateCount; }
            int getAddressPort() { return ap; }
            int getAddressChannel() { return ad; }
            int getChannelNumber() { return ch; }
            int getChannelPort() { return cp; }
            int getLevelPort() { return lp; }
            int getLevelValue() { return lv; }
            std::string getText(int inst=0);
            std::string getTextColor(int inst=0);
            std::string getTextEffectColor(int inst=0);
            void setTextEffectColor(const std::string& ec, int inst=-1);
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

            /**
             * @brief setBitmap Sets a new bitmap to the button
             * If there was already a bitmap on this button and if this bitmap
             * is different from the one in \p file, then it is erased.
             * The new bitmap file name is set and it will be loaded and created.
             *
             * @param file      File name of a bitmap file.
             * @param instance  The instance where to put the new bitmap. If
             *                  this is 0, the bitmap is set on all instances.
             * @return TRUE if no errors occures, otherwise FALSE.
             */
            bool setBitmap(const std::string& file, int instance);
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
            bool setFont(int id, int instance);
            void setTop(int top);
            void setLeft(int left);
            void setLeftTop(int left, int top);
            void setResourceName(const std::string& name, int instance);
            int getBitmapJustification(int *x, int *y, int instance);
            void setBitmapJustification(int j, int x, int y, int instance);
            int getIconJustification(int *x, int *y, int instance);
            void setIconJustification(int j, int x, int y, int instance);
            int getTextJustification(int *x, int *y, int instance);
            void setTextJustification(int j, int x, int y, int instance);
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
             * @brief setTextColor Set the text color.
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
            bool setTextColor(const std::string& color, int instance);
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
             * Set a border to a specific border style associated with a border
             * value for those buttons with a defined address range.
             * @param style     The name of the border style
             * @param instance  -1 = style for all instances
             * > 0 means the style is valid only for this instance.
             */
            bool setBorderStyle(const std::string& style, int instance=-1);
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
            void registerCallback(std::function<void (ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> displayButton)
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
             * @return
             * On error returns FALSE.
             */
            bool drawButton(int instance=0, bool show=true);
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
             * @param bt    A structure containing all informations to create a button.
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

        protected:
            BUTTONTYPE getButtonType(const std::string& bt);
            FEEDBACK getButtonFeedback(const std::string& fb);
            SkBitmap drawImageButton(SkBitmap& imgRed, SkBitmap& imgMask, int width, int height, SkColor col1, SkColor col2);
            SkBitmap combineImages(SkBitmap& base, SkBitmap& alpha, SkColor col);

            void funcTimer(const amx::ANET_BLINK& blink);
            void funcNetwork(int state);
            void funcResource(const RESOURCE_T *resource, const std::string& url, BITMAP_CACHE bc, int instance);
            void funcBattery(int level, bool charging, int chargeType);
            void funcNetworkState(int level);

        private:
            typedef struct IMAGE_t
            {
                int number{0};
                SkBitmap imageMi;
                SkBitmap imageBm;

                void clear()
                {
                    number = 0;
                    imageMi.reset();
                    imageBm.reset();
                }
            }IMAGE_t;

            std::function<void (ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> _displayButton{nullptr};
            std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _playVideo{nullptr};

            POSITION_t calcImagePosition(int width, int height, CENTER_CODE cc, int number, int line = 0);
            IMAGE_SIZE_t calcImageSize(int imWidth, int imHeight, int instance, bool aspect=false);
            int getBorderSize(const std::string& name);
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
            bool buttonDynamic(SkBitmap *bm, int instance, bool show, bool *state=nullptr);
            bool buttonIcon(SkBitmap *bm, int instance);
            bool buttonText(SkBitmap *bm, int instance);
            bool buttonBorder(SkBitmap *bm, int instance);
            bool isClickable();
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
            std::string getFormatString(TEXT_ORIENTATION to);
            bool checkForSound();
            bool scaleImage(SkBitmap *bm, double scaleWidth, double scaleHeight);
            bool stretchImageWidth(SkBitmap *bm, int width);
            bool stretchImageHeight(SkBitmap *bm, int height);
            bool colorImage(SkBitmap *img, int width, int height, SkColor col, SkColor bg=0, bool useBG=false);
            bool retrieveImage(const std::string& path, SkBitmap *image);
            SkBitmap drawSliderButton(const std::string& slider, SkColor col);

            void addToBitmapCache(BITMAP_CACHE& bc);
            BITMAP_CACHE& getBCentryByHandle(ulong handle, ulong parent);
            BITMAP_CACHE& getBCentryByBI(int bIdx);
            void removeBCentry(std::vector<BITMAP_CACHE>::iterator *elem);
            void setReady(ulong handle);
            void setInvalid(ulong handle);
            void setBCBitmap(ulong handle, SkBitmap& bm);
            void showBitmapCache();

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
            std::string dr;         // Level "horizontal" or "vertical"
            int va{0};
            int stateCount{0};      // State count with multistate buttons
            int rm{0};              // State count with multistate buttons?
            int nu{2};              // Animate time up
            int nd{2};              // Animate time down
            int ar{0};              // Auto repeat (1 = true)
            int ru{0};              // Animate time up (bargraph)
            int rd{0};              // Animate time down (bargraph)
            int lu{0};              // Animate time up (Bargraph)
            int ld{0};              // Animate time down (Bargraph)
            int rv{0};
            int rl{0};              // Range low
            int rh{0};              // Range high
            int ri{0};              // Bargraph inverted (0 = normal, 1 = inverted)
            int rn{0};              // Bargraph: Range drag increment
            int ac_di{0};           // ???
            int hd{0};              // 1 = Hidden, 0 = Normal visible
            int da{0};              // 1 = Disabled, 0 = Normal active
            std::string lf;         // Bargraph function: empty = display only, active, active centering, drag, drag centering
            std::string sd;         // Name/Type of slider for a bargraph
            std::string sc;         // Color of slider (for bargraph)
            int mt{0};              // Length of text area (0 = 2000)
            std::string dt;         // "multiple" textarea has multiple lines, else single line
            std::string im;         // Input mask of a text area
            std::string pc;         // Password character for text area
            std::string op;         // String the button send
            bool visible{true};     // TRUE=Button is visible
            std::vector<PUSH_FUNC_T> pushFunc;  // Push functions: This are executed on button press
            std::vector<SR_T> sr;   // The elements the button consists of

            TPalette *mPalette{nullptr}; // The color palette
            // Image management
            std::map<int, IMAGE_t> mImages; // Contains the images in ready to use format, if there any
            SkBitmap mLastImage;    // The last calculated image
            ulong mHandle{0};       // internal used handle to identify button
            int mParentHeight{0};   // The height of the parent page / subpage
            int mParentWidth{0};    // The width of the parent page / subpage
            bool mEnabled{true};    // By default a button is enabled (TRUE); FALSE = Button disabled
            TFont *mFonts{nullptr}; // The font table
            int mGlobalOO{-1};      // Opacity of the whole subpage, if any
            int mActInstance{0};    // Active instance
            DRAW_ORDER mDOrder[ORD_ELEM_COUNT];  // The order to draw the elements of a button
            std::thread mThrAni;    // Thread handle for animation
            std::thread mThrRes;    // A resouce (download of a remote image/video) running in background
            std::atomic<bool> mAniRunning{false}; // TRUE = Animation is running
            std::atomic<bool> mAniStop{false};  // If TRUE, the running animation will stop
            int mLastLevel{0};      // The last level value for a bargraph
            bool mSystemReg{false}; // TRUE = registered as system button
            amx::ANET_BLINK mLastBlink; // This is used for the system clock buttons
            TTimer *mTimer{nullptr};    // This is for buttons displaying the time or a date. It's a thread running in background.
            static THR_REFRESH_t *mThrRefresh;  // If  we have a source to reread periodicaly, this starts a thread to do that.
            ulong mAniRunTime{0};   // The time in milliseconds an animation should run. 0 = run forever.
            BITMAP_CACHE mBCDummy;  // A dummy retuned in case no cache exists or the element was not found.
    };

    typedef struct BUTTONS_T
    {
        TButton *button{nullptr};
        BUTTONS_T *previous{nullptr};
        BUTTONS_T *next{nullptr};
    } BUTTONS_T;
}

#endif

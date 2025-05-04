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

#ifndef __TPAGEINTERFACE_H__
#define __TPAGEINTERFACE_H__

#include <functional>

#include "tfont.h"
#include "tbutton.h"
#include "tsystembutton.h"

#define REGULAR_PAGE_START              0
#define REGULAR_SUBPAGE_START           500

#define SYSTEM_PAGE_START               5000
#define SYSTEM_SUBPAGE_START            5500

#define SYSTEM_PAGE_LOGGING             5001
#define SYSTEM_PAGE_LOG_TXLOGFILE       22
#define SYSTEM_PAGE_LOG_BTRESET         23
#define SYSTEM_PAGE_LOG_BTFILE          24

#define SYSTEM_PAGE_CONTROLLER          5002
#define SYSTEM_PAGE_CTRL_SURFACE        23
#define SYSTEM_PAGE_CTRL_DOWNLOAD       27
#define SYSTEM_SUBPAGE_SURFACE          5502

#define SYSTEM_PAGE_SOUND               5005
#define SYSTEM_PAGE_SOUND_TXSYSSOUND    10
#define SYSTEM_PAGE_SOUND_TXSINGLEBEEP  12
#define SYSTEM_PAGE_SOUND_TXDOUBLEBEEP  14
#define SYSTEM_SUBPAGE_SYSTEMSOUND      5503
#define SYSTEM_SUBPAGE_SINGLEBEEP       5504
#define SYSTEM_SUBPAGE_DOUBLEBEEP       5505

#define SYSTEM_LIST_SURFACE             2023
#define SYSTEM_LIST_SYSTEMSOUND         2024
#define SYSTEM_LIST_SINGLEBEEP          2025
#define SYSTEM_LIST_DOUBLEBEEP          2026

#define SYSTEM_ITEM_SYSGAIN             6
#define SYSTEM_ITEM_CONNSTATE           8
#define SYSTEM_ITEM_SYSVOLUME           9
#define SYSTEM_ITEM_SOUNDSWITCH         17
#define SYSTEM_ITEM_FTPSURFACE          25
#define SYSTEM_ITEM_SETUPPAGE           73
#define SYSTEM_ITEM_SHUTDOWN            80
#define SYSTEM_ITEM_CONNSTRENGTH        81
#define SYSTEM_ITEM_NETLINX_IP          122
#define SYSTEM_ITEM_NETLINX_CHANNEL     123
#define SYSTEM_ITEM_NETLINX_PORT        124

#define SYSTEM_ITEM_STANDARDTIME        141
#define SYSTEM_ITEM_TIMEAMPM            142
#define SYSTEM_ITEM_TIME24              143
#define SYSTEM_ITEM_NETLINXPORT         144
#define SYSTEM_ITEM_DATEWEEKDAY         151
#define SYSTEM_ITEM_DATEMMDD            152
#define SYSTEM_ITEM_DATEDDMM            153
#define SYSTEM_ITEM_DATEMMDDYYYY        154
#define SYSTEM_ITEM_DATEDDMMYYYY        155
#define SYSTEM_ITEM_DATEMONDDYYYY       156
#define SYSTEM_ITEM_DATEDDMONYYYY       157
#define SYSTEM_ITEM_DATEYYYYMMDD        158
#define SYSTEM_ITEM_SOUNDPLAYTESTSOUND  159
#define SYSTEM_ITEM_VOLUMEUP            171
#define SYSTEM_ITEM_VOLUMEDOWN          172
#define SYSTEM_ITEM_VOLUMEMUTE          173

#define SYSTEM_ITEM_NETLINX_PTYPE       199

#define SYSTEM_ITEM_BATTERYCHARGING     234
#define SYSTEM_ITEM_BATTERYLEVEL        242

#define SYSTEM_ITEM_SINGLEBEEP          404
#define SYSTEM_ITEM_DOUBLEBEEP          405

#define SYSTEM_ITEM_BTSAVESETTINGS      412
#define SYSTEM_ITEM_BTCANCELSETTINGS    413
#define SYSTEM_ITEM_SIPENABLE           416
#define SYSTEM_ITEM_SIPPROXY            418
#define SYSTEM_ITEM_SIPPORT             419
#define SYSTEM_ITEM_SIPSTUN             420
#define SYSTEM_ITEM_SIPDOMAIN           421
#define SYSTEM_ITEM_SIPUSER             422
#define SYSTEM_ITEM_SIPPASSWORD         423

#define SYSTEM_ITEM_SYSTEMSOUND         1143

#define SYSTEM_ITEM_DEBUGINFO           2000
#define SYSTEM_ITEM_DEBUGWARNING        2001
#define SYSTEM_ITEM_DEBUGERROR          2002
#define SYSTEM_ITEM_DEBUGTRACE          2003
#define SYSTEM_ITEM_DEBUGDEBUG          2004
#define SYSTEM_ITEM_DEBUGPROTOCOL       2005
#define SYSTEM_ITEM_DEBUGALL            2006
#define SYSTEM_ITEM_DEBUGPROFILE        2007
#define SYSTEM_ITEM_DEBUGLONG           2008
#define SYSTEM_ITEM_LOGLOGFILE          2009
#define SYSTEM_ITEM_LOGRESET            2010
#define SYSTEM_ITEM_LOGFILEOPEN         2011

#define SYSTEM_ITEM_FTPUSER             2020
#define SYSTEM_ITEM_FTPPASSWORD         2021
#define SYSTEM_ITEM_FTPDOWNLOAD         2030
#define SYSTEM_ITEM_FTPPASSIVE          2031

#define SYSTEM_ITEM_SOUNDPLAYSYSSOUND   2050
#define SYSTEM_ITEM_SOUNDPLAYBEEP       2051
#define SYSTEM_ITEM_SOUNDPLAYDBEEP      2052
#define SYSTEM_ITEM_SIPIPV4             2060
#define SYSTEM_ITEM_SIPIPV6             2061
#define SYSTEM_ITEM_SIPIPHONE           2062
#define SYSTEM_ITEM_VIEWSCALEFIT        2070
#define SYSTEM_ITEM_VIEWBANNER          2071
#define SYSTEM_ITEM_VIEWNOTOOLBAR       2072
#define SYSTEM_ITEM_VIEWTOOLBAR         2073
#define SYSTEM_ITEM_VIEWROTATE          2074

#define MAX_IMAGES                      5       // G5: maximum number of images

class TSubPage;

enum SHOWEFFECT
{
    SE_NONE,
    SE_FADE,
    SE_SLIDE_LEFT,
    SE_SLIDE_RIGHT,
    SE_SLIDE_TOP,
    SE_SLIDE_BOTTOM,
    SE_SLIDE_LEFT_FADE,
    SE_SLIDE_RIGHT_FADE,
    SE_SLIDE_TOP_FADE,
    SE_SLIDE_BOTTOM_FADE
};

typedef SHOWEFFECT SHOWEFFECT_t;

typedef struct ANIMATION_t
{
    SHOWEFFECT_t showEffect{SE_NONE};
    int showTime{0};
    SHOWEFFECT_t hideEffect{SE_NONE};
    int hideTime{0};
    int offset{0};
}ANIMATION_t;

// The following enum is for G5 only
typedef enum
{
    EV_NONE,                        // Invalid event
    EV_PGFLIP,                      // Do a page flip
    EV_COMMAND,                     // Execute a command (can be a self feed command)
    EV_LAUNCH                       // Launch an external application.
}EVENT_TYPE_t;

typedef struct EVENT_t
{
    EVENT_TYPE_t evType{EV_NONE};
    int item{0};                    // The position; Lines are sorted by this number
    std::string evAction;           // The action to take on type EV_PGFLIP (sShow, ...).
    std::string name;               // The page/program to take the action on if type is EV_PGFLIP or EV_LAUNCH
    int ID{0};                      // The ID of an application to launch
    int port{0};                    // The port number if the type is EV_COMMAND.
}EVENT_t;

typedef enum
{
    COL_CLOSED,
    COL_SMALL,
    COL_FULL
}COLLAPS_STATE_t;

// Collapse direction (animation)
typedef enum
{
    COLDIR_NONE,                    // Not collapsible
    COLDIR_LEFT,                    // Collapse to left
    COLDIR_RIGHT,                   // Collapse to right
    COLDIR_UP,                      // Collapse up
    COLDIR_DOWN                     // Collapse down
}COLDIR_t;

typedef struct PAGE_T
{
    std::string popupType;                  // The type of the popup (popup only)
    int pageID{0};                          // Unique ID of popup/page
    std::string name;                       // The name of the popup/page
    int left{0};                            // Left position of popup (always 0 for pages)
    int leftOrig{0};                        // Original left position; Not used for pages
    int top{0};                             // Top position of popup (always 0 for pages)
    int topOrig{0};                         // Original top position; (not used for pages)
    int width{0};                           // Width of popup
    int widthOrig{0};                       // The original width of the popup
    int height{0};                          // Height of popup
    int heightOrig{0};                      // The original height of the popup
    int modal{0};                           // 0 = Popup/Page = non modal
    int showLockX{0};                       // G5 ?
    COLDIR_t collapseDirection{COLDIR_NONE};// G5: The direction the popup should move on collapse
    int collapseOffset{0};                  // G5: The offset to collapse to.
    bool collapsible{false};                // G5: Internal use: TRUE = popup is collapsible.
    COLLAPS_STATE_t colState{COL_CLOSED};   // G5: Internal use: Defines the state of a collapsable popup.
    std::string group;                      // Name of the group the popup belongs (popup only)
    int timeout{0};                         // Time after the popup hides in 1/10 seconds (popup only)
    SHOWEFFECT showEffect{SE_NONE};         // The effect when the popup is shown (popup only)
    int showTime{0};                        // The time reserved for the show effect (popup only)
    int showX{0};                           // End of show effect position (by default "left+width"); (popup only)
    int showY{0};                           // End of show effect position (by default "top+height"); (popup only)
    SHOWEFFECT hideEffect{SE_NONE};         // The effect when the popup hides (popup only)
    int hideTime{0};                        // The time reserved for the hide effect (popup only)
    int hideX{0};                           // End of hide effect position (by default "left"); (popup only)
    int hideY{0};                           // End of hide effect position (by default "top"); (popup only)
    int resetPos{0};                        // If this is 1 the popup is reset to it's original position and size on open
    std::vector<Button::SR_T> sr;           // Page/Popup description
    std::vector<EVENT_t> eventShow;         // G5: Events to start showing
    std::vector<EVENT_t> eventHide;         // G5: Events to start hiding
}PAGE_T;

/**
 * This class in an interface to manage pages and subpages. Both classes of
 * TPage and TSubpage have a lot of identical functions. This functions are
 * in this class.
 */
class TPageInterface : public TSystemButton
{
    public:
        virtual ~TPageInterface() {}

        bool drawText(PAGE_T& pinfo, SkBitmap *img);
        bool drawFrame(PAGE_T& pinfo, SkBitmap* bm);
#ifdef _OPAQUE_SKIA_
        bool setOpacity(SkBitmap *bm, int oo);
#endif
        virtual int getNumber() = 0;
        virtual std::string& getName() = 0;
        virtual void show() = 0;
        virtual void drop() = 0;
        virtual SkBitmap& getBgImage() = 0;
        virtual std::string getFillColor() = 0;

        void setButtons(Button::BUTTONS_T *bt) { mButtons = bt; }
        Button::BUTTONS_T *getButtons() { return mButtons; }
        Button::BUTTONS_T *addButton(Button::TButton* button);
        Button::TButton *getButton(int id);
        std::vector<Button::TButton *> getButtons(int ap, int ad);
        std::vector<Button::TButton *> getAllButtons();
        bool hasButton(int id);
        bool sortButtons();
        Button::TButton *getFirstButton();
        Button::TButton *getNextButton();
        Button::TButton *getLastButton();
        Button::TButton *getPreviousButton();

        void setSR(std::vector<Button::SR_T>& s) { sr = s; }
        std::vector<Button::SR_T>& getSR() { return sr; }

        void setFonts(TFont *f);
        TFont *getFonts() { return mFonts; }

        void setSelectedRow(ulong handle, int row);
        int getSelectedRow(ulong handle);
        std::string getSelectedItem(ulong handle);
        bool haveImage(const Button::SR_T& sr);
        bool tp5Image(SkBitmap *bm, Button::SR_T& sr, int wt, int ht, bool ignFirst=false);
        SkRect justifyBitmap5(Button::SR_T& sr, int wt, int ht, int index, int width, int height, int border_size);

        template<typename T>
        inline void registerListCallback(Button::TButton *button, T *pg)
        {
            if (!button || !pg)
                return;

            button->regCallListContent(std::bind(&TPageInterface::getListContent, pg, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
            button->regCallListRow(std::bind(&TPageInterface::getListRow, pg, std::placeholders::_1, std::placeholders::_2));
            button->regCallGlobalSettings(std::bind(&TPageInterface::setGlobalSettings, pg, std::placeholders::_1));
        }

        static bool isSystemPage(int ID) { return ID >= SYSTEM_PAGE_START && ID < SYSTEM_SUBPAGE_START; }
        static bool isSystemSubPage(int ID) { return ID >= SYSTEM_SUBPAGE_START; }
        static bool isRegularPage(int ID) { return ID > REGULAR_PAGE_START && ID < REGULAR_SUBPAGE_START; }
        static bool isRegularSubPage(int ID) { return ID >= REGULAR_SUBPAGE_START && ID < SYSTEM_PAGE_START; }

    protected:
        typedef struct LIST_t
        {
            ulong handle{0};
            int ap{0};
            int ta{0};
            int ti{0};
            int rows{0};
            int columns{0};
            int selected{-1};
            std::vector<std::string> list;
        }LIST_t;

        std::vector<std::string> getListContent(ulong handle, int ap, int ta, int ti, int rows, int columns);
        std::string getListRow(int ti, int row);
        void setGlobalSettings(Button::TButton *button);

    private:
        Button::POSITION_t calcImagePosition(PAGE_T *page, int width, int height, Button::CENTER_CODE cc, int line);
        int calcLineHeight(const std::string& text, SkFont& font);
        int numberLines(const std::string& str);
        int getSystemSelection(int ta, std::vector<std::string>& list);
        bool getBorderFragment(const std::string& path, const std::string& pathAlpha, SkBitmap* image, SkColor color);
        SkBitmap retrieveBorderImage(const std::string& pa, const std::string& pb, SkColor color, SkColor bgColor);
        bool retrieveImage(const std::string& path, SkBitmap* image);
        SkBitmap colorImage(SkBitmap& base, SkBitmap& alpha, SkColor col, SkColor bg, bool useBG);
        bool stretchImageWidth(SkBitmap *bm, int width);
        bool stretchImageHeight(SkBitmap *bm, int height);

        Button::BUTTONS_T *mButtons{nullptr};   // Chain of buttons
        int mLastButton{0};                     // Internal counter for iterating through button chain.
        std::vector<Button::SR_T> sr;           // Button instances
        TFont *mFonts{nullptr};                 // Holds the class with the font list
        std::vector<LIST_t> mLists;             // Lists of page
};

#endif

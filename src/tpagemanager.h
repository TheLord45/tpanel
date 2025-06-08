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

#ifndef __TPAGEMANAGER_H__
#define __TPAGEMANAGER_H__

#include <functional>
#include <thread>
#ifdef __ANDROID__
#include <jni.h>
#endif
#include <qglobal.h>

#include "tpagelist.h"
#include "tpage.h"
#include "tsubpage.h"
#include "tsettings.h"
#include "tpalette.h"
#include "tbutton.h"
#include "tfont.h"
#include "texternal.h"
#include "tamxnet.h"
#include "tamxcommands.h"
#include "tsystemdraw.h"
#include "tsipclient.h"
#include "tvector.h"
#include "tbitmap.h"
#include "tbuttonstates.h"
#include "tqintercom.h"
#include "tapps.h"

#define REG_CMD(func, name)     registerCommand(bind(&TPageManager::func, this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3),name)

class TIcons;
class TPageManager;

extern bool prg_stopped;
extern TIcons *gIcons;
extern TPrjResources *gPrjResources;
extern TPageManager *gPageManager;

#ifdef __ANDROID__
#define NETSTATE_WIFI   1
#define NETSTATE_CELL   2
#endif

/**
 * @brief PGSUBVIEWATOM_T
 * This struct contains the elements of an item inside a subview list.
 */
typedef struct PGSUBVIEWATOM_T
{
    ulong handle{0};
    ulong parent{0};
    int top{0};
    int left{0};
    int width{0};
    int height{0};
    int instance{0};
    TColor::COLOR_T bgcolor{0};
    TBitmap image;
    std::string bounding;

    void clear()
    {
        handle = parent = 0;
        top = left = width = height = instance = 0;
        bgcolor.alpha = bgcolor.blue = bgcolor.green = bgcolor.red = 0;
        image.clear();
        bounding.clear();
    }
}PGSUBVIEWATOM_T;

/**
 * @brief PGSUBVIEWITEM_T
 * This struct contains the overall dimensions and definition of an item of a
 * subview list. Each item may consist of one or more buttons. Each item is a
 * subpage of it's own.
 * All subpage together are the subview list.
 */
typedef struct PGSUBVIEWITEM_T
{
    ulong handle{0};                                            // The handle of the subpage
    ulong parent{0};                                            // The handle of the parent object, which contains the subview list
    int width{0};                                               // Width of subpage
    int height{0};                                              // Height of subpage
    bool scrollbar{false};                                      // TRUE = show a scrollbar
    int scrollbarOffset{0};                                     // Define the offset of the scrollbar; Which item should be visible initially?
    Button::SUBVIEW_POSITION_t position{Button::SVP_CENTER};    // Position of where to place the subpage
    bool wrap{false};                                           // TRUE = Wrap around scroll area
    TColor::COLOR_T bgcolor;                                    // Background color of subpage
    TBitmap image;                                              // Background image of subpage
    std::string bounding;                                       // Defines the bounding; This is whether it should count the whole subpage, only visible pixels or ignore it completely.
    bool show{true};                                            // TRUE = show subpage (default).
    bool dynamic{false};                                        // TRUE = Dynamic ordering of items; FALSE = Fixed ordering
    std::vector<PGSUBVIEWATOM_T> atoms;                         // Elements (buttons) of subpage

    void clear()
    {
        handle = parent = 0;
        width = height = 0;
        bgcolor.alpha = bgcolor.blue = bgcolor.green = bgcolor.red = 0;
        scrollbar = wrap = dynamic = false;
        scrollbarOffset = 0;
        position = Button::SVP_CENTER;
        image.clear();
        bounding.clear();
        show = true;
        atoms.clear();
    }
}PGSUBVIEWITEM_T;

// G5 command table for animating a popup (open, close)
//    Note: Animation of opening or closing is currently not supported.
typedef enum
{
    POPSTATE_UNKNOWN,
    POPSTATE_CLOSED,
    POPSTATE_OPEN,
    POPSTATE_DYNAMIC,   // Either minimized or maximized
    POPSTATE_ANY        // wildcard
}POPSTATE_t;

/**
 * @brief typedef struct SUBCOMMAND_t
 * This struct is used as a command table for the G5 command ^PCT. It defines
 * from which state into which a collapsible popup should change.
 */
typedef struct SUBCOMMAND_t
{
    POPSTATE_t from{POPSTATE_UNKNOWN};      // The from state (e.g. closed)
    POPSTATE_t to{POPSTATE_UNKNOWN};        // The to state (e.g. open)
    int offset{0};                          // Point of animation start (currently not supported!)
}SUBCOMMAND_t;

class TPageManager : public TAmxCommands
{
    public:
        typedef enum J_ORIENTATION
        {
            O_UNDEFINED = -1,
            O_LANDSCAPE = 0,
            O_PORTRAIT = 1,
            O_REVERSE_LANDSCAPE = 8,
            O_REVERSE_PORTRAIT = 9,
            O_FACE_UP = 15,
            O_FACE_DOWN = 16
        }J_ORIENTATION;

        typedef enum SWIPES
        {
            SW_UNKNOWN,
            SW_LEFT,
            SW_RIGHT,
            SW_UP,
            SW_DOWN
        }SWIPES;

        TPageManager();
        ~TPageManager();

        bool readPages();                                       // Read all pages and subpages
        bool readPage(const std::string& name);                 // Read the page with name \p name
        bool readPage(int ID);                                  // Read the page with id \p ID
        bool readSubPage(const std::string& name);              // Read the subpage with name \p name
        bool readSubPage(int ID);                               // Read the subpage with ID \p ID
        void updateActualPage();                                // Updates all elements of the actual page
        void updateSubpage(int ID);                             // Updates all elements of a subpage
        void updateSubpage(const std::string& name);            // Updates all elements of a subpage
        std::vector<TSubPage *> createSubViewList(int id);      // Create a list of subview pages
        void showSubViewList(int id, Button::TButton *bt);      // Creates and displays a subview list
        void updateSubViewItem(Button::TButton *bt);            // Updates an existing subview item

        TPageList *getPageList() { return mPageList; }          // Get the list of all pages
        TSettings *getSettings() { return mTSettings; }         // Get the (system) settings of the panel

        TPage *getActualPage();                                 // Get the actual page
        int getActualPageNumber() { return mActualPage; }       // Get the ID of the actual page
        int getPreviousPageNumber() { return mPreviousPage; }   // Get the ID of the previous page, if there was any.
        TSubPage *getFirstSubPage();
        TSubPage *getNextSubPage();
        TSubPage *getPrevSubPage();
        TSubPage *getLastSubPage();
        TSubPage *getFirstSubPageGroup(const std::string& group);
        TSubPage *getNextSubPageGroup();
        TSubPage *getNextSubPageGroup(const std::string& group, TSubPage *pg);
        TSubPage *getTopPage();

        TPage *getPage(int pageID);
        TPage *getPage(const std::string& name);
        bool setPage(int PageID, bool forget=false);
        bool setPage(const std::string& name, bool forget=false);
        TSubPage *getSubPage(int pageID);
        TSubPage *getSubPage(const std::string& name);
        TSubPage *deliverSubPage(const std::string& name, TPage **pg=nullptr);
        TSubPage *deliverSubPage(int number, TPage **pg=nullptr);
        bool havePage(const std::string& name);
        bool haveSubPage(const std::string& name);
        bool haveSubPage(int id);
        bool haveSubPage(const std::string& page, const std::string& name);
        bool haveSubPage(const std::string& page, int id);
        TFont *getFonts() { return mFonts; }
        Button::TButton *findButton(ulong handle);
        Button::TButton *findBargraph(int lp, int lv, ulong parent);
        void onSwipeEvent(SWIPES sw);
        double getDPI() { return mDPI; }
        void setDPI(const double dpi) { mDPI = dpi; }

        void registerCallbackDB(std::function<void(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)> displayButton) { _displayButton = displayButton; }
        void registerSetMarqueeText(std::function<void(Button::TButton *button)> marquee) { _setMarqueeText = marquee; }
        void registerDropButton(std::function<void(ulong hanlde)> dropButton) { _dropButton = dropButton; }
        void registerCBsetVisible(std::function<void(ulong handle, bool state)> setVisible) { _setVisible = setVisible; }
        void registerCallbackSP(std::function<void (ulong handle, int width, int height)> setPage) { _setPage = setPage; }
        void registerCallbackSSP(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate, bool modal, bool collapsible)> setSubPage) { _setSubPage = setSubPage; }
#ifdef _OPAQUE_SKIA_
        void registerCallbackSB(std::function<void (ulong handle, TBitmap image, int width, int height, ulong color)> setBackground) {_setBackground = setBackground; }
#else
        void registerCallbackSB(std::function<void (ulong handle, TBitmap image, int width, int height, ulong color, int opacity)> setBackground) {_setBackground = setBackground; }
#endif
        void deployCallbacks();
#ifdef __ANDROID__
        void initNetworkState();
        void stopNetworkState();
        void initBatteryState();
        void stopBatteryState();
        void initPhoneState();
        void informTPanelNetwork(jboolean conn, jint level, jint type);
        void informBatteryStatus(jint level, jboolean charging, jint chargeType);
        void informPhoneState(bool call, const std::string& pnumber);
        void initOrientation();
        void enterSetup();
#endif
#ifdef Q_OS_IOS
        void informBatteryStatus(int level, int state);
        void informTPanelNetwork(bool conn, int level, int type);
#endif
        void regCallMinimizeSubpage(std::function<void (ulong handle, int offset)> callMinimizeSubpage) { _callMinimizeSubpage = callMinimizeSubpage; }
        void regCallMaximizeSubpage(std::function<void (ulong handle, int offset)> callMaximizeSubpage) { _callMaximizeSubpage = callMaximizeSubpage; }
        void regCallDropPage(std::function<void (ulong handle)> callDropPage) { _callDropPage = callDropPage; }
        void regCallDropSubPage(std::function<void (ulong handle, ulong parent)> callDropSubPage) { _callDropSubPage = callDropSubPage; }
        void regCallPlayVideo(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> callPlayVideo) { _callPlayVideo = callPlayVideo; };
        void regCallInputText(std::function<void (Button::TButton *button, Button::BITMAP_t& bm, int frame)> callInputText) { _callInputText = callInputText; }
        void regCallListBox(std::function<void (Button::TButton *button, Button::BITMAP_t& bm, int frame)> callListBox) { _callListBox = callListBox; }
        void regCallbackKeyboard(std::function<void (const std::string& init, const std::string& prompt, bool priv)> callKeyboard) { _callKeyboard = callKeyboard; }
        void regCallbackKeypad(std::function<void (const std::string& init, const std::string& prompt, bool priv)> callKeypad) { _callKeypad = callKeypad; }
        void regCallResetKeyboard(std::function<void ()> callResetKeyboard) { _callResetKeyboard = callResetKeyboard; }
        void regCallShowSetup(std::function<void ()> callShowSetup) { _callShowSetup = callShowSetup; }
        void regCallbackNetState(std::function<void (int level)> callNetState, ulong handle);
        void unregCallbackNetState(ulong handle);
#ifdef Q_OS_ANDROID
        void regCallbackBatteryState(std::function<void (int level, bool charging, int chargeType)> callBatteryState, ulong handle);
#endif
#ifdef Q_OS_IOS
        void regCallbackBatteryState(std::function<void (int level, int state)> callBatteryState, ulong handle);
#endif
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void unregCallbackBatteryState(ulong handle);
#endif
        void regCallbackResetSurface(std::function<void ()> resetSurface) { _resetSurface = resetSurface; }
        void regCallbackShutdown(std::function<void ()> shutdown) { _shutdown = shutdown; }
        void regCallbackPlaySound(std::function<void (const std::string& file)> playSound) { _playSound = playSound; }
        void regCallbackStopSound(std::function<void ()> stopSound) { _stopSound = stopSound; }
        void regCallbackMuteSound(std::function<void (bool state)> muteSound) { _muteSound = muteSound; }
        void regCallbackSetVolume(std::function<void (int volume)> setVolume) { _setVolume = setVolume; }
        void regSendVirtualKeys(std::function<void (const std::string& str)> sendVirtualKeys) { _sendVirtualKeys = sendVirtualKeys; }
        void regShowPhoneDialog(std::function<void (bool state)> showPhoneDialog) { _showPhoneDialog = showPhoneDialog; }
        void regSetPhoneNumber(std::function<void (const std::string& number)> setPhoneNumber) { _setPhoneNumber = setPhoneNumber; }
        void regSetPhoneStatus(std::function<void (const std::string& msg)> setPhoneStatus) { _setPhoneStatus = setPhoneStatus; }
        void regSetPhoneState(std::function<void (int state, int id)> setPhoneState) { _setPhoneState = setPhoneState; }
        void regDisplayMessage(std::function<void (const std::string& msg, const std::string& title)> msg) { _displayMessage = msg; }
        void regAskPassword(std::function<void (ulong handle, const std::string& msg, const std::string& title, int x, int y)> pw) { _askPassword = pw; }
        void regFileDialogFunction(std::function<void (ulong handle, const std::string& path, const std::string& extension, const std::string& suffix)> fdlg) { _fileDialog = fdlg; }
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void regOnOrientationChange(std::function<void (int orientation)> orientationChange) { _onOrientationChange = orientationChange; }
        void regOnSettingsChanged(std::function<void (const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce)> settingsChanged) { _onSettingsChanged = settingsChanged; }
#endif
        void regRepaintWindows(std::function<void ()> repaintWindows) { _repaintWindows = repaintWindows; }
        void regToFront(std::function<void (ulong handle)> toFront) { _toFront = toFront; }
        void regSetMainWindowSize(std::function<void (int width, int height)> setSize) { _setMainWindowSize = setSize; }
        void regDownloadSurface(std::function<void (const std::string& file, size_t size)> dl) { _downloadSurface = dl; }
        void regStartWait(std::function<void (const std::string& text)> sw) { _startWait = sw; }
        void regStopWait(std::function<void ()> sw) { _stopWait = sw; }
        void regPageFinished(std::function<void (ulong handle)> pf) { _pageFinished = pf; }
        void regDisplayViewButton(std::function<void (ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T)> dvb) { _displayViewButton = dvb; }
        void regAddViewButtonItem(std::function<void (Button::TButton& button, unsigned char *buffer, int pixline)> avbi) { _addViewButtonItem = avbi; }
        void regAddViewButtonItems(std::function<void (ulong parent, std::vector<PGSUBVIEWITEM_T> items)> abvi) { _addViewButtonItems = abvi; }
        void regUpdateViewButton(std::function<void (ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor)> uvb) { _updateViewButton = uvb; }
        void regUpdateViewButtonItem(std::function<void (PGSUBVIEWITEM_T& item, ulong parent)> uvb) { _updateViewButtonItem = uvb; }
        void regShowSubViewItem(std::function<void (ulong handle, ulong parent, int position, int timer)> ssvi) { _showSubViewItem = ssvi; }
        void regToggleSubViewItem(std::function<void (ulong handle, ulong parent, int position, int timer)> tsvi) { _toggleSubViewItem = tsvi; }
        void regHideAllSubViewItems(std::function<void (ulong handle)> hasvi) { _hideAllSubViewItems = hasvi; }
        void regHideSubViewItem(std::function<void (ulong handle, ulong parent)> hsvi) { _hideSubViewItem = hsvi; }
        void regSetSubViewPadding(std::function<void (ulong handle, int padding)> ssvp) { _setSubViewPadding = ssvp; }
        void regSetSubViewAnimation(std::function<void (ulong handle, ANIMATION_t ani)> ssva) { _setSubViewAnimation = ssva; }
        void regInitializeIntercom(std::function<void (INTERCOM_t ic)> intercom) { _initializeIntercom = intercom; }
        void regIntercomStart(std::function<void ()> start) { _intercomStart = start; }
        void regIntercomStop(std::function<void ()> stop) { _intercomStop = stop; }
        void regIntercomSpkLevel(std::function<void (int level)> spk) { _intercomSpkLevel = spk; }
        void regIntercomMicLevel(std::function<void (int level)> mic) { _intercomSpkLevel = mic; }
        void regIntercomMute(std::function<void (bool mute)> mute) { _intercomMute = mute; }

        /**
         * The following function must be called to start non graphics part
         * of the panel simulator. If everything worked well, it returns TRUE.
         * otherwise a FALSE is returned and the program should be terminated.
         */
        bool run();
        /**
         * Set an X value to add to the coordinated reported by the mouse catch
         * event. In case the X coordinate reported by the event is not the real
         * X coordinate, it's possible to set an X coordinate to translate the
         * internal used coordinates.
         *
         * @param x
         * The left most pixel which correspond to X = 0
         */
        void setFirstLeftPixel(int x) { mFirstLeftPixel = x; }
        /**
         * Set an Y value to add to the coordinated reported by the mouse catch
         * event. In case the Y coordinate reported by the event is not the real
         * Y coordinate, it's possible to set an Y coordinate to translate the
         * internal used coordinates.
         *
         * @param y
         * The top most pixel which correspond to Y = 0
         */
        void setFirstTopPixel(int y) { mFirstTopPixel = y; }
        /**
         * This function must be called from a GUI whenever the left mouse
         * button was pressed or released. Also if there was a touch event
         * this function must be called in the same way. It's up to any GUI
         * to handle the mouse and or touch events.
         *
         * @param x
         * the X coordinate of the mouse/touch event
         *
         * @param y
         * the y coordinate if the mouse/touch event
         *
         * @return
         * pressed TRUE = button was pressed; FALSE = button was released
         */
        void mouseEvent(int x, int y, bool pressed);
        /**
         * @brief mouseEvent
         * This function can be called from a GUI if the coordinates of the
         * mouse click are not available but the handle of an object who
         * received a mouse click.
         *
         * @param handle    The handle of an object
         * @param x         The x coordinate inside an object where the click appeared
         * @param y         The y coordinate inside an object where the click appeared
         * @param pressed   TRUE=The mouse button is pressed.
         */
        void mouseEvent(ulong handle, int x , int y, bool pressed);
        /**
         * @brief mouseMoveEvent - Moves bar on a bargraph
         * This method looks for an object which is a bargraph. If so it is
         * moved to the position of the mouse pointer. The behavior depends on
         * the settings.
         * This method just queues the event. For real handling of it look at
         * method _mouseEvent().
         *
         * @param x     The x coordinate of the mouse pointer
         * @param y     The y coordinate of the mouse pointer
         */
        void mouseMoveEvent(int x, int y);
        /**
         * Searches for a button with the handle \p handle and determines all
         * buttons with the same port and channel. Then it sets \p txt to all
         * found buttons.
         *
         * @param handle    The handle of a button.
         * @param txt       The text usualy comming from an input line.
         * @param redraw    If this is set to true the button is redrawn.
         */
        void setTextToButton(ulong handle, const std::string& txt, bool redraw=false);
        /**
         * Iterates through all active subpages of the active page and drops
         * them.
         */
        void dropAllSubPages();
        /**
         * Closes all subpages in the group \p group.
         *
         * @param group The name of the group.
         */
        void closeGroup(const std::string& group);
        /**
         * Displays the subpage \p name. If the subpage is part of a group
         * the open subpage in the group is closed, if there was one open. Then
         * the subpage is displayed. If the subpage is not already in the
         * internal buffer it's configuration file is read and the page is
         * created.
         *
         * @param name  The name of the subpage to display.
         */
        void showSubPage(const std::string& name);
        /**
         * Loads a subpage if it was not already loaded, and returns a pointer
         * to it.
         *
         * @param name The name of the subpage
         * @return A pointer to the subpage. If the subpage couldn't be loaded,
         * it returns a nullptr.
         */
        TSubPage *loadSubPage(const std::string& name);
        /**
         * Displays the subpage \p number. If the subpage is part of a group
         * the open subpage in the group is closed, if there was one open. Then
         * the subpage is displayed. If the subpage is not already in the
         * internal buffer it's configuration file is read and the page is
         * created.
         *
         * @param name     The name of the subpage to display.
         */
        void showSubPage(int number, bool force=false);
        /**
         * Hides the subpage \p name.
         *
         * @param name  The name of the subpage to hide.
         */
        void hideSubPage(const std::string& name);
        /**
         * This is called whenever an input button finished or changed it's
         * content. It is called indirectly out of the class TQEditLine.
         * The method writes the content into the text area of the button the
         * handle points to.
         *
         * @param handle   The handle of the button.
         * @param content  The content of the text area.
         */
        void inputButtonFinished(ulong handle, const std::string& content);
        void inputCursorPositionChanged(ulong handle, int oldPos, int newPos);
        void inputFocusChanged(ulong handle, bool in);
#ifdef _SCALE_SKIA_
        void setScaleFactor(double scale) { mScaleFactor = scale; }
        double getScaleFactor() { return mScaleFactor; }
        void setScaleFactorWidth(double scale) { mScaleFactorWidth = scale; }
        double getScaleFactorWidth() { return mScaleFactorWidth; }
        void setScaleFactorHeight(double scale) { mScaleFactorHeight = scale; }
        double getScaleFactorHeight() { return mScaleFactorHeight; }
#endif
        /**
         * This method handles some external buttons. Some original panels from
         * AMX have external hard buttons. TPanel simulates some of them like
         * the arrow keys, enter button and volume. The graphical surface may
         * display a toolbar on the right side (only if there is enough space
         * left) which offers this buttons. When such a button was pressed, this
         * method is called to send them to the controller.
         *
         * @param bt        The button who was pressed
         * @param checked   On button press this is TRUE, and on release it is FALSE.
         */
        void externalButton(extButtons_t bt, bool checked);

        void sendKeyboard(const std::string& text);
        void sendKeypad(const std::string& text);
        void sendString(uint handle, const std::string& text);
        void sendGlobalString(const std::string& text);
        void sendPHNcommand(const std::string& cmd);
        void sendKeyStroke(char key);
        void sendCommandString(int port, const std::string& cmd);
        void sendLevel(int lp, int lv, int level);
        void sendInternalLevel(int lp, int lv, int level);
        void callSetPassword(ulong handle, const std::string& pw, int x, int y);
        TButtonStates *addButtonState(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv);
        TButtonStates *addButtonState(const TButtonStates& bs);
        TButtonStates *getButtonState(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv);
        TButtonStates *getButtonState(uint32_t id);
        TButtonStates *getButtonState(BUTTONTYPE t, uint32_t id);

        /**
         * This starts the communication with the AMX controller. The method
         * registers the callbacks and starts a thread. This thread runs as
         * long as a valid communication is possible or until the communication
         * end by a command.
         */
        void startUp();
        /**
         * This starts a thread running the command loop. Each event from the
         * Netlinx is entered into a vector array (doCommand()) and this
         * method starts the event loop as a thread running as long as this
         * class exists.
         */
        void runCommands();
        /**
         * This method is the thread started by the command runCommands().
         */
        void commandLoop();
        /**
         * Callback function for the AMX controller part. This function must
         * be registered to the class TAmxNet and is called from this module
         * every time an event occured from the controller.
         * This method handles the commands comming from the controller and
         * draws the necessary elements before they are sent to the GUI.
         *
         * @param cmd
         * An ANET_COMMAND structure containing the received command.
         */
        void doCommand(const amx::ANET_COMMAND& cmd);
        /**
         * Activates the setup pages unless they are not visible already.
         */
        void showSetup();
        /**
         * Determines the page or subpage the handle points to and returns the
         * selected row of a list, if there is one.
         * The row index starts by 1!
         *
         * @param handle    The handle of the button.
         * @return If the button is a list and a row was selected then a number
         * grater than 0 is returned. Otherwise -1 is returned.
         */
        int getSelectedRow(ulong handle);
        /**
         * Finds the page or subpage and returns the selected item as a string.
         * If there was no list, or no items in the list, or nothing selected,
         * an empty string is returned.
         *
         * @param handle    The handle of the button
         * @return The string of the selected item or an empty string.
         */
        std::string getSelectedItem(ulong handle);
        /**
         * Finds the corresponding list and sets the selected row. The \b row
         * must be a value starting by 1. It must not be bigger that items in
         * the list.
         *
         * @param handle    The handle of the button
         * @param row       The number of the selected row.
         */
        void setSelectedRow(ulong handle, int row, const std::string& text);
        /**
         * @brief redrawObject - redraws an object
         * The method sends the last stored graphic to the surface where it is
         * drawn.
         * If the object is not visible, it is not redrawn.
         *
         * @param handle    The handle of the object.
         */
        void redrawObject(ulong handle);
#ifdef Q_OS_IOS
        void setBattery(int level, int state) { mLastBatteryLevel = level; mLastBatteryState = state; }
#endif
#ifdef _SCALE_SKIA_
        /**
         * Set the scale factor for the system setup pages. On a desktop this
         * factor is in relation to the size of the normal pages, if there any.
         * On a mobile device the factor is in relation to the resolution of
         * the display.
         * The scale factor is calculated in qtmain.cpp: MainWindow::calcScaleSetup()
         *
         * @param scale  The overall scale factor. This is used for calculations
         * and ensures that the ratio of the objects is maintained.
         * @param sw     The scale factor for the width.
         * @param sh     The scale factor for the height.
         */
        void setSetupScaleFactor(double scale, double sw, double sh);
#endif
        // Callbacks who will be registered by the graphical surface.
        std::function<void (ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)> getCallbackDB() { return _displayButton; }
        std::function<void (Button::TButton *button)> getSetMarqueeText() { return _setMarqueeText; }
        std::function<void (ulong handle)> getCallDropButton() { return _dropButton; }
        std::function<void (ulong handle, bool state)> getVisible() { return _setVisible; };
#ifdef _OPAQUE_SKIA_
        std::function<void (ulong handle, TBitmap image, int width, int height, ulong color)> getCallbackBG() { return _setBackground; }
#else
        std::function<void (ulong handle, TBitmap image, int width, int height, ulong color, int opacity)> getCallbackBG() { return _setBackground; }
#endif
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> getCallbackPV() { return _callPlayVideo; }
        std::function<void (ulong handle, int width, int height)> getCallbackSetPage() { return _setPage; }
        std::function<void (Button::TButton *button, Button::BITMAP_t& bm, int frame)> getCallbackInputText() { return _callInputText; }
        std::function<void (Button::TButton *button, Button::BITMAP_t& bm, int frame)> getCallbackListBox() { return _callListBox; }
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate, bool modal, bool collapsible)> getCallbackSetSubPage() { return _setSubPage; }
        std::function<void (ulong handle, int offset)> getCallMinimizeSubpage() { return _callMinimizeSubpage; }
        std::function<void (ulong handle, int offset)> getCallMaximizeSubpage() { return _callMaximizeSubpage; }
        std::function<void (ulong handle)> getCallDropPage() { return _callDropPage; }
        std::function<void (ulong handle, ulong parent)> getCallDropSubPage() { return _callDropSubPage; }
        std::function<void (const std::string& file)> getCallPlaySound() { return _playSound; }
        std::function<void ()> getCallStopSound() { return _stopSound; }
        std::function<void (bool state)> getCallMuteSound() { return _muteSound; }
        std::function<void (int volume)> getCallSetVolume() { return _setVolume; }
        std::function<void (const std::string& str)> sendVirtualKeys() { return _sendVirtualKeys; }
        std::function<void (bool state)> getShowPhoneDialog() { return _showPhoneDialog; }
        std::function<void (const std::string& number)> getSetPhoneNumber() { return _setPhoneNumber; }
        std::function<void (const std::string& msg)> getSetPhoneStatus() { return _setPhoneStatus; }
        std::function<void (int state, int id)> getSetPhoneState() { return _setPhoneState; }
        std::function<void (ulong handle)> getToFront() { return _toFront; }
        std::function<void (int width, int height)> getMainWindowSizeFunc() { return _setMainWindowSize; }
        std::function<void (const std::string& file, size_t size)> getDownloadSurface() { return _downloadSurface; }
        std::function<void (const std::string& msg, const std::string& title)> getDisplayMessage() { return _displayMessage; }
        std::function<void (ulong handle, const std::string& msg, const std::string& title, int x, int y)> getAskPassword() { return _askPassword; }
        std::function<void (ulong handle, const std::string& path, const std::string& extension, const std::string& suffix)> getFileDialogFunction() { return _fileDialog; }
        std::function<void (const std::string& text)> getStartWait() { return _startWait; }
        std::function<void ()> getStopWait() { return _stopWait; }
        std::function<void (ulong handle)> getPageFinished() { return _pageFinished; }
        std::function<void (ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T fillColor)> getDisplayViewButton() { return _displayViewButton; }
        std::function<void (Button::TButton& button, unsigned char *buffer, int pixline)> getAddViewButtonItem() { return _addViewButtonItem; }
        std::function<void (ulong parent, std::vector<PGSUBVIEWITEM_T> items)> getAddViewButtonItems() { return _addViewButtonItems; }
        std::function<void (ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor)> getUpdateViewButton() { return _updateViewButton; }
        std::function<void (PGSUBVIEWITEM_T& item, ulong parent)> getUpdateViewButtonItem() { return _updateViewButtonItem; }
        std::function<void (ulong handle, ulong parent, int position, int timer)> getShowSubViewItem() { return _showSubViewItem; }
        std::function<void (ulong handle, ulong parent, int position, int timer)> getToggleSubViewItem() { return _toggleSubViewItem; }
        std::function<void (ulong handle)> getHideAllSubViewItems() { return _hideAllSubViewItems; }
        std::function<void (ulong handle, ulong parent)> getHideSubViewItem() { return _hideSubViewItem; }
        std::function<void (ulong handle, int padding)> getSetSubViewPadding() { return _setSubViewPadding; }
        std::function<void (ulong handle, ANIMATION_t ani)> getSetSubViewAnimation() { return _setSubViewAnimation; }
        std::function<void (INTERCOM_t ic)> getInitializeIntercom() { return _initializeIntercom; }
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        std::function<void (int orientation)> onOrientationChange() { return _onOrientationChange; }
        std::function<void (const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce)> onSettingsChanged() { return _onSettingsChanged; }
#endif
        bool getLevelSendState() { return mLevelSend; }
        bool getRxSendState() { return mRxOn; }
        std::function<void ()> getRepaintWindows() { return _repaintWindows; }
        int getOrientation() { return mOrientation; }
        void setOrientation(int ori) { mOrientation = ori; }
        bool getInformOrientation() { return mInformOrientation; }
        void sendOrientation();
        bool havePlaySound() { return _playSound != nullptr; }
        TSystemDraw *getSystemDraw() { return mSystemDraw; }
        void reset();
        bool getPassThrough() { return mPassThrough; }
        bool haveSetupPage() { return _callShowSetup != nullptr; }
        bool haveShutdown() { return _shutdown != nullptr; }
        void callSetupPage() { if (_callShowSetup) _callShowSetup(); }
        void callShutdown() { if (_shutdown) _shutdown(); }
#ifndef _NOSIP_
        bool getPHNautoanswer() { return mPHNautoanswer; }
        void sendPHN(std::vector<std::string>& cmds);
        void actPHN(std::vector<std::string>& cmds);
        void phonePickup(int id);
        void phoneHangup(int id);
#endif
        void addFtpSurface(const std::string& file, size_t size) { mFtpSurface.push_back(_FTP_SURFACE_t{file, size}); };

        inline size_t getFtpSurfaceSize(const std::string& file)
        {
            if (mFtpSurface.empty())
                return 0;

            std::vector<_FTP_SURFACE_t>::iterator iter;

            for (iter = mFtpSurface.begin(); iter != mFtpSurface.end(); ++iter)
            {
                if (iter->file == file)
                    return iter->size;
            }

            return 0;
        }

        void clearFtpSurface() { mFtpSurface.clear(); }

    protected:
        PAGELIST_T findPage(const std::string& name);
        PAGELIST_T findPage(int ID);
        SUBPAGELIST_T findSubPage(const std::string& name);
        SUBPAGELIST_T findSubPage(int ID);

        bool addPage(TPage *pg);
        bool addSubPage(TSubPage *pg);
        TSubPage *getCoordMatch(int x, int y);
        Button::TButton *getCoordMatchPage(int x, int y);
        void initialize();
        void dropAllPages();
        bool startComm();

    private:
        std::function<void (ulong handle, ulong parent, TBitmap image, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)> _displayButton{nullptr};
        std::function<void (Button::TButton *button)> _setMarqueeText{nullptr};
        std::function<void (ulong handle)> _dropButton{nullptr};
        std::function<void (ulong handle, bool state)> _setVisible{nullptr};
        std::function<void (ulong handle, int width, int height)> _setPage{nullptr};
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate, bool modal, bool collapsible)> _setSubPage{nullptr};
#ifdef _OPAQUE_SKIA_
        std::function<void (ulong handle, TBitmap image, int width, int height, ulong color)> _setBackground{nullptr};
#else
        std::function<void (ulong handle, TBitmap image, int width, int height, ulong color, int opacity)> _setBackground{nullptr};
#endif
        std::function<void (ulong handle, const std::string& text, const std::string& font, const std::string& family, int size, int x, int y, ulong color, ulong effectColor, FONT_STYLE style, Button::ORIENTATION ori, Button::TEXT_EFFECT effect, bool ww)> _setText{nullptr};
        std::function<void (ulong handle, int offset)> _callMinimizeSubpage{nullptr};
        std::function<void (ulong handle, int offset)> _callMaximizeSubpage{nullptr};
        std::function<void (ulong handle)> _callDropPage{nullptr};
        std::function<void (ulong handle, ulong parent)> _callDropSubPage{nullptr};
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _callPlayVideo{nullptr};
        std::function<void (Button::TButton *button, Button::BITMAP_t& bm, int frame)> _callInputText{nullptr};
        std::function<void (Button::TButton *button, Button::BITMAP_t& bm, int frame)> _callListBox{nullptr};
        std::function<void (const std::string& init, const std::string& prompt, bool priv)> _callKeyboard{nullptr};
        std::function<void (const std::string& init, const std::string& prompt, bool priv)> _callKeypad{nullptr};
        std::function<void ()> _callResetKeyboard{nullptr};
        std::function<void ()> _callShowSetup{nullptr};
        std::function<void ()> _resetSurface{nullptr};
        std::function<void ()> _shutdown{nullptr};
        std::function<void (const std::string& file)> _playSound{nullptr};
        std::function<void ()> _stopSound{nullptr};
        std::function<void (bool state)> _muteSound{nullptr};
        std::function<void (int volume)> _setVolume{nullptr};
        std::function<void (const std::string& str)> _sendVirtualKeys{nullptr};
        std::function<void (bool state)> _showPhoneDialog{nullptr};
        std::function<void (const std::string& number)> _setPhoneNumber{nullptr};
        std::function<void (const std::string& msg)> _setPhoneStatus{nullptr};
        std::function<void (int state, int id)> _setPhoneState{nullptr};
        std::function<void ()> _repaintWindows{nullptr};
        std::function<void (uint handle)> _toFront{nullptr};
        std::function<void (int width, int height)> _setMainWindowSize{nullptr};
        std::function<void (const std::string& file, size_t size)> _downloadSurface{nullptr};
        std::function<void (const std::string& msg, const std::string& title)> _displayMessage{nullptr};
        std::function<void (ulong handle, const std::string& msg, const std::string& title, int x, int y)> _askPassword{nullptr};
        std::function<void (ulong handle, const std::string& path, const std::string& extension, const std::string& suffix)> _fileDialog{nullptr};
        std::function<void (const std::string& text)> _startWait{nullptr};
        std::function<void ()> _stopWait{nullptr};
        std::function<void (ulong handle)> _pageFinished{nullptr};
        std::function<void (ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T fillColor)> _displayViewButton{nullptr};
        std::function<void (Button::TButton& button, unsigned char *buffer, int pixline)> _addViewButtonItem{nullptr};
        std::function<void (ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor)> _updateViewButton{nullptr};
        std::function<void (ulong parent, std::vector<PGSUBVIEWITEM_T> items)> _addViewButtonItems{nullptr};
        std::function<void (PGSUBVIEWITEM_T& item, ulong parent)> _updateViewButtonItem{nullptr};
        std::function<void (ulong handle, ulong parent, int position, int timer)> _showSubViewItem{nullptr};
        std::function<void (ulong handle, ulong parent, int position, int timer)> _toggleSubViewItem{nullptr};
        std::function<void (ulong handle)> _hideAllSubViewItems{nullptr};
        std::function<void (ulong handle, ulong parent)> _hideSubViewItem{nullptr};
        std::function<void (ulong handle, int padding)> _setSubViewPadding{nullptr};
        std::function<void (ulong handle, ANIMATION_t ani)> _setSubViewAnimation{nullptr};
        std::function<void (INTERCOM_t ic)> _initializeIntercom{nullptr};
        std::function<void ()> _intercomStart{nullptr};
        std::function<void ()> _intercomStop{nullptr};
        std::function<void (int level)> _intercomSpkLevel{nullptr};
        std::function<void (int level)> _intercomMicLevel{nullptr};
        std::function<void (bool mute)> _intercomMute{nullptr};
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        std::function<void (int orientation)> _onOrientationChange{nullptr};
        std::function<void (const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce)> _onSettingsChanged{nullptr};
#endif
        typedef struct _FTP_SURFACE_t
        {
            std::string file;
            size_t size{0};
        }_FTP_SURFACE_t;

        typedef enum _EVENT_TYPE
        {
            _EVENT_MOUSE_CLICK,
            _EVENT_MOUSE_MOVE
        }_EVENT_TYPE;

        typedef struct _CLICK_QUEUE_t
        {
            bool coords{false};
            ulong handle{0};
            bool pressed{false};
            int x{0};
            int y{0};
            _EVENT_TYPE eventType{_EVENT_MOUSE_CLICK};
        }_CLICK_QUEUE_t;

        std::vector<SUBCOMMAND_t> mCmdTable;

        /**
         * @brief doOverlap checks for overlapping objects
         *
         * The function checks whether some objects on the surface overlap or
         * not. If they do it returns TRUE.
         * This is used for images where we should check for a transparent
         * pixel.
         *
         * @param r1    The rectangular of the first object
         * @param r2    The rectangular of the second object
         * @return If the objects \p r1 and \p r2 overlap at least partialy,
         * TRUE is returned. Otherwise FALSE.
         */
        bool doOverlap(RECT_T r1, RECT_T r2);
        /**
         * The following function is used internaly. It searches in the map
         * table for the button corresponding to the port and channel number
         * and puts the result into a chain of buttons. This chain is returned.
         *
         * If there are some pages not yet in memory, they will be created just
         * to be able to set the button(s).
         */
        std::vector<Button::TButton *> collectButtons(std::vector<TMap::MAP_T>& map);
        /**
         * This internal function frees and destroys all settings, loaded pages
         * and sub pages as well as all buttons ens elements belonging to this
         * pages. Wehen the function is finished, the state is the same as it
         * was immediately at startup.
         * This method is called when a filetransfer starts. It is necessary to
         * start all over and read in the changed pages.
         *
         * @return Returns TRUE on success. Else it returns FALSE and the error
         * flag is set.
         */
        bool destroyAll();

        bool overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
        TPage *loadPage(PAGELIST_T& pl, bool *refresh=nullptr);
        void setButtonCallbacks(Button::TButton *bt);
        bool sendCustomEvent(int value1, int value2, int value3, const std::string& msg, int evType, int cp, int cn);
        void reloadSystemPage(TPage *page);
        bool _setPageDo(int pageID, const std::string& name, bool forget);
        void runClickQueue();
        void runUpdateSubViewItem();
        void _mouseEvent(int x, int y, bool pressed);
        void _mouseEvent(ulong handle, int x, int y, bool pressed);
        void _updateSubViewItem(Button::TButton *bt);
        void _mouseMoveEvent(int x, int y);
#ifndef _NOSIP_
        std::string sipStateToString(TSIPClient::SIP_STATE_t s);
#endif
        // List of command functions
        void doFTR(int port, std::vector<int>&, std::vector<std::string>& pars);
        void doLEVON(int, std::vector<int>&, std::vector<std::string>&);
        void doLEVOF(int, std::vector<int>&, std::vector<std::string>&);
        void doRXON(int, std::vector<int>&, std::vector<std::string>&);
        void doRXOF(int, std::vector<int>&, std::vector<std::string>&);

        void doON(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doOFF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLEVEL(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBLINK(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doVER(int port, std::vector<int>& channels, std::vector<std::string>& pars);
#ifndef _NOSIP_
        void doWCN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
#endif
        void doAFP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAPG(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doCPG(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doDPG(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPHE(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPHP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPHT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPOP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPA(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPG(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPK(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPM(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPPX(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPSE(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPSP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPST(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPAGE(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPCL(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPCT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPTC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPTO(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void doANI(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAPF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBAT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBAU(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBCB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getBCB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBCF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getBCF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBCT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getBCT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBDO(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBFB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBIM(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBMC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBMF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBML(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBMP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getBMP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBOP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getBOP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBOR(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBOS(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBRD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getBRD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBSP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBSM(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBSO(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBWW(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getBWW(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doCPF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doDPF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doENA(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doFON(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getFON(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGDI(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGIV(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGLH(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGLL(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGSC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGRD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGRU(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGSN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doICO(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getICO(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doJSB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getJSB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doJSI(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getJSI(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doJST(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getJST(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doMSP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSHO(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTEC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getTEC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTEF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getTEF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTXT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getTXT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doUNI(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doUTF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doVTP(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void doKPS(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doVKS(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void doAPWD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPWD(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void doBBR(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doRAF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doRFR(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doRMF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doRSR(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void doAKB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAKEYB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAKP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAKEYP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAKEYR(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAKR(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doABEEP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doADBEEP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBEEP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doDBEEP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doEKP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPKB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPKP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doRPP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSetup(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doShutdown(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSOU(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doMUT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTKP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doVKB(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void doLPB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLPC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLPR(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLPS(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void getMODEL(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doICS(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doICE(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doICM(int port, std::vector<int>& channels, std::vector<std::string>& pars);

#ifndef _NOSIP_
        void doPHN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getPHN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
#endif
        // Commands for subviews (G4/G5)
        void doSHA(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSHD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSPD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSSH(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSTG(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        // Commands for ListView (G5)
        void doLVD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLVE(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLVF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLVL(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLVM(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLVN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLVR(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLVS(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        // Commands inherited from TPControl (Touch Panel Control)
        void doTPCCMD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTPCACC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
#ifndef _NOSIP_
        void doTPCSIP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
#endif
        std::mutex surface_mutex;
        std::mutex click_mutex;
        std::mutex updview_mutex;

        bool mLevelSend{false};                         // TRUE = Level changes are send to the master
        bool mRxOn{false};                              // TRUE = String changes are send to master
        int mActualPage{0};                             // The number of the actual visible page
        int mPreviousPage{0};                           // The number of the previous page
        int mLastSubPage{0};                            // The last sorted subpage accessed
        int mSavedPage{0};                              // The number of the last normal page. This is set immediately before a setup page is shown
        int mFirstLeftPixel{0};                         // Pixels to add to left (x) mouse coordinate
        int mFirstTopPixel{0};                          // Pixels to add to top (y) mouse position
        std::string mActualGroupName;                   // The name of the actual group
        TSubPage *mActualGroupPage{nullptr};            // Pointer to subpage of a group which is visible

        amx::TAmxNet *mAmxNet{nullptr};                 // Pointer to class TAmxNet; Handles the communication with the controller
        TPageList *mPageList{nullptr};                  // List of available pages and subpages
        PCHAIN_T *mPchain{nullptr};                     // Pointer to chain of pages in memory
        SPCHAIN_T *mSPchain{nullptr};                   // Pointer to chain of subpages in memory for the actual page
        TSettings *mTSettings{nullptr};                 // Pointer to basic settings for the panel
        TPalette *mPalette{nullptr};                    // Pointer to the color handler
        TFont *mFonts{nullptr};                         // Pointer to the font handler
        TExternal *mExternal{nullptr};                  // Pointer to the external buttons (if any)
        TApps *mApps{nullptr};                          // Pointer to external apps for Android (G5)
        TSystemDraw *mSystemDraw{nullptr};              // A pointer to the (optional) system resources
        std::thread mThreadAmxNet;                      // The thread handle to the controler handler
        TVector<amx::ANET_COMMAND> mCommands;           // Command queue of commands received from controller
        std::string mCmdBuffer;                         // Internal used buffer for commands who need more than one network package
        std::string mAkbText;                           // This is the text for the virtual keyboard (@AKB)
        std::string mAkpText;                           // This is the text for the virtual keyad (@AKP)
        bool mPassThrough{false};                       // Can ve set to true with the command ^KPS
        bool mInformOrientation{false};                 // TRUE = The actual screen orientation is reported to the controller if it change.
        int mOrientation{0};                            // Contains the actual orientation.
        int mLastPagePush{0};                           // The number of the last page received a push (key press / mouse hit)
        double mDPI{96.0};                              // DPI (Dots Per Inch) of the primary display.
        std::atomic<bool> cmdLoop_busy{false};          // As long as this is true the command loop thread is active
        std::thread mThreadCommand;                     // Thread handle for command loop thread.
        std::vector<int> mSavedSubpages;                // When setup pages are called this contains the actual open subpages
        std::vector<_FTP_SURFACE_t> mFtpSurface;        // Contains a list of TP4 surface files gained from a NetLinx.
        std::atomic<bool> mClickQueueRun{false};        // TRUE = The click queue thread is running. FALSE: the thread should stop or is not running.
        std::vector<_CLICK_QUEUE_t> mClickQueue;        // A queue holding click requests. Needed to serialize the clicks.
        std::vector<Button::TButton *> mUpdateViews;    // A queue for the method "updateSubViewItem()"
        bool mUpdateViewsRun{false};                    // TRUE = The thread for the queue mUpdateViews is running
        std::vector<TButtonStates *> mButtonStates;       // Holds the states for each button
        // SIP
#ifndef _NOSIP_
        bool mPHNautoanswer{false};                     // The state of the SIP autoanswer
        TSIPClient *mSIPClient{nullptr};                // Includes the SIP client
#endif
#ifdef _SCALE_SKIA_
        double mScaleFactor{1.0};                       // The scale factor to zoom or shrink all components
        double mScaleFactorWidth{1.0};                  // The individual scale factor for the width
        double mScaleFactorHeight{1.0};                 // The individual scale factor for the height

        double mScaleSystem{1.0};                       // The scale factor for the system setup pages
        double mScaleSystemWidth{1.0};                  // The width scale factor for the system setup pages
        double mScaleSystemHeight{1.0};                 // The height scale factor for the system setup pages
#endif
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        int mNetState{0};                               // On Android and IOS remembers the type of connection to the network (cell or wifi)
#endif
        std::map<int, std::function<void (int level)> > mNetCalls;  // List of callbacks for the network state multistate bargraph
#ifdef Q_OS_ANDROID
        std::map<int, std::function<void (int level, bool charging, int chargeType)> > mBatteryCalls;
#endif
#ifdef Q_OS_IOS
        std::map<int, std::function<void (int level, int state)> > mBatteryCalls;
        int mLastBatteryLevel{0};
        int mLastBatteryState{0};
#endif
};

#ifdef __ANDROID__
extern "C" {
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_NetworkStatus_informTPanelNetwork(JNIEnv */*env*/, jclass /*clazz*/, jboolean conn, jint level, jint type);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_BatteryState_informBatteryStatus(JNIEnv */*env*/, jclass /*clazz*/, jint level, jboolean charge, jint chargeType);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_PhoneCallState_informPhoneState(JNIEnv *env, jclass cl, jboolean call, jstring pnumber);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_Logger_logger(JNIEnv *env, jclass cl, jint mode, jstring msg);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_Orientation_informTPanelOrientation(JNIEnv */*env*/, jclass /*clazz*/, jint orientation);
    // Settings
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxIp(JNIEnv *env, jclass clazz, jstring ip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxPort(JNIEnv *env, jclass clazz, jint port);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxChannel(JNIEnv *env, jclass clazz, jint channel);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxType(JNIEnv *env, jclass clazz, jstring type);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxFtpUser(JNIEnv *env, jclass clazz, jstring user);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxFtpPassword(JNIEnv *env, jclass clazz, jstring pw);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxSurface(JNIEnv *env, jclass clazz, jstring surface);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxFtpPassive(JNIEnv *env, jclass clazz, jboolean passive);

    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewScale(JNIEnv *env, jclass clazz, jboolean scale);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewToolbar(JNIEnv *env, jclass clazz, jboolean bar);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewToolbarForce(JNIEnv *env, jclass clazz, jboolean bar);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewRotation(JNIEnv *env, jclass clazz, jboolean rotate);

    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundSystem(JNIEnv *env, jclass clazz, jstring sound);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundSingle(JNIEnv *env, jclass clazz, jstring sound);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundDouble(JNIEnv *env, jclass clazz, jstring sound);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundEnable(JNIEnv *env, jclass clazz, jboolean sound);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundVolume(JNIEnv *env, jclass clazz, jint sound);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundGaim(JNIEnv *env, jclass clazz, jint sound);

    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipProxy(JNIEnv *env, jclass clazz, jstring sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipPort(JNIEnv *env, jclass clazz, jint sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipTlsPort(JNIEnv *env, jclass clazz, jint sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipStun(JNIEnv *env, jclass clazz, jstring sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipDomain(JNIEnv *env, jclass clazz, jstring sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipUser(JNIEnv *env, jclass clazz, jstring sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipPassword(JNIEnv *env, jclass clazz, jstring sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipIpv4(JNIEnv *env, jclass clazz, jboolean sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipIpv6(JNIEnv *env, jclass clazz, jboolean sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipEnabled(JNIEnv *env, jclass clazz, jboolean sip);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipIphone(JNIEnv *env, jclass clazz, jboolean sip);

    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogInfo(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogWarning(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogError(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogTrace(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogDebug(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogProfile(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogLongFormat(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogEnableFile(JNIEnv *env, jclass clazz, jboolean log);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogFile(JNIEnv *env, jclass clazz, jstring log);

    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword1(JNIEnv *env, jclass clazz, jstring pw);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword2(JNIEnv *env, jclass clazz, jstring pw);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword3(JNIEnv *env, jclass clazz, jstring pw);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword4(JNIEnv *env, jclass clazz, jstring pw);

    JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_saveSettings(JNIEnv *env, jclass clazz);
}
#endif  // __ANDROID__

#endif

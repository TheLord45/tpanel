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

#ifndef __TPAGEMANAGER_H__
#define __TPAGEMANAGER_H__

#include <functional>
#include <thread>
#ifdef __ANDROID__
#include <jni.h>
#endif
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

        bool readPages();                           //!< Read all pages and subpages
        bool readPage(const std::string& name);     //!< Read the page with name \p name
        bool readPage(int ID);                      //!< Read the page with id \p ID
        bool readSubPage(const std::string& name);  //!< Read the subpage with name \p name
        bool readSubPage(int ID);                   //!< Read the subpage with ID \p ID

        TPageList *getPageList() { return mPageList; }      //!< Get the list of all pages
        TSettings *getSettings() { return mTSettings; }     //!< Get the (system) settings of the panel

        TPage *getActualPage();                             //!< Get the actual page
        int getActualPageNumber() { return mActualPage; }   //!< Get the ID of the actual page
        int getPreviousPageNumber() { return mPreviousPage; }   //!< Get the ID of the previous page, if there was any.
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
        bool setPage(int PageID);
        bool setPage(const std::string& name, bool forget=false);
        TSubPage *getSubPage(int pageID);
        TSubPage *getSubPage(const std::string& name);
        TSubPage *deliverSubPage(const std::string& name, TPage **pg=nullptr);
        bool havePage(const std::string& name);
        bool haveSubPage(const std::string& name);
        bool haveSubPage(int id);
        bool haveSubPage(const std::string& page, const std::string& name);
        bool haveSubPage(const std::string& page, int id);
        TFont *getFonts() { return mFonts; }
        Button::TButton *findButton(ulong handle);
        void onSwipeEvent(SWIPES sw);
        double getDPI() { return mDPI; }
        void setDPI(const double dpi) { mDPI = dpi; }

        void registerCallbackDB(std::function<void(ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> displayButton) { _displayButton = displayButton; }
        void registerCBsetVisible(std::function<void(ulong handle, bool state)> setVisible) { _setVisible = setVisible; }
        void registerCallbackSP(std::function<void (ulong handle, int width, int height)> setPage) { _setPage = setPage; }
        void registerCallbackSSP(std::function<void (ulong handle, int left, int top, int width, int height, ANIMATION_t animate)> setSubPage) { _setSubPage = setSubPage; }
        void registerCallbackSB(std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> setBackground) {_setBackground = setBackground; }
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
#endif
        void regCallDropPage(std::function<void (ulong handle)> callDropPage) { _callDropPage = callDropPage; }
        void regCallDropSubPage(std::function<void (ulong handle)> callDropSubPage) { _callDropSubPage = callDropSubPage; }
        void regCallPlayVideo(std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> callPlayVideo) { _callPlayVideo = callPlayVideo; };
        void regCallInputText(std::function<void (Button::TButton *button, Button::BITMAP_t& bm)> callInputText) { _callInputText = callInputText; }
        void regCallbackKeyboard(std::function<void (const std::string& init, const std::string& prompt, bool priv)> callKeyboard) { _callKeyboard = callKeyboard; }
        void regCallbackKeypad(std::function<void (const std::string& init, const std::string& prompt, bool priv)> callKeypad) { _callKeypad = callKeypad; }
        void regCallResetKeyboard(std::function<void ()> callResetKeyboard) { _callResetKeyboard = callResetKeyboard; }
        void regCallShowSetup(std::function<void ()> callShowSetup) { _callShowSetup = callShowSetup; }
        void regCallbackNetState(std::function<void (int level)> callNetState, ulong handle);
        void unregCallbackNetState(ulong handle);
        void regCallbackBatteryState(std::function<void (int level, bool charging, int chargeType)> callBatteryState, ulong handle);
        void unregCallbackBatteryState(ulong handle);
        void regCallbackResetSurface(std::function<void ()> resetSurface) { _resetSurface = resetSurface; }
        void regCallbackShutdown(std::function<void ()> shutdown) { _shutdown = shutdown; }
        void regCallbackPlaySound(std::function<void (const std::string& file)> playSound) { _playSound = playSound; }
        void regCallbackStopSound(std::function<void ()> stopSound) { _stopSound = stopSound; }
        void regCallbackMuteSound(std::function<void (bool state)> muteSound) { _muteSound = muteSound; }
        void regSendVirtualKeys(std::function<void (const std::string& str)> sendVirtualKeys) { _sendVirtualKeys = sendVirtualKeys; }
        void regShowPhoneDialog(std::function<void (bool state)> showPhoneDialog) { _showPhoneDialog = showPhoneDialog; }
        void regSetPhoneNumber(std::function<void (const std::string& number)> setPhoneNumber) { _setPhoneNumber = setPhoneNumber; }
        void regSetPhoneStatus(std::function<void (const std::string& msg)> setPhoneStatus) { _setPhoneStatus = setPhoneStatus; }
        void regSetPhoneState(std::function<void (int state, int id)> setPhoneState) { _setPhoneState = setPhoneState; }
#ifdef __ANDROID__
        void regOnOrientationChange(std::function<void (int orientation)> orientationChange) { _onOrientationChange = orientationChange; }
#endif
        void regRepaintWindows(std::function<void ()> repaintWindows) { _repaintWindows = repaintWindows; }
        void regToFront(std::function<void (ulong handle)> toFront) { _toFront = toFront; }

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
         * Searches for a button with the handle \p handle and determines all
         * buttons with the same port and channel. Then it sets \p txt to all
         * found buttons.
         *
         * @param handle    The handle of a button.
         * @param txt       The text usualy comming from an input line.
         */
        void setTextToButton(ulong handle, const std::string& txt);
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
         * Displays a the subpage \p name. If the subpage is part of a group
         * the open subpage in the group is closed, if there was one open. Then
         * the subpage is displayed. If the subpage is not already in the
         * internal buffer it's configuration file is read and the page is
         * created.
         *
         * @param name  The name of the subpage to display.
         */
        void showSubPage(const std::string& name);
        /**
         * Hides the subpage \p name.
         *
         * @param name  The name of the subpage to hide.
         */
        void hideSubPage(const std::string& name);
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
        /**
         * This starts the communication with the AMX controller. The method
         * registers the callbacks and starts a thread. This thread runs as
         * long as a valid communication is possible or until the communication
         * end by a command.
         */
        void startUp();
        /**
         * This starts a thread running the command loop. Each event from the
         * Netlinx is entered into a vector array (doCommand()) and this this
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

        // Callbacks who will be registered by the graphical surface.
        std::function<void (ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> getCallbackDB() { return _displayButton; }
        std::function<void (ulong handle, bool state)> getVisible() { return _setVisible; };
        std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> getCallbackBG() { return _setBackground; }
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> getCallbackPV() { return _callPlayVideo; }
        std::function<void (ulong handle, int width, int height)> getCallbackSetPage() { return _setPage; }
        std::function<void (Button::TButton *button, Button::BITMAP_t& bm)> getCallbackInputText() { return _callInputText; }
        std::function<void (ulong handle, int left, int top, int width, int height, ANIMATION_t animate)> getCallbackSetSubPage() { return _setSubPage; }
        std::function<void (ulong handle)> getCallDropPage() { return _callDropPage; }
        std::function<void (ulong handle)> getCallDropSubPage() { return _callDropSubPage; }
        std::function<void (const std::string& file)> getCallPlaySound() { return _playSound; }
        std::function<void ()> getCallStopSound() { return _stopSound; }
        std::function<void (bool state)> getCallMuteSound() { return _muteSound; }
        std::function<void (const std::string& str)> sendVirtualKeys() { return _sendVirtualKeys; }
        std::function<void (bool state)> getShowPhoneDialog() { return _showPhoneDialog; }
        std::function<void (const std::string& number)> getSetPhoneNumber() { return _setPhoneNumber; }
        std::function<void (const std::string& msg)> getSetPhoneStatus() { return _setPhoneStatus; }
        std::function<void (int state, int id)> getSetPhoneState() { return _setPhoneState; }
        std::function<void (ulong handle)> getToFront() { return _toFront; }
#ifdef __ANDROID__
        std::function<void (int orientation)> onOrientationChange() { return _onOrientationChange; }
#endif
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
        std::function<void (ulong handle, ulong parent, unsigned char *buffer, int width, int height, int pixline, int left, int top)> _displayButton{nullptr};
        std::function<void (ulong handle, bool state)> _setVisible{nullptr};
        std::function<void (ulong handle, int width, int height)> _setPage{nullptr};
        std::function<void (ulong handle, int left, int top, int width, int height, ANIMATION_t animate)> _setSubPage{nullptr};
        std::function<void (ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)> _setBackground{nullptr};
        std::function<void (ulong handle, const std::string& text, const std::string& font, const std::string& family, int size, int x, int y, ulong color, ulong effectColor, FONT_STYLE style, Button::TEXT_ORIENTATION ori, Button::TEXT_EFFECT effect, bool ww)> _setText{nullptr};
        std::function<void (ulong handle)> _callDropPage{nullptr};
        std::function<void (ulong handle)> _callDropSubPage{nullptr};
        std::function<void (ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw)> _callPlayVideo{nullptr};
        std::function<void (Button::TButton *button, Button::BITMAP_t& bm)> _callInputText{nullptr};
        std::function<void (const std::string& init, const std::string& prompt, bool priv)> _callKeyboard{nullptr};
        std::function<void (const std::string& init, const std::string& prompt, bool priv)> _callKeypad{nullptr};
        std::function<void ()> _callResetKeyboard{nullptr};
        std::function<void ()> _callShowSetup{nullptr};
        std::function<void ()> _resetSurface{nullptr};
        std::function<void ()> _shutdown{nullptr};
        std::function<void (const std::string& file)> _playSound{nullptr};
        std::function<void ()> _stopSound{nullptr};
        std::function<void (bool state)> _muteSound{nullptr};
        std::function<void (const std::string& str)> _sendVirtualKeys{nullptr};
        std::function<void (bool state)> _showPhoneDialog{nullptr};
        std::function<void (const std::string& number)> _setPhoneNumber{nullptr};
        std::function<void (const std::string& msg)> _setPhoneStatus{nullptr};
        std::function<void (int state, int id)> _setPhoneState{nullptr};
        std::function<void ()> _repaintWindows{nullptr};
        std::function<void (uint handle)> _toFront{nullptr};
#ifdef __ANDROID__
        std::function<void (int orientation)> _onOrientationChange{nullptr};
#endif
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
        std::vector<Button::TButton *> collectButtons(std::vector<MAP_T>& map);
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
        TPage *loadPage(PAGELIST_T& pl);
        void setButtonCallbacks(Button::TButton *bt);
        bool sendCustomEvent(int value1, int value2, int value3, const std::string& msg, int evType, int cp, int cn);
#ifndef _NOSIP_
        std::string sipStateToString(TSIPClient::SIP_STATE_t s);
#endif
        // List of command functions
        void doFTR(int port, std::vector<int>&, std::vector<std::string>& pars);

        void doON(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doOFF(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doLEVEL(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doBLINK(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doVER(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doWCN(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        void doAFP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doAPG(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doCPG(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doDPG(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPHE(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPHP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doPHT(int port, std::vector<int>& channels, std::vector<std::string>& pars);
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
        void doGLH(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGLL(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doGSC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doICO(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getICO(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doJSB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getJSB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doJSI(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getJSI(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doJST(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getJST(int port, std::vector<int>& channels, std::vector<std::string>& pars);
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
        void doSetup(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doShutdown(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doSOU(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTKP(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doVKB(int port, std::vector<int>& channels, std::vector<std::string>& pars);
#ifndef _NOSIP_
        void doPHN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void getPHN(int port, std::vector<int>& channels, std::vector<std::string>& pars);
#endif
        // Commands inherited from TPControl (Touch Panel Control)
        void doTPCCMD(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTPCACC(int port, std::vector<int>& channels, std::vector<std::string>& pars);
        void doTPCSIP(int port, std::vector<int>& channels, std::vector<std::string>& pars);

        int mActualPage{0};                             // The number of the actual visible page
        int mPreviousPage{0};                           // The number of the previous page
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
        TSystemDraw *mSystemDraw{nullptr};              // A pointer to the (optional) system resources
        std::thread mThreadAmxNet;                      // The thread handle to the controler handler
        TVector<amx::ANET_COMMAND> mCommands;           // Command queue of commands received from controller
        std::atomic<bool> mBusy{false};                 // Internal used to block the command handler
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
        // SIP
#ifndef _NOSIP_
        bool mPHNautoanswer{false};                     // The state of the SIP autoanswer
        TSIPClient *mSIPClient{nullptr};                // Includes the SIP client
#endif
#ifdef _SCALE_SKIA_
        double mScaleFactor{1.0};                       // The scale factor to zoom or shrink all components
        double mScaleFactorWidth{1.0};                  // The individual scale factor for the width
        double mScaleFactorHeight{1.0};                 // The individual scale factor for the height
#endif
#ifdef __ANDROID__
        int mNetState{0};                               // On Android remembers the type of connection to the network (cell or wifi)
#endif
        std::map<int, std::function<void (int level)> > mNetCalls;  // List of callbacks for the network state multistate bargraph
        std::map<int, std::function<void (int level, bool charging, int chargeType)> > mBatteryCalls;
};

#ifdef __ANDROID__
extern "C" {
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_NetworkStatus_informTPanelNetwork(JNIEnv */*env*/, jclass /*clazz*/, jboolean conn, jint level, jint type);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_BatteryState_informBatteryStatus(JNIEnv */*env*/, jclass /*clazz*/, jint level, jboolean charge, jint chargeType);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_PhoneCallState_informPhoneState(JNIEnv *env, jclass cl, jboolean call, jstring pnumber);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_Logger_logger(JNIEnv *env, jclass cl, jint mode, jstring msg);
    JNIEXPORT void JNICALL Java_org_qtproject_theosys_Orientation_informTPanelOrientation(JNIEnv */*env*/, jclass /*clazz*/, jint orientation);
}
#endif  // __ANDROID__

#endif

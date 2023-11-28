/*
 * Copyright (C) 2020 to 2023 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef __TQTMAIN_H__
#define __TQTMAIN_H__

#include <QMainWindow>
#include <QMetaType>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QGeoPositionInfoSource>
#endif
#include "QMediaPlayer"

#include "tpagemanager.h"
#include "tobject.h"
//#include "tqemitqueue.h"
#include "tpagelist.h"
#include "tqgesturefilter.h"

extern bool prg_stopped;
extern std::atomic<bool> killed;

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QSessionManager;
class QLabel;
class QWidget;
class QByteArray;
class QMouseEvent;
class QMoveEvent;
class QGestureEvent;
class QEvent;
class QSound;
//class QMediaPlayer;
class QOrientationSensor;
class QStackedWidget;
class QListWidgetItem;
#ifdef Q_OS_IOS
class QGeoPositionInfo;
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QAudioOutput;
#endif
class TQtSettings;
class TQKeyboard;
class TQKeypad;
class TQBusy;
class TqDownload;
class TQtPhone;
class TBitmap;
class TQScrollArea;
QT_END_NAMESPACE
#ifdef Q_OS_IOS
class TIOSBattery;
class TIOSRotate;
#endif
class TQtWait;

Q_DECLARE_METATYPE(size_t)

int qtmain(int argc, char **argv, TPageManager *pmanager);

class MainWindow : public QMainWindow, public TObject
{
    Q_OBJECT

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

        MainWindow();
        ~MainWindow();
        void setConfigFile(const std::string& file) { mFileConfig = file; }
        void setSetChange(bool state) { settingsChanged = state; }
        void setScaleFactor(double scale) { mScaleFactor = scale; }
        void setSetupScaleFactor(double scale) { mSetupScaleFactor = scale; }
        double getScaleFactor() { return mScaleFactor; }
        void setOrientation(Qt::ScreenOrientation ori) { mOrientation = ori; }
        void disconnectArea(TQScrollArea *area);
        void disconnectList(QListWidget *list);
        void reconnectArea(TQScrollArea *area);
        void reconnectList(QListWidget *list);
#if defined(QT_DEBUG) && (defined(Q_OS_IOS) || defined(Q_OS_ANDROID))
        static std::string orientationToString(Qt::ScreenOrientation ori);
#endif
    signals:
        void sigDisplayButton(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq);
        void sigSetMarqueeText(Button::TButton* button);
        void sigDisplayViewButton(ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T fillColor);
        void sigAddViewButtonItems(ulong parent, std::vector<PGSUBVIEWITEM_T> items);
        void sigUpdateViewButton(ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor);
        void sigUpdateViewButtonItem(PGSUBVIEWITEM_T& item, ulong parent);
        void sigShowViewButtonItem(ulong handle, ulong parent, int position, int timer);
        void sigToggleViewButtonItem(ulong handle, ulong parent, int position, int timer);
        void sigHideAllViewItems(ulong handle);
        void sigHideViewItem(ulong handle, ulong parent);
        void sigSetSubViewPadding(ulong handle, int padding);
        void sigSetVisible(ulong handle, bool state);
        void sigSetPage(ulong handle, int width, int height);
        void sigSetSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate);
#ifndef _OPAQUE_SKIA_
        void sigSetBackground(ulong handle, TBitmap image, int width, int height, ulong color, int opacity=255);
#else
        void sigSetBackground(ulong handle, TBitmap image, int width, int height, ulong color);
#endif
        void sigDropPage(ulong handle);
        void sigDropSubPage(ulong handle, ulong parent);
        void sigDropButton(ulong handle);
        void sigPlayVideo(ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw);
        void sigInputText(Button::TButton *button, QByteArray buffer, int width, int height, int frame, size_t pixline);
        void sigListBox(Button::TButton *button, QByteArray buffer, int width, int height, int frame, size_t pixline);
        void sigKeyboard(const std::string& init, const std::string& prompt, bool priv);
        void sigKeypad(const std::string& init, const std::string& prompt, bool priv);
        void sigResetKeyboard();
        void sigShowSetup();
        void sigPlaySound(const std::string& file);
        void sigStopSound();
        void sigMuteSound(bool state);
        void sigSetVolume(int volume);
        void sigSendVirtualKeys(const std::string& str);
        void sigShowPhoneDialog(bool state);
        void sigSetPhoneNumber(const std::string& number);
        void sigSetPhoneStatus(const std::string& msg);
        void sigSetPhoneState(int state, int id);
        void sigRepaintWindows();
        void sigToFront(ulong handle);
        void sigDownloadSurface(const std::string& file, size_t size);
        void sigOnProgressChanged(int percent);
        void sigDisplayMessage(const std::string& msg, const std::string& title);
        void sigAskPassword(ulong handle, const std::string& msg, const std::string& title);
        void sigFileDialog(ulong handle, const std::string& path, const std::string& extension, const std::string& suffix);
        void sigSetSizeMainWindow(int width, int height);
        void sigStartWait(const std::string& text);
        void sigStopWait();
        void sigPageFinished(uint handle);
        void sigListViewArea(ulong handle, ulong parent, Button::TButton& button, SUBVIEWLIST_T& list);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void sigActivateSettings(const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce);
#endif
    protected:
        bool event(QEvent *event) override;
        void closeEvent(QCloseEvent *event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;

    private slots:
        void settings();
        void about();
#ifndef QT_NO_SESSIONMANAGER
        void commitData(QSessionManager &);
#endif
        void displayButton(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq);
        void setMarqueeText(Button::TButton *button);
        void displayViewButton(ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T fillColor);
        void addViewButtonItems(ulong parent, std::vector<PGSUBVIEWITEM_T> items);
        void updateViewButton(ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor);
        void updateViewButtonItem(PGSUBVIEWITEM_T& item, ulong parent);
        void showViewButtonItem(ulong handle, ulong parent, int position, int timer);
        void toggleViewButtonItem(ulong handle, ulong parent, int position, int timer);
        void hideAllViewItems(ulong handle);
        void hideViewItem(ulong handle, ulong parent);
        void setSubViewPadding(ulong handle, int padding);
        void SetVisible(ulong handle, bool state);
        void setPage(ulong handle, int width, int height);
        void setSubPage(ulong hanlde, ulong parent, int left, int top, int width, int height, ANIMATION_t animate);
#ifdef _OPAQUE_SKIA_
        void setBackground(ulong handle, TBitmap image, int width, int height, ulong color);
#else
        void setBackground(ulong handle, TBitnap image, int width, int height, ulong color, int opacity=255);
#endif  // _OPAQUE_SKIA_
        void dropPage(ulong handle);
        void dropSubPage(ulong handle, ulong parent);
        void dropButton(ulong handle);
        void playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw);
        void inputText(Button::TButton *button, QByteArray buffer, int width, int height, int frame, size_t pixline);
        void listBox(Button::TButton *button, QByteArray buffer, int width, int height, int frame, size_t pixline);
        void showKeyboard(const std::string& init, const std::string& prompt, bool priv);
        void showKeypad(const std::string& init, const std::string& prompt, bool priv);
        void sendVirtualKeys(const std::string& str);
        void resetKeyboard();
        void showSetup();
        void playSound(const std::string& file);
        void stopSound();
        void muteSound(bool state);
        void setVolume(int volume);
        void onAppStateChanged(Qt::ApplicationState state);
        void onScreenOrientationChanged(Qt::ScreenOrientation ori);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void activateSettings(const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce);
#ifdef Q_OS_IOS
        void onPositionUpdated(const QGeoPositionInfo &update);
        void onErrorOccurred(QGeoPositionInfoSource::Error positioningError);
#endif  // Q_OS_IOS
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void onTListCallbackCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
        void setSizeMainWindow(int width, int height);
        // Slots for the phone dialog
        void showPhoneDialog(bool state);
        void setPhoneNumber(const std::string& number);
        void setPhoneStatus(const std::string& msg);
        void setPhoneState(int state, int id);
        // Slots for the toolbar buttons.
        void arrowLeft();
        void arrowRight();
        void arrowUp();
        void arrowDown();
        void selectOk();
        void volumeUpPressed();
        void volumeUpReleased();
        void volumeDownPressed();
        void volumeDownReleased();
//        void volumeMute();
        // Slots for widget actions and button element actions
        void animationFinished();
        void animationInFinished();
        void repaintWindows();
        void toFront(ulong handle);
        void downloadSurface(const std::string& file, size_t size);
        void displayMessage(const std::string& msg, const std::string& title);
        void askPassword(ulong handle, const std::string msg, const std::string& title);
        void fileDialog(ulong handle, const std::string& path, const std::string& extension, const std::string& suffix);
        // Progress bar (busy indicator)
        void onProgressChanged(int percent);
        void startWait(const std::string& text);
        void stopWait();
        void pageFinished(ulong handle);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        void onPlayingChanged(bool plying);
#endif
        void onPlayerError(QMediaPlayer::Error error, const QString &errorString);
        void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    private:
        bool gestureEvent(QGestureEvent *event);
        void createActions();
        void writeSettings();
//        void playShowList();
        int scale(int value);
        int scaleSetup(int value);
        bool isScaled();
        bool isSetupScaled();
        bool startAnimation(TObject::OBJECT_t *obj, ANIMATION_t& ani, bool in = true);
        void downloadBar(const std::string& msg, QWidget *parent);
        void runEvents();
        void onSubViewItemClicked(ulong handle, bool pressed);
        void onInputChanged(ulong handle, std::string& text);
        void onFocusChanged(ulong handle, bool in);
        void onCursorChanged(ulong handle, int oldPos, int newPos);
        void onGestureEvent(QObject *obj, QGestureEvent *event);
        QPixmap scaleImage(QPixmap& pix);
        QPixmap scaleImage(unsigned char *buffer, int width, int height, int pixline);

        void _displayButton(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq);
        void _setMarqueeText(Button::TButton *button);
        void _displayViewButton(ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T fillColor);
        void _addViewButtonItems(ulong parent, std::vector<PGSUBVIEWITEM_T> items);
        void _updateViewButton(ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor);
        void _updateViewButtonItem(PGSUBVIEWITEM_T& item, ulong parent);
        void _showViewButtonItem(ulong handle, ulong parent, int position, int timer);
        void _toggleViewButtonItem(ulong handle, ulong parent, int position, int timer);
        void _hideAllViewItems(ulong handle);
        void _hideViewItem(ulong handle, ulong parent);
        void _setSubViewPadding(ulong handle, int padding);
        void _setVisible(ulong handle, bool state);
        void _setPage(ulong handle, int width, int height);
        void _setSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate);
#ifdef _OPAQUE_SKIA_
        void _setBackground(ulong handle, TBitmap image, int width, int height, ulong color);
#else
        void _setBackground(ulong handle, TBitmap image, int width, int height, ulong color, int opacity=255);
#endif  // _OPAQUE_SKIA_
        void _dropPage(ulong handle);
        void _dropSubPage(ulong handle, ulong parent);
        void _dropButton(ulong handle);
        void _playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw);
        void _inputText(Button::TButton *button, Button::BITMAP_t& bm, int frame);
        void _listBox(Button::TButton *button, Button::BITMAP_t& bm, int frame);
        void _showKeyboard(const std::string& init, const std::string& prompt, bool priv=false);
        void _showKeypad(const std::string& init, const std::string& prompt, bool priv=false);
        void _resetKeyboard();
        void _showSetup();
        void _resetSurface();
        void _shutdown();
        void _playSound(const std::string& file);
        void _stopSound();
        void _muteSound(bool state);
        void _setVolume(int volume);
        void _setOrientation(J_ORIENTATION ori);
        void _sendVirtualKeys(const std::string& str);
        void _showPhoneDialog(bool state);
        void _setPhoneNumber(const std::string& number);
        void _setPhoneStatus(const std::string& msg);
        void _setPhoneState(int state, int id);
        void _onProgressChanged(int percent);
        void _displayMessage(const std::string& msg, const std::string& title);
        void _askPassword(ulong handle, const std::string& msg, const std::string& title);
        void _fileDialog(ulong handle, const std::string& path, const std::string& extension, const std::string& suffix);
        void _setSizeMainWindow(int width, int height);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void _signalState(Qt::ApplicationState state);
        void _orientationChanged(int orientation);
        void _activateSettings(const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        void _freezeWorkaround();
#endif  // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        void _repaintWindows();
        void _toFront(ulong handle);
        void _downloadSurface(const std::string& file, size_t size);
        void _startWait(const std::string& text);
        void _stopWait();
        void _pageFinished(uint handle);
        void _listViewArea(ulong handle, ulong parent, Button::TButton& button, SUBVIEWLIST_T& list);
        void doReleaseButton();
        void repaintObjects();
        void refresh(ulong handle);
        void markDirty(ulong handle);
        QFont loadFont(int number, const FONT_T& f, const FONT_STYLE fs);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        int calcVolume(int value);
#else
        double calcVolume(int value);
#endif
        std::string convertMask(const std::string& mask);
#ifdef Q_OS_ANDROID
        void hideAndroidBars();
#endif  // Q_OS_ANDROID
#ifdef Q_OS_IOS
        void setNotch();
        void initGeoLocation();
        Qt::ScreenOrientation getRealOrientation();
#endif  // Q_OS_IOS

        std::mutex draw_mutex;              // We're using threads and need to block execution sometimes
        std::mutex click_mutex;
        std::mutex anim_mutex;

        bool mWasInactive{false};           // If the application was inactive this is set to true until everything was repainted.
        bool mDoRepaint{false};             // This is set to TRUE whenever a reconnection to the controller happened.
        TQtSettings *mSettings{nullptr};    // The pointer to the settings dialog
        bool settingsChanged{false};        // true = settings have changed

        QToolBar *mToolbar{nullptr};        // The toolbar, if there is any
        QStackedWidget *mCentralWidget{nullptr};        // A stacked widget where all pages are stored
        bool mCentralInitialized{false};    // True = The central widget and all dependencies are initialized
        std::string mVideoURL;              // If the button is a video, this is the URL where it plays from.
        std::string mFileConfig;            // Path and file name of the config file
        bool mHasFocus{true};               // If this is FALSE, no output to sceen is allowed.
        std::atomic<double> mScaleFactor{1.0}; // The actual scale factor
        std::atomic<double> mSetupScaleFactor{1.0};     // The scale factor for the setup pages
        OBJECT_t *mLastObject{nullptr};     // This is for the hide effect of widgets.
        std::map<ulong, OBJECT_t *> mAnimObjects;   // List of started animations
        ulong mActualPageHandle{0};         // Holds the handle of the active page.
        std::atomic<bool> mBusy{false};     // If TRUE the busy indicator is active
        TqDownload *mDownloadBar{nullptr};  // Pointer to a dialog showing a progress bar
        TQKeyboard *mQKeyboard{nullptr};    // Pointer to an active virtual keyboard
        TQKeypad *mQKeypad{nullptr};        // Pointer to an active virtual keypad
        bool mKeyboard{false};              // TRUE = keyboard is visible
        bool mKeypad{false};                // TRUE = keypad is visible
        TQtPhone *mPhoneDialog{nullptr};    // Points to the internal phone dialog
        // Mouse
        int mLastPressX{-1};                // Remember the last mouse press X coordinate
        int mLastPressY{-1};                // Remember the last mouse press Y coordinate
        Qt::ScreenOrientation mOrientation{Qt::PrimaryOrientation};
        QOrientationSensor *mSensor{nullptr};   // Basic sensor for orientation
        QMediaPlayer *mMediaPlayer{nullptr};// Class to play sound files.
        TQtWait *mWaitBox{nullptr};         // This is a wait dialog.
        TQGestureFilter *mGestureFilter{nullptr};   // This handles the pinch and swipe gestures
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QGeoPositionInfoSource *mSource{nullptr};   // The geo location is used on IOS to keep app running in background
        QAudioOutput *mAudioOutput{nullptr};
#endif
#ifdef Q_OS_IOS
        TIOSBattery *mIosBattery{nullptr};  // Class to retrive the battery status on an iPhone or iPad
        TIOSRotate *mIosRotate{nullptr};    // Class to control rotation
        bool mIOSSettingsActive{false};     // TRUE: IOS settings are active.
        QMargins mNotchPortrait;            // The margins (notch, if any) for portrait orientation
        QMargins mNotchLandscape;           // The margins (notch, if any) for landscape orientation
        bool mHaveNotchPortrait{false};     // TRUE = Notch was already fetched for portrait orientation
        bool mHaveNotchLandscape{false};    // TRUE = Notch was already fetched for landscape orientation
        bool mGeoHavePermission{false};     // TRUE = The app has permission for geo location
#endif
        std::chrono::steady_clock::time_point mTouchStart;  // Time in micro seconds of the start of a touch event
        int mTouchX{0};                        // The X coordinate of the mouse pointer
        int mTouchY{0};                        // The Y coordinate of the mouse pointer
};

#endif

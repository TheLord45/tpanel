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
#ifndef __TQTMAIN_H__
#define __TQTMAIN_H__

#include <QMainWindow>
#include <QMetaType>

#include "tpagemanager.h"
#include "tobject.h"
#include "tqemitqueue.h"
#include "terror.h"

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
class QEvent;
class QSound;
class QMediaPlayer;
class TQtSettings;
class TQKeyboard;
class TQKeypad;
class TQBusy;
class TQtPhone;
QT_END_NAMESPACE

Q_DECLARE_METATYPE(size_t)

int qtmain(int argc, char **argv, TPageManager *pmanager);

class MainWindow : public QMainWindow, TQManageQueue
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
        double getScaleFactor() { return mScaleFactor; }

    signals:
        void sigDisplayButton(ulong handle, ulong parent, QByteArray buffer, int width, int height, int pixline, int left, int top);
        void sigSetVisible(ulong handle, bool state);
        void sigSetPage(ulong handle, int width, int height);
        void sigSetSubPage(ulong handle, int left, int top, int width, int height, ANIMATION_t animate);
        void sigSetBackground(ulong handle, QByteArray image, size_t rowBytes, int width, int height, ulong color);
        void sigDropPage(ulong handle);
        void sigDropSubPage(ulong handle);
        void sigDropButton(ulong handle);
        void sigPlayVideo(ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw);
        void sigInputText(Button::TButton * button, QByteArray buffer, int width, int height, size_t pixline);
        void sigKeyboard(const std::string& init, const std::string& prompt, bool priv);
        void sigKeypad(const std::string& init, const std::string& prompt, bool priv);
        void sigResetKeyboard();
        void sigShowSetup();
        void sigPlaySound(const std::string& file);
        void sigStopSound();
        void sigMuteSound(bool state);
        void sigSendVirtualKeys(const std::string& str);
        void sigShowPhoneDialog(bool state);
        void sigSetPhoneNumber(const std::string& number);
        void sigSetPhoneStatus(const std::string& msg);
        void sigSetPhoneState(int state, int id);
        void sigRepaintWindows();
        void sigToFront(ulong handle);

    protected:
        bool event(QEvent *event) override;
        void closeEvent(QCloseEvent *event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        bool eventFilter(QObject *obj, QEvent *ev) override;

    private slots:
        void settings();
        void about();
#ifndef QT_NO_SESSIONMANAGER
        void commitData(QSessionManager &);
#endif
        void displayButton(ulong handle, ulong parent, QByteArray buffer, int width, int height, int pixline, int left, int top);
        void SetVisible(ulong handle, bool state);
        void setPage(ulong handle, int width, int height);
        void setSubPage(ulong hanlde, int left, int top, int width, int height, ANIMATION_t animate);
        void setBackground(ulong handle, QByteArray image, size_t rowBytes, int width, int height, ulong color);
        void dropPage(ulong handle);
        void dropSubPage(ulong handle);
        void dropButton(ulong handle);
        void playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw);
        void inputText(Button::TButton *button, QByteArray buffer, int width, int height, size_t pixline);
        void showKeyboard(const std::string& init, const std::string& prompt, bool priv);
        void showKeypad(const std::string& init, const std::string& prompt, bool priv);
        void sendVirtualKeys(const std::string& str);
        void resetKeyboard();
        void showSetup();
        void playSound(const std::string& file);
        void stopSound();
        void muteSound(bool state);
        void appStateChanged(Qt::ApplicationState state);
        void onScreenOrientationChanged(Qt::ScreenOrientation ori);
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
        void textChangedMultiLine();
        void textSingleLineReturn();
        void repaintWindows();
        void toFront(ulong handle);

    private:
        bool gestureEvent(QGestureEvent *event);
        void createActions();
        void writeSettings();
        void playShowList();
        int scale(int value);
        bool isScaled() { return (mScaleFactor != 1.0); }
        void startAnimation(TObject::OBJECT_t *obj, ANIMATION_t& ani, bool in = true);
        void busyIndicator(const std::string& msg, QWidget *parent);
        void runEvents();

        void _displayButton(ulong handle, ulong parent, unsigned char* buffer, int width, int height, int pixline, int left, int top);
        void _setVisible(ulong handle, bool state);
        void _setPage(ulong handle, int width, int height);
        void _setSubPage(ulong handle, int left, int top, int width, int height, ANIMATION_t animate);
        void _setBackground(ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color);
        void _dropPage(ulong handle);
        void _dropSubPage(ulong handle);
        void _dropButton(ulong handle);
        void _playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const std::string& url, const std::string& user, const std::string& pw);
        void _inputText(Button::TButton *button, Button::BITMAP_t& bm);
        void _showKeyboard(const std::string& init, const std::string& prompt, bool priv=false);
        void _showKeypad(const std::string& init, const std::string& prompt, bool priv=false);
        void _resetKeyboard();
        void _showSetup();
        void _resetSurface();
        void _shutdown();
        void _playSound(const std::string& file);
        void _stopSound();
        void _muteSound(bool state);
        void _setOrientation(J_ORIENTATION ori);
        void _sendVirtualKeys(const std::string& str);
        void _showPhoneDialog(bool state);
        void _setPhoneNumber(const std::string& number);
        void _setPhoneStatus(const std::string& msg);
        void _setPhoneState(int state, int id);
#ifdef __ANDROID__
        void _signalState(Qt::ApplicationState state);
        void _orientationChanged(int orientation);
#endif
        void _repaintWindows();
        void _toFront(ulong handle);
        void doReleaseButton();
        void repaintObjects();
        int calcVolume(int value);

        bool mWasInactive{false};           // If the application was inactive this is set to true until everything was repainted.
        bool mDoRepaint{false};             // This is set to TRUE whenever a reconnection to the controller happened.
        TQtSettings *mSettings{nullptr};    // The pointer to the settings dialog
        bool settingsChanged{false};        // true = settings have changed
        QWidget *mBackground{nullptr};      // The background of the visible page
        QToolBar *mToolbar{nullptr};        // The toolbar, if there is any
        std::string mVideoURL;              // If the button is a video, this is the URL where it plays from.
        std::string mFileConfig;            // Path and file name of the config file
        bool mHasFocus{true};               // If this is FALSE, no output to sceen is allowed.
        std::atomic<double> mScaleFactor{1.0}; // The actual scale factor
        std::atomic<TObject::OBJECT_t *> mLastObject{nullptr};     // This is for the hide effect of widgets.
        std::atomic<bool> mBusy{false};     // If TRUE the busy indicator is active
        TQBusy *mBusyDialog{nullptr};       // Pointer to busy indicator dialog window
        TQKeyboard *mQKeyboard{nullptr};    // Pointer to an active virtual keyboard
        TQKeypad *mQKeypad{nullptr};        // Pointer to an active virtual keypad
//        std::thread mThreadBusy;            // The thread of the busy indicator.
        bool mKeyboard{false};              // TRUE = keyboard is visible
        bool mKeypad{false};                // TRUE = keypad is visible
        TQtPhone *mPhoneDialog{nullptr};    // Points to the internal phone dialog
        // Mouse
        int mLastPressX{-1};                // Remember the last mouse press X coordinate
        int mLastPressY{-1};                // Remember the last mouse press Y coordinate
        Qt::ScreenOrientations mOrientation{Qt::PrimaryOrientation};
        QMediaPlayer *mMediaPlayer{nullptr};// Class to play sound files.
        std::chrono::steady_clock::time_point mTouchStart;           // Time in micro seconds of the start of a touch event
        int mTouchX{0};                        // The X coordinate of the mouse pointer
        int mTouchY{0};                        // The Y coordinate of the mouse pointer
};

#endif


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

/** @file tqtmain.cpp
 * @brief Implements the surface of the application.
 *
 * This file implements the callback functions of the suface. While the most
 * classes are drawing the elements, the methods here take the ready elements
 * and display them. This file makes the surface completely independent of
 * the rest of the application which makes it easy to change the surface by
 * any other technology.
 */
#include <QApplication>
#include <QGuiApplication>
#include <QByteArray>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLabel>
#include <QtWidgets>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QTouchEvent>
#include <QPalette>
#include <QPixmap>
#include <QFont>
#include <QFontDatabase>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtMultimedia/QMediaPlayer>
#ifdef QT5_LINUX
#   include <QtMultimedia/QMediaPlaylist>
#else
#   include <QAudioOutput>
#endif
#include <QLayout>
#include <QSizePolicy>
#include <QUrl>
#include <QThread>
#ifdef QT5_LINUX
#   include <QSound>
#   include <QtSensors/QOrientationSensor>
#endif
#ifdef __ANDROID__
#include <QAndroidJniObject>
#endif
#include <functional>
#include <mutex>
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include "tpagemanager.h"
#include "tqtmain.h"
#include "tconfig.h"
#include "tqtsettings.h"
#include "tqkeyboard.h"
#include "tqkeypad.h"
#include "tcolor.h"
#include "texcept.h"
#include "ttpinit.h"
#include "tqtbusy.h"
#include "tqdownload.h"
#include "tqtphone.h"

/**
 * @def THREAD_WAIT
 * This defines a time in milliseconds. It is used in the callbacks for the
 * functions to draw the elements. Qt need a little time to do it's work.
 * Therefore we're waiting a little bit. Otherwise it may result in missing
 * elements or black areas on the screen.
 */
#define THREAD_WAIT     1

extern amx::TAmxNet *gAmxNet;                   //!< Pointer to the class running the thread which handles the communication with the controller.
extern bool _restart_;                          //!< If this is set to true then the whole program will start over.
extern TPageManager *gPageManager;              //!< The pointer to the global defined main class.
std::mutex draw_mutex;                          //!< We're using threads and need to block execution sometimes
static bool isRunning = false;                  //!< TRUE = the pageManager was started.
TObject *gObject = nullptr;                     //!< Internal used pointer to a TObject class. (Necessary because of threads!)
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
static double gScale = 1.0;                     //!< Global variable holding the scale factor.
static int gFullWidth = 0;                      //!< Global variable holding the width of the AMX screen. This is used to calculate the scale factor for the settings dialog.
std::atomic<double> mScaleFactorW{1.0};
std::atomic<double> mScaleFactorH{1.0};
int gScreenWidth{0};
int gScreenHeight{0};
#endif

using std::bind;
using std::pair;
using std::string;
using std::vector;

string _NO_OBJECT = "The global class TObject is not available!";

/**
 * @brief qtmain is used here as the entry point for the surface.
 *
 * The main entry function parses the command line parameters, if there were
 * any and sets the basic attributes. It creates the main window and starts the
 * application.
 *
 * @param argc      The number of command line arguments
 * @param argv      A pointer to a 2 dimensional array containing the command
 *                  line parameters.
 * @param pmanager  A pointer to the page manager class which is the main class
 *                  managing everything.
 *
 * @return If no errors occured it returns 0.
 */
int qtmain(int argc, char **argv, TPageManager *pmanager)
{
    DECL_TRACER("qtmain(int argc, char **argv, TPageManager *pmanager)");

    if (!pmanager)
    {
        MSG_ERROR("Fatal: No pointer to the page manager received!");
        return 1;
    }

    if (!gPageManager || gPageManager != pmanager)
        gPageManager = pmanager;
#ifdef __ANDROID__
    MSG_INFO("Android API version: " << __ANDROID_API__);

#   if __ANDROID_API__ < 29
#       warn "The Android API version is less than 29! Some functions may not work!"
#   endif
#endif
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QApplication::setAttribute(Qt::AA_ForceRasterWidgets);
    QApplication::setAttribute(Qt::AA_Use96Dpi);
//    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
#endif

    QApplication app(argc, argv);
    // Set the orientation
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QScreen *screen = QGuiApplication::primaryScreen();
    QOrientationReading::Orientation ori = QOrientationReading::Undefined;
    Qt::ScreenOrientations mask = Qt::PrimaryOrientation;
    QOrientationSensor qOri;
    QOrientationReading *pORead = qOri.reading();

    if (pORead)
    {
        ori = pORead->orientation();

        switch(ori)
        {
            case QOrientationReading::LeftUp:   mask = Qt::InvertedLandscapeOrientation; break;
            case QOrientationReading::RightUp:  mask = Qt::LandscapeOrientation; break;
            case QOrientationReading::TopDown:  mask = Qt::InvertedPortraitOrientation; break;
            default:
                mask = Qt::PortraitOrientation;
        }

        MSG_PROTOCOL("Detected orientation: " << mask);
    }

    if (!screen)
    {
        MSG_ERROR("Couldn't determine the primary screen!")
        return 1;
    }

    if (pmanager->getSettings()->isPortrait())  // portrait?
    {
        MSG_INFO("Orientation set to portrait mode.");

        if (mask == Qt::InvertedPortraitOrientation)
            screen->setOrientationUpdateMask(Qt::InvertedPortraitOrientation);
        else
            screen->setOrientationUpdateMask(Qt::PortraitOrientation);
    }
    else
    {
        MSG_INFO("Orientation set to landscape mode.");

        if (mask == Qt::InvertedLandscapeOrientation)
            screen->setOrientationUpdateMask(Qt::InvertedLandscapeOrientation);
        else
            screen->setOrientationUpdateMask(Qt::LandscapeOrientation);
    }

    double scale = 1.0;
    // Calculate the scale factor
    if (TConfig::getScale())
    {
        // Because we've no window here we can not know on which screen, if
        // there are more than one, the application will start. Because on a
        // mobile mostly no external screen is connected, we take always the
        // resolution of the first (built in) screen.
        // TODO: Find a way to get the screen the application will start and
        // take this screen to calculate the scale factor.
        QList<QScreen *> screens = QGuiApplication::screens();
        QRect screenGeometry = screens.at(0)->availableGeometry();
        double width = 0.0;
        double height = 0.0;
        gScreenWidth = screenGeometry.width();
        gScreenHeight = screenGeometry.height();
        int minWidth = pmanager->getSettings()->getWith();
        int minHeight = pmanager->getSettings()->getHeight();

        if (pmanager->getSettings()->isPortrait())  // portrait?
        {
            width = std::min(screenGeometry.width(), screenGeometry.height());
            height = std::max(screenGeometry.height(), screenGeometry.width());
        }
        else
        {
            width = std::max(screenGeometry.width(), screenGeometry.height());
            height = std::min(screenGeometry.height(), screenGeometry.width());
        }

        if (!TConfig::getToolbarSuppress() && TConfig::getToolbarForce())
            minWidth += 48;

        MSG_INFO("Dimension of AMX screen:" << minWidth << " x " << minHeight);
        MSG_INFO("Screen size: " << width << " x " << height);
        // The scale factor is always calculated in difference to the prefered
        // size of the original AMX panel.
        mScaleFactorW = width / minWidth;
        mScaleFactorH = height / minHeight;
        scale = std::min(mScaleFactorW, mScaleFactorH);

        gScale = scale;     // The calculated scale factor
        gFullWidth = width;
        MSG_INFO("Calculated scale factor: " << scale);
        // This preprocessor variable allows the scaling to be done by the Skia
        // library, which is used to draw everything. In comparison to Qt this
        // library is a bit slower and sometimes does not honor the aspect ratio
        // correct. But in case there is another framework than Qt in use, this
        // could be necessary.
#ifdef _SCALE_SKIA_
        if (scale != 0.0)
        {
            pmanager->setScaleFactor(scale);
            MSG_INFO("Scale factor: " << scale);
        }

        if (scaleW != 0.0)
            pmanager->setScaleFactorWidth(scaleW);

        if (scaleH != 0.0)
            pmanager->setScaleFactorHeight(scaleH);
#endif  // _SCALE_SKIA_
    }
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)

    // Initialize the application
    pmanager->setDPI(QGuiApplication::primaryScreen()->logicalDotsPerInch());
    QCoreApplication::setOrganizationName(TConfig::getProgName().c_str());
    QCoreApplication::setApplicationName("TPanel");
    QCoreApplication::setApplicationVersion(VERSION_STRING());
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    MainWindow mainWin;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#   ifndef _SCALE_SKIA_
    if (TConfig::getScale() && scale != 1.0)
        mainWin.setScaleFactor(scale);
#   endif   // _SCALE_SKIA_
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mainWin.setConfigFile(TConfig::getConfigPath() + "/" + TConfig::getConfigFileName());
    mainWin.setStyleSheet("QMainWindow {background: 'black';}");    // Keep the background black. Helps to save battery on OLED displays.
    mainWin.grabGesture(Qt::PinchGesture);
    mainWin.grabGesture(Qt::SwipeGesture);

    mainWin.show();
    return app.exec();
}

/**
 * @brief MainWindow::MainWindow constructor
 *
 * This method is the constructor for this class. It registers the callback
 * functions to the class TPageManager and starts the main run loop.
 *
 * Qt is used only to manage widgets to handle pages and subpages. A page as
 * well as a subpage may contain a background graphic and some elements. The
 * elements could be buttons, bargraphs and other objects. The underlying layer
 * draw every element as a ready graphic image and call a callback function to
 * let Qt display the graphic.
 * If there are some animations on a subpage defined, this is also handled by
 * Qt. Especialy sliding and fading.
 */
MainWindow::MainWindow()
{
    DECL_TRACER("MainWindow::MainWindow()");

    gObject = new TObject;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    setAttribute(Qt::WA_AcceptTouchEvents, true);   // We accept touch events
    grabGesture(Qt::PinchGesture);                  // We use a pinch gesture to open the settings dialog
    grabGesture(Qt::SwipeGesture);                  // We support swiping also

    if (gPageManager && gPageManager->getSettings()->isPortrait() == 1)  // portrait?
    {
        MSG_INFO("Orientation set to portrait mode.");
        _setOrientation(O_PORTRAIT);
    }
    else
    {
        MSG_INFO("Orientation set to landscape mode.");
        _setOrientation(O_LANDSCAPE);
    }
#else
    setWindowIcon(QIcon(":images/icon.png"));
#endif
    if (!gPageManager)
    {
        EXCEPTFATALMSG("The class TPageManager was not initialized!");
    }

    // First we register all our surface callbacks to the underlying work
    // layer. All the graphics are drawn by the Skia library. The layer below
    // call the following functions to let Qt display the graphics on the
    // screen let it manage the widgets containing the graphics.
    gPageManager->registerCallbackDB(bind(&MainWindow::_displayButton, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4,
                                       std::placeholders::_5,
                                       std::placeholders::_6,
                                       std::placeholders::_7,
                                       std::placeholders::_8));

    gPageManager->registerCallbackSP(bind(&MainWindow::_setPage, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3));

    gPageManager->registerCallbackSSP(bind(&MainWindow::_setSubPage, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3,
                                          std::placeholders::_4,
                                          std::placeholders::_5,
                                          std::placeholders::_6));

    gPageManager->registerCallbackSB(bind(&MainWindow::_setBackground, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3,
                                         std::placeholders::_4,
                                         std::placeholders::_5,
                                         std::placeholders::_6,
                                         std::placeholders::_7));

    gPageManager->regCallDropPage(bind(&MainWindow::_dropPage, this, std::placeholders::_1));
    gPageManager->regCallDropSubPage(bind(&MainWindow::_dropSubPage, this, std::placeholders::_1));
    gPageManager->regCallPlayVideo(bind(&MainWindow::_playVideo, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4,
                                       std::placeholders::_5,
                                       std::placeholders::_6,
                                       std::placeholders::_7,
                                       std::placeholders::_8,
                                       std::placeholders::_9));
    gPageManager->regCallInputText(bind(&MainWindow::_inputText, this, std::placeholders::_1, std::placeholders::_2));
    gPageManager->regCallbackKeyboard(bind(&MainWindow::_showKeyboard, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gPageManager->regCallbackKeypad(bind(&MainWindow::_showKeypad, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gPageManager->regCallResetKeyboard(bind(&MainWindow::_resetKeyboard, this));
    gPageManager->regCallShowSetup(bind(&MainWindow::_showSetup, this));
    gPageManager->regCallbackResetSurface(bind(&MainWindow::_resetSurface, this));
    gPageManager->regCallbackShutdown(bind(&MainWindow::_shutdown, this));
    gPageManager->regCallbackPlaySound(bind(&MainWindow::_playSound, this, std::placeholders::_1));
    gPageManager->regCallbackStopSound(bind(&MainWindow::_stopSound, this));
    gPageManager->regCallbackMuteSound(bind(&MainWindow::_muteSound, this, std::placeholders::_1));
    gPageManager->registerCBsetVisible(bind(&MainWindow::_setVisible, this, std::placeholders::_1, std::placeholders::_2));
    gPageManager->regSendVirtualKeys(bind(&MainWindow::_sendVirtualKeys, this, std::placeholders::_1));
    gPageManager->regShowPhoneDialog(bind(&MainWindow::_showPhoneDialog, this, std::placeholders::_1));
    gPageManager->regSetPhoneNumber(bind(&MainWindow::_setPhoneNumber, this, std::placeholders::_1));
    gPageManager->regSetPhoneStatus(bind(&MainWindow::_setPhoneStatus, this, std::placeholders::_1));
    gPageManager->regSetPhoneState(bind(&MainWindow::_setPhoneState, this, std::placeholders::_1, std::placeholders::_2));
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    gPageManager->regOnOrientationChange(bind(&MainWindow::_orientationChanged, this, std::placeholders::_1));
#endif
    gPageManager->regRepaintWindows(bind(&MainWindow::_repaintWindows, this));
    gPageManager->regToFront(bind(&MainWindow::_toFront, this, std::placeholders::_1));

    gPageManager->deployCallbacks();
    createActions();

#ifndef QT_NO_SESSIONMANAGER
#if defined(QT5_LINUX) && !defined(QT6_LINUX)
    QGuiApplication::setFallbackSessionManagementEnabled(false);
    connect(qApp, &QGuiApplication::commitDataRequest,
            this, &MainWindow::writeSettings);
#endif
#endif
    // Some types used to transport data from the layer below.
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<QByteArray>("QByteArray");
    qRegisterMetaType<ANIMATION_t>("ANIMATION_t");
    qRegisterMetaType<Button::TButton *>("TButton");
    qRegisterMetaType<std::string>("std::string");

    // All the callback functions doesn't act directly. Instead they emit an
    // event. Then Qt decides whether the real function is started directly and
    // immediately or if the call is queued and called later in a thread. To
    // handle this we're "connecting" the real functions to some signals.
    try
    {
        connect(this, &MainWindow::sigDisplayButton, this, &MainWindow::displayButton);
        connect(this, &MainWindow::sigSetPage, this, &MainWindow::setPage);
        connect(this, &MainWindow::sigSetSubPage, this, &MainWindow::setSubPage);
        connect(this, &MainWindow::sigSetBackground, this, &MainWindow::setBackground);
        connect(this, &MainWindow::sigDropPage, this, &MainWindow::dropPage);
        connect(this, &MainWindow::sigDropSubPage, this, &MainWindow::dropSubPage);
        connect(this, &MainWindow::sigPlayVideo, this, &MainWindow::playVideo);
        connect(this, &MainWindow::sigInputText, this, &MainWindow::inputText);
        connect(this, &MainWindow::sigKeyboard, this, &MainWindow::showKeyboard);
        connect(this, &MainWindow::sigKeypad, this, &MainWindow::showKeypad);
        connect(this, &MainWindow::sigShowSetup, this, &MainWindow::showSetup);
        connect(this, &MainWindow::sigPlaySound, this, &MainWindow::playSound);
        connect(this, &MainWindow::sigDropButton, this, &MainWindow::dropButton);
        connect(this, &MainWindow::sigSetVisible, this, &MainWindow::SetVisible);
        connect(this, &MainWindow::sigSendVirtualKeys, this, &MainWindow::sendVirtualKeys);
        connect(this, &MainWindow::sigShowPhoneDialog, this, &MainWindow::showPhoneDialog);
        connect(this, &MainWindow::sigSetPhoneNumber, this, &MainWindow::setPhoneNumber);
        connect(this, &MainWindow::sigSetPhoneStatus, this, &MainWindow::setPhoneStatus);
        connect(this, &MainWindow::sigSetPhoneState, this, &MainWindow::setPhoneState);
        connect(this, &MainWindow::sigRepaintWindows, this, &MainWindow::repaintWindows);
        connect(this, &MainWindow::sigToFront, this, &MainWindow::toFront);
        connect(this, &MainWindow::sigOnProgressChanged, this, &MainWindow::onProgressChanged);
        connect(qApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::appStateChanged);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        QScreen *screen = QGuiApplication::primaryScreen();
        connect(screen, &QScreen::orientationChanged, this, &MainWindow::onScreenOrientationChanged);
#endif
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Connection error: " << e.what());
    }
    catch(...)
    {
        MSG_ERROR("Unexpected exception occured [MainWindow::MainWindow()]");
    }

    setUnifiedTitleAndToolBarOnMac(true);
#ifdef __ANDROID__
    // At least initialize the phone call listener
    if (gPageManager)
        gPageManager->initPhoneState();

    /*
     * In case this class was created not the first time, we initiate a small
     * thread to send the signal ApplicationActive to initiate the communication
     * with the controller again. This also starts the page manager thread
     * (TPageManager), which handles all elements of the surface.
     */
    if (_restart_ && gPageManager)
    {
        try
        {
            std::thread mThread = std::thread([=] { this->_signalState(Qt::ApplicationActive); });
            mThread.detach();   // We detach immediately to leave this method.
        }
        catch (std::exception& e)
        {
            MSG_ERROR("Error starting the thread to reinvoke communication!");
        }
        catch(...)
        {
            MSG_ERROR("Unexpected exception occured [MainWindow::MainWindow()]");
        }
    }
#endif
    _restart_ = false;
}

MainWindow::~MainWindow()
{
    DECL_TRACER("MainWindow::~MainWindow()");

    killed = true;
    prg_stopped = true;

    if (mMediaPlayer)
    {
#ifdef QT6_LINUX
        delete mAudioOutput;
#endif
        delete mMediaPlayer;
    }

    if (gAmxNet && !gAmxNet->isStopped())
        gAmxNet->stop();

    if (mToolbar)
    {
        removeToolBar(mToolbar);
        mToolbar = nullptr;
    }

    if (gObject)
    {
        delete gObject;
        gObject = nullptr;
    }

    isRunning = false;
}

#ifdef __ANDROID__
/**
 * @brief Small thread to invoke the initialization on an Android device.
 *
 * On Android devices the signal ApplicationActive is not send if the class
 * \p MainWindow is destroyed and recreated. Therefore we need this little
 * helper to send the signal when the class is initialized.
 *
 * @param state     This defines the signal to send.
 */
void MainWindow::_signalState(Qt::ApplicationState state)
{
    DECL_TRACER("MainWindow::_signalState(Qt::ApplicationState state)");

    std::this_thread::sleep_for(std::chrono::seconds(1));   // Wait a second
    appStateChanged(state);
}

void MainWindow::_orientationChanged(int orientation)
{
    DECL_TRACER("MainWindow::_orientationChanged(int orientation)");

    if (gPageManager && gPageManager->getSettings()->isPortrait() == 1)  // portrait?
    {
        if (orientation == O_REVERSE_PORTRAIT && mOrientation != Qt::InvertedPortraitOrientation)
            _setOrientation((J_ORIENTATION)orientation);
        else if (orientation == O_PORTRAIT && mOrientation != Qt::PortraitOrientation)
            _setOrientation((J_ORIENTATION)orientation);
    }
    else
    {
        if (orientation == O_REVERSE_LANDSCAPE && mOrientation != Qt::InvertedLandscapeOrientation)
            _setOrientation((J_ORIENTATION)orientation);
        else if (orientation == O_LANDSCAPE && mOrientation != Qt::LandscapeOrientation)
            _setOrientation((J_ORIENTATION)orientation);
    }
}
#endif

void MainWindow::_repaintWindows()
{
    DECL_TRACER("MainWindow::_repaintWindows()");

    emit sigRepaintWindows();
}

void MainWindow::_toFront(ulong handle)
{
    DECL_TRACER("MainWindow::_toFront(ulong handle)");

    emit sigToFront(handle);
}

/**
 * @brief MainWindow::closeEvent called when the application receives an exit event.
 *
 * If the user clicks on the exit icon or on the menu point _Exit_ this method
 * is called. It makes sure everything is written to the configuration file
 * and accepts the evernt.
 *
 * @param event The exit event
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    DECL_TRACER("MainWindow::closeEvent(QCloseEvent *event)");
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG,"tpanel","Close event; settingsChanged=%s", (settingsChanged ? "true":"false"));
#endif
    if (settingsChanged)
    {
        writeSettings();
        event->accept();
    }
    else
    {
        prg_stopped = true;
        killed = true;
        MSG_INFO("Program will stop!");
#ifdef __ANDROID__
        if (gPageManager)
            gPageManager->stopNetworkState();
#endif
        event->accept();
    }
}

/**
 * @brief MainWindow::eventFilter filters the QEvent::MouseMove event
 * Beside the filtering of the MouseEvent, it waits for a gesture and
 * call the method gestureEvent() which handles the gesture and opens the
 * setting dialog.
 * @param obj   The object where the event occured.
 * @param event The event.
 * @return `true` when the event should be ignored.
 */
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove)
        return true;    // Filter event out, i.e. stop it being handled further.

    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));
    else if (event->type() == QEvent::OrientationChange)
    {
        MSG_TRACE("The orientation has changed!");
    }

    return QMainWindow::eventFilter(obj, event);
}

/**
 * @brief MainWindow::event Looks for a gesture
 * @param event The event occured
 * @return TRUE if event was accepted
 */
bool MainWindow::event(QEvent* event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));
    else if (event->type() == QEvent::OrientationChange)
    {
        MSG_TRACE("The orientation has changed!");
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *sKey = static_cast<QKeyEvent*>(event);

        if (sKey && sKey->key() == Qt::Key_Back && !mToolbar)
        {
            QMessageBox msgBox(this);
            msgBox.setText(QString("Should TPanel display the setup dialog or quit?"));
            msgBox.addButton("Quit", QMessageBox::AcceptRole);      // This is correct! QT seems to change here the buttons.
            msgBox.addButton("Setup", QMessageBox::RejectRole);     // This is correct! QT seems to change here the buttons.
            int ret = msgBox.exec();

            if (ret == QMessageBox::Accepted)   // This is correct! QT seems to change here the buttons.
            {
                showSetup();
                return true;
            }
            else
                close();
        }
    }
    else if (event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *sKey = static_cast<QKeyEvent*>(event);

        if (sKey && sKey->key() == Qt::Key_Back && !mToolbar)
            return true;
    }

    return QWidget::event(event);
}

/**
 * @brief MainWindow::gestureEvent handles a pinch event
 * If a pinch event occured where the scale factor increased, the settings
 * dialog is called. This exists for devices, where the left toolbox is not
 * visible.
 * @param event The guesture occured
 * @return FALSE
 */
bool MainWindow::gestureEvent(QGestureEvent* event)
{
    DECL_TRACER("MainWindow::gestureEvent(QGestureEvent* event)");

    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
    {
        string gs;

        QPinchGesture *pg = static_cast<QPinchGesture *>(pinch);

        switch(pg->state())
        {
            case Qt::NoGesture:         gs.assign("no gesture"); break;
            case Qt::GestureStarted:    gs.assign("gesture started"); break;
            case Qt::GestureUpdated:    gs.assign("gesture updated"); break;
            case Qt::GestureFinished:   gs.assign("gesture finished"); break;
            case Qt::GestureCanceled:   gs.assign("gesture canceled"); break;
        }

        MSG_DEBUG("PinchGesture state " << gs << " detected");

        if (pg->state() == Qt::GestureFinished)
        {
            MSG_DEBUG("total scale: " << pg->totalScaleFactor() << ", scale: " << pg->scaleFactor() << ", last scale: " << pg->lastScaleFactor());

            if (pg->totalScaleFactor() > pg->scaleFactor())
                settings();
        }
    }
    else if (QGesture *swipe = event->gesture(Qt::SwipeGesture))
    {
        string gs;

        if (!gPageManager)
            return false;

        QSwipeGesture *sw = static_cast<QSwipeGesture *>(swipe);
        MSG_DEBUG("Swipe gesture detected.");

        if (sw->state() == Qt::GestureFinished)
        {
            if (sw->horizontalDirection() == QSwipeGesture::Left)
                gPageManager->onSwipeEvent(TPageManager::SW_LEFT);
            else if (sw->horizontalDirection() == QSwipeGesture::Right)
                gPageManager->onSwipeEvent(TPageManager::SW_RIGHT);
            else if (sw->verticalDirection() == QSwipeGesture::Up)
                gPageManager->onSwipeEvent(TPageManager::SW_UP);
            else if (sw->verticalDirection() == QSwipeGesture::Down)
                gPageManager->onSwipeEvent(TPageManager::SW_DOWN);
        }
    }

    return false;
}

/**
 * @brief MainWindow::onScreenOrientationChanged sets invers or normal orientation.
 * This method sets according to the actual set orientation the invers
 * orientations or switches back to normal orientation.
 * For example: When the orientation is set to portrait and the device is turned
 * to top down, then the orientation is set to invers portrait.
 * @param ori   The detected orientation
 */
void MainWindow::onScreenOrientationChanged(Qt::ScreenOrientation ori)
{
    DECL_TRACER("MainWindow::onScreenOrientationChanged(int ori)");

    MSG_PROTOCOL("Orientation changed to " << ori);

    if (mOrientation == Qt::PrimaryOrientation || mOrientation == ori)
        return;

    if ((mOrientation == Qt::LandscapeOrientation || mOrientation == Qt::InvertedLandscapeOrientation) &&
            (ori == Qt::PortraitOrientation || ori == Qt::InvertedPortraitOrientation))
        return;

    if ((mOrientation == Qt::PortraitOrientation || mOrientation == Qt::InvertedPortraitOrientation) &&
            (ori == Qt::LandscapeOrientation || ori == Qt::InvertedLandscapeOrientation))
        return;

    J_ORIENTATION jori = O_UNDEFINED;

    switch(mOrientation)
    {
        case Qt::LandscapeOrientation:          jori = O_LANDSCAPE; break;
        case Qt::InvertedLandscapeOrientation:  jori = O_REVERSE_LANDSCAPE; break;
        case Qt::PortraitOrientation:           jori = O_PORTRAIT; break;
        case Qt::InvertedPortraitOrientation:   jori = O_REVERSE_PORTRAIT; break;
        default:
            return;
    }

    _setOrientation(jori);
}

/**
 * @brief Displays or hides a phone dialog window.
 * This method creates and displays a phone dialog window containing everything
 * a simple phone needs. Depending on the parameter \p state the dialog is
 * created or an exeisting dialog is closed.
 * @param state     If TRUE the dialog is created or if it is not visible
 * brought to front and is made visible.
 * If this is FALSE an existing dialog window is destroid and disappears. If
 * the window didn't exist nothing happens.
 */
void MainWindow::showPhoneDialog(bool state)
{
    DECL_TRACER("MainWindow::showPhoneDialog(bool state)");

    if (mPhoneDialog)
    {
        if (!state)
        {
            mPhoneDialog->close();
            delete mPhoneDialog;
            mPhoneDialog = nullptr;
            return;
        }

        if (!mPhoneDialog->isVisible())
            mPhoneDialog->setVisible(true);

        return;
    }

    if (!state)
        return;

    mPhoneDialog = new TQtPhone(this);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // On mobile devices we set the scale factor always because otherwise the
    // dialog will be unusable.
    mPhoneDialog->setScaleFactor(gScale);
    mPhoneDialog->doResize();
#endif
    mPhoneDialog->open();
}

/**
 * Displays a phone number (can also be an URL) on a label in the phone dialog
 * window.
 * @param number    The string contains the phone number to display.
 */
void MainWindow::setPhoneNumber(const std::string& number)
{
    DECL_TRACER("MainWindow::setPhoneNumber(const std::string& number)");

    if (!mPhoneDialog)
        return;

    mPhoneDialog->setPhoneNumber(number);
}

/**
 * Displays a message in the status line on the bottom of the phone dialog
 * window.
 * @param msg   The string conaining a message.
 */
void MainWindow::setPhoneStatus(const std::string& msg)
{
    DECL_TRACER("MainWindow::setPhoneStatus(const std::string& msg)");

    if (!mPhoneDialog)
        return;

    mPhoneDialog->setPhoneStatus(msg);
}

void MainWindow::setPhoneState(int state, int id)
{
    DECL_TRACER("MainWindow::setPhoneState(int state)");

    if (mPhoneDialog)
        mPhoneDialog->setPhoneState(state, id);
}

/**
 * @brief MainWindow::createActions creates the toolbar on the right side.
 */
void MainWindow::createActions()
{
    DECL_TRACER("MainWindow::createActions()");

    // If the toolbar should not be visible at all we return here immediately.
    if (TConfig::getToolbarSuppress())
        return;

    // Add a mToolbar (on the right side)
    mToolbar = new QToolBar(this);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mToolbar->setAllowedAreas(Qt::RightToolBarArea);
    mToolbar->setFloatable(false);
    mToolbar->setMovable(false);

    if (TConfig::getScale() && gPageManager && gScale != 1.0)
    {
        int width = (int)((double)gPageManager->getSettings()->getWith() * gScale);
        int tbWidth = (int)(48.0 * gScale);
        int icWidth = (int)(40.0 * gScale);

        if ((gFullWidth - width) < tbWidth && !TConfig::getToolbarForce())
        {
            delete mToolbar;
            mToolbar = nullptr;
            return;
        }

        QSize iSize(icWidth, icWidth);
        mToolbar->setIconSize(iSize);
    }
#else
    mToolbar->setFloatable(true);
    mToolbar->setMovable(true);
    mToolbar->setAllowedAreas(Qt::RightToolBarArea | Qt::BottomToolBarArea);
#endif
    QAction *arrowUpAct = new QAction(QIcon(":/images/arrow_up.png"), tr("Up"), this);
    connect(arrowUpAct, &QAction::triggered, this, &MainWindow::arrowUp);
    mToolbar->addAction(arrowUpAct);

    QAction *arrowLeftAct = new QAction(QIcon(":/images/arrow_left.png"), tr("Left"), this);
    connect(arrowLeftAct, &QAction::triggered, this, &MainWindow::arrowLeft);
    mToolbar->addAction(arrowLeftAct);

    QAction *arrowRightAct = new QAction(QIcon(":/images/arrow_right.png"), tr("Right"), this);
    connect(arrowRightAct, &QAction::triggered, this, &MainWindow::arrowRight);
    mToolbar->addAction(arrowRightAct);

    QAction *arrowDownAct = new QAction(QIcon(":/images/arrow_down.png"), tr("Down"), this);
    connect(arrowDownAct, &QAction::triggered, this, &MainWindow::arrowDown);
    mToolbar->addAction(arrowDownAct);

    QAction *selectOkAct = new QAction(QIcon(":/images/ok.png"), tr("Ok"), this);
    connect(selectOkAct, &QAction::triggered, this, &MainWindow::selectOk);
    mToolbar->addAction(selectOkAct);

    mToolbar->addSeparator();

    QToolButton *btVolUp = new QToolButton(this);
    btVolUp->setIcon(QIcon(":/images/vol_up.png"));
    connect(btVolUp, &QToolButton::pressed, this, &MainWindow::volumeUpPressed);
    connect(btVolUp, &QToolButton::released, this, &MainWindow::volumeUpReleased);
    mToolbar->addWidget(btVolUp);

    QToolButton *btVolDown = new QToolButton(this);
    btVolDown->setIcon(QIcon(":/images/vol_down.png"));
    connect(btVolDown, &QToolButton::pressed, this, &MainWindow::volumeDownPressed);
    connect(btVolDown, &QToolButton::released, this, &MainWindow::volumeDownReleased);
    mToolbar->addWidget(btVolDown);
/*
    QAction *volMute = new QAction(QIcon(":/images/vol_mute.png"), tr("X"), this);
    connect(volMute, &QAction::triggered, this, &MainWindow::volumeMute);
    mToolbar->addAction(volMute);
*/
    mToolbar->addSeparator();

    const QIcon settingsIcon = QIcon::fromTheme("settings-configure", QIcon(":/images/settings.png"));
    QAction *settingsAct = new QAction(settingsIcon, tr("&Settings..."), this);
    settingsAct->setStatusTip(tr("Change the settings"));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::settings);
    mToolbar->addAction(settingsAct);

    const QIcon aboutIcon = QIcon::fromTheme("help-about", QIcon(":/images/info.png"));
    QAction *aboutAct = new QAction(aboutIcon, tr("&About..."), this);
    aboutAct->setShortcuts(QKeySequence::Open);
    aboutAct->setStatusTip(tr("About this program"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);
    mToolbar->addAction(aboutAct);

    const QIcon exitIcon = QIcon::fromTheme("application-exit", QIcon(":/images/off.png"));
    QAction *exitAct = mToolbar->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));

    addToolBar(Qt::RightToolBarArea, mToolbar);
}

/**
 * @brief MainWindow::mousePressEvent catches the event Qt::LeftButton.
 *
 * If the user presses the left mouse button somewhere in the main window, this
 * method is triggered. It retrieves the position of the mouse pointer and
 * sends it to the page manager TPageManager.
 *
 * @param event The event
 */
void MainWindow::mousePressEvent(QMouseEvent* event)
{
    DECL_TRACER("MainWindow::mousePressEvent(QMouseEvent* event)");

    if(event->button() == Qt::LeftButton)
    {
        int x = event->x();
        int y = event->y();

        mLastPressX = x;
        mLastPressY = y;

        if (isScaled())
        {
            x = (int)((double)x / mScaleFactor);
            y = (int)((double)y / mScaleFactor);
        }

        gPageManager->mouseEvent(x, y, true);
        mTouchStart = std::chrono::steady_clock::now();
        mTouchX = mLastPressX;
        mTouchY = mLastPressY;
    }
    else if (event->button() == Qt::MiddleButton)
        settings();
}

/**
 * @brief MainWindow::mouseReleaseEvent catches the event Qt::LeftButton.
 *
 * If the user releases the left mouse button somewhere in the main window, this
 * method is triggered. It retrieves the position of the mouse pointer and
 * sends it to the page manager TPageManager.
 *
 * @param event The event
 */
void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    DECL_TRACER("MainWindow::mouseReleaseEvent(QMouseEvent* event)");

    if (!gPageManager)
        return;

    if(event->button() == Qt::LeftButton)
    {
        int x = ((mLastPressX >= 0) ? mLastPressX : event->x());
        int y = ((mLastPressY >= 0) ? mLastPressY : event->y());

        mLastPressX = mLastPressY = -1;

        if (isScaled())
        {
            x = (int)((double)x / mScaleFactor);
            y = (int)((double)y / mScaleFactor);
        }

        gPageManager->mouseEvent(x, y, false);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::nanoseconds difftime = end - mTouchStart;
        std::chrono::milliseconds msecs = std::chrono::duration_cast<std::chrono::milliseconds>(difftime);

        if (msecs.count() < 100)    // 1/10 of a second
        {
            MSG_DEBUG("Time was too short: " << msecs.count());
            return;
        }

        x = event->x();
        y = event->y();
        int width = scale(gPageManager->getSettings()->getWith());
        int height = scale(gPageManager->getSettings()->getHeight());
        MSG_DEBUG("Coordinates: x1=" << mTouchX << ", y1=" << mTouchY << ", x2=" << x << ", y2=" << y << ", width=" << width << ", height=" << height);

        if (mTouchX < x && (x - mTouchX) > (width / 3))
            gPageManager->onSwipeEvent(TPageManager::SW_RIGHT);
        else if (x < mTouchX && (mTouchX - x) > (width / 3))
            gPageManager->onSwipeEvent(TPageManager::SW_LEFT);
        else if (mTouchY < y && (y - mTouchY) > (height / 3))
            gPageManager->onSwipeEvent(TPageManager::SW_DOWN);
        else if (y < mTouchY && (mTouchY - y) > (height / 3))
            gPageManager->onSwipeEvent(TPageManager::SW_UP);
    }
}

/**
 * @brief MainWindow::settings initiates the configuration dialog.
 */
void MainWindow::settings()
{
    DECL_TRACER("MainWindow::settings()");

    // Save some old values to decide whether to start over or not.
    string oldHost = TConfig::getController();
    int oldPort = TConfig::getPort();
    int oldChannelID = TConfig::getChannel();
    string oldSurface = TConfig::getFtpSurface();
    bool oldToolbar = TConfig::getToolbarForce();
    bool oldToolbarSuppress = TConfig::getToolbarSuppress();
    // Initialize and open the settings dialog.
    TQtSettings *dlg_settings = new TQtSettings(this);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // On mobile devices we set the scale factor always because otherwise the
    // dialog will be unusable.
    qreal ratio = 1.0;

    if (gPageManager->getSettings()->isPortrait())
    {
        if (gScreenWidth > gScreenHeight)
        {
            qreal refHeight = dlg_settings->geometry().height();
            qreal refWidth = dlg_settings->geometry().width();
            QRect rect = QGuiApplication::primaryScreen()->geometry();
            qreal height = qMax(rect.width(), rect.height());
            qreal width = qMin(rect.width(), rect.height());
            ratio = qMin(height / refHeight, width / refWidth);
        }
        else
        {
            qreal refWidth = dlg_settings->geometry().height();
            qreal refHeight = dlg_settings->geometry().width();
            QRect rect = QGuiApplication::primaryScreen()->geometry();
            qreal width = qMax(rect.width(), rect.height());
            qreal height = qMin(rect.width(), rect.height());
            ratio = qMin(height / refHeight, width / refWidth);
        }
    }
    else
        ratio = gScale;

    dlg_settings->setScaleFactor(ratio);
    dlg_settings->doResize();
#endif
    int ret = dlg_settings->exec();
    bool rebootAnyway = false;

    if ((ret && dlg_settings->hasChanged()) || (ret && dlg_settings->downloadForce()))
    {
        writeSettings();

        if (!TConfig::getToolbarSuppress() && oldToolbar != TConfig::getToolbarForce())
        {
            QMessageBox msgBox(this);
            msgBox.setText("The change for the visibility of the toolbar will be active on the next start of TPanel!");
            msgBox.exec();
        }
        else if (oldToolbarSuppress != TConfig::getToolbarSuppress() && TConfig::getToolbarSuppress())
        {
            if (mToolbar)
            {
                mToolbar->close();
                delete mToolbar;
                mToolbar = nullptr;
            }
        }

        if (TConfig::getFtpSurface() != oldSurface || dlg_settings->downloadForce())
        {
            bool dlYes = true;

            MSG_DEBUG("Surface should be downloaded (Old: " << oldSurface << ", New: " << TConfig::getFtpSurface() << ")");

            if (!dlg_settings->downloadForce())
            {
                QMessageBox msgBox(this);
                msgBox.setText(QString("Should the surface <b>") + TConfig::getFtpSurface().c_str() + "</b> be installed?");
                msgBox.addButton(QMessageBox::Yes);
                msgBox.addButton(QMessageBox::No);
                int ret = msgBox.exec();

                if (ret == QMessageBox::No)
                    dlYes = false;
            }

            if (dlYes)
            {
                TTPInit tpinit;
                tpinit.regCallbackProcessEvents(bind(&MainWindow::runEvents, this));
                tpinit.regCallbackProgressBar(bind(&MainWindow::_onProgressChanged, this, std::placeholders::_1));
                tpinit.setPath(TConfig::getProjectPath());
                tpinit.setFileSize(dlg_settings->getSelectedFtpFileSize());
                string msg = "Loading file <b>" + TConfig::getFtpSurface() + "</b>.";
                MSG_DEBUG("Download of surface " << TConfig::getFtpSurface() << " was forced!");

//                busyIndicator(msg, this);
                downloadBar(msg, this);

                if (tpinit.loadSurfaceFromController(true))
                    rebootAnyway = true;

//                mBusyDialog->close();
                mDownloadBar->close();
                mBusy = false;
            }
            else
            {
                MSG_DEBUG("No change of surface. Old surface " << oldSurface << " was saved again.");
                TConfig::saveFtpSurface(oldSurface);
                writeSettings();
            }
        }

        if (TConfig::getController() != oldHost ||
            TConfig::getChannel() != oldChannelID ||
            TConfig::getPort() != oldPort || rebootAnyway)
        {
            // Start over by exiting this class
            MSG_INFO("Program will start over!");
            _restart_ = true;
            prg_stopped = true;
            killed = true;

            if (gAmxNet)
                gAmxNet->stop();

            close();
        }
    }
    else if (!ret && dlg_settings->hasChanged())
    {
        TConfig cf(TConfig::getConfigPath() + "/" + TConfig::getConfigFileName());
    }

    delete dlg_settings;
}

/**
 * @brief MainWindow::writeSettings Writes the settings into the configuration file.
 */
void MainWindow::writeSettings()
{
    DECL_TRACER("MainWindow::writeSettings()");

    TConfig::saveSettings();
    MSG_INFO("Wrote settings.");
}

/**
 * @brief MainWindow::about displays the _about_ dialog.
 */
void MainWindow::about()
{
    DECL_TRACER("MainWindow::about()");

    std::string msg = "Simulation of an AMX G4 panel\n";
    msg.append("Version v").append(VERSION_STRING()).append("\n");
    msg.append("(C) Copyright 2020 to 2022 by Andreas Theofilu <andreas@theosys.at>\n");
    msg.append("This program is under the terms of GPL version 3");

    QMessageBox::about(this, tr("About TPanel"),
                       tr(msg.c_str()));
}

void MainWindow::arrowUp()
{
    DECL_TRACER("MainWindow::arrowUp()");

    extButtons_t btType = EXT_CURSOR_UP;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_UP;

    gPageManager->externalButton(btType, true);
    gPageManager->externalButton(btType, false);
}

void MainWindow::arrowLeft()
{
    DECL_TRACER("MainWindow::arrowLeft()");
    extButtons_t btType = EXT_CURSOR_LEFT;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_LEFT;

    gPageManager->externalButton(btType, true);
    gPageManager->externalButton(btType, false);
}

void MainWindow::arrowRight()
{
    DECL_TRACER("MainWindow::arrowRight()");
    extButtons_t btType = EXT_CURSOR_RIGHT;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_RIGHT;

    gPageManager->externalButton(btType, true);
    gPageManager->externalButton(btType, false);
}

void MainWindow::arrowDown()
{
    DECL_TRACER("MainWindow::arrowDown()");
    extButtons_t btType = EXT_CURSOR_DOWN;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_DOWN;

    gPageManager->externalButton(btType, true);
    gPageManager->externalButton(btType, false);
}

void MainWindow::selectOk()
{
    DECL_TRACER("MainWindow::selectOk()");
    extButtons_t btType = EXT_CURSOR_SELECT;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_DOUBLE_PRESS;

    gPageManager->externalButton(btType, true);
    gPageManager->externalButton(btType, false);
}

void MainWindow::volumeUpPressed()
{
    DECL_TRACER("MainWindow::volumeUpPressed()");
    extButtons_t btType = EXT_CURSOR_ROTATE_RIGHT;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_ROTATE_RIGHT;

    gPageManager->externalButton(btType, true);
}

void MainWindow::volumeUpReleased()
{
    DECL_TRACER("MainWindow::volumeUpReleased()");
    extButtons_t btType = EXT_CURSOR_ROTATE_RIGHT;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_ROTATE_RIGHT;

    gPageManager->externalButton(btType, false);
}

void MainWindow::volumeDownPressed()
{
    DECL_TRACER("MainWindow::volumeDownPressed()");
    extButtons_t btType = EXT_CURSOR_ROTATE_LEFT;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_ROTATE_LEFT;

    gPageManager->externalButton(btType, true);
}

void MainWindow::volumeDownReleased()
{
    DECL_TRACER("MainWindow::volumeDownReleased()");
    extButtons_t btType = EXT_CURSOR_ROTATE_LEFT;

    if (TConfig::getPanelType().find("Android") != string::npos)
        btType = EXT_GESTURE_ROTATE_LEFT;

    gPageManager->externalButton(btType, false);
}
/*
void MainWindow::volumeMute()
{
    DECL_TRACER("MainWindow::volumeMute()");
    gPageManager->externalButton(EXT_GENERAL, true);
    gPageManager->externalButton(EXT_GENERAL, false);
}
*/
void MainWindow::animationFinished()
{
    DECL_TRACER("MainWindow::animationFinished()");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    TObject::OBJECT_t *obj = gObject->getMarkedRemove();

    while (obj)
    {
        if (obj->animation && obj->animation->state() != QAbstractAnimation::Running)
            break;

        obj = gObject->getNextMarkedRemove(obj);
    }

    if (obj && obj->animation)
    {
        MSG_DEBUG("Dropping object " << TObject::handleToString(obj->handle));
        delete obj->animation;
        obj->animation = nullptr;
        gObject->dropContent(obj);

        if (mLastObject == obj)
            mLastObject = nullptr;

        gObject->removeObject(obj->handle);
    }
    else
    {
        MSG_WARNING("No or invalid object to delete!");
    }
}

void MainWindow::textChangedMultiLine()
{
    DECL_TRACER("MainWindow::textChangedMultiLine(const QString& text)");
    // Find out from which input line the signal was send
    QTextEdit* edit = qobject_cast<QTextEdit*>(sender());

    if (!edit)
    {
        MSG_ERROR("No QTextEdit widget found! External sender?");
        return;
    }

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    QString text = edit->toPlainText();
    WId id = edit->winId();
    TObject::OBJECT_t *obj = gObject->findObject(id);

    if (!obj)
    {
        MSG_ERROR("No object witb WId " << id << " found!");
        return;
    }

    if (gPageManager)
        gPageManager->setTextToButton(obj->handle, text.toStdString());
}

void MainWindow::textSingleLineReturn()
{
    DECL_TRACER("MainWindow::textChangedSingleLine(const QString& text)");

    // Find out from which input line the signal was send
    QLineEdit* edit = qobject_cast<QLineEdit*>(sender());

    if (!edit)
    {
        MSG_ERROR("No QLineEdit widget found! External sender?");
        return;
    }

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    QString text = edit->text();
    WId id = edit->winId();
    TObject::OBJECT_t *obj = gObject->findObject(id);

    if (!obj)
    {
        MSG_ERROR("No object with WId " << id << " found!");
        return;
    }

    MSG_DEBUG("Writing text: " << text.toStdString());

    if (gPageManager)
        gPageManager->setTextToButton(obj->handle, text.toStdString());
}

void MainWindow::repaintWindows()
{
    DECL_TRACER("MainWindow::repaintWindows()");

    if (mWasInactive)
    {
        MSG_INFO("Refreshing of visible popups will be requested.");
        mDoRepaint = true;
    }
}

void MainWindow::toFront(ulong handle)
{
    DECL_TRACER("MainWindow::toFront(ulong handle)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    TObject::OBJECT_t *obj = gObject->findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object with " << TObject::handleToString(handle) << " not found!");
        return;
    }

    if (obj->type == TObject::OBJ_SUBPAGE && obj->object.widget)
        obj->object.widget->raise();
}

void MainWindow::onProgressChanged(int percent)
{
    DECL_TRACER("MainWindow::onProgressChanged(int percent)");

    if (!mDownloadBar || !mBusy)
        return;

    mDownloadBar->setProgress(percent);
}

/**
 * @brief MainWindow::appStateChanged - Is called whenever the state of the app changes.
 * This callback method is called whenever the state of the application
 * changes. This is mostly usefull on mobile devices. Whenever the main window
 * looses the focus (screen closed, application is put into background, ...)
 * this method is called and updates a flag. If the application is not able
 * to draw to the screen (suspended) all events are cached. At the moment the
 * application becomes active, all queued messages are applied.
 * @param state     The new state of the application.
 */
void MainWindow::appStateChanged(Qt::ApplicationState state)
{
    DECL_TRACER("MainWindow::appStateChanged(Qt::ApplicationState state)");

    switch (state)
    {
        case Qt::ApplicationSuspended:              // Should not occure on a normal desktop
            MSG_INFO("Switched to mode SUSPEND");
            mHasFocus = false;
#ifdef Q_OS_ANDROID
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
#endif
        break;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)      // On a normal desktop we can ignore this signals
        case Qt::ApplicationInactive:
            MSG_INFO("Switched to mode INACTIVE");
            mHasFocus = false;
            mWasInactive = true;
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
        break;

        case Qt::ApplicationHidden:
            MSG_INFO("Switched to mode HIDDEN");
            mHasFocus = false;
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
        break;
#endif
        case Qt::ApplicationActive:
            MSG_INFO("Switched to mode ACTIVE");
            mHasFocus = true;

            if (!isRunning && gPageManager)
            {
                // Start the core application
                gPageManager->startUp();
                gPageManager->run();
                isRunning = true;
                mWasInactive = false;
            }
            else
            {
                playShowList();

                if (mDoRepaint && mWasInactive)
                    repaintObjects();

                mDoRepaint = false;
                mWasInactive = false;
            }
#ifdef Q_OS_ANDROID
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "resumeOrientationListener", "()V");
#endif
        break;
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
        default:
            mHasFocus = true;
#endif
    }
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (mHasFocus && gPageManager)
    {
        gPageManager->initNetworkState();
        gPageManager->initBatteryState();
    }
    else if (gPageManager)
    {
        gPageManager->stopNetworkState();
        gPageManager->stopBatteryState();
    }
#endif
}

void MainWindow::_shutdown()
{
    DECL_TRACER("MainWindow::_shutdown()")

    close();
}

/******************* Signal handling *************************/

void MainWindow::_resetSurface()
{
    DECL_TRACER("MainWindow::_resetSurface()");

    // Start over by exiting this class
    MSG_INFO("Program will start over!");
    _restart_ = true;
    prg_stopped = true;
    killed = true;

    if (gAmxNet)
        gAmxNet->stop();

    close();
}

void MainWindow::_displayButton(ulong handle, ulong parent, unsigned char* buffer, int width, int height, int pixline, int left, int top)
{
    DECL_TRACER("MainWindow::_displayButton(ulong handle, ulong parent, unsigned char* buffer, int width, int height, int pixline, int left, int top)");

    if (prg_stopped)
        return;

    if (!mHasFocus)     // Suspended?
    {
        addButton(handle, parent, buffer, pixline, left, top, width, height);
        return;
    }

    QByteArray buf;

    if (buffer && pixline > 0)
    {
        size_t size = width * height * (pixline / width);
        MSG_DEBUG("Buffer size=" << size << ", width=" << width << ", height=" << height << ", left=" << left << ", top=" << top);
        buf.insert(0, (const char *)buffer, size);
    }

    try
    {
        emit sigDisplayButton(handle, parent, buf, width, height, pixline, left, top);
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error triggering function \"displayButton()\": " << e.what());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
}

void MainWindow::_setVisible(ulong handle, bool state)
{
    DECL_TRACER("MainWindow::_setVisible(ulong handle, bool state)");

    if (prg_stopped)
        return;

    emit sigSetVisible(handle, state);
}

void MainWindow::_setPage(ulong handle, int width, int height)
{
    DECL_TRACER("MainWindow::_setPage(ulong handle, int width, int height)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        addPage(handle, width, height);
        return;
    }

    emit sigSetPage(handle, width, height);
#ifndef __ANDROID__
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
#endif
}

void MainWindow::_setSubPage(ulong handle, int left, int top, int width, int height, ANIMATION_t animate)
{
    DECL_TRACER("MainWindow::_setSubPage(ulong handle, int left, int top, int width, int height)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        addSubPage(handle, left, top, width, height, animate);
        return;
    }

    emit sigSetSubPage(handle, left, top, width, height, animate);
#ifndef __ANDROID__
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
#endif
}

void MainWindow::_setBackground(ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color)
{
    DECL_TRACER("MainWindow::_setBackground(ulong handle, unsigned char *image, size_t size, size_t rowBytes, ulong color)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        addBackground(handle, image, size, rowBytes, width, height, color);
        return;
    }

    QByteArray buf;

    if (image && size > 0)
        buf.insert(0, (const char *)image, size);

    emit sigSetBackground(handle, buf, rowBytes, width, height, color);
#ifndef __ANDROID__
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
#endif
}

void MainWindow::_dropPage(ulong handle)
{
    DECL_TRACER("MainWindow::_dropPage(ulong handle)");

    doReleaseButton();

    if (!mHasFocus)
    {
        markDrop(handle);
        return;
    }

    emit sigDropPage(handle);

#ifndef __ANDROID__
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
#endif
}

void MainWindow::_dropSubPage(ulong handle)
{
    DECL_TRACER("MainWindow::_dropSubPage(ulong handle)");

    doReleaseButton();

    if (!mHasFocus)
    {
        markDrop(handle);
        return;
    }

    emit sigDropSubPage(handle);

#ifndef __ANDROID__
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
#endif
}

void MainWindow::_dropButton(ulong handle)
{
    DECL_TRACER("MainWindow::_dropButton(ulong handle)");

    if (!mHasFocus)
    {
        markDrop(handle);
        return;
    }

    emit sigDropButton(handle);
}

void MainWindow::_playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const string& url, const string& user, const string& pw)
{
    DECL_TRACER("MainWindow::_playVideo(ulong handle, const string& url)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        addVideo(handle, parent, left, top, width, height, url, user, pw);
        return;
    }

    emit sigPlayVideo(handle, parent, left, top, width, height, url, user, pw);
#ifndef __ANDROID__
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
#endif
}

void MainWindow::_inputText(Button::TButton* button, Button::BITMAP_t& bm)
{
    DECL_TRACER("MainWindow::_inputText(Button::TButton* button, Button::BITMAP_t& bm)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        addInText(button->getHandle(), button, bm);
        return;
    }

    QByteArray buf;

    if (bm.buffer && bm.rowBytes > 0)
    {
        size_t size = bm.width * bm.height * (bm.rowBytes / bm.width);
        buf.insert(0, (const char *)bm.buffer, size);
    }

    emit sigInputText(button, buf, bm.width, bm.height, bm.rowBytes);
#ifndef __ANDROID__
    std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT));
#endif
}

void MainWindow::_showKeyboard(const std::string& init, const std::string& prompt, bool priv)
{
    DECL_TRACER("MainWindow::_showKeyboard(std::string &init, std::string &prompt, bool priv)");

    if (prg_stopped)
        return;

    doReleaseButton();
    emit sigKeyboard(init, prompt, priv);
}

void MainWindow::_showKeypad(const std::string& init, const std::string& prompt, bool priv)
{
    DECL_TRACER("MainWindow::_showKeypad(std::string &init, std::string &prompt, bool priv)");

    if (prg_stopped)
        return;

    doReleaseButton();
    emit sigKeypad(init, prompt, priv);
}

void MainWindow::_resetKeyboard()
{
    DECL_TRACER("MainWindow::_resetKeyboard()");

    emit sigResetKeyboard();
}

void MainWindow::_showSetup()
{
    DECL_TRACER("MainWindow::_showSetup()");

    emit sigShowSetup();
}

void MainWindow::_playSound(const string& file)
{
    DECL_TRACER("MainWindow::_playSound(const string& file)");

    emit sigPlaySound(file);
}

void MainWindow::_stopSound()
{
    DECL_TRACER("MainWindow::_stopSound()");

    emit sigStopSound();
}

void MainWindow::_muteSound(bool state)
{
    DECL_TRACER("MainWindow::_muteSound(bool state)");

    emit sigMuteSound(state);
}

void MainWindow::_setOrientation(J_ORIENTATION ori)
{
#ifdef __ANDROID__
    DECL_TRACER("MainWindow::_setOriantation(J_ORIENTATION ori)");

    if (ori == O_FACE_UP || ori == O_FACE_DOWN)
        return;

    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");

    if ( activity.isValid() )
    {
        activity.callMethod<void>
                ("setRequestedOrientation"  // method name
                 , "(I)V"                   // signature
                 , ori);

        switch(ori)
        {
            case O_LANDSCAPE:           mOrientation = Qt::LandscapeOrientation; break;
            case O_PORTRAIT:            mOrientation = Qt::PortraitOrientation; break;
            case O_REVERSE_LANDSCAPE:   mOrientation = Qt::InvertedLandscapeOrientation; break;
            case O_REVERSE_PORTRAIT:    mOrientation = Qt::InvertedPortraitOrientation; break;
            default:
                MSG_WARNING("Orientation is undefined!");
                mOrientation = Qt::PrimaryOrientation;
        }
    }
#endif
}

void MainWindow::_sendVirtualKeys(const string& str)
{
    DECL_TRACER("MainWindow::_sendVirtualKeys(const string& str)");

    emit sigSendVirtualKeys(str);
}

void MainWindow::_showPhoneDialog(bool state)
{
    DECL_TRACER("MainWindow::_showPhoneDialog(bool state)");

    emit sigShowPhoneDialog(state);
}

void MainWindow::_setPhoneNumber(const std::string& number)
{
    DECL_TRACER("MainWindow::_setPhoneNumber(const std::string& number)");

    emit sigSetPhoneNumber(number);
}

void MainWindow::_setPhoneStatus(const std::string& msg)
{
    DECL_TRACER("MainWindow::_setPhoneStatus(const std::string& msg)");

    emit sigSetPhoneStatus(msg);
}

void MainWindow::_setPhoneState(int state, int id)
{
    DECL_TRACER("MainWindow::_setPhoneState(int state, int id)");

    emit sigSetPhoneState(state, id);
}

void MainWindow::_onProgressChanged(int percent)
{
    DECL_TRACER("MainWindow::_onProgressChanged(int percent)");

    emit sigOnProgressChanged(percent);
}

void MainWindow::doReleaseButton()
{
    DECL_TRACER("MainWindow::doReleaseButton()");

    if (mLastPressX >= 0 && mLastPressX >= 0 && gPageManager)
    {
        MSG_DEBUG("Sending outstanding mouse release event for coordinates x" << mLastPressX << ", y" << mLastPressY);
        int x = mLastPressX;
        int y = mLastPressY;

        if (isScaled())
        {
            x = (int)((double)x / mScaleFactor);
            y = (int)((double)y / mScaleFactor);
        }

        gPageManager->mouseEvent(x, y, false);
    }

    mLastPressX = mLastPressY = -1;
}

/**
 * @brief MainWindow::repaintObjects
 * On a mobile device it is possible that the content, mostly the background, of
 * a window becomes destroyed and it is not repainted. This may be the case at
 * the moment where the device was disconnected from the controller and
 * reconnected while the application was inactive. In such a case usualy the
 * surface is completely repainted. But because of inactivity this was not
 * possible and some components may look destroyed. They are still functional
 * allthough.
 */
void MainWindow::repaintObjects()
{
    DECL_TRACER("MainWindow::repaintObjects()");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    draw_mutex.lock();
#ifdef Q_OS_ANDROID
    std::this_thread::sleep_for(std::chrono::milliseconds(200));    // Necessary for slow devices
#endif
    TObject::OBJECT_t *obj = gObject->findFirstWindow();

    while (obj)
    {
        if (!obj->remove && obj->object.widget)
        {
            MSG_PROTOCOL("Refreshing widget " << TObject::handleToString (obj->handle));
            obj->object.widget->repaint();
        }

        obj = gObject->findNextWindow(obj);
    }

    draw_mutex.unlock();
}

int MainWindow::calcVolume(int value)
{
    DECL_TRACER("TQtSettings::calcVolume(int value)");

    // volumeSliderValue is in the range [0..100]
#ifdef QT5_LINUX
    qreal linearVolume = QAudio::convertVolume(value / qreal(100.0),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    return qRound(linearVolume * 100);
#else
    return value;
#endif
}

/******************* Draw elements *************************/

void MainWindow::displayButton(ulong handle, ulong parent, QByteArray buffer, int width, int height, int pixline, int left, int top)
{
    DECL_TRACER("MainWindow::displayButton(ulong handle, unsigned char* buffer, size_t size, int width, int height, int pixline, int left, int top)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    draw_mutex.lock();

    if (isScaled())
    {
        MSG_DEBUG("Scaling to factor " << mScaleFactor);
    }

    TObject::OBJECT_t *obj = gObject->findObject(handle);
    TObject::OBJECT_t *par = gObject->findObject(parent);
    MSG_TRACE("Processing button " << TObject::handleToString(handle) << " from parent " << TObject::handleToString(parent));

    if (!par)
    {
        if (TStreamError::checkFilter(HLOG_DEBUG))
            MSG_WARNING("Button " << TObject::handleToString(handle) << " has no parent (" << TObject::handleToString(parent) << ")! Ignoring it.");

        draw_mutex.unlock();
        return;
    }

    if (par->animation && !par->aniDirection)
    {
        if (par->animation->state() == QAbstractAnimation::Running)
        {
            MSG_WARNING("Object " << TObject::handleToString(parent) << " is busy with an animation!");
            par->animation->stop();
        }
        else
        {
            MSG_WARNING("Object " << TObject::handleToString(parent) << " has not finished the animation!");
        }

        draw_mutex.unlock();
        return;
    }
    else if (par->remove)
    {
        MSG_WARNING("Object " << TObject::handleToString(parent) << " is marked for remove. Will not draw image!");
        draw_mutex.unlock();
        return;
    }

    if (!obj)
    {
        MSG_DEBUG("Adding new object " << TObject::handleToString(handle) << " ...");
        obj = gObject->addObject();

        if (!obj)
        {
            MSG_ERROR("Error creating an object!");
            TError::setError();
            draw_mutex.unlock();
            return;
        }

        obj->type = TObject::OBJ_BUTTON;
        obj->handle = handle;
        obj->width = scale(width);
        obj->height = scale(height);
        obj->left = scale(left);
        obj->top = scale(top);

        if (par->type == TObject::OBJ_PAGE)
            obj->object.label = new QLabel("", mBackground);
        else
            obj->object.label = new QLabel("", par->object.widget);

        obj->object.label->installEventFilter(this);
        obj->object.label->grabGesture(Qt::PinchGesture);
        obj->object.label->grabGesture(Qt::SwipeGesture);
        obj->object.label->setFixedSize(obj->width, obj->height);
        obj->object.label->move(obj->left, obj->top);
        obj->object.label->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    else
        MSG_DEBUG("Object " << TObject::handleToString(handle) << " of type " << TObject::objectToString(obj->type) << " found!");

    try
    {
        if (buffer.size() > 0 && pixline > 0)
        {
            MSG_DEBUG("Setting image for " << TObject::handleToString(handle) << " ...");
            QImage img((unsigned char *)buffer.data(), width, height, pixline, QImage::Format_ARGB32);  // Original size

            if (img.isNull() || !img.valid(width-1, height-1))
            {
                MSG_ERROR("Unable to create a valid image!");
                draw_mutex.unlock();
                return;
            }

            QSize size(obj->width, obj->height);
            QPixmap pixmap;
            bool ret = false;

            if (isScaled())
                ret = pixmap.convertFromImage(img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)); // Scaled size
            else
                ret = pixmap.convertFromImage(img);

            if (!ret || pixmap.isNull())
            {
                MSG_ERROR("Unable to create a pixmap out of an image!");
                draw_mutex.unlock();
                return;
            }

            if (obj->object.label)
                obj->object.label->setPixmap(pixmap);
            else
            {
                MSG_WARNING("Object " << TObject::handleToString(handle) << " does not exist any more!");
            }
        }

        if (obj->object.label)
            obj->object.label->show();
    }
    catch(std::exception& e)
    {
        MSG_ERROR("Error drawing button " << TObject::handleToString(handle) << ": " << e.what());
    }
    catch(...)
    {
        MSG_ERROR("Unexpected exception occured [MainWindow::displayButton()]");
    }

    gObject->cleanMarked();     // We want to be sure to have no dead entries.
    draw_mutex.unlock();
}

void MainWindow::SetVisible(ulong handle, bool state)
{
    DECL_TRACER("MainWindow::SetVisible(ulong handle, bool state)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    TObject::OBJECT_t *obj = gObject->findObject(handle);

    if (!obj)
    {
        MSG_ERROR("Object " << TObject::handleToString(handle) << " not found!");
        return;
    }

    if (obj->type == TObject::OBJ_BUTTON && obj->object.label)
    {
        MSG_DEBUG("Setting object " << TObject::handleToString(handle) << " visibility to " << (state ? "TRUE" : "FALSE"));
        obj->object.label->setVisible(state);
    }
    else
    {
        MSG_DEBUG("Ignoring non button object " << TObject::handleToString(handle));
    }
}

void MainWindow::setPage(ulong handle, int width, int height)
{
    DECL_TRACER("MainWindow::setPage(ulong handle, int width, int height)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    draw_mutex.lock();

    QSize qs = menuBar()->sizeHint();
    this->setMinimumSize(scale(width), scale(height) + qs.height());
    TObject::OBJECT_t *obj = gObject->findObject(handle);

    if (!obj)
    {
        obj = gObject->addObject();

        if (!obj)
        {
            MSG_ERROR("Error crating an object for handle " << TObject::handleToString(handle));
            TError::setError();
            draw_mutex.unlock();
            return;
        }

        obj->handle = handle;
        obj->height = scale(height);
        obj->width = scale(width);
        obj->type = TObject::OBJ_PAGE;
    }

    bool newBackground = false;

    if (!mBackground)
    {
        mBackground = new QWidget();
        mBackground->grabGesture(Qt::PinchGesture);
        mBackground->grabGesture(Qt::SwipeGesture);
        mBackground->setAutoFillBackground(true);
        mBackground->setBackgroundRole(QPalette::Window);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        mBackground->setFixedSize(obj->width, obj->height);
#else
//        QRect rectBack = mBackground->geometry();
        QRect rectMain = this->geometry();

        QSize icSize =  this->iconSize();
        int width = rectMain.width() + icSize.width() + 16;
        rectMain.setWidth(width);
        setGeometry(rectMain);
        MSG_DEBUG("Height of main window:  " << rectMain.height());
        MSG_DEBUG("Height of panel screen: " << height);
        // If our first top pixel is not 0, maybe because of a menu, window
        // decorations or a toolbar, we must add this extra height to the
        // positions of widgets and mouse presses.
        int avHeight = rectMain.height() - height;
        MSG_DEBUG("Difference in height:   " << avHeight);
//        MSG_DEBUG("avHeight=" << avHeight);
        gPageManager->setFirstTopPixel(avHeight);
#endif
        newBackground = true;
    }

    // By default set a transparent background
    QPixmap pix(obj->width, obj->height);
    pix.fill(QColor::fromRgba(qRgba(0,0,0,0xff)));
    QPalette palette;
    palette.setBrush(QPalette::Window, pix);
    mBackground->setPalette(palette);

    if (newBackground)
        this->setCentralWidget(mBackground);

    mBackground->show();
    draw_mutex.unlock();
}

void MainWindow::setSubPage(ulong handle, int left, int top, int width, int height, ANIMATION_t animate)
{
    draw_mutex.lock();
    DECL_TRACER("MainWindow::setSubPage(ulong handle, int left, int top, int width, int height)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        draw_mutex.unlock();
        return;
    }

    if (isScaled())
    {
        MSG_DEBUG("Scaling to factor " << mScaleFactor);
    }

    TObject::OBJECT_t *obj = gObject->addObject();

    if (!obj)
    {
        MSG_ERROR("Error adding an object!");
        TError::setError();
        draw_mutex.unlock();
        return;
    }

    int scLeft = scale(left);
    int scTop = scale(top);
    int scWidth = scale(width);
    int scHeight = scale(height);

    obj->type = TObject::OBJ_SUBPAGE;
    obj->handle = handle;
    obj->object.widget = new QWidget(centralWidget());
    obj->object.widget->setAutoFillBackground(true);
    obj->object.widget->setFixedSize(scWidth, scHeight);
    obj->object.widget->move(scLeft, scTop);
    obj->left = scLeft;
    obj->top = scTop;
    obj->width = scWidth;
    obj->height = scHeight;
    // filter move event
    obj->object.widget->installEventFilter(this);
    obj->object.widget->grabGesture(Qt::PinchGesture);
    obj->object.widget->grabGesture(Qt::SwipeGesture);
    // By default set a transparent background
    QPixmap pix(scWidth, scHeight);
    pix.fill(QColor::fromRgba(qRgba(0,0,0,0xff)));
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(pix));
    obj->object.widget->setPalette(palette);
    obj->aniDirection = true;

    startAnimation(obj, animate);
    draw_mutex.unlock();
}

void MainWindow::setBackground(ulong handle, QByteArray image, size_t rowBytes, int width, int height, ulong color)
{
    draw_mutex.lock();
    DECL_TRACER("MainWindow::setBackground(ulong handle, QByteArray image, size_t rowBytes, ulong color)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        draw_mutex.unlock();
        return;
    }


    TObject::OBJECT_t *obj = gObject->findObject(handle);

    if (!obj)
    {
        MSG_WARNING("No object " << TObject::handleToString(handle) << " found!");
        draw_mutex.unlock();
        return;
    }

    MSG_TRACE("Object " << TObject::handleToString(handle) << " of type " << gObject->objectToString(obj->type) << " found!");

    if (obj->type == TObject::OBJ_BUTTON || obj->type == TObject::OBJ_SUBPAGE)
    {
        MSG_DEBUG("Processing object " << gObject->objectToString(obj->type));
        QPixmap pix(obj->width, obj->height);
        pix.fill(QColor::fromRgba(qRgba(TColor::getRed(color),TColor::getGreen(color),TColor::getBlue(color),TColor::getAlpha(color))));

        if (image.size() > 0)
        {
            MSG_DEBUG("Setting image of size " << image.size() << " (" << width << " x " << height << ")");
            QImage img((unsigned char *)image.data(), width, height, rowBytes, QImage::Format_ARGB32);

            if (isScaled())
            {
                QSize size(obj->width, obj->height);
                pix.convertFromImage(img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            else
                pix.convertFromImage(img);
        }

        if (obj->type == TObject::OBJ_BUTTON)
        {
            obj->object.label->setPixmap(pix);

            if (mHasFocus)
                obj->object.label->show();
        }
        else
        {
            MSG_DEBUG("Setting image as background for page " << ((handle >> 16) & 0x0000ffff));
            QPalette palette;
            palette.setBrush(QPalette::Window, QBrush(pix));
            obj->object.widget->setPalette(palette);

            obj->object.widget->show();
        }
    }
    else if (obj->type == TObject::OBJ_PAGE)
    {
        bool newBackground = false;

        if (!mBackground)
        {
            mBackground = new QWidget();
            mBackground->setAutoFillBackground(true);
            mBackground->setBackgroundRole(QPalette::Window);
            mBackground->setFixedSize(obj->width, obj->height);
            newBackground = true;
            MSG_DEBUG("New background image added to page with size " << obj->width << " x " << obj->height);
        }

        QPixmap pix(obj->width, obj->height);
        pix.fill(QColor::fromRgba(qRgba(TColor::getRed(color),TColor::getGreen(color),TColor::getBlue(color),TColor::getAlpha(color))));

        if (image.size() > 0)
        {
            QImage img((unsigned char *)image.data(), width, height, rowBytes, QImage::Format_ARGB32);

            if (isScaled())
                pix.convertFromImage(img.scaled(obj->width, obj->height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            else
                pix.convertFromImage(img);

            MSG_DEBUG("Image converted. Width=" << obj->width << ", Height=" << obj->height);
        }

        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(pix));
        mBackground->setPalette(palette);

        if (newBackground)
            this->setCentralWidget(mBackground);

        if (mHasFocus)
            mBackground->show();

        MSG_DEBUG("Background set");
    }

    draw_mutex.unlock();
}

void MainWindow::dropPage(ulong handle)
{
    draw_mutex.lock();
    DECL_TRACER("MainWindow::dropPage(ulong handle)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        draw_mutex.unlock();
        return;
    }

    gObject->removeAllChilds(handle);
    gObject->removeObject(handle);

    if (mBackground)
    {
        delete mBackground;
        mBackground = nullptr;
    }

    draw_mutex.unlock();
}

void MainWindow::dropSubPage(ulong handle)
{
    draw_mutex.lock();
    DECL_TRACER("MainWindow::dropSubPage(ulong handle)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        draw_mutex.unlock();
        return;
    }

    gObject->removeAllChilds(handle);
    TObject::OBJECT_t *obj = gObject->findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object " << TObject::handleToString(handle) << " does not exist. Ignoring!");
        draw_mutex.unlock();
        return;
    }

    obj->aniDirection = false;
    startAnimation(obj, obj->animate, false);
    TObject::OBJECT_t *o = mLastObject;

    if (obj->animate.hideEffect == SE_NONE || !o)
    {
        gObject->dropContent(obj);
        gObject->removeObject(handle);
    }

    draw_mutex.unlock();
}

void MainWindow::dropButton(ulong handle)
{
    draw_mutex.lock();
    DECL_TRACER("MainWindow::dropButton(ulong handle)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        draw_mutex.unlock();
        return;
    }

    TObject::OBJECT_t *obj = gObject->findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object " << TObject::handleToString(handle) << " does not exist. Ignoring!");
        draw_mutex.unlock();
        return;
    }

    if (obj->type == TObject::OBJ_BUTTON && obj->object.label)
    {
        obj->object.label->close();
        obj->object.label = nullptr;
    }

    gObject->removeObject(handle);
    draw_mutex.unlock();
}

void MainWindow::playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const string& url, const string& user, const string& pw)
{
    draw_mutex.lock();
    DECL_TRACER("MainWindow::playVideo(ulong handle, const string& url, const string& user, const string& pw))");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        draw_mutex.unlock();
        return;
    }

    TObject::OBJECT_t *obj = gObject->findObject(handle);
    TObject::OBJECT_t *par = gObject->findObject(parent);
    MSG_TRACE("Processing button " << TObject::handleToString(handle) << " from parent " << TObject::handleToString(parent));

    if (!par)
    {
        MSG_WARNING("Button has no parent! Ignoring it.");
        draw_mutex.unlock();
        return;
    }

    if (!obj)
    {
        MSG_DEBUG("Adding new video object ...");
        obj = gObject->addObject();

        if (!obj)
        {
            MSG_ERROR("Error creating a video object!");
            TError::setError();
            draw_mutex.unlock();
            return;
        }

        obj->type = TObject::OBJ_VIDEO;
        obj->handle = handle;
        obj->width = width;
        obj->height = height;
        obj->left = left;
        obj->top = top;
        obj->object.vwidget = new QVideoWidget(par->object.widget);
        obj->object.vwidget->installEventFilter(this);
    }
    else
        MSG_DEBUG("Object " << TObject::handleToString(handle) << " of type " << gObject->objectToString(obj->type) << " found!");

#if defined(QT5_LINUX) && !defined(QT6_LINUX)
    QMediaPlaylist *playlist = new QMediaPlaylist;
#endif
    QUrl qurl(url.c_str());

    if (!user.empty())
        qurl.setUserName(user.c_str());

    if (!pw.empty())
        qurl.setPassword(pw.c_str());

#if defined(QT5_LINUX) && !defined(QT6_LINUX)
    playlist->addMedia(qurl);
#endif
    obj->player = new QMediaPlayer;
#if defined(QT5_LINUX) && !defined(QT6_LINUX)
    obj->player->setPlaylist(playlist);
#else
    obj->player->setSource(qurl);
#endif
    obj->player->setVideoOutput(obj->object.vwidget);

    obj->object.vwidget->show();
    obj->player->play();
}

void MainWindow::inputText(Button::TButton* button, QByteArray buf, int width, int height, size_t pixline)
{
    DECL_TRACER("MainWindow::inputText(Button::TButton* button)");

    if (!gObject)
    {
        MSG_ERROR(_NO_OBJECT);
        return;
    }

    if (!button)
    {
        MSG_WARNING("No valid button!");
        return;
    }

    ulong handle = button->getHandle();
    ulong parent = button->getParent();
    TObject::OBJECT_t *obj = gObject->findObject(handle);
    TObject::OBJECT_t *par = gObject->findObject(parent);
    MSG_TRACE("Processing button " << TObject::handleToString(handle) << " from parent " << gObject->handleToString(parent));

    if (!par)
    {
        MSG_WARNING("Button has no parent! Ignoring it.");
        return;
    }

    if (!obj)
    {
        MSG_DEBUG("Adding new input object ...");
        obj = gObject->addObject();

        if (!obj)
        {
            MSG_ERROR("Error creating an input object!");
            TError::setError();
            return;
        }

        obj->type = TObject::OBJ_INPUT;
        obj->handle = handle;
        obj->width = scale(width);
        obj->height = scale(height);
        obj->left = scale(button->getLeftPosition());
        obj->top = scale(button->getTopPosition());

        if (button->isSingleLine())
        {
            obj->object.linetext = new QLineEdit(button->getText().c_str(), par->object.widget);
            obj->object.linetext->setFixedSize(obj->width, obj->height);
            obj->object.linetext->move(obj->left, obj->top);
//            obj->object.linetext->setAutoFillBackground(true);
            obj->object.linetext->installEventFilter(this);
            obj->object.linetext->connect(obj->object.linetext, &QLineEdit::editingFinished, this, &MainWindow::textSingleLineReturn);
            obj->wid = obj->object.linetext->winId();
        }
        else
        {
            obj->object.multitext = new QTextEdit(button->getText().c_str(), par->object.widget);
            obj->object.multitext->setFixedSize(obj->width, obj->height);
            obj->object.multitext->move(obj->left, obj->top);
//            obj->object.multitext->setAutoFillBackground(true);
            obj->object.multitext->installEventFilter(this);
            obj->object.multitext->connect(obj->object.multitext, &QTextEdit::textChanged, this, &MainWindow::textChangedMultiLine);
            obj->wid = obj->object.multitext->winId();
        }

        if (!buf.size() || pixline == 0)
        {
            MSG_ERROR("No image!");
            TError::setError();
            return;
        }

        MSG_DEBUG("Background image size: " << width << " x " << height << ", rowBytes: " << pixline);
        QPixmap pix(width, height);
        QImage img((uchar *)buf.data(), width, height, QImage::Format_ARGB32);

        if (isScaled())
            pix.convertFromImage(img.scaled(scale(width), scale(height), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        else
            pix.convertFromImage(img);

        // Load the font
        FONT_T font = button->getFont();
        int fontID = 0;
        vector<string> fontList = TFont::getFontPathList();
        vector<string>::iterator iter;
        string ffile;

        for (iter = fontList.begin(); iter != fontList.end(); ++iter)
        {
            TValidateFile vf;

            if (!vf.isValidFile(*iter + "/" + font.file))
                continue;

            ffile = *iter + "/" + font.file;
            break;
        }

        if (ffile.empty())
        {
            MSG_ERROR("Font " << font.file << " doesn't exists!");
            return;
        }

        if ((fontID = QFontDatabase::addApplicationFont(ffile.c_str())) == -1)
        {
            MSG_ERROR("Font " << ffile << " could not be loaded!");
            TError::setError();
            return;
        }

        QFont ft;
        ft.setFamily(font.name.c_str());
        ft.setPointSize(font.size);
        MSG_DEBUG("Using font \"" << font.name << "\" with size " << font.size << "pt.");

        switch (button->getFontStyle())
        {
            case FONT_BOLD:     ft.setBold(true); break;
            case FONT_ITALIC:   ft.setItalic(true); break;
            case FONT_BOLD_ITALIC:
                ft.setBold(true);
                ft.setItalic(true);
                break;

            default:
                ft.setBold(false);
                ft.setItalic(false);
        }

        QPalette palette;
        TColor::COLOR_T textColor = TColor::getAMXColor(button->getTextColor());
        TColor::COLOR_T fillColor = TColor::getAMXColor(button->getFillColor());
        QColor txcolor(QColor::fromRgba(qRgba(textColor.red, textColor.green, textColor.blue, textColor.alpha)));
        QColor cfcolor(QColor::fromRgba(qRgba(fillColor.red, fillColor.green, fillColor.blue, fillColor.alpha)));
        palette.setColor(QPalette::Base, cfcolor);
        palette.setColor(QPalette::Text, txcolor);
        //        pix.save("frame.png");

        if (button->isSingleLine())
        {
            MSG_DEBUG("Initializing a single line ...");
//            palette.setBrush(QPalette::Base, QBrush(pix));
            obj->object.linetext->setFont(ft);
            obj->object.linetext->setPalette(palette);
            obj->object.linetext->setMaxLength(button->getTextMaxChars());
//            obj->object.linetext->setFrame(false);
            obj->object.linetext->setText(button->getText().c_str());
            obj->object.linetext->show();
        }
        else
        {
            MSG_DEBUG("Initializing a multiline text area ...");
            palette.setBrush(QPalette::Base, QBrush(pix));
            obj->object.multitext->setPalette(palette);
            obj->object.multitext->setFont(ft);
            obj->object.multitext->setAcceptRichText(false);
            obj->object.multitext->setText(button->getText().c_str());

            if (button->getTextWordWrap())
                obj->object.multitext->setWordWrapMode(QTextOption::WordWrap);
            else
                obj->object.multitext->setWordWrapMode(QTextOption::NoWrap);

            obj->object.multitext->show();
        }
    }
    else
        MSG_DEBUG("Object " << TObject::handleToString(handle) << " of type " << gObject->objectToString(obj->type) << " found!");
}

void MainWindow::showKeyboard(const std::string& init, const std::string& prompt, bool priv)
{
    DECL_TRACER("MainWindow::showKeyboard(std::string &init, std::string &prompt, bool priv)");

    if (mKeyboard)
        return;

    mQKeyboard = new TQKeyboard(init, prompt, this);
    mKeyboard = true;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mQKeyboard->setScaleFactor(mScaleFactor);
#endif
    mQKeyboard->setPrivate(priv);
    mQKeyboard->doResize();
    mQKeyboard->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    int ret = mQKeyboard->exec();
    string text = "KEYB-";

    if (ret == QDialog::Accepted)
        text.append(mQKeyboard->getText());
    else
        text = "KEYB-ABORT";

    if (gPageManager)
        gPageManager->sendKeyboard(text);

    delete mQKeyboard;
    mQKeyboard = nullptr;
    mKeyboard = false;
}

void MainWindow::showKeypad(const std::string& init, const std::string& prompt, bool priv)
{
    DECL_TRACER("MainWindow::showKeypad(std::string& init, std::string& prompt, bool priv)");

    if (mKeypad)
        return;

    mQKeypad = new TQKeypad(init, prompt, this);
    mKeypad = true;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mQKeypad->setScaleFactor(mScaleFactor);
#endif
    mQKeypad->setPrivate(priv);
    mQKeypad->setMaxLength(50);     // Standard maximum length
    mQKeypad->doResize();
    mQKeypad->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    int ret = mQKeypad->exec();

    if (ret == QDialog::Accepted)
    {
        string text = "KEYP-";
        text.append(mQKeypad->getText());

        if (gPageManager)
            gPageManager->sendKeypad(text);
    }
    else
    {
        string text = "KEYP-ABORT";

        if (gPageManager)
            gPageManager->sendKeypad(text);
    }

    delete mQKeypad;
    mQKeypad = nullptr;
    mKeypad = false;
}

void MainWindow::resetKeyboard()
{
    DECL_TRACER("MainWindow::resetKeyboard()");

    if (mQKeyboard)
        mQKeyboard->reject();

    if (mQKeypad)
        mQKeyboard->reject();
}

void MainWindow::sendVirtualKeys(const string& str)
{
    DECL_TRACER("MainWindow::sendVirtualKeys(const string& str)");

    if (mKeyboard && mQKeyboard)
        mQKeyboard->setString(str);
    else if (mKeypad && mQKeypad)
        mQKeypad->setString(str);
}

void MainWindow::showSetup()
{
    DECL_TRACER("MainWindow::showSetup()");

    settings();
}

void MainWindow::playSound(const string& file)
{
    DECL_TRACER("MainWindow::playSound(const string& file)");

    MSG_DEBUG("Playing file " << file);

    if (TConfig::getMuteState())
        return;

    if (!mMediaPlayer)
    {
        mMediaPlayer = new QMediaPlayer;
#ifdef QT6_LINUX
        mAudioOutput = new QAudioOutput;
#endif
    }

#ifdef QT5_LINUX
    mMediaPlayer->setMedia(QUrl::fromLocalFile(file.c_str()));
    mMediaPlayer->setVolume(calcVolume(TConfig::getSystemVolume()));
#else
    mMediaPlayer->setSource(QUrl::fromLocalFile(file.c_str()));
    mAudioOutput->setVolume(TConfig::getSystemVolume());
#endif
    mMediaPlayer->play();
}

void MainWindow::stopSound()
{
    DECL_TRACER("MainWindow::stopSound()");

    if (mMediaPlayer)
        mMediaPlayer->stop();
}

void MainWindow::muteSound(bool state)
{
    DECL_TRACER("MainWindow::muteSound(bool state)");

#ifdef QT5_LINUX
    if (mMediaPlayer)
        mMediaPlayer->setMuted(state);
#else
    if (mAudioOutput)
        mAudioOutput->setMuted(state);
#endif
}

void MainWindow::playShowList()
{
    DECL_TRACER("MainWindow::playShowList()");

    _EMIT_TYPE_t etype = getNextType();

    while (etype != ET_NONE)
    {
        ulong handle = 0;
        ulong parent = 0;
        unsigned char *buffer;
        int pixline = 0;
        int left = 0;
        int top = 0;
        int width = 0;
        int height = 0;
        unsigned char *image;
        size_t size = 0;
        size_t rowBytes = 0;
        ulong color = 0;
        ANIMATION_t animate;
        std::string url;
        std::string user;
        std::string pw;
        Button::TButton *button;
        Button::BITMAP_t bm;

        switch(etype)
        {
            case ET_BACKGROUND:
                if (getBackground(&handle, &image, &size, &rowBytes, &width, &height, &color))
                {
                    QByteArray buf;

                    if (image && size > 0)
                        buf.insert(0, (const char *)image, size);

                    emit sigSetBackground(handle, buf, rowBytes, width, height, color);
                }
            break;

            case ET_BUTTON:
                if (getButton(&handle, &parent, &buffer, &pixline, &left, &top, &width, &height))
                {
                    QByteArray buf;

                    if (buffer && pixline > 0)
                    {
                        size_t size = width * height * (pixline / width);
                        MSG_DEBUG("Buffer size=" << size << ", width=" << width << ", height=" << height << ", left=" << left << ", top=" << top);
                        buf.insert(0, (const char *)buffer, size);
                    }

                    emit sigDisplayButton(handle, parent, buf, width, height, pixline, left, top);
                }
            break;

            case ET_INTEXT:
                if (getInText(&handle, &button, &bm))
                {
                    QByteArray buf;

                    if (bm.buffer && bm.rowBytes > 0)
                    {
                        size_t size = bm.width * bm.height * (bm.rowBytes / bm.width);
                        buf.insert(0, (const char *)bm.buffer, size);
                    }

                    emit sigInputText(button, buf, bm.width, bm.height, bm.rowBytes);
                }
            break;

            case ET_PAGE:
                if (getPage(&handle, &width, &height))
                {
                    if (isDeleted())
                        emit sigDropPage(handle);
                    else
                        emit sigSetPage(handle, width, height);
                }
            break;

            case ET_SUBPAGE:
                if (getSubPage(&handle, &left, &top, &width, &height, &animate))
                {
                    if (isDeleted())
                        emit sigDropSubPage(handle);
                    else
                        emit sigSetSubPage(handle, left, top, width, height, animate);
                }
            break;

            case ET_VIDEO:
                if (getVideo(&handle, &parent, &left, &top, &width, &height, &url, &user, &pw))
                {
                    emit sigPlayVideo(handle, parent, left, top, width, height, url, user, pw);
                }
            break;

            default:
                MSG_WARNING("Type " << etype << " is currently not supported!");
        }

        dropType(etype);
        etype = getNextType();
    }
/*
    std::map<ulong, QWidget *>::iterator iter;

    for (iter = mToShow.begin(); iter != mToShow.end(); ++iter)
    {
        OBJECT_t *obj = findObject(iter->first);

        if (obj)
            iter->second->show();
    }

    mToShow.clear();
*/
}


int MainWindow::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}

void MainWindow::startAnimation(TObject::OBJECT_t* obj, ANIMATION_t& ani, bool in)
{
    DECL_TRACER("MainWindow::startAnimation(OBJECT_t* obj, ANIMATION_t& ani)");

    if (!obj)
    {
        MSG_ERROR("Got no object to start the animation!");
        return;
    }

    SHOWEFFECT_t effect;
    int scLeft = obj->left;
    int scTop = obj->top;
    int scWidth = obj->width;
    int scHeight = obj->height;
    mLastObject = nullptr;

    obj->animate = ani;

    if (in)
        effect = ani.showEffect;
    else
        effect = ani.hideEffect;

    if (effect == SE_NONE)
        return;

    if (effect == SE_FADE)
    {
        MSG_DEBUG("Fading object " << TObject::handleToString(obj->handle) << (in ? " IN" : " OUT"));
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(obj->object.widget);
        obj->object.widget->setGraphicsEffect(effect);
        obj->animation = new QPropertyAnimation(effect, "opacity");
    }
    else
    {
        MSG_DEBUG("Moving object " << TObject::handleToString(obj->handle) << (in ? " IN" : " OUT"));
        obj->animation = new QPropertyAnimation(obj->object.widget);
        obj->animation->setTargetObject(obj->object.widget);
    }

    if (in)
        obj->animation->setDuration(ani.showTime * 100);    // convert 10th of seconds into milliseconds
    else
        obj->animation->setDuration(ani.hideTime * 100);    // convert 10th of seconds into milliseconds

    switch(effect)
    {
        case SE_SLIDE_BOTTOM_FADE:
        case SE_SLIDE_BOTTOM:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft, scTop + (scHeight * 2), scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop + (scHeight * 2), scWidth, scHeight));
                obj->remove = true;
                mLastObject = obj;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            obj->animation->start();
        break;

        case SE_SLIDE_LEFT_FADE:
        case SE_SLIDE_LEFT:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft - scWidth, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft - scWidth, scTop, scWidth, scHeight));
                obj->remove = true;
                mLastObject = obj;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            obj->animation->start();
        break;

        case SE_SLIDE_RIGHT_FADE:
        case SE_SLIDE_RIGHT:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft + scWidth, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft + scWidth, scTop, scWidth, scHeight));
                obj->remove = true;
                mLastObject = obj;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            obj->animation->start();
        break;

        case SE_SLIDE_TOP_FADE:
        case SE_SLIDE_TOP:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft, scTop - scHeight, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop - scHeight, scWidth, scHeight));
                obj->remove = true;
                mLastObject = obj;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            obj->animation->start();
        break;

        case SE_FADE:
            if (in)
            {
                obj->object.widget->setWindowOpacity(0.0);
                obj->object.widget->show();
                obj->animation->setStartValue(0.0);
                obj->animation->setEndValue(1.0);
            }
            else
            {
                obj->animation->setStartValue(1.0);
                obj->animation->setEndValue(0.0);
                obj->remove = true;
                mLastObject = obj;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            obj->animation->setEasingCurve(QEasingCurve::Linear);
            obj->animation->start();
        break;

        default:
            MSG_WARNING("Subpage effect " << ani.showEffect << " is not supported.");
    }
}

void MainWindow::busyIndicator(const string& msg, QWidget* parent)
{
    DECL_TRACER("MainWindow::busyIndicator(const string& msg, QWidget* parent)");

    if (mBusy)
        return;

    mBusy = true;
    mBusyDialog = new TQBusy(msg, parent);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mBusyDialog->setScaleFactor(gScale);
    mBusyDialog->doResize();
#endif
    mBusyDialog->show();
}

void MainWindow::downloadBar(const string &msg, QWidget *parent)
{
    DECL_TRACER("void MainWindow::downloadBar(const string &msg, QWidget *parent)");

    if (mBusy)
        return;

    mBusy = true;
    mDownloadBar = new TqDownload(msg, parent);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mDownloadBar->setScaleFactor(gScale);
    mDownloadBar->doResize();
#endif
    mDownloadBar->show();
}

void MainWindow::runEvents()
{
    DECL_TRACER("MainWindow::runEvents()");

    QApplication::processEvents();
}

#ifndef QT_NO_SESSIONMANAGER
void MainWindow::commitData(QSessionManager &manager)
{
    if (manager.allowsInteraction())
    {
        if (!settingsChanged)
            manager.cancel();
    }
    else
    {
        if (settingsChanged)
            writeSettings();
    }
}
#endif

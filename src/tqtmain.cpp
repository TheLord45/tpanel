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
#include <QStackedWidget>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QTouchEvent>
#include <QPalette>
#include <QPixmap>
#include <QFont>
#include <QFontDatabase>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtMultimedia/QMediaPlayer>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#   include <QtMultimedia/QMediaPlaylist>
#else
#   include <QAudioOutput>
#   include <QMediaMetaData>
#endif
#include <QListWidget>
#include <QLayout>
#include <QSizePolicy>
#include <QUrl>
#include <QThread>
#ifdef Q_OS_IOS
#include <QtSensors/QOrientationSensor>
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#   include <QSound>
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
#   include <QtSensors/QOrientationSensor>
#   include <qpa/qplatformscreen.h>
#endif
#else
#   include <QPlainTextEdit>
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
#   include <QGeoPositionInfoSource>
#   include <QtSensors/QOrientationReading>
#   include <QPermissions>
#endif
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) && defined(Q_OS_ANDROID)
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroidExtras/QtAndroid>
#endif
#include <functional>
//#include <mutex>
#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif
#include "tpagemanager.h"
#include "tqtmain.h"
#include "tconfig.h"
#if !defined(Q_OS_IOS) && !defined(Q_OS_ANDROID)
#include "tqtsettings.h"
#endif
#include "tqkeyboard.h"
#include "tqkeypad.h"
#include "tcolor.h"
#include "texcept.h"
#include "ttpinit.h"
#include "tqdownload.h"
#include "tqtphone.h"
#include "tqeditline.h"
#include "tqtinputline.h"
#include "tqmarquee.h"
#include "tqtwait.h"
#include "terror.h"
#include "tresources.h"
#include "tqscrollarea.h"
#include "tlock.h"
#ifdef Q_OS_IOS
#include "ios/QASettings.h"
#include "ios/tiosrotate.h"
#include "ios/tiosbattery.h"
#endif
#if TESTMODE == 1
#include "testmode.h"
#endif

#if __cplusplus < 201402L
#   error "This module requires at least C++14 standard!"
#else   // __cplusplus < 201402L
#   if __cplusplus < 201703L
#       include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#       warning "Support for C++14 and experimental filesystem will be removed in a future version!"
#   else    // __cplusplus < 201703L
#       include <filesystem>
#       ifdef __ANDROID__
namespace fs = std::__fs::filesystem;
#       else    // __ANDROID__
namespace fs = std::filesystem;
#       endif   // __ANDROID__
#   endif   // __cplusplus < 201703L
#endif  // __cplusplus < 201402L


extern amx::TAmxNet *gAmxNet;                   //!< Pointer to the class running the thread which handles the communication with the controller.
extern bool _restart_;                          //!< If this is set to true then the whole program will start over.
extern TPageManager *gPageManager;              //!< The pointer to the global defined main class.
static bool isRunning = false;                  //!< TRUE = the pageManager was started.
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
static double gScale = 1.0;                     //!< Global variable holding the scale factor.
static int gFullWidth = 0;                      //!< Global variable holding the width of the AMX screen. This is used to calculate the scale factor for the settings dialog.
std::atomic<double> mScaleFactorW{1.0};
std::atomic<double> mScaleFactorH{1.0};
int gScreenWidth{0};
int gScreenHeight{0};
bool isPortrait{false};
#endif

using std::bind;
using std::map;
using std::pair;
using std::string;
using std::vector;

static string _NO_OBJECT = "The global class TObject is not available!";

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

    gPageManager = pmanager;
#ifdef __ANDROID__
    MSG_INFO("Android API version: " << __ANDROID_API__);

#if __ANDROID_API__ < 30
#warning "The Android API version is less than 30! Some functions may not work!"
#endif  // __ANDROID_API__
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/HideToolbar", "hide", "(Landroid/app/Activity;Z)V", activity.object(), true);
#else
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/HideToolbar", "hide", "(Landroid/app/Activity;Z)V", activity.object(), true);
#endif  // QT5_LINUX
#endif  // __ANDROID__

#if defined(Q_OS_ANDROID)
    QApplication::setAttribute(Qt::AA_ForceRasterWidgets);
//    QApplication::setAttribute(Qt::AA_Use96Dpi);
//    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
#endif

    QApplication app(argc, argv);
    app.setApplicationName(QString::fromStdString(TConfig::getProgName()));
    app.setApplicationDisplayName("TPanel");
    app.setApplicationVersion(VERSION_STRING());
    // Set the orientation
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QScreen *screen = QGuiApplication::primaryScreen();

    if (!screen)
    {
        MSG_ERROR("Couldn't determine the primary screen!")
        return 1;
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (pmanager->getSettings()->isPortrait())  // portrait?
    {
        MSG_INFO("Orientation set to portrait mode.");
        screen->setOrientationUpdateMask(Qt::PortraitOrientation);
    }
    else
    {
        MSG_INFO("Orientation set to landscape mode.");
        screen->setOrientationUpdateMask(Qt::LandscapeOrientation);
    }
#endif  // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
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
        QSize screenSize = screen->size();
        double width = 0.0;
        double height = 0.0;
        gScreenWidth = std::max(screenSize.width(), screenSize.height());
        gScreenHeight = std::min(screenSize.height(), screenSize.width());

        if (screenSize.width() > screenSize.height())
            isPortrait = false;
        else
            isPortrait = true;

        int minWidth = pmanager->getSettings()->getWidth();
        int minHeight = pmanager->getSettings()->getHeight();

        if (pmanager->getSettings()->isPortrait())  // portrait?
        {
            width = std::min(gScreenWidth, gScreenHeight);
            height = std::max(gScreenHeight, gScreenWidth);
        }
        else
        {
            width = std::max(gScreenWidth, gScreenHeight);
            height = std::min(gScreenHeight, gScreenWidth);
        }

        if (!TConfig::getToolbarSuppress() && TConfig::getToolbarForce())
            minWidth += 48;

        MSG_INFO("Dimension of AMX screen:" << minWidth << " x " << minHeight);
        MSG_INFO("Screen size: " << width << " x " << height);
        // The scale factor is always calculated in difference to the prefered
        // size of the original AMX panel.
        mScaleFactorW = width / (double)minWidth;
        mScaleFactorH = height / (double)minHeight;
        scale = std::min(mScaleFactorW, mScaleFactorH);
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_DEBUG, "tpanel", "INF    ##, ???????????? scale: %f (Screen: %1.0fx%1.0f, Page: %dx%d)", scale, width, height, minWidth, minHeight);
#endif
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
    QPalette palette(mainWin.palette());
    palette.setColor(QPalette::Window, Qt::black);  // Keep the background black. Helps to save battery on OLED displays.
    mainWin.setPalette(palette);
    mainWin.grabGesture(Qt::PinchGesture);
    mainWin.grabGesture(Qt::SwipeGesture);
    mainWin.setOrientation(Qt::PrimaryOrientation);
//#ifdef Q_OS_IOS
 //   mainWin.setWindowFlag(Qt::MaximizeUsingFullscreenGeometryHint, true);
//#endif

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
    : mIntercom(this)
{
    DECL_TRACER("MainWindow::MainWindow()");

    TObject::setParent(this);

    if (!gPageManager)
    {
        EXCEPTFATALMSG("The class TPageManager was not initialized!");
    }

    mGestureFilter = new TQGestureFilter(this);
    connect(mGestureFilter, &TQGestureFilter::gestureEvent, this, &MainWindow::onGestureEvent);
    setAttribute(Qt::WA_AcceptTouchEvents, true);   // We accept touch events
    grabGesture(Qt::PinchGesture);                  // We use a pinch gesture to open the settings dialog
    grabGesture(Qt::SwipeGesture);                  // We support swiping also

#ifdef Q_OS_IOS                                     // Block autorotate on IOS
//    initGeoLocation();
    mIosRotate = new TIOSRotate;
//    mIosRotate->automaticRotation(false);           // FIXME: This doesn't work!

    if (!mSensor)
    {
        mSensor = new QOrientationSensor(this);

        if (mSensor)
        {
            mSensor->setAxesOrientationMode(QSensor::AutomaticOrientation);

            if (gPageManager && gPageManager->getSettings()->isPortrait())
                mSensor->setCurrentOrientation(Qt::PortraitOrientation);
            else
                mSensor->setCurrentOrientation(Qt::LandscapeOrientation);
        }
    }
#endif  // Q_OS_IOS

    // We create the central widget here to make sure the application
    // initializes correct. On mobiles the whole screen is used while on
    // desktops a window with the necessary size is created.
    QWidget *central = new QWidget;
    central->setObjectName("centralWidget");
    central->setBackgroundRole(QPalette::Window);
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
//    central->setAutoFillBackground(false);
    central->setFixedSize(gScreenWidth, gScreenHeight);
#endif  // defined(Q_OS_IOS) || defined(Q_OSANDROID)
    setCentralWidget(central);      // Here we set the central widget
    central->show();
    // This is a stacked widget used to hold all pages. With it we can also
    // simply manage the objects bound to a page.
    mCentralWidget = new QStackedWidget(central);
    mCentralWidget->setObjectName("stackedPageWidgets");
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    MSG_DEBUG("Size will be set for " << (isPortrait ? "PORTRAIT" : "LANDSCAPE"));

    if (gPageManager && gPageManager->getSettings()->isLandscape())
    {
        if (!isPortrait)
            mCentralWidget->setFixedSize((mToolbar ? gScreenWidth - mToolbar->width() : gScreenWidth), gScreenHeight);
        else
            mCentralWidget->setFixedSize(gScreenWidth, gScreenHeight);
    }
    else
    {
        if (isPortrait)
            mCentralWidget->setFixedSize((mToolbar ? gScreenHeight - mToolbar->width() : gScreenHeight), gScreenWidth);
        else
            mCentralWidget->setFixedSize(gScreenHeight, gScreenWidth);
    }
#else
    QSize qs = menuBar()->sizeHint();
    QSize icSize =  iconSize();
    int lwidth = gPageManager->getSettings()->getWidth() + icSize.width() + 16;
    int lheight = gPageManager->getSettings()->getHeight() + qs.height();
    mCentralWidget->setFixedSize(gPageManager->getSettings()->getWidth(), gPageManager->getSettings()->getHeight());
#endif  // defined(Q_OS_IOS) || defined(Q_OS_ANDROID)

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    if (gPageManager->getSettings()->isPortrait())  // portrait?
    {
        MSG_INFO("Orientation set to portrait mode.");
        _setOrientation(O_PORTRAIT);
        mOrientation = Qt::PortraitOrientation;
    }
    else
    {
        MSG_INFO("Orientation set to landscape mode.");
        _setOrientation(O_LANDSCAPE);
        mOrientation = Qt::LandscapeOrientation;
    }
#else
    QRect rectMain = geometry();
    rectMain.setWidth(lwidth);
    rectMain.setHeight(lheight);
    setGeometry(rectMain);
    MSG_DEBUG("Height of main window:  " << rectMain.height());
    MSG_DEBUG("Height of panel screen: " << lheight);
    // If our first top pixel is not 0, maybe because of a menu, window
    // decorations or a toolbar, we must add this extra height to the
    // positions of widgets and mouse presses.
    int avHeight = rectMain.height() - gPageManager->getSettings()->getHeight();
    MSG_DEBUG("Difference in height:   " << avHeight);
    gPageManager->setFirstTopPixel(avHeight);
#endif
    setWindowIcon(QIcon(":images/icon.png"));

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
                                       std::placeholders::_8,
                                       std::placeholders::_9,
                                       std::placeholders::_10));

    gPageManager->registerSetMarqueeText(bind(&MainWindow::_setMarqueeText, this, std::placeholders::_1));

    gPageManager->regDisplayViewButton(bind(&MainWindow::_displayViewButton, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3,
                                          std::placeholders::_4,
                                          std::placeholders::_5,
                                          std::placeholders::_6,
                                          std::placeholders::_7,
                                          std::placeholders::_8,
                                          std::placeholders::_9,
                                          std::placeholders::_10));

    gPageManager->regAddViewButtonItems(bind(&MainWindow::_addViewButtonItems, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2));

    gPageManager->regUpdateViewButton(bind(&MainWindow::_updateViewButton, this,
                                           std::placeholders::_1,
                                           std::placeholders::_2,
                                           std::placeholders::_3,
                                           std::placeholders::_4));

    gPageManager->regUpdateViewButtonItem(bind(&MainWindow::_updateViewButtonItem, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));

    gPageManager->regShowSubViewItem(bind(&MainWindow::_showViewButtonItem, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          std::placeholders::_3,
                                          std::placeholders::_4));

    gPageManager->regHideAllSubViewItems(bind(&MainWindow::_hideAllViewItems, this, std::placeholders::_1));
    gPageManager->regHideSubViewItem(bind(&MainWindow::_hideViewItem, this, std::placeholders::_1, std::placeholders::_2));
    gPageManager->regSetSubViewPadding(bind(&MainWindow::_setSubViewPadding, this, std::placeholders::_1, std::placeholders::_2));

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
                                          std::placeholders::_6,
                                          std::placeholders::_7,
                                          std::placeholders::_8,
                                          std::placeholders::_9));

    gPageManager->registerCallbackSB(bind(&MainWindow::_setBackground, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3,
                                         std::placeholders::_4,
#ifdef _OPAQUE_SKIA_
                                         std::placeholders::_5));
#else
                                         std::placeholders::_5,
                                         std::placeholders::_6));
#endif
    gPageManager->regCallMinimizeSubpage(bind(&MainWindow::_minimizeSubpage, this, std::placeholders::_1));
    gPageManager->regCallMaximizeSubpage(bind(&MainWindow::_maximizeSubpage, this, std::placeholders::_1));
    gPageManager->regCallDropPage(bind(&MainWindow::_dropPage, this, std::placeholders::_1));
    gPageManager->regCallDropSubPage(bind(&MainWindow::_dropSubPage, this, std::placeholders::_1, std::placeholders::_2));
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
    gPageManager->regCallInputText(bind(&MainWindow::_inputText, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gPageManager->regCallListBox(bind(&MainWindow::_listBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gPageManager->registerDropButton(bind(&MainWindow::_dropButton, this, std::placeholders::_1));
    gPageManager->regCallbackKeyboard(bind(&MainWindow::_showKeyboard, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gPageManager->regCallbackKeypad(bind(&MainWindow::_showKeypad, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gPageManager->regCallResetKeyboard(bind(&MainWindow::_resetKeyboard, this));
    gPageManager->regCallShowSetup(bind(&MainWindow::_showSetup, this));
    gPageManager->regCallbackResetSurface(bind(&MainWindow::_resetSurface, this));
    gPageManager->regCallbackShutdown(bind(&MainWindow::_shutdown, this));
    gPageManager->regCallbackPlaySound(bind(&MainWindow::_playSound, this, std::placeholders::_1));
    gPageManager->regCallbackStopSound(bind(&MainWindow::_stopSound, this));
    gPageManager->regCallbackMuteSound(bind(&MainWindow::_muteSound, this, std::placeholders::_1));
    gPageManager->regCallbackSetVolume(bind(&MainWindow::_setVolume, this, std::placeholders::_1));
    gPageManager->registerCBsetVisible(bind(&MainWindow::_setVisible, this, std::placeholders::_1, std::placeholders::_2));
    gPageManager->regSendVirtualKeys(bind(&MainWindow::_sendVirtualKeys, this, std::placeholders::_1));
    gPageManager->regShowPhoneDialog(bind(&MainWindow::_showPhoneDialog, this, std::placeholders::_1));
    gPageManager->regSetPhoneNumber(bind(&MainWindow::_setPhoneNumber, this, std::placeholders::_1));
    gPageManager->regSetPhoneStatus(bind(&MainWindow::_setPhoneStatus, this, std::placeholders::_1));
    gPageManager->regSetPhoneState(bind(&MainWindow::_setPhoneState, this, std::placeholders::_1, std::placeholders::_2));
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    gPageManager->regOnOrientationChange(bind(&MainWindow::_orientationChanged, this, std::placeholders::_1));
    gPageManager->regOnSettingsChanged(bind(&MainWindow::_activateSettings, this, std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3,
                                            std::placeholders::_4,
                                            std::placeholders::_5,
                                            std::placeholders::_6));
#endif
    gPageManager->regRepaintWindows(bind(&MainWindow::_repaintWindows, this));
    gPageManager->regToFront(bind(&MainWindow::_toFront, this, std::placeholders::_1));
#if !defined (Q_OS_ANDROID) && !defined(Q_OS_IOS)
    gPageManager->regSetMainWindowSize(bind(&MainWindow::_setSizeMainWindow, this, std::placeholders::_1, std::placeholders::_2));
#endif
    gPageManager->regDownloadSurface(bind(&MainWindow::_downloadSurface, this, std::placeholders::_1, std::placeholders::_2));
    gPageManager->regDisplayMessage(bind(&MainWindow::_displayMessage, this, std::placeholders::_1, std::placeholders::_2));
    gPageManager->regAskPassword(bind(&MainWindow::_askPassword, this, std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      std::placeholders::_4,
                                      std::placeholders::_5));
    gPageManager->regFileDialogFunction(bind(&MainWindow::_fileDialog, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2,
                                             std::placeholders::_3,
                                             std::placeholders::_4));
    gPageManager->regStartWait(bind(&MainWindow::_startWait, this, std::placeholders::_1));
    gPageManager->regStopWait(bind(&MainWindow::_stopWait, this));
    gPageManager->regPageFinished(bind(&MainWindow::_pageFinished, this, std::placeholders::_1));
    gPageManager->regInitializeIntercom(bind(&MainWindow::_initializeIntercom, this, std::placeholders::_1));
    gPageManager->regIntercomStart(bind(&MainWindow::_intercomStart, this));
    gPageManager->regIntercomStop(bind(&MainWindow::_intercomStop, this));
    gPageManager->regIntercomSpkLevel(bind(&MainWindow::_intercomSpkLevel, this, std::placeholders::_1));
    gPageManager->regIntercomMicLevel(bind(&MainWindow::_intercomMicLevel, this, std::placeholders::_1));
    gPageManager->regIntercomMute(bind(&MainWindow::_intercomMicMute, this, std::placeholders::_1));
    gPageManager->deployCallbacks();

    createActions();        // Create the toolbar, if enabled by settings.

#ifndef QT_NO_SESSIONMANAGER
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setFallbackSessionManagementEnabled(false);
    connect(qApp, &QGuiApplication::commitDataRequest,
            this, &MainWindow::writeSettings);
#endif  // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#endif  // QT_NO_SESSIONMANAGER

    // Some types used to transport data from the layer below.
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<QByteArray>("QByteArray");
    qRegisterMetaType<ANIMATION_t>("ANIMATION_t");
    qRegisterMetaType<Button::TButton*>("Button::TButton*");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<SUBVIEWLIST_T>("SUBVIEWLIST_T");
    qRegisterMetaType<PGSUBVIEWITEM_T>("PGSUBVIEWITEM_T");
    qRegisterMetaType<PGSUBVIEWITEM_T>("PGSUBVIEWITEM_T&");
    qRegisterMetaType<PGSUBVIEWATOM_T>("PGSUBVIEWATOM_T");
    qRegisterMetaType<TBitmap>("TBitmap");
    qRegisterMetaType<INTERCOM_t>("INTERCOM_t");

    // All the callback functions doesn't act directly. Instead they emit an
    // event. Then Qt decides whether the real function is started directly and
    // immediately or if the call is queued and called later in a thread. To
    // handle this we're "connecting" the real functions to some signals.
    try
    {
        connect(this, &MainWindow::sigDisplayButton, this, &MainWindow::displayButton);
        connect(this, &MainWindow::sigSetMarqueeText, this, &MainWindow::setMarqueeText);
        connect(this, &MainWindow::sigDisplayViewButton, this, &MainWindow::displayViewButton);
        connect(this, &MainWindow::sigAddViewButtonItems, this, &MainWindow::addViewButtonItems);
        connect(this, &MainWindow::sigShowViewButtonItem, this, &MainWindow::showViewButtonItem);
        connect(this, &MainWindow::sigUpdateViewButton, this, &MainWindow::updateViewButton);
        connect(this, &MainWindow::sigUpdateViewButtonItem, this, &MainWindow::updateViewButtonItem);
        connect(this, &MainWindow::sigHideViewItem, this, &MainWindow::hideViewItem);
        connect(this, &MainWindow::sigHideAllViewItems, this, &MainWindow::hideAllViewItems);
        connect(this, &MainWindow::sigSetSubViewPadding, this, &MainWindow::setSubViewPadding);
        connect(this, &MainWindow::sigSetSubViewAnimation, this, &MainWindow::setSubViewAnimation);
        connect(this, &MainWindow::sigToggleViewButtonItem, this, &MainWindow::toggleViewButtonItem);
        connect(this, &MainWindow::sigSetPage, this, &MainWindow::setPage);
        connect(this, &MainWindow::sigSetSubPage, this, &MainWindow::setSubPage);
        connect(this, &MainWindow::sigSetBackground, this, &MainWindow::setBackground);
        connect(this, &MainWindow::sigMinimizeSubpage, this, &MainWindow::minimizeSubpage);
        connect(this, &MainWindow::sigMaximizeSubpage, this, &MainWindow::maximizeSubpage);
        connect(this, &MainWindow::sigDropPage, this, &MainWindow::dropPage);
        connect(this, &MainWindow::sigDropSubPage, this, &MainWindow::dropSubPage);
        connect(this, &MainWindow::sigPlayVideo, this, &MainWindow::playVideo);
        connect(this, &MainWindow::sigInputText, this, &MainWindow::inputText);
        connect(this, &MainWindow::sigListBox, this, &MainWindow::listBox);
        connect(this, &MainWindow::sigKeyboard, this, &MainWindow::showKeyboard);
        connect(this, &MainWindow::sigKeypad, this, &MainWindow::showKeypad);
        connect(this, &MainWindow::sigShowSetup, this, &MainWindow::showSetup);
        connect(this, &MainWindow::sigPlaySound, this, &MainWindow::playSound);
        connect(this, &MainWindow::sigSetVolume, this, &MainWindow::setVolume);
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
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        connect(this, &MainWindow::sigSetSizeMainWindow, this, &MainWindow::setSizeMainWindow);
#endif
        connect(this, &MainWindow::sigDownloadSurface, this, &MainWindow::downloadSurface);
        connect(this, &MainWindow::sigDisplayMessage, this, &MainWindow::displayMessage);
        connect(this, &MainWindow::sigAskPassword, this, &MainWindow::askPassword);
        connect(this, &MainWindow::sigFileDialog, this, &MainWindow::fileDialog);
        connect(this, &MainWindow::sigStartWait, this, &MainWindow::startWait);
        connect(this, &MainWindow::sigStopWait, this, &MainWindow::stopWait);
        connect(this, &MainWindow::sigPageFinished, this, &MainWindow::pageFinished);
        connect(this, &MainWindow::sigInitializeIntercom, this, &MainWindow::initializeIntercom);
        connect(this, &MainWindow::sigIntercomStart, this, &MainWindow::intercomStart);
        connect(this, &MainWindow::sigIntercomStop, this, &MainWindow::intercomStop);
        connect(this, &MainWindow::sigIntercomSpkLevel, this, &MainWindow::intercomSpkLevel);
        connect(this, &MainWindow::sigIintercomMicLevel, this, &MainWindow::intercomMicLevel);
        connect(this, &MainWindow::sigIntercomMicMute, this, &MainWindow::intercomMicMute);
        connect(qApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::onAppStateChanged);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        QScreen *screen = QGuiApplication::primaryScreen();
        connect(screen, &QScreen::orientationChanged, this, &MainWindow::onScreenOrientationChanged);
        connect(this, &MainWindow::sigActivateSettings, this, &MainWindow::activateSettings);
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
#ifdef Q_OS_ANDROID
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

#ifdef Q_OS_IOS
    // To get the battery level periodicaly we setup a timer.
    if (!mIosBattery)
        mIosBattery = new TIOSBattery;

    mIosBattery->update();

    int left = mIosBattery->getBatteryLeft();
    int state = mIosBattery->getBatteryState();
    // At this point no buttons are registered and therefore the battery
    // state will not be visible. To have the state at the moment a button
    // is registered, we tell the page manager to store the values.
    gPageManager->setBattery(left, state);
    MSG_DEBUG("Battery state was set to " << left << "% and state " << state);
#endif  // Q_OS_IOS

    _restart_ = false;
}

MainWindow::~MainWindow()
{
    DECL_TRACER("MainWindow::~MainWindow()");

    killed = true;
    prg_stopped = true;

    disconnect(this, &MainWindow::sigDisplayButton, this, &MainWindow::displayButton);
    disconnect(this, &MainWindow::sigDisplayViewButton, this, &MainWindow::displayViewButton);
    disconnect(this, &MainWindow::sigAddViewButtonItems, this, &MainWindow::addViewButtonItems);
    disconnect(this, &MainWindow::sigSetPage, this, &MainWindow::setPage);
    disconnect(this, &MainWindow::sigSetSubPage, this, &MainWindow::setSubPage);
    disconnect(this, &MainWindow::sigSetBackground, this, &MainWindow::setBackground);
    disconnect(this, &MainWindow::sigDropPage, this, &MainWindow::dropPage);
    disconnect(this, &MainWindow::sigDropSubPage, this, &MainWindow::dropSubPage);
    disconnect(this, &MainWindow::sigPlayVideo, this, &MainWindow::playVideo);
    disconnect(this, &MainWindow::sigInputText, this, &MainWindow::inputText);
    disconnect(this, &MainWindow::sigListBox, this, &MainWindow::listBox);
    disconnect(this, &MainWindow::sigKeyboard, this, &MainWindow::showKeyboard);
    disconnect(this, &MainWindow::sigKeypad, this, &MainWindow::showKeypad);
    disconnect(this, &MainWindow::sigShowSetup, this, &MainWindow::showSetup);
    disconnect(this, &MainWindow::sigPlaySound, this, &MainWindow::playSound);
    disconnect(this, &MainWindow::sigDropButton, this, &MainWindow::dropButton);
    disconnect(this, &MainWindow::sigSetVisible, this, &MainWindow::SetVisible);
    disconnect(this, &MainWindow::sigSendVirtualKeys, this, &MainWindow::sendVirtualKeys);
    disconnect(this, &MainWindow::sigShowPhoneDialog, this, &MainWindow::showPhoneDialog);
    disconnect(this, &MainWindow::sigSetPhoneNumber, this, &MainWindow::setPhoneNumber);
    disconnect(this, &MainWindow::sigSetPhoneStatus, this, &MainWindow::setPhoneStatus);
    disconnect(this, &MainWindow::sigSetPhoneState, this, &MainWindow::setPhoneState);
    disconnect(this, &MainWindow::sigRepaintWindows, this, &MainWindow::repaintWindows);
    disconnect(this, &MainWindow::sigToFront, this, &MainWindow::toFront);
    disconnect(this, &MainWindow::sigOnProgressChanged, this, &MainWindow::onProgressChanged);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    disconnect(this, &MainWindow::sigSetSizeMainWindow, this, &MainWindow::setSizeMainWindow);
#endif
    disconnect(this, &MainWindow::sigDownloadSurface, this, &MainWindow::downloadSurface);
    disconnect(this, &MainWindow::sigDisplayMessage, this, &MainWindow::displayMessage);
    disconnect(this, &MainWindow::sigFileDialog, this, &MainWindow::fileDialog);
    disconnect(this, &MainWindow::sigStartWait, this, &MainWindow::startWait);
    disconnect(this, &MainWindow::sigStopWait, this, &MainWindow::stopWait);
    disconnect(this, &MainWindow::sigPageFinished, this, &MainWindow::pageFinished);
    disconnect(qApp, &QGuiApplication::applicationStateChanged, this, &MainWindow::onAppStateChanged);

#ifdef Q_OS_IOS
    if (mSource)
    {
        delete mSource;
        mSource = nullptr;
    }
#endif
    if (mMediaPlayer)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
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

    isRunning = false;
#ifdef Q_OS_IOS
    if (mIosRotate)
        mIosRotate->automaticRotation(true);

    if (mIosBattery)
    {
        delete mIosBattery;
        mIosBattery = nullptr;
    }

    if (mIosRotate)
        delete mIosRotate;
#endif

    if (mGestureFilter)
    {
        disconnect(mGestureFilter, &TQGestureFilter::gestureEvent, this, &MainWindow::onGestureEvent);
        delete mGestureFilter;
    }
}

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
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
    onAppStateChanged(state);
}

void MainWindow::_orientationChanged(int orientation)
{
    DECL_TRACER("MainWindow::_orientationChanged(int orientation)");

    if (gPageManager && gPageManager->getSettings()->isPortrait())  // portrait?
    {
        if (orientation == O_REVERSE_PORTRAIT && mOrientation != Qt::InvertedPortraitOrientation)
        {
            _setOrientation((J_ORIENTATION)orientation);
            mOrientation = Qt::InvertedPortraitOrientation;
        }
        else if (orientation == O_PORTRAIT && mOrientation != Qt::PortraitOrientation)
        {
            _setOrientation((J_ORIENTATION)orientation);
            mOrientation = Qt::PortraitOrientation;
        }
    }
    else
    {
        if (orientation == O_REVERSE_LANDSCAPE && mOrientation != Qt::InvertedLandscapeOrientation)
        {
            _setOrientation((J_ORIENTATION)orientation);
            mOrientation = Qt::InvertedLandscapeOrientation;
        }
        else if (orientation == O_LANDSCAPE && mOrientation != Qt::LandscapeOrientation)
        {
            _setOrientation((J_ORIENTATION)orientation);
            mOrientation = Qt::LandscapeOrientation;
        }
    }
}

void MainWindow::_activateSettings(const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce)
{
    DECL_TRACER("MainWindow::_activateSettings(const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce)");

    if (!mHasFocus)
        return;

    emit sigActivateSettings(oldNetlinx, oldPort, oldChannelID, oldSurface, oldToolbarSuppress, oldToolbarForce);
}

/**
 * @brief MainWindow::activateSettings
 * This method activates some urgent settings. It is called on Android and IOS
 * after the setup dialog was closed. The method expects some values taken
 * immediately before the setup dialog was started. If takes some actions like
 * downloading a surface when the setting for it changed or removes the
 * toolbar on the right if the user reuqsted it.
 *
 * @param oldNetlinx        The IP or name of the Netlinx
 * @param oldPort           The network port number used to connect to Netlinx
 * @param oldChannelID      The channel ID TPanel uses to identify against the
 *                          Netlinx.
 * @param oldSurface        The name of theprevious TP4 file.
 * @param oldToolbarSuppress    State of toolbar suppress switch
 * @param oldToolbarForce   The state of toolbar force switch
 */
void MainWindow::activateSettings(const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce)
{
    DECL_TRACER("MainWindow::activateSettings(const std::string& oldNetlinx, int oldPort, int oldChannelID, const std::string& oldSurface, bool oldToolbarSuppress, bool oldToolbarForce)");

#ifdef Q_OS_IOS
    TConfig cf(TConfig::getConfigPath() + "/" + TConfig::getConfigFileName());
#endif
    bool rebootAnyway = false;
    bool doDownload = false;
    string newSurface = TConfig::getFtpSurface();

    if (!TConfig::getToolbarSuppress() && oldToolbarForce != TConfig::getToolbarForce())
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
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (newSurface != oldSurface || TTPInit::haveSystemMarker())
#else
    time_t dlTime = TConfig::getFtpDownloadTime();
    time_t actTime = time(NULL);

    if (newSurface != oldSurface || dlTime == 0 || dlTime < (actTime - 60))
#endif
    {
        MSG_DEBUG("Surface should be downloaded (Old: " << oldSurface << ", New: " << newSurface << ")");

        QMessageBox msgBox(this);
        msgBox.setText(QString("Should the surface <b>") + newSurface.c_str() + "</b> be installed?");
        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        int ret = msgBox.exec();

        if (ret == QMessageBox::Yes)
        {
            doDownload = true;
            TTPInit tpinit;
            std::vector<TTPInit::FILELIST_t> mFileList;
            // Get the list of TP4 files from NetLinx, if there are any.
            TQtWait waitBox(this, string("Please wait while I'm looking at the disc of Netlinx (") + TConfig::getController().c_str() + ") for TP4 files ...");
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
            waitBox.setScaleFactor(mScaleFactor);
            waitBox.doResize();
            waitBox.start();
#endif
            tpinit.setPath(TConfig::getProjectPath());
            tpinit.regCallbackProcessEvents(bind(&MainWindow::runEvents, this));
            tpinit.regCallbackProgressBar(bind(&MainWindow::_onProgressChanged, this, std::placeholders::_1));
            mFileList = tpinit.getFileList(".tp4");
            bool found = false;

            if (mFileList.size() > 0)
            {
                vector<TTPInit::FILELIST_t>::iterator iter;

                for (iter = mFileList.begin(); iter != mFileList.end(); ++iter)
                {
                    if (iter->fname == newSurface)
                    {
                        tpinit.setFileSize(iter->size);
                        found = true;
                        break;
                    }
                }
            }

            waitBox.end();

            if (found)
            {
                string msg = "Loading file <b>" + newSurface + "</b>.";
                MSG_DEBUG("Download of surface " << newSurface << " was forced!");

                downloadBar(msg, this);

                if (tpinit.loadSurfaceFromController(true))
                    rebootAnyway = true;

                mDownloadBar->close();
                mBusy = false;
            }
            else
            {
                MSG_PROTOCOL("The surface " << newSurface << " does not exist on NetLinx or the NetLinx " << TConfig::getController() << " was not found!");
                displayMessage("The surface " + newSurface + " does not exist on NetLinx or the NetLinx " + TConfig::getController() + " was not found!", "Information");
            }
        }
    }

    if (doDownload &&
        (TConfig::getController() != oldNetlinx ||
        TConfig::getChannel() != oldChannelID ||
        TConfig::getPort() != oldPort || rebootAnyway))
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
#ifdef Q_OS_ANDROID
    else
    {
        TConfig cf(TConfig::getConfigPath() + "/" + TConfig::getConfigFileName());
    }
#endif
}

/**
 * @brief MainWindow::_freezeWorkaround: A workaround for the screen freeze.
 * On Mobiles the screen sometimes stay frozen after the application state
 * changes to ACTIVE or some yet unidentified things happened.
 * The workaround produces a faked geometry change which makes the Qt framework
 * to reattach to the screen.
 * There may be situations where this workaround could trigger a repaint of all
 * objects on the screen but then the surface is still frozen. At the moment of
 * writing this comment I have no workaround or even an explanation for this.
 */
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
/**
 * @brief MainWindow::_freezeWorkaround
 * This hack was made from Thomas Andersen. You'll find it at:
 * https://bugreports.qt.io/browse/QTBUG-76142
 */
void MainWindow::_freezeWorkaround()
{
    DECL_TRACER("MainWindow::_freezeWorkaround()");

    QScreen* scr = QGuiApplication::screens().first();
    QPlatformScreen* l_pScr = scr->handle(); /*QAndroidPlatformScreen*/
    QRect l_geomHackAdjustedRect = l_pScr->availableGeometry();
    QRect l_geomHackRect = l_geomHackAdjustedRect;
    l_geomHackAdjustedRect.adjust(0, 0, 0, 5);
    QMetaObject::invokeMethod(dynamic_cast<QObject*>(l_pScr), "setAvailableGeometry", Qt::DirectConnection, Q_ARG( const QRect &, l_geomHackAdjustedRect ));
    QMetaObject::invokeMethod(dynamic_cast<QObject*>(l_pScr), "setAvailableGeometry", Qt::QueuedConnection, Q_ARG( const QRect &, l_geomHackRect ));
}
#endif  // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#endif  // Q_OS_ANDROID || Q_OS_IOS

void MainWindow::_repaintWindows()
{
    DECL_TRACER("MainWindow::_repaintWindows()");

    if (mHasFocus)
        emit sigRepaintWindows();
}

void MainWindow::_toFront(ulong handle)
{
    DECL_TRACER("MainWindow::_toFront(ulong handle)");

    if (mHasFocus)
        emit sigToFront(handle);
}

void MainWindow::_downloadSurface(const string &file, size_t size)
{
    DECL_TRACER("MainWindow::_downloadSurface(const string &file, size_t size)");

    if (mHasFocus)
        emit sigDownloadSurface(file, size);
}

void MainWindow::_startWait(const string& text)
{
    DECL_TRACER("MainWindow::_startWait(const string& text)");

    emit sigStartWait(text);
}

void MainWindow::_stopWait()
{
    DECL_TRACER("MainWindow::_stopWait()");

    emit sigStopWait();
}

void MainWindow::_pageFinished(uint handle)
{
    DECL_TRACER("MainWindow::_pageFinished(uint handle)");

    emit sigPageFinished(handle);
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
 * @brief MainWindow::event Looks for a gesture
 * @param event The event occured
 * @return TRUE if event was accepted
 */
bool MainWindow::event(QEvent* event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent*>(event));

    return QMainWindow::event(event);
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
        QPinchGesture *pg = static_cast<QPinchGesture *>(pinch);
#ifdef QT_DEBUG
        string gs;

        switch(pg->state())
        {
            case Qt::NoGesture:         gs.assign("no gesture"); break;
            case Qt::GestureStarted:    gs.assign("gesture started"); break;
            case Qt::GestureUpdated:    gs.assign("gesture updated"); break;
            case Qt::GestureFinished:   gs.assign("gesture finished"); break;
            case Qt::GestureCanceled:   gs.assign("gesture canceled"); break;
        }

        MSG_DEBUG("PinchGesture state " << gs << " detected");
#endif
        if (pg->state() == Qt::GestureFinished)
        {
            MSG_DEBUG("total scale: " << pg->totalScaleFactor() << ", scale: " << pg->scaleFactor() << ", last scale: " << pg->lastScaleFactor());

            if (pg->totalScaleFactor() > pg->scaleFactor())
                settings();

            return true;
        }
    }
    else if (QGesture *swipe = event->gesture(Qt::SwipeGesture))
    {
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

            return true;
        }
    }

    return false;
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

    if (!gPageManager)
        return;

    if(event->button() == Qt::LeftButton)
    {
        int nx = 0, ny = 0;
#ifdef Q_OS_IOS
        if (mHaveNotchPortrait && gPageManager->getSettings()->isPortrait())
        {
            nx = mNotchPortrait.left();
            ny = mNotchPortrait.top();
        }
        else if (mHaveNotchLandscape && gPageManager->getSettings()->isLandscape())
        {
            nx = mNotchLandscape.left();
            ny = mNotchLandscape.top();
        }
        else
        {
            MSG_WARNING("Have no notch distances!");
        }
#endif
        int x = static_cast<int>(event->position().x()) - nx;
        int y = static_cast<int>(event->position().y()) - ny;
        MSG_DEBUG("Mouse press coordinates: x: " << event->position().x() << ", y: " << event->position().y() << " [new x: " << x << ", y: " << y << " -- \"notch\" nx: " << nx << ", ny: " << ny << "]");

        mLastPressX = x;
        mLastPressY = y;
/*
        QWidget *w = childAt(event->x(), event->y());

        if (w)
        {
            MSG_DEBUG("Object " << w->objectName().toStdString() << " is under mouse cursor.");
            QObject *par = w->parent();

            if (par)
            {
                MSG_DEBUG("The parent is " << par->objectName().toStdString());

                if (par->objectName().startsWith("Item_"))
                {
                    QObject *ppar = par->parent();

                    if (ppar)
                    {
                        MSG_DEBUG("The pparent is " << ppar->objectName().toStdString());

                        if (ppar->objectName().startsWith("View_"))
                        {
                            QMouseEvent *mev = new QMouseEvent(event->type(), event->localPos(), event->globalPos(), event->button(), event->buttons(), event->modifiers());
                            QApplication::postEvent(ppar, mev);
                            return;
                        }
                    }
                }
            }
        }
*/
        if (isScaled())
        {
            x = static_cast<int>(static_cast<double>(x) / mScaleFactor);
            y = static_cast<int>(static_cast<double>(y) / mScaleFactor);
        }

        gPageManager->mouseEvent(x, y, true);
        mTouchStart = std::chrono::steady_clock::now();
        mTouchX = mLastPressX;
        mTouchY = mLastPressY;
        event->accept();
    }
    else if (event->button() == Qt::MiddleButton)
    {
        event->accept();
        settings();
    }
    else
        QMainWindow::mousePressEvent(event);
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
        int nx = 0, ny = 0;
#ifdef Q_OS_IOS
        if (mHaveNotchPortrait && gPageManager->getSettings()->isPortrait())
        {
            nx = mNotchPortrait.left();
            ny = mNotchPortrait.top();
        }
        else if (mHaveNotchLandscape && gPageManager->getSettings()->isLandscape())
        {
            nx = mNotchLandscape.left();
            ny = mNotchLandscape.top();
        }
#endif
        int x = ((mLastPressX >= 0) ? mLastPressX : (static_cast<int>(event->position().x()) - nx));
        int y = ((mLastPressY >= 0) ? mLastPressY : (static_cast<int>(event->position().y()) - ny));
        MSG_DEBUG("Mouse press coordinates: x: " << event->position().x() << ", y: " << event->position().y());
        mLastPressX = mLastPressY = -1;
/*
        QWidget *w = childAt(event->x(), event->y());

        if (w)
        {
            MSG_DEBUG("Object " << w->objectName().toStdString() << " is under mouse cursor.");
            QObject *par = w->parent();

            if (par)
            {
                MSG_DEBUG("The parent is " << par->objectName().toStdString());

                if (par->objectName().startsWith("Item_"))
                {
                    QObject *ppar = par->parent();

                    if (ppar)
                    {
                        MSG_DEBUG("The pparent is " << ppar->objectName().toStdString());

                        if (ppar->objectName().startsWith("View_"))
                        {
                            QMouseEvent *mev = new QMouseEvent(event->type(), event->localPos(), event->globalPos(), event->button(), event->buttons(), event->modifiers());
                            QApplication::postEvent(ppar, mev);
                            return;
                        }
                    }
                }
            }
        }
*/
        if (isScaled())
        {
            x = static_cast<int>(static_cast<double>(x) / mScaleFactor);
            y = static_cast<int>(static_cast<double>(y) / mScaleFactor);
        }

        gPageManager->mouseEvent(x, y, false);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::nanoseconds difftime = end - mTouchStart;
        std::chrono::milliseconds msecs = std::chrono::duration_cast<std::chrono::milliseconds>(difftime);

        if (msecs.count() < 100)    // 1/10 of a second
        {
            MSG_DEBUG("Time was too short: " << msecs.count());
            event->accept();
            return;
        }

        x = static_cast<int>(event->position().x());
        y = static_cast<int>(event->position().y());
        int width = scale(gPageManager->getSettings()->getWidth());
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

        event->accept();
    }
    else
        QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    DECL_TRACER("MainWindow::mouseMoveEvent(QMouseEvent* event)");

    if (!gPageManager)
        return;

    QWidget *w = childAt(event->position().x(), event->position().y());

    if (w)
    {
        MSG_DEBUG("Object " << w->objectName().toStdString() << " is under mouse cursor.");

        gPageManager->mouseMoveEvent(event->position().x(), event->position().y());
        mLastPressX = event->position().x();
        mLastPressY = event->position().y();
    }
/*        QObject *par = w->parent();

        if (par)
        {
            MSG_DEBUG("The parent is " << par->objectName().toStdString());

            if (par->objectName().startsWith("Item_"))
            {
                QObject *ppar = par->parent();

                if (ppar)
                {
                    MSG_DEBUG("The pparent is " << ppar->objectName().toStdString());

                    if (ppar->objectName().startsWith("View_"))
                    {
                        QMouseEvent *mev = new QMouseEvent(event->type(), event->localPos(), event->globalPos(), event->button(), event->buttons(), event->modifiers());
                        QApplication::postEvent(ppar, mev);
                        return;
                    }
                }
            }
        }
    }
*/
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    DECL_TRACER("MainWindow::keyPressEvent(QKeyEvent *event)");

    if (event && event->key() == Qt::Key_Back && !mToolbar)
    {
        QMessageBox msgBox(this);
        msgBox.setText("Select what to do next:");
        msgBox.addButton("Quit", QMessageBox::AcceptRole);
        msgBox.addButton("Setup", QMessageBox::RejectRole);
        msgBox.addButton("Cancel", QMessageBox::ResetRole);
        int ret = msgBox.exec();

        if (ret == QMessageBox::Accepted)   // This is correct! QT seems to change here the buttons.
        {
            showSetup();
            event->accept();
            return;
        }
        else if (ret == QMessageBox::Rejected)  // This is correct! QT seems to change here the buttons.
        {
            event->accept();
            close();
        }
    }
    else
        QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    DECL_TRACER("MainWindow::keyReleaseEvent(QKeyEvent *event)");

    QMainWindow::keyReleaseEvent(event);
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
#if defined(QT_DEBUG) && (defined(Q_OS_IOS) || defined(Q_OS_ANDROID))
    MSG_DEBUG("Orientation changed to " << orientationToString(ori) << " (mOrientation: " << orientationToString(mOrientation) << ")");
#endif
    if (!gPageManager)
        return;

    if (gPageManager->getSettings()->isPortrait())
    {
#ifdef Q_OS_IOS
        if (!mHaveNotchPortrait)
            setNotch();
#endif
        if (ori == Qt::PortraitOrientation || ori == Qt::InvertedPortraitOrientation)
        {
#ifdef Q_OS_IOS
            if (mSensor)
                mSensor->setCurrentOrientation(ori);
#endif
            if (mOrientation == ori)
                return;

            mOrientation = ori;
        }
        else if (mOrientation != Qt::PortraitOrientation && mOrientation != Qt::InvertedPortraitOrientation)
            mOrientation = Qt::PortraitOrientation;
    }
    else
    {
#ifdef Q_OS_IOS
        if (!mHaveNotchLandscape)
            setNotch();
#endif
        if (ori == Qt::LandscapeOrientation || ori == Qt::InvertedLandscapeOrientation)
        {
#ifdef Q_OS_IOS
            if (mSensor)
                mSensor->setCurrentOrientation(ori);
#endif
            if (mOrientation == ori)
                return;

            mOrientation = ori;
        }
        else if (mOrientation != Qt::LandscapeOrientation && mOrientation != Qt::InvertedLandscapeOrientation)
            mOrientation = Qt::LandscapeOrientation;
    }

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
#ifdef Q_OS_IOS
    setNotch();
#endif
}
#ifdef Q_OS_IOS
/**
 * @brief MainWindow::onPositionUpdated
 * This method is a callback function for the Qt framework. It is called
 * whenever the geo position changes. The position information is never really
 * used and is implemented only to keep the application on IOS running in the
 * background.
 *
 * @param update    A structure containing the geo position information.
 */

void MainWindow::onPositionUpdated(const QGeoPositionInfo &update)
{
    DECL_TRACER("MainWindow::onPositionUpdated(const QGeoPositionInfo &update)");

    QGeoCoordinate coord = update.coordinate();
    MSG_DEBUG("Geo location: " << coord.toString().toStdString());
}

void MainWindow::onErrorOccurred(QGeoPositionInfoSource::Error positioningError)
{
    DECL_TRACER("MainWindow::onErrorOccurred(QGeoPositionInfoSource::Error positioningError)");

    switch(positioningError)
    {
        case QGeoPositionInfoSource::AccessError:
            MSG_ERROR("The connection setup to the remote positioning backend failed because the application lacked the required privileges.");
            mGeoHavePermission = false;
        break;

        case QGeoPositionInfoSource::ClosedError:   MSG_ERROR("The remote positioning backend closed the connection, which happens for example in case the user is switching location services to off. As soon as the location service is re-enabled regular updates will resume."); break;
        case QGeoPositionInfoSource::UnknownSourceError: MSG_ERROR("An unidentified error occurred."); break;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        case QGeoPositionInfoSource::UpdateTimeoutError: MSG_ERROR("Current position could not be retrieved within the specified timeout."); break;
#endif
        default:
            return;
    }
}
#endif  // Q_OS_IOS
/**
 * @brief Displays or hides a phone dialog window.
 * This method creates and displays a phone dialog window containing everything
 * a simple phone needs. Depending on the parameter \p state the dialog is
 * created or an existing dialog is closed.
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
#if defined(Q_OS_ANDROID)
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
    QPalette palette(mToolbar->palette());
    palette.setColor(QPalette::Window, QColorConstants::Svg::honeydew);
    mToolbar->setPalette(palette);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mToolbar->setAllowedAreas(Qt::RightToolBarArea);
    mToolbar->setFloatable(false);
    mToolbar->setMovable(false);
#if defined(Q_OS_ANDROID) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (isScaled())
    {
        int width = (int)((double)gPageManager->getSettings()->getWidth() * gScale);
        int tbWidth = (int)(48.0 * gScale);
        int icWidth = (int)(40.0 * gScale);

        if ((gFullWidth - width) < tbWidth && !TConfig::getToolbarForce())
        {
            delete mToolbar;
            mToolbar = nullptr;
            return;
        }

        MSG_DEBUG("Icon size: " << icWidth << "x" << icWidth << ", Toolbar width: " << tbWidth);
        QSize iSize(icWidth, icWidth);
        mToolbar->setIconSize(iSize);
    }
#endif  // QT_VERSION
#if (defined(Q_OS_ANDROID) || defined(Q_OS_IOS)) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (isScaled())
    {
        int panwidth = (int)((double)gPageManager->getSettings()->getWidth() * gScale);
        int toolwidth = mToolbar->width();

        if ((gFullWidth - panwidth) < toolwidth && !TConfig::getToolbarForce())
        {
            delete mToolbar;
            mToolbar = nullptr;
            return;
        }
    }
#endif
#else
    mToolbar->setFloatable(true);
    mToolbar->setMovable(true);
    mToolbar->setAllowedAreas(Qt::RightToolBarArea | Qt::BottomToolBarArea);
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
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
 * @brief MainWindow::settings initiates the configuration dialog.
 */
void MainWindow::settings()
{
    DECL_TRACER("MainWindow::settings()");
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
#ifdef Q_OS_ANDROID
    if (gPageManager)
    {
        gPageManager->showSetup();
        return;
    }
    else    // This "else" should never be executed!
        displayMessage("<b>Fatal error</b>: An internal mandatory class was not initialized!<br>Unable to show setup dialog!", "Fatal error");
#else
    mIOSSettingsActive = true;
    QASettings::openSettings();
#endif
#else
    // Save some old values to decide whether to start over or not.
    string oldHost = TConfig::getController();
    int oldPort = TConfig::getPort();
    int oldChannelID = TConfig::getChannel();
    string oldSurface = TConfig::getFtpSurface();
    bool oldToolbar = TConfig::getToolbarForce();
    bool oldToolbarSuppress = TConfig::getToolbarSuppress();
    // Initialize and open the settings dialog.
    TQtSettings *dlg_settings = new TQtSettings(this);
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
                tpinit.setFileSize(static_cast<off64_t>(dlg_settings->getSelectedFtpFileSize()));
                string msg = "Loading file <b>" + TConfig::getFtpSurface() + "</b>.";
                MSG_DEBUG("Download of surface " << TConfig::getFtpSurface() << " was forced!");

                downloadBar(msg, this);

                if (tpinit.loadSurfaceFromController(true))
                    rebootAnyway = true;

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
#endif  // defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
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

#ifdef Q_OS_IOS
    // On IOS the explicit about dialog is shown over the whole screen with
    // the text in a small stripe on the left. This looks ugly and therefor
    // we construct our own about dialog.
    std::string msg = "About TPanel\n\n";
    msg.append("Implementation of an AMX G4/G5 panel\n");
    msg.append("Version v").append(VERSION_STRING()).append("\n");
    msg.append("(C) Copyright 2020 to 2025 by Andreas Theofilu (andreas@theosys.at)\n");

    QMessageBox about(this);
    about.addButton(QMessageBox::Ok);
    about.setWindowTitle(tr("About TPanel"));
    about.setIconPixmap(QPixmap(":images/icon.png"));
    about.setTextFormat(Qt::PlainText);
    about.setText(tr(msg.c_str()));
    about.setInformativeText(tr("This program is under the terms of GPL version 3!"));
    about.exec();
#else
    std::string msg = "Implementation of an AMX G4/G5 panel\n";
    msg.append("Version v").append(VERSION_STRING()).append("\n");
    msg.append("(C) Copyright 2020 to 2025 by Andreas Theofilu <andreas@theosys.at>\n");
    msg.append("This program is under the terms of GPL version 3!");
    QMessageBox::about(this, tr("About TPanel"), tr(msg.c_str()));
#endif
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

void MainWindow::animationInFinished()
{
    DECL_TRACER("MainWindow::animationInFinished()");

    if (mAnimObjects.empty())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

//    TLOCKER(anim_mutex);
    map<ulong, OBJECT_t *>::iterator iter;

    for (iter = mAnimObjects.begin(); iter != mAnimObjects.end(); ++iter)
    {
        if (!iter->second->animation)
            continue;

        if (!iter->second->invalid && iter->second->type == OBJ_SUBPAGE && iter->second->animation->state() == QAbstractAnimation::Stopped)
        {
            if (iter->second->object.widget)
            {
                iter->second->object.widget->lower();
                iter->second->object.widget->show();
                iter->second->object.widget->raise();
            }

            disconnect(iter->second->animation, &QPropertyAnimation::finished, this, &MainWindow::animationInFinished);
            delete iter->second->animation;
            iter->second->animation = nullptr;
        }
    }

    // Delete all empty/finished animations
    bool repeat = false;

    do
    {
        repeat = false;

        for (iter = mAnimObjects.begin(); iter != mAnimObjects.end(); ++iter)
        {
            if (!iter->second->remove && !iter->second->animation)
            {
                mAnimObjects.erase(iter);
                repeat = true;
                break;
            }
        }
    }
    while (repeat);
#if TESTMODE == 1
    __success = true;
    setScreenDone();
#endif
}

void MainWindow::animationFinished()
{
    DECL_TRACER("MainWindow::animationFinished()");

    if (mAnimObjects.empty())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

//    TLOCKER(anim_mutex);
    map<ulong, OBJECT_t *>::iterator iter;

    for (iter = mAnimObjects.begin(); iter != mAnimObjects.end(); ++iter)
    {
        OBJECT_t *obj = findObject(iter->first);

        if (obj && obj->remove && obj->animation && obj->animation->state() == QAbstractAnimation::Stopped)
        {
            MSG_DEBUG("Invalidating object " << handleToString(iter->first));
            disconnect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            delete obj->animation;
            obj->animation = nullptr;
            invalidateAllSubObjects(iter->first);
            invalidateObject(iter->first);

            if (obj->type == OBJ_SUBPAGE && obj->object.widget)
                obj->object.widget->hide();
        }
    }
    // Delete all empty/finished animations
    bool repeat = false;

    do
    {
        repeat = false;

        for (iter = mAnimObjects.begin(); iter != mAnimObjects.end(); ++iter)
        {
            if (iter->second->remove && !iter->second->animation)
            {
                mAnimObjects.erase(iter);
                repeat = true;
                break;
            }
        }
    }
    while (repeat);
#if TESTMODE == 1
    __success = true;
    setScreenDone();
#endif
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

    TObject::OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object with " << handleToString(handle) << " not found!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (obj->type == TObject::OBJ_SUBPAGE && obj->object.widget)
        obj->object.widget->raise();
#if TESTMODE == 1
    __success = true;
    setScreenDone();
#endif
}

void MainWindow::downloadSurface(const string &file, size_t size)
{
    DECL_TRACER("MainWindow::downloadSurface(const string &file, size_t size)");

    if (mBusy)
        return;

    QMessageBox msgBox(this);
    msgBox.setText(QString("Should the surface <b>") + file.c_str() + "</b> be installed?<br><i><u>Hint</u>: This will also save all current made settings.</i>");
    msgBox.addButton(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes)
    {
        TTPInit tpinit;
        tpinit.regCallbackProcessEvents(bind(&MainWindow::runEvents, this));
        tpinit.regCallbackProgressBar(bind(&MainWindow::_onProgressChanged, this, std::placeholders::_1));
        tpinit.setPath(TConfig::getProjectPath());

        if (size)
            tpinit.setFileSize(static_cast<off64_t>(size));
        else
        {
            size = static_cast<size_t>(tpinit.getFileSize(file));

            if (!size)
            {
                displayMessage("File <b>" + file + "</b> either doesn't exist on " + TConfig::getController() + " or the NetLinx is not reachable!", "Error");
                return;
            }

            tpinit.setFileSize(static_cast<off64_t>(size));
        }

        string msg = "Loading file <b>" + file + "</b>.";

        downloadBar(msg, this);
        bool reboot = false;

        if (tpinit.loadSurfaceFromController(true))
            reboot = true;
        else
            displayMessage("Error downloading file <b>" + file + "</b>!", "Error");

        mDownloadBar->close();
        TConfig::setTemporary(true);
        TConfig::saveSettings();

        if (reboot)
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

    mBusy = false;
}

void MainWindow::displayMessage(const string &msg, const string &title)
{
    DECL_TRACER("MainWindow::displayMessage(const string &msg, const string &title)");

    QMessageBox msgBox(this);
    msgBox.setText(msg.c_str());

    if (!title.empty())
        msgBox.setWindowTitle(title.c_str());

    msgBox.setWindowModality(Qt::WindowModality::ApplicationModal);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::askPassword(ulong handle, const string msg, const string& title, int x, int y)
{
    DECL_TRACER("MainWindow::askPassword(const string msg, const string& title, int x, int y)");

    TQtInputLine *inputLine = new TQtInputLine(this);
    inputLine->setMessage(msg);
    inputLine->setWindowTitle(title.c_str());
    inputLine->setWindowModality(Qt::WindowModality::ApplicationModal);
    inputLine->setPassword(true);
    int bt = inputLine->exec();

    if (bt == QDialog::Rejected)
    {
        if (gPageManager)
            gPageManager->callSetPassword(handle, "", x, y);

        delete inputLine;
        return;
    }

    if (gPageManager)
        gPageManager->callSetPassword(handle, inputLine->getText(), x, y);

    delete inputLine;
}

void MainWindow::fileDialog(ulong handle, const string &path, const std::string& extension, const std::string& suffix)
{
    DECL_TRACER("MainWindow::fileDialog(ulong handle, const string &path, const std::string& extension, const std::string& suffix)");

    std::string pt = path;

    if (fs::exists(path) && fs::is_regular_file(path))
    {
        size_t pos = pt.find_last_of("/");

        if (pos != std::string::npos)
            pt = pt.substr(0, pos);
        else
        {
            char hv0[4096];
            getcwd(hv0, sizeof(hv0));
            pt = hv0;
        }
    }

    QString actPath(pt.c_str());
    QFileDialog fdialog(this, tr("File"), actPath, tr(extension.c_str()));
    fdialog.setAcceptMode(QFileDialog::AcceptSave);

    if (!suffix.empty())
        fdialog.setDefaultSuffix(suffix.c_str());

    fdialog.setOption(QFileDialog::DontConfirmOverwrite);
    QString fname;

    if (fdialog.exec())
    {
        QDir dir = fdialog.directory();
        QStringList list = fdialog.selectedFiles();

        if (list.size() > 0)
            fname = dir.absoluteFilePath(list.at(0));
        else
            return;
    }
    else
        return;

#ifdef Q_OS_ANDROID
    // In case of Android we get some kind of URL instead of a clear
    // path. Because of this we must call some Java API functions to find the
    // real path.
    QString fileName = fname;

    if (fileName.contains("content://"))
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QAndroidJniObject uri = QAndroidJniObject::callStaticObjectMethod(
            "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
            QAndroidJniObject::fromString(fileName).object<jstring>());

        fileName = QAndroidJniObject::callStaticObjectMethod(
            "org/qtproject/theosys/UriToPath", "getFileName",
            "(Landroid/net/Uri;Landroid/content/Context;)Ljava/lang/String;",
            uri.object(), QtAndroid::androidContext().object()).toString();
#else   // QT5_LINUX
        // For QT6 the API has slightly changed. Especialy the class name to
        // call Java objects.
        QJniObject uri = QJniObject::callStaticObjectMethod(
            "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
            QJniObject::fromString(fileName).object<jstring>());

        QJniObject context = QNativeInterface::QAndroidApplication::context();
        fileName = QJniObject::callStaticObjectMethod(
            "org/qtproject/theosys/UriToPath", "getFileName",
            "(Landroid/net/Uri;Landroid/content/Context;)Ljava/lang/String;",
            uri.object(), &context).toString();
#endif  // QT5_LINUX
        if (fileName.length() > 0)
            fname = fileName;
    }
    else
    {
        MSG_WARNING("Not an Uri? (" << fname.toStdString() << ")");
    }
#endif  // Q_OS_ANDROID

    if (gPageManager)
        gPageManager->setTextToButton(handle, fname.toStdString(), true);
}

void MainWindow::onTListCallbackCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    DECL_TRACER("MainWindow::onTListCallbackCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)");

    if (!current || current == previous)
        return;

    QListWidget *w = current->listWidget();
    TObject::OBJECT_t *objWindow = findFirstWindow();

    while(objWindow)
    {
        TObject::OBJECT_t *objItem = findFirstChild(objWindow->handle);

        while (objItem)
        {
            if (objItem->type == TObject::OBJ_LIST && objItem->object.list == w)
            {
                int row = objItem->object.list->currentRow();
                gPageManager->setSelectedRow(objItem->handle, row + 1, current->text().toStdString());
                return;
            }

            objItem = findNextChild(objItem->handle);
        }

        objWindow = findNextWindow(objWindow);
    }
}

void MainWindow::onProgressChanged(int percent)
{
    DECL_TRACER("MainWindow::onProgressChanged(int percent)");

    if (!mDownloadBar || !mBusy)
        return;

    mDownloadBar->setProgress(percent);
}

void MainWindow::startWait(const string& text)
{
    DECL_TRACER("MainWindow::startWait(const string& text)");

    if (mWaitBox)
    {
        mWaitBox->setText(text);
        return;
    }

    mWaitBox = new TQtWait(this, text);
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    mWaitBox->setScaleFactor(mScaleFactor);
    mWaitBox->doResize();
    mWaitBox->start();
#endif
}

void MainWindow::stopWait()
{
    DECL_TRACER("MainWindow::stopWait()");

    if (!mWaitBox)
        return;

    mWaitBox->end();
    delete mWaitBox;
    mWaitBox = nullptr;
}

void MainWindow::pageFinished(ulong handle)
{
    DECL_TRACER("MainWindow::pageFinished(uint handle)");

    TObject::OBJECT_t *obj = findObject(handle);

    if (obj)
    {
        if (obj->type == TObject::OBJ_SUBPAGE && (obj->animate.showEffect == SE_NONE || obj->animate.showTime <= 0) && obj->object.widget)
        {
            if (!obj->object.widget->isEnabled())
                obj->object.widget->setEnabled(true);

            obj->object.widget->show();
            obj->object.widget->lower();
            obj->object.widget->raise();
        }

        if (obj->type == TObject::OBJ_SUBPAGE && obj->animate.showEffect != SE_NONE && obj->object.widget)
        {
            if (startAnimation(obj, obj->animate))
                return;
        }

        if ((obj->type == TObject::OBJ_PAGE || obj->type == TObject::OBJ_SUBPAGE) && obj->object.widget)
        {
            QObjectList list = obj->object.widget->children();
            QObjectList::Iterator iter;

            for (iter = list.begin(); iter != list.end(); ++iter)
            {
                QObject *o = *iter;
                ulong child = extractHandle(o->objectName().toStdString());
                OBJECT_t *obj = nullptr;

                if (child && (obj = findObject(child)) != nullptr)
                {
                    if (obj->invalid && obj->type != OBJ_SUBPAGE)
                        obj->invalid = false;

                    if (obj->remove)
                        obj->remove = false;

                    switch (obj->type)
                    {
                        case OBJ_PAGE:
                        case OBJ_SUBPAGE:
                            if (obj->object.widget && !obj->invalid && obj->object.widget->isHidden())
                            {
                                if (!obj->object.widget->isEnabled())
                                    obj->object.widget->setEnabled(true);

                                obj->object.widget->show();
                                obj->object.widget->lower();
                                obj->object.widget->raise();
                            }
                        break;

                        case OBJ_BUTTON:
                            if (obj->object.label && obj->object.label->isHidden())
                                obj->object.label->show();
                        break;

                        case OBJ_MARQUEE:
                            if (obj->object.marquee && obj->object.marquee->isHidden())
                                obj->object.marquee->show();
                        break;

                        case OBJ_INPUT:
                        case OBJ_TEXT:
                            if (obj->object.plaintext && obj->object.plaintext->isHidden())
                                obj->object.plaintext->show();
                        break;

                        case OBJ_LIST:
                            if (obj->object.list && obj->object.list->isHidden())
                                obj->object.list->show();
                        break;

                        case OBJ_SUBVIEW:
                            if (obj->object.area && obj->object.area->isHidden())
                            {
                                obj->object.area->lower();
                                obj->object.area->show();
                                obj->object.area->raise();
                            }
                        break;

                        case OBJ_VIDEO:
                            if (obj->vwidget && obj->vwidget->isHidden())
                                obj->vwidget->show();
                        break;

                        default:
                            MSG_WARNING("Object " << handleToString(child) << " is an invalid type!");
                    }
                }
                else
                {
                    QString obName = o->objectName();

                    if (obName.startsWith("Label_"))
                    {
                        QLabel *l = dynamic_cast<QLabel *>(o);

                        if (l->isHidden())
                            l->show();
                    }
                }
            }
        }

        if (obj->type == OBJ_SUBVIEW && obj->object.area)
            obj->object.area->show();
#if TESTMODE == 1
        __success = true;
#endif
    }
#ifdef QT_DEBUG
    else
    {
        MSG_WARNING("Object " << handleToString(handle) << " not found!");
    }
#endif
#if TESTMODE == 1
    setScreenDone();
#endif
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
void MainWindow::onAppStateChanged(Qt::ApplicationState state)
{
    DECL_TRACER("MainWindow::onAppStateChanged(Qt::ApplicationState state)");

    switch (state)
    {
        case Qt::ApplicationSuspended:              // Should not occure on a normal desktop
            MSG_INFO("Switched to mode SUSPEND");
            mHasFocus = false;
#ifdef Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
#else
            QJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
#endif
#endif
        break;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)      // On a normal desktop we can ignore this signals
        case Qt::ApplicationInactive:
            MSG_INFO("Switched to mode INACTIVE");
            mHasFocus = false;
            mWasInactive = true;
#ifdef Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
#else
            QJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
#endif
#endif
        break;

        case Qt::ApplicationHidden:
            MSG_INFO("Switched to mode HIDDEN");
            mHasFocus = false;
#ifdef Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
#else
            QJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "pauseOrientationListener", "()V");
#endif
#endif
        break;
#endif
        case Qt::ApplicationActive:
            MSG_INFO("Switched to mode ACTIVE");
            mHasFocus = true;
#ifdef Q_OS_IOS
            initGeoLocation();
#endif
            if (!isRunning && gPageManager)
            {
                // Start the core application
                gPageManager->startUp();
                gPageManager->run();
                isRunning = true;
                mWasInactive = false;
#ifdef Q_OS_IOS
                // To get the battery level periodicaly we setup a timer.
                if (!mIosBattery)
                    mIosBattery = new TIOSBattery;

                mIosBattery->update();

                int left = mIosBattery->getBatteryLeft();
                int stat = mIosBattery->getBatteryState();
                MSG_DEBUG("iOS battery state: " << left << "%, State: " << stat);
                // At this point no buttons are registered and therefore the battery
                // state will not be visible. To have the state at the moment a button
                // is registered, we tell the page manager to store the values.
                gPageManager->setBattery(left, stat);
                gPageManager->informBatteryStatus(left, stat);

                if (mSensor)
                {
                    if (mIosRotate && mOrientation == Qt::PrimaryOrientation) // Unknown?
                    {
                        switch(mIosRotate->getCurrentOrientation())
                        {
                            case O_PORTRAIT:            mOrientation = Qt::PortraitOrientation; break;
                            case O_REVERSE_PORTRAIT:    mOrientation = Qt::InvertedPortraitOrientation; break;
                            case O_REVERSE_LANDSCAPE:   mOrientation = Qt::InvertedLandscapeOrientation; break;
                            case O_LANDSCAPE:           mOrientation = Qt::LandscapeOrientation; break;
                        }
                    }
#if defined(QT_DEBUG) && (defined(Q_OS_IOS) || defined(Q_OS_ANDROID))
                    MSG_DEBUG("Orientation after activate: " << orientationToString(mOrientation));
#endif
                    if (gPageManager && mIosRotate)
                    {
                        if (gPageManager->getSettings()->isPortrait() && mOrientation != Qt::PortraitOrientation)
                        {
                            mIosRotate->rotate(O_PORTRAIT);
                            mOrientation = Qt::PortraitOrientation;
                        }
                        else if (mOrientation != Qt::LandscapeOrientation)
                        {
                            mIosRotate->rotate(O_LANDSCAPE);
                            mOrientation = Qt::LandscapeOrientation;
                        }
                    }

                    setNotch();
                }
#endif
            }
            else
            {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) && defined(Q_OS_ANDROID)
                _freezeWorkaround();
#endif
                if (mDoRepaint || mWasInactive)
                    repaintObjects();

                mDoRepaint = false;
                mWasInactive = false;
            }
#ifdef Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QAndroidJniObject activity = QtAndroid::androidActivity();
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/HideToolbar", "hide", "(Landroid/app/Activity;Z)V", activity.object(), true);
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "resumeOrientationListener", "()V");
#else
            QJniObject activity = QNativeInterface::QAndroidApplication::context();
//            QJniObject::callStaticMethod<void>("org/qtproject/theosys/HideToolbar", "hide", "(Landroid/app/Activity;Z)V", activity.object(), true);
            QJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "resumeOrientationListener", "()V");
#endif
#endif
#ifdef Q_OS_IOS
            // We do this to make sure the battery state is up to date after the
            // screen was reactivated.
            int left = mIosBattery->getBatteryLeft();
            int stat = mIosBattery->getBatteryState();
            gPageManager->informBatteryStatus(left, stat);

            if (mIOSSettingsActive)
            {
                mIOSSettingsActive = false;
                MSG_DEBUG("Activating settings");
                activateSettings(QASettings::getOldNetlinx(),
                                  QASettings::getOldPort(),
                                  QASettings::getOoldChannelID(),
                                  QASettings::getOldSurface(),
                                  QASettings::getOldToolbarSuppress(),
                                  QASettings::getOldToolbarForce());
            }
#endif
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->run();

            _run_test_ready = true;
#endif
        break;
#if not defined(Q_OS_IOS) && not defined(Q_OS_ANDROID)
        default:
            mHasFocus = true;
#endif
    }
#if defined(Q_OS_ANDROID)
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

void MainWindow::_displayButton(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)
{
    DECL_TRACER("MainWindow::_displayButton(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        markDirty(handle);
        return;
    }

    emit sigDisplayButton(handle, parent, buffer, width, height, left, top, passthrough, marqtype, marq);
}

void MainWindow::_setMarqueeText(Button::TButton* button)
{
    DECL_TRACER("MainWindow::_setMarqueeText(Button::TButton* button)");

    if (prg_stopped)
        return;

    emit sigSetMarqueeText(button);
}

void MainWindow::_displayViewButton(ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T fillColor)
{
    DECL_TRACER("MainWindow::_displayViewButton(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        markDirty(handle);
        return;
    }

    emit sigDisplayViewButton(handle, parent, vertical, buffer, width, height, left, top, space, fillColor);
}

void MainWindow::_addViewButtonItems(ulong parent, vector<PGSUBVIEWITEM_T> items)
{
    DECL_TRACER("MainWindow::_addViewButtonItems(ulong parent, vector<PGSUBVIEWITEM_T> items)");

    if (prg_stopped)
        return;

    emit sigAddViewButtonItems(parent, items);
}

void MainWindow::_updateViewButton(ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor)
{
    DECL_TRACER("MainWindow::_updateViewButton(ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor)");

    if (prg_stopped || !mHasFocus)
        return;

    if (!mHasFocus)
    {
        markDirty(handle);
        return;
    }

    emit sigUpdateViewButton(handle, parent, buffer, fillColor);
}

void MainWindow::_updateViewButtonItem(PGSUBVIEWITEM_T& item, ulong parent)
{
    DECL_TRACER("MainWindow::_updateViewButtonItem(PGSUBVIEWITEM_T& item, ulong parent)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigUpdateViewButtonItem(item, parent);
}

void MainWindow::_showViewButtonItem(ulong handle, ulong parent, int position, int timer)
{
    DECL_TRACER("MainWindow::_showViewButtonItem(ulong handle, ulong parent, int position, int timer)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        markDirty(handle);
        return;
    }

    emit sigShowViewButtonItem(handle, parent, position, timer);
}

void MainWindow::_hideAllViewItems(ulong handle)
{
    DECL_TRACER("MainWindow::_hideAllViewItems(ulong handle)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigHideAllViewItems(handle);
}

void MainWindow::_toggleViewButtonItem(ulong handle, ulong parent, int position, int timer)
{
    DECL_TRACER("MainWindow::_toggleViewButtonItem(ulong handle, ulong parent, int position, int timer)");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        markDirty(handle);
        return;
    }

    emit sigToggleViewButtonItem(handle, parent, position, timer);
}

void MainWindow::_hideViewItem(ulong handle, ulong parent)
{
    DECL_TRACER("MainWindow::_hideViewItem(ulong handle, ulong parent)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigHideViewItem(handle, parent);
}

void MainWindow::_setVisible(ulong handle, bool state)
{
    DECL_TRACER("MainWindow::_setVisible(ulong handle, bool state)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigSetVisible(handle, state);
}

void MainWindow::_setSubViewPadding(ulong handle, int padding)
{
    DECL_TRACER("MainWindow::_setSubViewPadding(ulong handle, int padding)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigSetSubViewPadding(handle, padding);
}

void MainWindow::_setPage(ulong handle, int width, int height)
{
    DECL_TRACER("MainWindow::_setPage(ulong handle, int width, int height)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigSetPage(handle, width, height);
}

void MainWindow::_setSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate, bool modal, bool collapsible)
{
    DECL_TRACER("MainWindow::_setSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate, bool modal, bool collapsible)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigSetSubPage(handle, parent, left, top, width, height, animate, modal, collapsible);
}

#ifdef _OPAQUE_SKIA_
void MainWindow::_setBackground(ulong handle, TBitmap image, int width, int height, ulong color)
#else
void MainWindow::_setBackground(ulong handle, TBitmap image, int width, int height, ulong color, int opacity)
#endif
{
    DECL_TRACER("MainWindow::_setBackground(ulong handle, TBitmap image, int width, int height, ulong color [, int opacity])");

    if (prg_stopped)
        return;

    if (!mHasFocus)
    {
        markDirty(handle);
        return;
    }

#ifdef _OPAQUE_SKIA_
    emit sigSetBackground(handle, image, width, height, color);
#else
    emit sigSetBackground(handle, image, width, height, color, opacity);
#endif
}

void MainWindow::_minimizeSubpage(ulong handle)
{
    DECL_TRACER("MainWindow::_minimizeSubpage(ulong handle)");

    emit sigMinimizeSubpage(handle);
}

void MainWindow::_maximizeSubpage(ulong handle)
{
    DECL_TRACER("MainWindow::_maximizeSubpage(ulong handle)");

    emit sigMaximizeSubpage(handle);
}

void MainWindow::_dropPage(ulong handle)
{
    DECL_TRACER("MainWindow::_dropPage(ulong handle)");

    if (!mHasFocus)
        return;

    doReleaseButton();

    if (!mHasFocus)
    {
        markDroped(handle);
        return;
    }

    emit sigDropPage(handle);
}

void MainWindow::_dropSubPage(ulong handle, ulong parent)
{
    DECL_TRACER("MainWindow::_dropSubPage(ulong handle, ulong parent)");

    if (!mHasFocus)
        return;

    doReleaseButton();

    if (!mHasFocus)
    {
        markDroped(handle);
        return;
    }

    emit sigDropSubPage(handle, parent);
}

void MainWindow::_dropButton(ulong handle)
{
    DECL_TRACER("MainWindow::_dropButton(ulong handle)");

    if (!mHasFocus)
    {
        markDroped(handle);
        return;
    }

    emit sigDropButton(handle);
}

void MainWindow::_playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const string& url, const string& user, const string& pw)
{
    DECL_TRACER("MainWindow::_playVideo(ulong handle, const string& url)");

    if (prg_stopped || !mHasFocus)
        return;

    emit sigPlayVideo(handle, parent, left, top, width, height, url, user, pw);
}

void MainWindow::_inputText(Button::TButton *button, Button::BITMAP_t& bm, int frame)
{
    DECL_TRACER("MainWindow::_inputText(Button::TButton *button, Button::BITMAP_t& bm, int frame)");

    if (prg_stopped || !button || !mHasFocus)
        return;

    QByteArray buf;

    if (bm.buffer && bm.rowBytes > 0)
    {
        size_t size = static_cast<size_t>(bm.width) * static_cast<size_t>(bm.height) * static_cast<size_t>(bm.rowBytes / static_cast<size_t>(bm.width));
        buf.insert(0, (const char *)bm.buffer, static_cast<qsizetype>(size));
    }

    emit sigInputText(button, buf, bm.width, bm.height, frame, bm.rowBytes);
}

void MainWindow::_listBox(Button::TButton *button, Button::BITMAP_t& bm, int frame)
{
    DECL_TRACER("MainWindow::_listBox(Button::TButton& button, Button::BITMAP_t& bm, int frame)");

    if (prg_stopped || !mHasFocus)
        return;

    QByteArray buf;

    if (bm.buffer && bm.rowBytes > 0)
    {
        size_t size = static_cast<size_t>(bm.width) * static_cast<size_t>(bm.height) * static_cast<size_t>(bm.rowBytes / static_cast<size_t>(bm.width));
        buf.insert(0, (const char *)bm.buffer, static_cast<qsizetype>(size));
    }

    emit sigListBox(button, buf, bm.width, bm.height, frame, bm.rowBytes);
}

void MainWindow::_showKeyboard(const std::string& init, const std::string& prompt, bool priv)
{
    DECL_TRACER("MainWindow::_showKeyboard(std::string &init, std::string &prompt, bool priv)");

    if (prg_stopped || !mHasFocus)
        return;

    doReleaseButton();
    emit sigKeyboard(init, prompt, priv);
}

void MainWindow::_showKeypad(const std::string& init, const std::string& prompt, bool priv)
{
    DECL_TRACER("MainWindow::_showKeypad(std::string &init, std::string &prompt, bool priv)");

    if (prg_stopped || !mHasFocus)
        return;

    doReleaseButton();
    emit sigKeypad(init, prompt, priv);
}

void MainWindow::_resetKeyboard()
{
    DECL_TRACER("MainWindow::_resetKeyboard()");

    if (mHasFocus)
        emit sigResetKeyboard();
}

void MainWindow::_showSetup()
{
    DECL_TRACER("MainWindow::_showSetup()");

    if (mHasFocus)
        emit sigShowSetup();
}

void MainWindow::_playSound(const string& file)
{
    DECL_TRACER("MainWindow::_playSound(const string& file)");

    if (mHasFocus)
        emit sigPlaySound(file);
}

void MainWindow::_stopSound()
{
    DECL_TRACER("MainWindow::_stopSound()");

    if (mHasFocus)
        emit sigStopSound();
}

void MainWindow::_muteSound(bool state)
{
    DECL_TRACER("MainWindow::_muteSound(bool state)");

    if (mHasFocus)
        emit sigMuteSound(state);
}

void MainWindow::_setVolume(int volume)
{
    DECL_TRACER("MainWindow::_setVolume(int volume)");

    if (mHasFocus)
        emit sigSetVolume(volume);
}

void MainWindow::_setOrientation(J_ORIENTATION ori)
{
#ifdef Q_OS_ANDROID
    DECL_TRACER("MainWindow::_setOriantation(J_ORIENTATION ori)");

    if (ori == O_FACE_UP || ori == O_FACE_DOWN)
        return;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
#else
    QJniObject activity = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
#endif
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
#elif defined(Q_OS_IOS)
    if (mIosRotate)
    {
        mIosRotate->rotate(ori);
#ifdef QT_DEBUG
        string msg;

        switch(ori)
        {
            case O_LANDSCAPE:           msg = "LANDSCAPE"; break;
            case O_PORTRAIT:            msg = "PORTRAIT"; break;
            case O_REVERSE_PORTRAIT:    msg = "INVERTED PORTRAIT"; break;
            case O_REVERSE_LANDSCAPE:   msg = "INVERTED LANDSCAPE"; break;
            default:
                msg = "unknown";
        }

        MSG_DEBUG("Rotated to " << msg);
#endif
    }
#else
    Q_UNUSED(ori);
#endif
}

void MainWindow::_sendVirtualKeys(const string& str)
{
    DECL_TRACER("MainWindow::_sendVirtualKeys(const string& str)");

    if (mHasFocus)
        emit sigSendVirtualKeys(str);
}

void MainWindow::_showPhoneDialog(bool state)
{
    DECL_TRACER("MainWindow::_showPhoneDialog(bool state)");

    if (mHasFocus)
        emit sigShowPhoneDialog(state);
}

void MainWindow::_setPhoneNumber(const std::string& number)
{
    DECL_TRACER("MainWindow::_setPhoneNumber(const std::string& number)");

    if (mHasFocus)
        emit sigSetPhoneNumber(number);
}

void MainWindow::_setPhoneStatus(const std::string& msg)
{
    DECL_TRACER("MainWindow::_setPhoneStatus(const std::string& msg)");

    if (mHasFocus)
        emit sigSetPhoneStatus(msg);
}

void MainWindow::_setPhoneState(int state, int id)
{
    DECL_TRACER("MainWindow::_setPhoneState(int state, int id)");

    if (mHasFocus)
        emit sigSetPhoneState(state, id);
}

void MainWindow::_onProgressChanged(int percent)
{
    DECL_TRACER("MainWindow::_onProgressChanged(int percent)");

    if (mHasFocus)
        emit sigOnProgressChanged(percent);
}

void MainWindow::_displayMessage(const string &msg, const string &title)
{
    DECL_TRACER("MainWindow::_displayMessage(const string &msg, const string &title)");

    if (mHasFocus)
        emit sigDisplayMessage(msg, title);
}

void MainWindow::_askPassword(ulong handle, const string& msg, const string& title, int x, int y)
{
    DECL_TRACER("MainWindow::_askPassword(ulong handle, const string& msg, const string& title, int x int y)");

    if (mHasFocus)
        emit sigAskPassword(handle, msg, title, x, y);
}

void MainWindow::_fileDialog(ulong handle, const string &path, const std::string& extension, const std::string& suffix)
{
    DECL_TRACER("MainWindow::_fileDialog(ulong handle, const string &path, const std::string& extension, const std::string& suffix)");

    if (!handle || path.empty())
    {
        MSG_WARNING("Invalid parameter handle or no path!");
        return;
    }

    emit sigFileDialog(handle, path, extension, suffix);
}

void MainWindow::_setSizeMainWindow(int width, int height)
{
#if !defined (Q_OS_ANDROID) && !defined(Q_OS_IOS)
    DECL_TRACER("MainWindow::_setSizeMainWindow(int width, int height)");

    emit sigSetSizeMainWindow(width, height);
#else
    Q_UNUSED(width);
    Q_UNUSED(height);
#endif
}

void MainWindow::_listViewArea(ulong handle, ulong parent, Button::TButton& button, SUBVIEWLIST_T& list)
{
    DECL_TRACER("MainWindow::_listViewArea(ulong handle, ulong parent, Button::TButton *button, SUBVIEWLIST_T& list)");

    if (!handle || !parent || list.id <= 0)
    {
        MSG_WARNING("Invalid parameters for scroll area!");
        return;
    }

    if (!mHasFocus)
    {
        markDirty(handle);
        return;
    }

    emit sigListViewArea(handle, parent, button, list);
}

void MainWindow::_initializeIntercom(INTERCOM_t ic)
{
    DECL_TRACER("MainWindow::_initializeIntercom(INTERCOM_t ic)");

    emit sigInitializeIntercom(ic);
}

void MainWindow::_intercomStart()
{
    DECL_TRACER("MainWindow::_intercomStart()");

    emit sigIntercomStart();
}

void MainWindow::_intercomStop()
{
    DECL_TRACER("MainWindow::_intercomStop()");

    emit sigIntercomStop();
}

void MainWindow::_intercomSpkLevel(int level)
{
    DECL_TRACER("MainWindow::_intercomSpkLevel(int level)");

    emit sigIntercomSpkLevel(level);
}

void MainWindow::_intercomMicLevel(int level)
{
    DECL_TRACER("MainWindow::_intercomMicLevel(int level)");

    emit sigIintercomMicLevel(level);
}

void MainWindow::_intercomMicMute(bool mute)
{
    DECL_TRACER("MainWindow::_intercomMicMute(bool mute)");

    emit sigIntercomMicMute(mute);
}

void MainWindow::doReleaseButton()
{
    DECL_TRACER("MainWindow::doReleaseButton()");

    if (mLastPressX >= 0 && mLastPressY >= 0 && gPageManager)
    {
        MSG_DEBUG("Sending outstanding mouse release event for coordinates x" << mLastPressX << ", y" << mLastPressY);
        int x = mLastPressX;
        int y = mLastPressY;

        if (isScaled())
        {
            x = static_cast<int>(static_cast<double>(x) / mScaleFactor);
            y = static_cast<int>(static_cast<double>(y) / mScaleFactor);
        }

        gPageManager->mouseEvent(x, y, false);
        mLastPressX = mLastPressY = -1;
    }
}

/**
 * @brief MainWindow::repaintObjects
 * If the application was suspended, which is only on mobile devices possible,
 * the surface can't be drawn. If there was a change on a visible object it
 * was marked "dirty". This methos searches for all dirty marked objects and
 * asks the TPageManager to resend the last drawn graphic of the object. If the
 * object was a page or subpage, the whole page or subpage is redrawn. Otherwise
 * only the changed object.
 */
void MainWindow::repaintObjects()
{
    DECL_TRACER("MainWindow::repaintObjects()");

    if (mRunRedraw)
        return;

    std::thread thr = std::thread([=] {
        mRunRedraw = true;
        TObject::OBJECT_t *obj = getFirstDirty();

        while (obj)
        {
            if (!obj->remove && !obj->invalid && obj->dirty)
            {
                MSG_PROTOCOL("Refreshing widget " << handleToString (obj->handle));

                if (gPageManager)
                    gPageManager->redrawObject(obj->handle);

                obj->dirty = false;
            }

            obj = getNextDirty(obj);
        }

        mRunRedraw = false;
    });

    thr.detach();
}

void MainWindow::refresh(ulong handle)
{
    DECL_TRACER("MainWindow::refresh(ulong handle)");

    if (!handle)
        return;

    OBJECT_t *obj = findFirstChild(handle);

    while (obj)
    {
        MSG_DEBUG("Object " << handleToString(obj->handle) << " of type " << objectToString(obj->type) << ". Invalid: " << (obj->invalid ? "YES" : "NO") << ", Pointer: " << (obj->object.widget ? "YES" : "NO"));

        if (obj->type == OBJ_SUBVIEW && !obj->invalid && obj->object.area)
        {
            obj->object.area->setHidden(true);
            obj->object.area->setHidden(false);
            obj->object.area->setEnabled(true);
            MSG_DEBUG("Subview refreshed");
        }
        else if (obj->type == OBJ_LIST && !obj->invalid && obj->object.list)
        {
            if (!obj->object.list->isEnabled())
                obj->object.list->setEnabled(true);
        }
        else if (obj->type == OBJ_BUTTON && !obj->invalid && obj->object.label)
        {
            if (!obj->object.label->isEnabled())
                obj->object.label->setEnabled(true);
        }
        else if (obj->type == OBJ_MARQUEE && !obj->invalid && obj->object.marquee)
        {
            if (!obj->object.marquee->isEnabled())
                obj->object.marquee->setEnabled(true);
        }
        else if ((obj->type == OBJ_SUBPAGE || obj->type == OBJ_PAGE) && !obj->invalid && obj->object.widget)
        {
            if (!obj->object.widget->isEnabled())
                obj->object.widget->setEnabled(true);
        }

        obj = findNextChild(obj->handle);
    }
}

void MainWindow::markDirty(ulong handle)
{
    DECL_TRACER("MainWindow::markDirty(ulong handle)");

    OBJECT_t *obj = findObject(handle);

    if (!obj)
        return;

    MSG_DEBUG("Object " << handleToString(handle) << " marked dirty.");
    obj->dirty = true;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
int MainWindow::calcVolume(int value)
#else
double MainWindow::calcVolume(int value)
#endif
{
    DECL_TRACER("MainWindow::calcVolume(int value)");

    // volumeSliderValue is in the range [0..100]
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qreal linearVolume = QAudio::convertVolume(value / qreal(100.0),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    return qRound(linearVolume * 100);
#else
    return static_cast<double>(value) / 100.0;
#endif
}

QFont MainWindow::loadFont(int number, const FONT_T& f, const FONT_STYLE style)
{
    DECL_TRACER("MainWindow::loadFont(int number, const FONT_t& f, const FONT_STYLE style)");

    QString path;
    string prjPath = TConfig::getProjectPath();

    if (number < 32)    // System font?
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        path.append(prjPath).append("/__system/graphics/fonts/").append(f.file);
#else
        path.append(prjPath.c_str()).append("/__system/graphics/fonts/").append(f.file.c_str());
#endif

        if (!fs::is_regular_file(path.toStdString()))
        {
            MSG_WARNING("Seem to miss system fonts ...");
            path.clear();
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            path.append(prjPath).append("/fonts/").append(f.file);
#else
            path.append(prjPath.c_str()).append("/fonts/").append(f.file.c_str());
#endif
        }
    }
    else
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        path.append(prjPath).append("/fonts/").append(f.file);
#else
        path.append(prjPath.c_str()).append("/fonts/").append(f.file.c_str());
#endif
        if (!fs::exists(path.toStdString()))
        {
            string pth = prjPath + "/__system/fonts/" + f.file;

            if (fs::exists(pth))
            {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                path.assign(pth);
#else
                path = pth.c_str();
#endif
            }
        }
    }

    const QStringList ffamilies = QFontDatabase::families();
    bool haveFont = false;
    QString fname = QString::fromStdString(f.name);

    for (const QString &family : ffamilies)
    {
        if (family == fname)
        {
            haveFont = true;
            break;
        }
    }

    // Scale the font size
    int pix = f.size;

    if (mScaleFactor > 0.0 && mScaleFactor != 1.0)
        pix = static_cast<int>(static_cast<double>(f.size) / mScaleFactor);

    QString qstyle;
    QFont font;

    switch (style)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        case FONT_BOLD:         qstyle.assign("Bold"); break;
        case FONT_ITALIC:       qstyle.assign("Italic"); break;
        case FONT_BOLD_ITALIC:  qstyle.assign("Bold Italic"); break;

        default:
            qstyle.assign("Normal");
#else
        case FONT_BOLD:         qstyle = "Bold"; break;
        case FONT_ITALIC:       qstyle = "Italic"; break;
        case FONT_BOLD_ITALIC:  qstyle = "Bold Italic"; break;

        default:
            qstyle = "Normal";
#endif
    }

    if (!haveFont)  // Did we found the font?
    {               // No, then load it
        QFontDatabase::addApplicationFont(path);
        font = QFontDatabase::font(fname, qstyle, pix);
        MSG_DEBUG("Font \"" << path.toStdString() << "\" was loaded");
    }
    else
    {
        font.setFamily(fname);
        font.setPointSize(pix);
        font.setStyleName(qstyle);
    }

    string family = font.family().toStdString();

    if (!font.exactMatch() && (family != f.name || font.styleName() != qstyle || font.pointSize() != pix))
    {
        MSG_WARNING("Using font "
                    << family << "|" << font.styleName().toStdString() << "|" << font.pointSize()
                    << " but requested font "
                    << f.name << "|" << qstyle.toStdString() << "|" << pix << "!");
    }
    else
    {
        MSG_DEBUG("Font was set to " << f.name << "|" << qstyle.toStdString() << "|" << pix << "! " << (font.exactMatch() ? "[original]" : "[replacement]"));
    }

    return font;
}

/**
 * @brief MainWindow::convertMask
 * Converts the AMX mask for input lines into Qt mask sympols for input lines.
 *
 * @param mask  A string containing the AMX mask symbols.
 *
 * @return The converted mask string for Qt input lines.
 */
string MainWindow::convertMask(const string& mask)
{
    DECL_TRACER("MainWindow::convertMask(const string& mask)");

    string qMask;

    for (size_t i = 0; i < mask.length(); ++i)
    {
        switch (mask[i])
        {
            case '0': qMask += "9"; break;
            case '9': qMask += "0"; break;
            case 'A': qMask += "N"; break;
            case 'a': qMask += "n"; break;
            case 'L': qMask += "X"; break;
            case '?': qMask += "x"; break;
            case '&': qMask += "A"; break;
            case 'C': qMask += "a"; break;
            case '^': qMask += ";"; break;

            default:
                qMask += mask[i];
        }
    }

    return qMask;
}

#ifdef Q_OS_ANDROID
void MainWindow::hideAndroidBars()
{
    DECL_TRACER("MainWindow::hideAndroidBars()");
}
#endif
#ifdef Q_OS_IOS
void MainWindow::setNotch()
{
    DECL_TRACER("MainWindow::setNotch()");

    Qt::ScreenOrientation so = getRealOrientation();

    if (so == Qt::PrimaryOrientation)
        return;

    QMargins margins;

    if (mHaveNotchPortrait && (so == Qt::PortraitOrientation || so == Qt::InvertedPortraitOrientation))
        margins = mNotchPortrait;
    else if (mHaveNotchLandscape && (so == Qt::LandscapeOrientation || so == Qt::InvertedLandscapeOrientation))
        margins = mNotchLandscape;
    else
    {
        margins = QASettings::getNotchSize();

        if (gPageManager && gPageManager->getSettings()->isPortrait())
        {
            if (so == Qt::LandscapeOrientation)
            {
                int left = margins.left();
                int top = margins.top();
                margins.setTop(margins.right());
                margins.setLeft(top);
                margins.setRight(margins.bottom());
                margins.setBottom(left);
            }
            else if (so == Qt::InvertedLandscapeOrientation)
            {
                int right = margins.right();
                int top = margins.top();
                margins.setTop(margins.left());
                margins.setLeft(top);
                margins.setRight(margins.bottom());
                margins.setBottom(right);
            }
        }
        else if (gPageManager && gPageManager->getSettings()->isLandscape())
        {
            if (so == Qt::PortraitOrientation)
            {
                int top = margins.top();
                int right = margins.right();
                margins.setTop(margins.left());
                margins.setLeft(top);
                margins.setRight(margins.bottom());
                margins.setBottom(right);
            }
            else if (so == Qt::InvertedPortraitOrientation)
            {
                int top = margins.top();
                int left = margins.left();
                margins.setTop(margins.right());
                margins.setLeft(margins.bottom());
                margins.setRight(top);
                margins.setBottom(left);
            }
        }
    }
#if defined(QT_DEBUG) && (defined(Q_OS_IOS) || defined(Q_OS_ANDROID))
    MSG_DEBUG("Notch top: " << margins.top() << ", bottom: " << margins.bottom() << ", left: " << margins.left() << ", right: " << margins.right() << ", Orientation real: " << orientationToString(so) << ", estimated: " << orientationToString(mOrientation));
#endif
    if (gPageManager)
    {
        // If the real orientation "so" differs from "mOrientation" then
        // "mOrientation" contains the wanted orientation and not the real one!
        if (gPageManager->getSettings()->isPortrait() &&
            (mOrientation == Qt::PortraitOrientation || mOrientation == Qt::InvertedPortraitOrientation))
        {
            mNotchPortrait = margins;
            mHaveNotchPortrait = true;
        }
        else if (gPageManager->getSettings()->isLandscape() &&
            (mOrientation == Qt::LandscapeOrientation || mOrientation == Qt::InvertedLandscapeOrientation))
        {
            mNotchLandscape = margins;
            mHaveNotchLandscape = true;
        }
    }
}

/**
 * @brief MainWindow::initGeoLocation
 * This method is only used on IOS to let the application run in the background.
 * It is necessary because it is the only way to let an application run in
 * the background.
 * The method initializes the geo position module of Qt. If the app doesn't
 * have the permissions to retrieve the geo positions, it will not run in the
 * background. It stops at the moment the app is not in front or the display is
 * closed. This makes it stop communicate with the NetLinx and looses the
 * network connection. When the app gets the focus again, it must reconnect to
 * the NetLinx.
 */
void MainWindow::initGeoLocation()
{
    DECL_TRACER("MainWindow::initGeoLocation()");

    if (mSource && mGeoHavePermission)
        return;

    if (!mSource)
    {
        mGeoHavePermission = true;
        mSource = QGeoPositionInfoSource::createDefaultSource(this);

        if (!mSource)
        {
            MSG_WARNING("Error creating geo positioning source!");
            mGeoHavePermission = false;
            return;
        }

        mSource->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
        mSource->setUpdateInterval(800);    // milli seconds
        // Connecting some callbacks to the class
        connect(mSource, &QGeoPositionInfoSource::positionUpdated, this, &MainWindow::onPositionUpdated);
        connect(mSource, &QGeoPositionInfoSource::errorOccurred, this, &MainWindow::onErrorOccurred);
#ifdef Q_OS_IOS
        QLocationPermission perm;
        perm.setAccuracy(QLocationPermission::Approximate);
        perm.setAvailability(QLocationPermission::Always);
        mGeoHavePermission = false;

        switch (qApp->checkPermission(perm))
        {
            case Qt::PermissionStatus::Undetermined:
                qApp->requestPermission(perm, [this] (const QPermission& permission)
                {
                    if (permission.status() == Qt::PermissionStatus::Granted)
                    {
                        mGeoHavePermission = true;
                        mSource->startUpdates();
                    }
                    else
                        onErrorOccurred(QGeoPositionInfoSource::AccessError);
                });
            break;

            case Qt::PermissionStatus::Denied:
                MSG_WARNING("Location permission is denied");
                onErrorOccurred(QGeoPositionInfoSource::AccessError);
            break;

            case Qt::PermissionStatus::Granted:
                mSource->startUpdates();
                mGeoHavePermission = true;
            break;
        }
#endif
    }
}

Qt::ScreenOrientation MainWindow::getRealOrientation()
{
    DECL_TRACER("MainWindow::getRealOrientation()");

    QScreen *screen = QGuiApplication::primaryScreen();

    if (!screen)
    {
        MSG_ERROR("Couldn't determine the primary screen!")
        return Qt::PrimaryOrientation;
    }

    QRect rect = screen->availableGeometry();

    if (rect.width() > rect.height())
        return Qt::LandscapeOrientation;

    return Qt::PortraitOrientation;
}
#endif  // Q_OS_IOS
#if defined(QT_DEBUG) && (defined(Q_OS_IOS) || defined(Q_OS_ANDROID))
string MainWindow::orientationToString(Qt::ScreenOrientation ori)
{
    string sori;

    switch(ori)
    {
        case Qt::PortraitOrientation:           sori = "PORTRAIT"; break;
        case Qt::InvertedPortraitOrientation:   sori = "INVERTED PORTRAIT"; break;
        case Qt::LandscapeOrientation:          sori = "LANDSCAPE"; break;
        case Qt::InvertedLandscapeOrientation:  sori = "INVERTED LANDSCAPE"; break;
        default:
            sori = "Unknown: ";
            sori.append(intToString(ori));
    }

    return sori;
}
#endif

/******************* Draw elements *************************/

/**
 * @brief Displays an image.
 * The method is a callback function and is called whenever an image should be
 * displayed. It defines a label, set it to the (scaled) \b width and \b height
 * and moves it to the (scaled) position \b left and \b top.
 *
 * @param handle    The unique handle of the object
 * @param parent    The unique handle of the parent object.
 * @param buffer    A byte array containing the image.
 * @param width     The width of the object
 * @param height    The height of the object
 * @param pixline   The number of pixels in one line of the image.
 * @param left      The prosition from the left.
 * @param top       The position from the top.
 * @param marqtype  The type of marquee line
 * @param marq      Enabled/disabled marquee
 */
void MainWindow::displayButton(ulong handle, ulong parent, TBitmap buffer, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)
{
    DECL_TRACER("MainWindow::displayButton(ulong handle, TBitmap buffer, size_t size, int width, int height, int left, int top, bool passthrough, int marqtype, int marq)");

    TObject::OBJECT_t *obj = findObject(handle);
    TObject::OBJECT_t *par = findObject(parent);
    MSG_TRACE("Processing button " << handleToString(handle) << " from parent " << handleToString(parent));

    if (!par)
    {
        if (TStreamError::checkFilter(HLOG_DEBUG))
            MSG_WARNING("Button " << handleToString(handle) << " has no parent (" << handleToString(parent) << ")! Ignoring it.");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (par->animation && !par->aniDirection)
    {
        if (par->animation->state() == QAbstractAnimation::Running)
        {
            MSG_WARNING("Object " << handleToString(parent) << " is busy with an animation!");
            par->animation->stop();
        }
        else
        {
            MSG_WARNING("Object " << handleToString(parent) << " has not finished the animation!");
        }

#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }
    else if (par->remove)
    {
        MSG_WARNING("Object " << handleToString(parent) << " is marked for remove. Will not draw image!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!obj)
    {
        if (!par->object.widget)
        {
            MSG_ERROR("Object " << handleToString(parent) << " has no valid widget!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }

        MSG_DEBUG("Adding new object " << handleToString(handle) << " ...");
        OBJECT_t nobj;

        if (marqtype > 0 && marq)
            nobj.type = OBJ_MARQUEE;
        else
            nobj.type = OBJ_BUTTON;

        nobj.handle = handle;

        nobj.width = scale(width);
        nobj.height = scale(height);
        nobj.left = scale(left);
        nobj.top = scale(top);

        if (nobj.type == OBJ_MARQUEE)
        {
            nobj.object.marquee = new TQMarquee(par->object.widget, 1, static_cast<TQMarquee::MQ_TYPES>(marqtype));
            nobj.object.marquee->setObjectName(QString("Marquee_") + handleToString(handle).c_str());

            if (mGestureFilter)
            {
                nobj.object.marquee->installEventFilter(mGestureFilter);
                nobj.object.marquee->grabGesture(Qt::PinchGesture);
                nobj.object.marquee->grabGesture(Qt::SwipeGesture);
            }

            nobj.object.marquee->setGeometry(nobj.left, nobj.top, nobj.width, nobj.height);

            if (passthrough)
                nobj.object.marquee->setAttribute(Qt::WA_TransparentForMouseEvents);
        }
        else
        {
            nobj.object.label = new QLabel(par->object.widget);
            nobj.object.label->setObjectName(QString("Label_") + handleToString(handle).c_str());

            if (mGestureFilter)
            {
                nobj.object.label->installEventFilter(mGestureFilter);
                nobj.object.label->grabGesture(Qt::PinchGesture);
                nobj.object.label->grabGesture(Qt::SwipeGesture);
            }

            nobj.object.label->setGeometry(nobj.left, nobj.top, nobj.width, nobj.height);

            if (passthrough)
                nobj.object.label->setAttribute(Qt::WA_TransparentForMouseEvents);
        }

        if (!addObject(nobj))
        {
            MSG_ERROR("Unable to add the new object " << handleToString(handle) << "!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }

        obj = findObject(handle);
    }
    else
    {
        MSG_DEBUG("Object " << handleToString(handle) << " of type " << TObject::objectToString(obj->type) << " found!");

        if (passthrough && obj->object.label)   // Because it's a union we can test on any of the pointer types
        {
            if (obj->type == OBJ_BUTTON)
                obj->object.label->setAttribute(Qt::WA_TransparentForMouseEvents);
            else if (obj->type == OBJ_MARQUEE)
                obj->object.marquee->setAttribute(Qt::WA_TransparentForMouseEvents);
        }

        if (!enableObject(handle))
        {
            MSG_ERROR("Object " << handleToString(handle) << " of type " << objectToString(obj->type) << " couldn't be enabled!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }

        // In case the dimensions or position has changed we calculate the
        // position and size again.
        int wt = scale(width);
        int ht = scale(height);
        int lt = scale(left);
        int tp = scale(top);

        if (obj->type != OBJ_INPUT && (wt != obj->width || ht != obj->height || lt != obj->left || tp != obj->top))
        {
            MSG_DEBUG("Scaled button with new size: lt: " << obj->left << ", tp: " << obj->top << ", wt: " << obj->width << ", ht: " << obj->height);

            if (obj->type == OBJ_MARQUEE)
                obj->object.marquee->setGeometry(lt, tp, wt, ht);
            else
                obj->object.label->setGeometry(lt, tp, wt, ht);

            obj->left = lt;
            obj->top = tp;
            obj->width = wt;
            obj->height = ht;
        }
    }

    if (obj->type != OBJ_INPUT)
    {
        try
        {
            if (buffer.getSize() > 0 && buffer.getPixline() > 0)
            {
                MSG_DEBUG("Setting image for " << handleToString(handle) << " ...");
                QPixmap pixmap = scaleImage(static_cast<unsigned char *>(buffer.getBitmap()), buffer.getWidth(), buffer.getHeight(), buffer.getPixline());

                if (obj->type == OBJ_MARQUEE && obj->object.marquee)
                {
                    obj->object.marquee->setBackground(pixmap);
#if TESTMODE == 1
                    __success = true;
#endif
                }
                else if (obj->object.label)
                {
                    obj->object.label->setPixmap(pixmap);
#if TESTMODE == 1
                    __success = true;
#endif
                }
                else
                {
                    MSG_WARNING("Object " << handleToString(handle) << " does not exist any more!");
                }
            }
        }
        catch(std::exception& e)
        {
            MSG_ERROR("Error drawing button " << handleToString(handle) << ": " << e.what());
        }
        catch(...)
        {
            MSG_ERROR("Unexpected exception occured [MainWindow::displayButton()]");
        }
    }
#if TESTMODE == 1
    setScreenDone();
#endif
}

void MainWindow::setMarqueeText(Button::TButton* button)
{
    DECL_TRACER("MainWindow::setMarqueeText(Button::TButton* button)");

    ulong handle = button->getHandle();
    TObject::OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_WARNING("No object " << handleToString(handle) << " found!");
        return;
    }

    if (obj->type != OBJ_MARQUEE || !obj->object.marquee)
    {
        MSG_WARNING("Object " << handleToString(handle) << " is not a Marquee type or does not exist!");
        return;
    }

    TQMarquee *marquee = obj->object.marquee;
    Button::ORIENTATION to = static_cast<Button::ORIENTATION>(button->getTextJustification(nullptr, nullptr, button->getActiveInstance()));

    switch(to)
    {
        case Button::ORI_TOP_LEFT:          marquee->setAlignment(Qt::AlignTop | Qt::AlignLeft); break;
        case Button::ORI_TOP_MIDDLE:        marquee->setAlignment(Qt::AlignTop | Qt::AlignHCenter); break;
        case Button::ORI_TOP_RIGHT:         marquee->setAlignment(Qt::AlignTop | Qt::AlignRight); break;

        case Button::ORI_CENTER_LEFT:       marquee->setAlignment(Qt::AlignHCenter | Qt::AlignLeft); break;
        case Button::ORI_CENTER_MIDDLE:     marquee->setAlignment(Qt::AlignCenter); break;
        case Button::ORI_CENTER_RIGHT:      marquee->setAlignment(Qt::AlignHCenter | Qt::AlignRight); break;

        case Button::ORI_BOTTOM_LEFT:       marquee->setAlignment(Qt::AlignBottom | Qt::AlignLeft); break;
        case Button::ORI_BOTTOM_MIDDLE:     marquee->setAlignment(Qt::AlignBottom | Qt::AlignHCenter); break;
        case Button::ORI_BOTTOM_RIGHT:      marquee->setAlignment(Qt::AlignBottom | Qt::AlignRight); break;

        default:
            marquee->setAlignment(Qt::AlignCenter);
    }

    marquee->setText(button->getText().c_str());
    marquee->setSpeed(button->getMarqueeSpeed(button->getActiveInstance()));
    int frameSize = scale(button->getBorderSize(button->getBorderStyle(button->getActiveInstance())));
    marquee->setFrame(frameSize, frameSize, frameSize, frameSize);
    QFont font = loadFont(button->getFontIndex(button->getActiveInstance()), button->getFont(), button->getFontStyle());
    marquee->setFont(font);
}

void MainWindow::displayViewButton(ulong handle, ulong parent, bool vertical, TBitmap buffer, int width, int height, int left, int top, int space, TColor::COLOR_T fillColor)
{
    DECL_TRACER("MainWindow::displayViewButton(ulong handle, TBitmap buffer, size_t size, int width, int height, int left, int top)");

    TObject::OBJECT_t *obj = findObject(handle);
    TObject::OBJECT_t *par = findObject(parent);
    MSG_TRACE("Processing button " << handleToString(handle) << " from parent " << handleToString(parent));

    if (!par)
    {
        if (TStreamError::checkFilter(HLOG_DEBUG))
            MSG_WARNING("Button " << handleToString(handle) << " has no parent (" << handleToString(parent) << ")! Ignoring it.");

#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (par->animation && !par->aniDirection)
    {
        if (par->animation->state() == QAbstractAnimation::Running)
        {
            MSG_WARNING("Object " << handleToString(parent) << " is busy with an animation!");
            par->animation->stop();
        }
        else
        {
            MSG_WARNING("Object " << handleToString(parent) << " has not finished the animation!");
        }

#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }
    else if (par->remove)
    {
        MSG_WARNING("Object " << handleToString(parent) << " is marked for remove. Will not draw image!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!obj)
    {
        if (!par->object.widget)
        {
#if TESTMODE == 1
            MSG_ERROR("Object " << handleToString(parent) << " has no valid object!");
#endif
            return;
        }

        MSG_DEBUG("Adding new object " << handleToString(handle) << " ...");
        OBJECT_t nobj;

        nobj.type = OBJ_SUBVIEW;
        nobj.handle = handle;

        nobj.width = scale(width);
        nobj.height = scale(height);
        nobj.left = scale(left);
        nobj.top = scale(top);

        nobj.object.area = new TQScrollArea(par->object.widget, nobj.width, nobj.height, vertical);
        nobj.object.area->setObjectName(QString("View_") + handleToString(handle).c_str());
        nobj.object.area->setScaleFactor(mScaleFactor);
        nobj.object.area->setSpace(space);
        nobj.object.area->move(nobj.left, nobj.top);
        nobj.connected = true;
        connect(nobj.object.area, &TQScrollArea::objectClicked, this, &MainWindow::onSubViewItemClicked);

        if (!addObject(nobj))
        {
            MSG_ERROR("Couldn't add the object " << handleToString(handle) << "!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }

        obj = findObject(handle);
    }
    else if (obj->type != OBJ_SUBVIEW)
    {
        MSG_ERROR("Object " << handleToString(handle) << " is of wrong type " << objectToString(obj->type) << "!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }
    else
    {
        MSG_DEBUG("Object " << handleToString(handle) << " of type " << TObject::objectToString(obj->type) << " found!");

        if (obj->object.area && !obj->connected)
            reconnectArea(obj->object.area);
    }

    try
    {
        // Set background color
        if (obj->object.area)
        {
            QColor color;

            if (fillColor.alpha == 0)
                color = Qt::transparent;
            else
                color = qRgba(fillColor.red, fillColor.green, fillColor.blue, fillColor.alpha);

            obj->object.area->setBackGroundColor(color);
        }

        if (buffer.getSize() > 0 && buffer.getPixline() > 0)
        {
            MSG_DEBUG("Setting image for " << handleToString(handle) << " ...");
            QPixmap pixmap = scaleImage(static_cast<unsigned char *>(buffer.getBitmap()), buffer.getWidth(), buffer.getHeight(), buffer.getPixline());

            if (pixmap.isNull())
            {
                MSG_ERROR("Unable to create a pixmap out of an image!");
#if TESTMODE == 1
                setScreenDone();
#endif
                return;
            }

            if (obj->object.area)
            {
                obj->object.area->setBackgroundImage(pixmap);
            }
            else
            {
                MSG_WARNING("Object " << handleToString(handle) << " does not exist any more!");
            }
        }
    }
    catch(std::exception& e)
    {
        MSG_ERROR("Error drawing button " << handleToString(handle) << ": " << e.what());
    }
    catch(...)
    {
        MSG_ERROR("Unexpected exception occured [MainWindow::displayViewButton()]");
    }
}

void MainWindow::addViewButtonItems(ulong parent, vector<PGSUBVIEWITEM_T> items)
{
    DECL_TRACER("MainWindow::addViewButtonItems(ulong parent, vector<PGSUBVIEWITEM_T> items)");

    if (items.empty())
        return;

    OBJECT_t *par = findObject(parent);

    if (!par || par->type != OBJ_SUBVIEW || !par->object.area)
    {
        MSG_ERROR("No object with handle " << handleToString(parent) << " found or object is not a subview list!");
        return;
    }

    if (par->invalid)
    {
        if (!enableObject(parent))
        {
            MSG_ERROR("Object " << handleToString(parent) << " of type " << objectToString(par->type) << " couldn't be enabled!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }
    }

    if (!items.empty())
    {
        par->object.area->setScrollbar(items[0].scrollbar);
        par->object.area->setScrollbarOffset(items[0].scrollbarOffset);
        par->object.area->setAnchor(items[0].position);
    }

    par->object.area->addItems(items);
}

void MainWindow::updateViewButton(ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor)
{
    DECL_TRACER("MainWindow::updateViewButton(ulong handle, ulong parent, TBitmap buffer, TColor::COLOR_T fillColor)");

    OBJECT_t *par = findObject(parent);

    if (!par || par->type != OBJ_SUBVIEW || !par->object.area)
    {
        MSG_ERROR("No object with handle " << handleToString(parent) << " found for update or object is not a subview list!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    PGSUBVIEWITEM_T item;
    item.handle = handle;
    item.parent = parent;
    item.image = buffer;
    item.bgcolor = fillColor;
    par->object.area->updateItem(item);
}

void MainWindow::updateViewButtonItem(PGSUBVIEWITEM_T& item, ulong parent)
{
    DECL_TRACER("MainWindow::updateViewButtonItem(PGSUBVIEWITEM_T& item, ulong parent)");

    OBJECT_t *par = findObject(parent);

    if (!par || par->type != OBJ_SUBVIEW || !par->object.area)
    {
        MSG_ERROR("No object with handle " << handleToString(parent) << " found for update or object is not a subview list!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    par->object.area->updateItem(item);
}

void MainWindow::showViewButtonItem(ulong handle, ulong parent, int position, int timer)
{
    DECL_TRACER("MainWindow::showViewButtonItem(ulong handle, ulong parent, int position, int timer)");

    Q_UNUSED(timer);

    OBJECT_t *par = findObject(parent);

    if (!par || par->type != OBJ_SUBVIEW || !par->object.area)
    {
        MSG_ERROR("No object with handle " << handleToString(parent) << " found for update or object is not a subview list!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    par->object.area->showItem(handle, position);
}

void MainWindow::toggleViewButtonItem(ulong handle, ulong parent, int position, int timer)
{
    DECL_TRACER("MainWindow::toggleViewButtonItem(ulong handle, ulong parent, int position, int timer)");

    Q_UNUSED(timer);

    OBJECT_t *par = findObject(parent);

    if (!par || par->type != OBJ_SUBVIEW || !par->object.area)
    {
        MSG_ERROR("No object with handle " << handleToString(parent) << " found for update or object is not a subview list!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    par->object.area->showItem(handle, position);
}

void MainWindow::hideAllViewItems(ulong handle)
{
    DECL_TRACER("MainWindow::hideAllViewItems(ulong handle)");

    MSG_DEBUG("Searching for object with handle " << handleToString(handle));
    OBJECT_t *obj = findObject(handle);

    if (!obj || obj->type != OBJ_SUBVIEW || !obj->object.area)
    {
        if (!obj)
        {
            MSG_ERROR("Object with handle " << handleToString(handle) << " not found!");
        }
        else if (obj->type != OBJ_SUBVIEW)
        {
            MSG_ERROR("Object with handle " << handleToString(handle) << " has wrong type " << obj->type << "!");
        }
        else
        {
            MSG_ERROR("Object with handle " << handleToString(handle) << " has no scroll area!");
        }
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    obj->object.area->hideAllItems();
}

void MainWindow::hideViewItem(ulong handle, ulong parent)
{
    DECL_TRACER("MainWindow::hideViewItem(ulong handle, ulong parent)");

    MSG_DEBUG("Searching for object with handle " << handleToString(handle) << " and parent " << handleToString(parent));
    OBJECT_t *obj = findObject(parent);

    if (!obj || obj->type != OBJ_SUBVIEW || !obj->object.area)
    {
        if (!obj)
        {
            MSG_ERROR("Object with handle " << handleToString(handle) << " and parent " << handleToString(parent) << " not found!");
        }
        else if (obj->type != OBJ_SUBVIEW)
        {
            MSG_ERROR("Object with handle " << handleToString(handle) << " and parent " << handleToString(parent) << " has wrong type " << obj->type << "!");
        }
        else
        {
            MSG_ERROR("Object with handle " << handleToString(handle) << " and parent " << handleToString(parent) << " has no scroll area!");
        }
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    obj->object.area->hideItem(handle);
}

void MainWindow::SetVisible(ulong handle, bool state)
{
    DECL_TRACER("MainWindow::SetVisible(ulong handle, bool state)");

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_ERROR("Object " << handleToString(handle) << " not found!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (obj->type == OBJ_BUTTON && obj->object.label)
    {
        obj->object.label->setVisible(state);
        obj->invalid = false;
        obj->remove = false;
    }
    else if (obj->type == OBJ_MARQUEE && obj->object.marquee)
    {
        obj->object.marquee->setVisible(state);
        obj->invalid = false;
        obj->remove = false;
    }
    else if (obj->type == OBJ_SUBVIEW && obj->object.area)
    {
        obj->object.area->setVisible(state);
        obj->invalid = false;
        obj->remove = false;
    }
    else
    {
        MSG_DEBUG("Ignoring non button object " << handleToString(handle));
    }
}

void MainWindow::setSubViewPadding(ulong handle, int padding)
{
    DECL_TRACER("MainWindow::setSubViewPadding(ulong handle, int padding)");

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_ERROR("Object " << handleToString(handle) << " not found!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (obj->type != OBJ_SUBVIEW || !obj->object.area)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    obj->object.area->setSpace(padding);
}

void MainWindow::setSubViewAnimation(ulong handle, ANIMATION_t ani)
{
    DECL_TRACER("MainWindow::setSubViewAnimation(ulong handle, ANIMATION_t ani)");

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_ERROR("Object " << handleToString(handle) << " not found!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    obj->animate = ani;
}

/**
 * @brief Prepares a new object.
 * The method first checks whether there exists a background widget or not. If
 * not it creates a background widget with a black background. On Android this
 * image is the size of the whole screen while on a desktop it is only the size
 * of a page plus the task bar, if any.
 * If the background image (centralWidget()) already exists, it set's only the
 * credentials of the object.
 * It makes sure that all child objects of the central widget are destroyed.
 *
 * @param handle    The handle of the new widget
 * @param width     The original width of the page
 * @param heoght    The original height of the page
 */
void MainWindow::setPage(ulong handle, int width, int height)
{
    DECL_TRACER("MainWindow::setPage(ulong handle, int width, int height)");

    if (handle == mActualPageHandle)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

//    TLOCKER(draw_mutex);
    QWidget *wBackground = centralWidget();

    if (!wBackground)
    {
        MSG_ERROR("No central widget!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!mCentralWidget)
    {
        MSG_ERROR("Stack for pages not initialized!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    // The following should be true only for the first time this method is called.
    if (!mCentralInitialized)
    {
        QSize qs = menuBar()->sizeHint();

        setMinimumSize(scale(width), scale(height) + qs.height());
        mCentralInitialized = true;
    }

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_DEBUG("Adding new object " << handleToString(handle));
        OBJECT_t nobj;

        nobj.handle = handle;
        nobj.type = OBJ_PAGE;
        nobj.object.widget = nullptr;
        nobj.height = scale(height);
        nobj.width = scale(width);

        if (!addObject(nobj))
        {
            MSG_ERROR("Error crating an object for handle " << handleToString(handle));
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }

        obj = findObject(handle);
    }
    else
    {
        if (obj->type != OBJ_PAGE)
        {
            MSG_WARNING("Object " << handleToString(handle) << " is not a page! Will not reuse it as a page.");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }

        if (obj->object.widget && obj->object.widget->isHidden() && mCentralWidget->indexOf(obj->object.widget) >= 0)
            obj->object.widget->setParent(wBackground);

        obj->invalid = false;
        obj->remove = false;
        MSG_DEBUG("Hidden object " << handleToString(handle) << " was reactivated.");
    }

    if (!obj->object.widget)
    {
        obj->object.widget = new QWidget;
        obj->object.widget->setObjectName(QString("Page_") + handleToString(handle).c_str());

        if (mGestureFilter)
        {
            obj->object.widget->installEventFilter(mGestureFilter);
            obj->object.widget->grabGesture(Qt::PinchGesture);
            obj->object.widget->grabGesture(Qt::SwipeGesture);
        }

        obj->object.widget->setAutoFillBackground(true);
        obj->invalid = false;
        obj->object.widget->move(0, 0);
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
        obj->object.widget->setFixedSize(obj->width, obj->height);
#else
        obj->object.widget->setGeometry(geometry());
#endif
        mCentralWidget->addWidget(obj->object.widget);
    }

    mActualPageHandle = handle;
    MSG_PROTOCOL("Current page: " << handleToString(handle));
}

void MainWindow::setSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate, bool modal, bool collapsible)
{
    DECL_TRACER("MainWindow::setSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t animate, bool modal, bool collapsible)");

    Q_UNUSED(height);
    Q_UNUSED(modal);
    OBJECT_t *par = findObject(parent);

    if (!par || par->type != OBJ_PAGE)
    {
        if (!par)
        {
            MSG_ERROR("Subpage " << handleToString(handle) << " has no parent! Ignoring it.");
        }
        else
        {
            MSG_ERROR("Subpage " << handleToString(handle) << " has invalid parent " << handleToString(parent) << " which is no page! Ignoring it.");
        }
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!par->object.widget)
    {
        MSG_ERROR("Parent page " << handleToString(parent) << " has no widget defined!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (mCentralWidget && mCentralWidget->currentWidget() != par->object.widget)
    {
        MSG_WARNING("The parent page " << handleToString(parent) << " is not the current page " << handleToString(handle) << "!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    OBJECT_t *obj = findObject(handle);
    OBJECT_t nobj;
    bool shouldAdd = false;

    if (obj && obj->type != OBJ_SUBPAGE)
    {
        MSG_WARNING("Object " << handleToString(handle) << " exists but is not a subpage! Refusing to create a new page with this handle.");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }
    else if (!obj)
    {
        obj = &nobj;
        shouldAdd = true;
    }
    else
    {
        obj->invalid = false;
        obj->remove = false;
    }

    int scLeft = scale(left);
    int scTop = scale(top);
    int scWidth = scale(width);
    int scHeight = scale(height);

    obj->type = OBJ_SUBPAGE;
    obj->handle = handle;
    obj->collapsible = collapsible;

    if (!obj->object.widget)
    {
        obj->object.widget = new QWidget(par->object.widget);
        obj->object.widget->setObjectName(QString("Subpage_%1").arg(handleToString(handle).c_str()));
/*
        if (modal)
        {
            obj->object.widget->setWindowModality(Qt::WindowModal);
            MSG_TRACE("Subpage " << handleToString(handle) << " is a modal page");
        }
*/
    }
    else
        obj->object.widget->setParent(par->object.widget);

    obj->object.widget->setAutoFillBackground(true);
    obj->object.widget->move(scLeft, scTop);
    obj->object.widget->setFixedSize(scWidth, scHeight);
    obj->left = scLeft;
    obj->top = scTop;
    obj->width = scWidth;
    obj->height = scHeight;
    obj->invalid = false;
    obj->remove = false;
    // filter move event
    if (mGestureFilter)
    {
        obj->object.widget->installEventFilter(mGestureFilter);
        obj->object.widget->grabGesture(Qt::PinchGesture);
        obj->object.widget->grabGesture(Qt::SwipeGesture);
    }

    obj->aniDirection = true;
    obj->animate = animate;

    if (shouldAdd)
    {
        if (!addObject(nobj))
        {
            MSG_ERROR("Couldn't add the object " << handleToString(handle) << "!");
            nobj.object.widget->close();
#if TESTMODE == 1
            setScreenDone();
#endif
        }
    }
}

#ifdef _OPAQUE_SKIA_
void MainWindow::setBackground(ulong handle, TBitmap image, int width, int height, ulong color)
#else
void MainWindow::setBackground(ulong handle, TBitmap image, int width, int height, ulong color, int opacity)
#endif
{
    DECL_TRACER("MainWindow::setBackground(ulong handle, TBitmap image, ulong color [, int opacity])");

    Q_UNUSED(height);

    if (!mCentralWidget)
    {
        MSG_ERROR("The internal page stack is not initialized!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    OBJECT_t *obj = findObject(handle);

    if (!obj || obj->remove)
    {
#ifdef QT_DEBUG
        if (obj)
        {
            MSG_WARNING("No object " << handleToString(handle) << " found! (Flag remove: " << (obj->remove ? "TRUE" : "FALSE") << ")");
        }
        else
        {
            MSG_WARNING("No object " << handleToString(handle) << " found!");
        }
#else
        MSG_WARNING("No object " << handleToString(handle) << " found!");
#endif
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }
    else if (obj->invalid)
    {
        if (!enableObject(handle))
        {
            MSG_ERROR("Object " << handleToString(handle) << " of type " << objectToString(obj->type) << " couldn't be anabled!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }
    }

    if (obj->type != OBJ_SUBPAGE && obj->type != OBJ_BUTTON && obj->type != OBJ_PAGE)
    {
        MSG_ERROR("Method does not support object type " << objectToString(obj->type) << " for object " << handleToString(handle) << "!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    MSG_DEBUG("Object " << handleToString(handle) << " of type " << objectToString(obj->type) << " found!");

    if (obj->type == OBJ_BUTTON || obj->type == OBJ_SUBPAGE)
    {
        MSG_DEBUG("Processing object " << objectToString(obj->type));

        if ((obj->type == OBJ_BUTTON || obj->type == OBJ_MARQUEE) && !obj->object.label)
        {
            MSG_ERROR("The label of the object " << handleToString(handle) << " was not initialized!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }
        else if (obj->type == OBJ_SUBPAGE && !obj->object.widget)
        {
            MSG_ERROR("The widget of the object " << handleToString(handle) << " was not initialized!");
#if TESTMODE == 1
            setScreenDone();
#endif
            return;
        }

        QPixmap pix(obj->width, obj->height);

        if (TColor::getAlpha(color) == 0)
            pix.fill(Qt::transparent);
        else
            pix.fill(QColor::fromRgba(qRgba(TColor::getRed(color),TColor::getGreen(color),TColor::getBlue(color),TColor::getAlpha(color))));

        if (image.isValid() > 0)
        {
            MSG_DEBUG("Setting image of size " << image.getSize() << " (" << image.getWidth() << " x " << image.getHeight() << ")");
            QImage img(image.getBitmap(), image.getWidth(), image.getHeight(), image.getPixline(), QImage::Format_ARGB32);

            if (isScaled())
            {
                QSize size(obj->width, obj->height);
                pix.convertFromImage(img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            else
                pix.convertFromImage(img);
        }

        if (obj->type == OBJ_BUTTON)
            obj->object.label->setPixmap(pix);
        else if (obj->type == OBJ_MARQUEE)
            obj->object.marquee->setBackground(pix);
        else
        {
            MSG_DEBUG("Setting image as background for subpage " << handleToString(handle));
            QPalette palette(obj->object.widget->palette());
#ifndef _OPAQUE_SKIA_
            qreal oo;

            if (opacity < 0)
                oo = 0.0;
            else if (opacity > 255)
                oo = 1.0;
            else
                oo = 1.0 / 255.0 * (qreal)opacity;

            if (oo < 1.0)
            {
                QPixmap image(pix.size());      //Image with given size and format.
                image.fill(Qt::transparent);    //fills with transparent
                QPainter p(&image);
                p.setOpacity(oo);               // set opacity from 0.0 to 1.0, where 0.0 is fully transparent and 1.0 is fully opaque.
                p.drawPixmap(0, 0, pix);        // given pixmap into the paint device.
                p.end();
                palette.setBrush(QPalette::Window, QBrush(image));
                MSG_DEBUG("Opacity was set to " << oo);
            }
            else
                palette.setBrush(QPalette::Window, QBrush(pix));
#else
            palette.setBrush(QPalette::Window, QBrush(pix));
#endif
            obj->object.widget->setPalette(palette);
        }
    }
    else if (obj->type == TObject::OBJ_PAGE)
    {
        MSG_DEBUG("Processing object of type PAGE ...");
        QWidget *central = obj->object.widget;

        if (!central)
        {
            MSG_ERROR("There is no page widget initialized for page " << handleToString(handle));
#if TESTMODE == 1
            setScreenDone();
#endif
            displayMessage("Can't set a background without an active page!", "Internal error");
            return;
        }

        QWidget *current = mCentralWidget->currentWidget();
        int index = -1;

        if (current && central != current)
        {
            if ((index = mCentralWidget->indexOf(central)) < 0)
            {
                QString obName = QString("Page_%1").arg(handleToString(handle).c_str());

                for (int i = 0; i < mCentralWidget->count(); ++i)
                {
                    QWidget *w = mCentralWidget->widget(i);
                    MSG_DEBUG("Checking widget " << w->objectName().toStdString());

                    if (w->objectName() == obName)
                    {
                        index = i;
                        break;
                    }
                }

                if (index < 0)
                {
                    MSG_WARNING("Missing page " << handleToString(handle) << " on stack! Will add it to the stack.");
                    index = mCentralWidget->addWidget(central);
                    MSG_DEBUG("Number pages on stack: " << mCentralWidget->count());
                    QRect geomMain = geometry();
                    QRect geomCent = mCentralWidget->geometry();
                    MSG_DEBUG("Geometry MainWindow: left: " << geomMain.left() << ", right: " << geomMain.right() << ", top: " << geomMain.top() << ", bottom: " << geomMain.bottom());
                    MSG_DEBUG("Geometry CentWindow: left: " << geomCent.left() << ", right: " << geomCent.right() << ", top: " << geomCent.top() << ", bottom: " << geomCent.bottom());
                }
            }
        }
        else
            index = mCentralWidget->indexOf(central);

        QPixmap pix(obj->width, obj->height);
        QColor backgroundColor;

        if (TColor::getAlpha(color) == 0)
            backgroundColor = Qt::transparent;
        else
            backgroundColor = QColor::fromRgba(qRgba(TColor::getRed(color),TColor::getGreen(color),TColor::getBlue(color),TColor::getAlpha(color)));

        pix.fill(backgroundColor);
        MSG_DEBUG("Filled background of size " << pix.width() << "x" << pix.height() << " with color #" << std::setfill('0') << std::setw(8) << std::hex << color);

        if (width > 0 && image.isValid())
        {
            QImage img(image.getBitmap(), image.getWidth(), image.getHeight(), image.getPixline(), QImage::Format_ARGB32);
            bool valid = false;

            if (!img.isNull())
            {
                if (isScaled())
                {
                    QImage bgImage = img.scaled(obj->width, obj->height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                    valid = pix.convertFromImage(bgImage);
                    MSG_DEBUG("Scaled image from " << width << "x" << height << " to " << obj->width << "x" << obj->height);
                }
                else
                {
                    valid = pix.convertFromImage(img);
                    MSG_DEBUG("Converted image to pixmap.");
                }
            }

            if (!valid || pix.isNull())
            {
                if (pix.isNull())
                    pix = QPixmap(obj->width, obj->height);

                pix.fill(backgroundColor);
                MSG_WARNING("Error converting an image! Size raw data: " << image.getSize() << ", Width: " << image.getWidth() << ", Height: " << image.getHeight() << ", Bytes per row: " << image.getPixline());
            }
        }

        QPalette palette(central->palette());
        palette.setBrush(QPalette::Window, QBrush(pix));
        central->setPalette(palette);

        if (index >= 0)
            mCentralWidget->setCurrentIndex(index);
        else if (mCentralWidget)
        {
            index = mCentralWidget->addWidget(central);
            mCentralWidget->setCurrentIndex(index);
            MSG_DEBUG("Page widget " << handleToString(handle) << " was added at index " << index);
        }

        MSG_DEBUG("Background set");
    }
}

void MainWindow::disconnectArea(TQScrollArea* area)
{
    DECL_TRACER("MainWindow::disconnectArea(TQScrollArea* area)");

    if (!area)
        return;

    disconnect(area, &TQScrollArea::objectClicked, this, &MainWindow::onSubViewItemClicked);
}

void MainWindow::disconnectList(QListWidget* list)
{
    DECL_TRACER("MainWindow::disconnectList(QListWidget* list)");

    if (!list)
        return;

    disconnect(list, &QListWidget::currentItemChanged, this, &MainWindow::onTListCallbackCurrentItemChanged);
}

void MainWindow::reconnectArea(TQScrollArea * area)
{
    DECL_TRACER("MainWindow::reconnectArea(TQScrollArea * area)");

    if (!area)
        return;

    connect(area, &TQScrollArea::objectClicked, this, &MainWindow::onSubViewItemClicked);
}

void MainWindow::reconnectList(QListWidget *list)
{
    DECL_TRACER("MainWindow::reconnectList(QListWidget *list)");

    if (!list)
        return;

    connect(list, &QListWidget::currentItemChanged, this, &MainWindow::onTListCallbackCurrentItemChanged);
}

void MainWindow::minimizeSubpage(ulong handle)
{
    DECL_TRACER("MainWindow::minimizeSubpage(ulong handle)");

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object " << handleToString(handle) << " (Subpage) doesn't exist!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (obj->type != OBJ_PAGE)
    {
        MSG_WARNING("Object " << handleToString(handle) << " is not a subpage!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!obj->collapsible)
    {
        MSG_WARNING("Object " << handleToString(handle) << " is not collapsible!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    bool ret = startAnimation(obj, obj->animate, false);

    if (!ret)
    {
#if TESTMODE == 1
        __success = false;
#endif
    }
    else
    {
#if TESTMODE == 1
        __success = true;
#endif
    }

#if TESTMODE == 1
    setScreenDone();
#endif
}

void MainWindow::maximizeSubpage(ulong handle)
{
    DECL_TRACER("MainWindow::maximizeSubpage(ulong handle)");

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object " << handleToString(handle) << " (Subpage) doesn't exist!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (obj->type != OBJ_PAGE)
    {
        MSG_WARNING("Object " << handleToString(handle) << " is not a subpage!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!obj->collapsible)
    {
        MSG_WARNING("Object " << handleToString(handle) << " is not collapsible!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    bool ret = startAnimation(obj, obj->animate, true);

    if (!ret)
    {
#if TESTMODE == 1
        __success = false;
#endif
    }
    else
    {
#if TESTMODE == 1
        __success = true;
#endif
    }

#if TESTMODE == 1
    setScreenDone();
#endif
}

/**
 * @brief MainWindow::dropPage - Marks a page invalid
 * This method marks a page and all the object on the page as invalid. They are
 * not deleted. Instead the page, a QWidget representing a page, is set hidden.
 * With it all childs of the QWidget are also hidden. So the page can be
 * displayed later when it is used again.
 *
 * @param handle    The handle of the page.
 */
void MainWindow::dropPage(ulong handle)
{
//    TLOCKER(draw_mutex);
    DECL_TRACER("MainWindow::dropPage(ulong handle)");

    MSG_PROTOCOL("Dropping page " << handleToString(handle));

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object " << handleToString(handle) << " (Page) does not exist. Ignoring!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (obj->type != OBJ_PAGE)
    {
        MSG_WARNING("Object " << handleToString(handle) << " is not a page!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    MSG_DEBUG("Dropping page " << handleToString(handle));
    invalidateAllSubObjects(handle);

    if (obj->object.widget)
    {
        obj->object.widget->setHidden(true);
#if TESTMODE == 1
        __success = true;
#endif
    }
#if TESTMODE == 1
    setScreenDone();
#endif
}

void MainWindow::dropSubPage(ulong handle, ulong parent)
{
    DECL_TRACER("MainWindow::dropSubPage(ulong handle, ulong parent)");

    OBJECT_t *obj = findObject(handle);
    OBJECT_t *par = findObject(parent);

    if (!obj)
    {
        MSG_WARNING("Object " << handleToString(handle) << " (SubPage) does not exist. Ignoring!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (!par)
    {
        MSG_DEBUG("Parent object " << handleToString(parent) << " not found!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (obj->type != OBJ_SUBPAGE)
    {
        MSG_WARNING("Object " << handleToString(handle) << " is not a SubPage!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    QWidget *w = par->object.widget->findChild<QWidget *>(QString("Subpage_%1").arg(QString::fromStdString(handleToString(handle))));

    if (!w)
    {
        MSG_DEBUG("Parent object " << handleToString(parent) << " has no child " << handleToString(handle) << "!");
        obj->object.widget = nullptr;
        obj->remove = true;
        obj->invalid = true;
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    MSG_DEBUG("Dropping subpage " << handleToString(handle));
    invalidateAllSubObjects(handle);
    obj->aniDirection = false;
    bool ret = startAnimation(obj, obj->animate, false);

    if (obj->animate.hideEffect == SE_NONE || !ret || !mLastObject)
    {
        obj->invalid = true;
        obj->remove = false;

        if (obj->object.widget)
        {
            obj->object.widget->hide();
#if TESTMODE == 1
            __success = true;
#endif
        }
    }
#if TESTMODE == 1
    setScreenDone();
#endif
}

void MainWindow::dropButton(ulong handle)
{
//    TLOCKER(draw_mutex);
    DECL_TRACER("MainWindow::dropButton(ulong handle)");

    OBJECT_t *obj = findObject(handle);

    if (!obj)
    {
        MSG_WARNING("Object " << handleToString(handle) << " does not exist. Ignoring!");
        return;
    }

    if (obj->type == OBJ_PAGE || obj->type == OBJ_SUBPAGE)
        return;

    invalidateObject(handle);
}

void MainWindow::setSizeMainWindow(int width, int height)
{
#if !defined (Q_OS_ANDROID) && !defined(Q_OS_IOS)
    DECL_TRACER("MainWindow::setSizeMainWindow(int width, int height)");

    QRect geo = geometry();
    setGeometry(geo.x(), geo.y(), width, height + menuBar()->height());
#else
    Q_UNUSED(width);
    Q_UNUSED(height);
#endif
}

void MainWindow::playVideo(ulong handle, ulong parent, int left, int top, int width, int height, const string& url, const string& user, const string& pw)
{
    DECL_TRACER("MainWindow::playVideo(ulong handle, const string& url, const string& user, const string& pw))");

    TObject::OBJECT_t *obj = findObject(handle);
    TObject::OBJECT_t *par = findObject(parent);
    MSG_TRACE("Processing button " << handleToString(handle) << " from parent " << handleToString(parent));

    if (!par)
    {
        MSG_WARNING("Button has no parent! Ignoring it.");
        return;
    }

    if (!obj)
    {
        MSG_DEBUG("Adding new video object ...");
        OBJECT_t nobj;

        nobj.type = TObject::OBJ_VIDEO;
        nobj.handle = handle;
        nobj.width = scale(width);
        nobj.height = scale(height);
        nobj.left = scale(left);
        nobj.top = scale(top);

        nobj.vwidget = new QVideoWidget(par->object.widget);
        nobj.vwidget->installEventFilter(this);
        nobj.vwidget->setGeometry(nobj.left, nobj.top, nobj.width, nobj.height);

        if (!addObject(nobj))
        {
            MSG_ERROR("Error creating a video object!");
            return;
        }

        obj = findObject(handle);
    }
    else if (obj->type != OBJ_VIDEO)
    {
        obj->type = OBJ_VIDEO;

        if (!obj->vwidget)
        {
            obj->vwidget = new QVideoWidget(par->object.widget);
            obj->vwidget->installEventFilter(this);
            obj->vwidget->setGeometry(obj->left, obj->top, obj->width, obj->height);
        }

        MSG_DEBUG("Object " << handleToString(handle) << " of type " << objectToString(obj->type) << " found!");
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QMediaPlaylist *playlist = new QMediaPlaylist;
#endif
    QUrl qurl(QString::fromStdString(url));

    if (!user.empty())
        qurl.setUserName(QString::fromStdString(user));

    if (!pw.empty())
        qurl.setPassword(QString::fromStdString(pw));

    obj->videoUrl = qurl;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    playlist->addMedia(qurl);
#endif
    if (!obj->player)
    {
        obj->player = new QMediaPlayer;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        obj->player->setPlaylist(playlist);
#else
        obj->player->setSource(qurl);
#endif
        obj->player->setVideoOutput(obj->vwidget);
    }

    obj->vwidget->show();
    obj->player->play();
}

void MainWindow::inputText(Button::TButton *button, QByteArray buf, int width, int height, int frame, size_t pixline)
{
    DECL_TRACER("MainWindow::inputText(Button::TButton& button, QByteArray buf, int width, int height, int frame, size_t pixline)");

    if (!button)
    {
        MSG_WARNING("Method was called with no button!");
        return;
    }

    ulong handle = button->getHandle();
    ulong parent = button->getParent();
    TObject::OBJECT_t *obj = findObject(handle);
    TObject::OBJECT_t *par = findObject(parent);
    MSG_TRACE("Processing button " << handleToString(handle) << " from parent " << handleToString(parent) << " with frame width " << frame);

    if (!par)
    {
        MSG_WARNING("Button has no parent! Ignoring it.");
        return;
    }

    int instance = button->getActiveInstance();
    MSG_DEBUG("Instance: " << instance);

    if (!obj)
    {
        MSG_DEBUG("Adding new input object ...");
        OBJECT_t nobj;

        nobj.type = TObject::OBJ_INPUT;
        nobj.handle = handle;
        nobj.width = scale(width);
        nobj.height = scale(height);
        nobj.left = scale(button->getLeftPosition());
        nobj.top = scale(button->getTopPosition());

        string text = button->getText(0);
        string placeholder = button->getText(1);
        string mask = button->getInputMask();

        if (button->isMultiLine())
            text = ReplaceString(text, "|", "\n");

        nobj.object.plaintext = new TQEditLine(text, par->object.widget, button->isMultiLine());
        nobj.object.plaintext->setObjectName(string("EditLine_") + handleToString(handle));
        nobj.object.plaintext->setHandle(handle);
        nobj.object.plaintext->move(nobj.left, nobj.top);
        nobj.object.plaintext->setFixedSize(nobj.width, nobj.height);
        nobj.object.plaintext->setPadding(frame, frame, frame, frame);
        nobj.object.plaintext->setPasswordChar(button->getPasswordChar());
        nobj.wid = nobj.object.plaintext->winId();

        if (!placeholder.empty())
            nobj.object.plaintext->setPlaceholderText(placeholder);

        bool sys = false;

        if (button->getAddressPort() == 0 || button->getChannelPort() == 0)
        {
            int ch = 0;

            if (button->getAddressPort() == 0 && button->getAddressChannel() > 0)
                ch = button->getAddressChannel();
            else if (button->getChannelPort() == 0 && button->getChannelNumber() > 0)
                ch = button->getChannelNumber();

            switch(ch)
            {
                case SYSTEM_ITEM_SIPPORT:
                case SYSTEM_ITEM_NETLINX_PORT:
                    nobj.object.plaintext->setInputMask("000000");
                    nobj.object.plaintext->setNumericInput();
                    sys = true;
                break;

                case SYSTEM_ITEM_NETLINX_CHANNEL:
                    nobj.object.plaintext->setInputMask("99999");
                    nobj.object.plaintext->setNumericInput();
                    sys = true;
                break;
            }

            if (sys)
                MSG_TRACE("System button " << ch << " detected.");
        }

        if (!sys && !mask.empty())
            nobj.object.plaintext->setInputMask(convertMask(mask));

        if (!buf.size() || pixline == 0)
        {
            MSG_ERROR("No image!");
            TError::SetError();
            return;
        }

        MSG_DEBUG("Background image size: " << width << " x " << height << ", rowBytes: " << pixline);
        QPixmap pix(width, height);
        QImage img((uchar *)buf.data(), width, height, QImage::Format_ARGB32);

        if (isScaled())
            pix.convertFromImage(img.scaled(scale(width), scale(height), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        else
            pix.convertFromImage(img);

        nobj.object.plaintext->setBackgroundPixmap(pix);
        // Load the font
        QFont font = loadFont(button->getFontIndex(button->getActiveInstance()), button->getFont(), button->getFontStyle());
        QPalette palette(this->palette());
        TColor::COLOR_T textColor = TColor::getAMXColor(button->getTextColor(instance));
        TColor::COLOR_T fillColor = TColor::getAMXColor(button->getFillColor(instance));
        QColor txcolor(QColor::fromRgba(qRgba(textColor.red, textColor.green, textColor.blue, textColor.alpha)));
        QColor cfcolor(QColor::fromRgba(qRgba(fillColor.red, fillColor.green, fillColor.blue, fillColor.alpha)));
        palette.setColor(QPalette::Window, cfcolor);
        palette.setColor(QPalette::Base, cfcolor);
        palette.setColor(QPalette::Text, txcolor);

        nobj.object.plaintext->setFont(font);
        nobj.object.plaintext->setPalette(palette);
        nobj.object.plaintext->setTextColor(txcolor);

        if (!addObject(nobj))
        {
            MSG_ERROR("Error creating an input object!");
            return;
        }

        connect(nobj.object.plaintext, &TQEditLine::inputChanged, this, &MainWindow::onInputChanged);
        connect(nobj.object.plaintext, &TQEditLine::cursorPositionChanged, this, &MainWindow::onCursorChanged);
        connect(nobj.object.plaintext, &TQEditLine::focusChanged, this, &MainWindow::onFocusChanged);
    }
    else
    {
        MSG_DEBUG("Object " << handleToString(handle) << " of type " << objectToString(obj->type) << " found!");

        string text = button->getText(0);
        string placeholder = button->getText(1);
        string mask = button->getInputMask();
        MSG_DEBUG("Setting text: \"" << text << "\" with mask: \"" << mask << "\"");

        if (!placeholder.empty())
            obj->object.plaintext->setPlaceholderText(placeholder);

        if (button->isMultiLine())
            text = ReplaceString(text, "|", "\n");

        obj->object.plaintext->setText(text);

        if (!mask.empty())
            obj->object.plaintext->setInputMask(convertMask(mask));

        QPixmap pix(obj->width, obj->height);
        QImage img((uchar *)buf.data(), width, height, QImage::Format_ARGB32);

        if (isScaled())
            pix.convertFromImage(img.scaled(obj->width, obj->height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        else
            pix.convertFromImage(img);

//        obj->object.plaintext->setBgPixmap(pix);
        obj->object.plaintext->setBackgroundPixmap(pix);
        QPalette palette(this->palette());
        TColor::COLOR_T textColor = TColor::getAMXColor(button->getTextColor(instance));
        TColor::COLOR_T fillColor = TColor::getAMXColor(button->getFillColor(instance));
        QColor txcolor(QColor::fromRgba(qRgba(textColor.red, textColor.green, textColor.blue, textColor.alpha)));
        QColor cfcolor(QColor::fromRgba(qRgba(fillColor.red, fillColor.green, fillColor.blue, fillColor.alpha)));
        palette.setColor(QPalette::Window, cfcolor);
        palette.setColor(QPalette::Base, cfcolor);
        palette.setColor(QPalette::Text, txcolor);
//        palette.setBrush(QPalette::Window, QBrush(pix));

        obj->object.plaintext->setPalette(palette);
        obj->object.plaintext->setTextColor(txcolor);
    }
}

void MainWindow::listBox(Button::TButton *button, QByteArray buffer, int width, int height, int frame, size_t pixline)
{
    DECL_TRACER("MainWindow::listBox(Button::TButton& button, QByteArray buffer, int width, int height, int frame, size_t pixline)");

    ulong handle = button->getHandle();
    ulong parent = button->getParent();
    TObject::OBJECT_t *obj = findObject(handle);
    TObject::OBJECT_t *par = findObject(parent);
    MSG_TRACE("Processing list " << handleToString(handle) << " from parent " << handleToString(parent) << " with frame width " << frame);

    if (!par)
    {
        MSG_WARNING("List has no parent! Ignoring it.");
        return;
    }

    if (!obj)
    {
        MSG_DEBUG("Adding new list object ...");
        OBJECT_t nobj;

        nobj.type = TObject::OBJ_LIST;
        nobj.handle = handle;
        nobj.rows = button->getListNumRows();
        nobj.cols = button->getListNumCols();
        nobj.width = scale(width);
        nobj.height = scale(height);
        nobj.left = scale(button->getLeftPosition());
        nobj.top = scale(button->getTopPosition());

        vector<string> listContent = button->getListContent();

        if (par->type == TObject::OBJ_PAGE)
            nobj.object.list = new QListWidget(par->object.widget ? par->object.widget : centralWidget());
        else
            nobj.object.list = new QListWidget(par->object.widget);

        nobj.object.list->move(nobj.left, nobj.top);
        nobj.object.list->setFixedSize(nobj.width, nobj.height);
        connect(nobj.object.list, &QListWidget::currentItemChanged, this, &MainWindow::onTListCallbackCurrentItemChanged);

        if (!buffer.size() || pixline == 0)
        {
            MSG_ERROR("No image!");
            TError::SetError();
            return;
        }

        MSG_DEBUG("Background image size: " << width << " x " << height << ", rowBytes: " << pixline);
        QPixmap pix(width, height);
        QImage img((uchar *)buffer.data(), width, height, QImage::Format_ARGB32);

        if (isScaled())
            pix.convertFromImage(img.scaled(scale(width), scale(height), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        else
            pix.convertFromImage(img);

        // Load the font
        FONT_T font = button->getFont();
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

        if (QFontDatabase::addApplicationFont(ffile.c_str()) == -1)
        {
            MSG_ERROR("Font " << ffile << " could not be loaded!");
            TError::SetError();
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
        palette.setBrush(QPalette::Base, QBrush(pix));

        nobj.object.list->setFont(ft);
        nobj.object.list->setPalette(palette);
        // Add content
        if (!listContent.empty())
        {
            vector<string>::iterator iter;

            if (nobj.object.list->count() > 0)
                nobj.object.list->clear();

            MSG_DEBUG("Adding " << listContent.size() << " entries to list.");
            string selected = gPageManager->getSelectedItem(handle);
            int index = 0;

            for (iter = listContent.begin(); iter != listContent.end(); ++iter)
            {
                nobj.object.list->addItem(iter->c_str());

                if (selected == *iter)
                    nobj.object.list->setCurrentRow(index);

                index++;
            }
        }
        else
            MSG_DEBUG("No items for list!");

//        nobj.object.list->show();

        if (!addObject(nobj))
        {
            MSG_ERROR("Error creating a list object!");
            return;
        }
    }
    else
    {
        MSG_DEBUG("Object " << handleToString(handle) << " of type " << objectToString(obj->type) << " found!");
        enableObject(handle);
    }
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
        mQKeypad->reject();
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
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    settings();
#else
#ifndef Q_OS_IOS
    if (gPageManager)
        gPageManager->showSetup();
#else
    mIOSSettingsActive = true;
    QASettings::openSettings();
#endif  // Q_OS_IOS
#endif  // !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
}

void MainWindow::playSound(const string& file)
{
    DECL_TRACER("MainWindow::playSound(const string& file)");

    MSG_DEBUG("Playing file " << file);

    if (TConfig::getMuteState())
    {
#if TESTMODE == 1
        __success = true;
        setAllDone();
#endif
        return;
    }

    if (!mMediaPlayer)
    {
        mMediaPlayer = new QMediaPlayer;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        mAudioOutput = new QAudioOutput;
        mMediaPlayer->setAudioOutput(mAudioOutput);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        connect(mMediaPlayer, &QMediaPlayer::playingChanged, this, &MainWindow::onPlayingChanged);
#endif
        connect(mMediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
        connect(mMediaPlayer, &QMediaPlayer::errorOccurred, this, &MainWindow::onPlayerError);
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mMediaPlayer->setMedia(QUrl::fromLocalFile(file.c_str()));
    mMediaPlayer->setVolume(calcVolume(TConfig::getSystemVolume()));

    if (mMediaPlayer->state() != QMediaPlayer::StoppedState)
        mMediaPlayer->stop();
#else   // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mMediaPlayer->setSource(QUrl::fromLocalFile(file.c_str()));
    mAudioOutput->setVolume(static_cast<float>(calcVolume(TConfig::getSystemVolume())));

    if (!mMediaPlayer->isAvailable())
    {
        MSG_WARNING("No audio modul found!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (mMediaPlayer->isPlaying())
        mMediaPlayer->setPosition(0);
//        mMediaPlayer->stop();
#else
    if (mMediaPlayer->playbackState() != QMediaPlayer::StoppedState)
        mMediaPlayer->stop();

    mMediaPlayer->setPosition(0);
#endif  // #if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)

#endif  // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mMediaPlayer->play();
#if TESTMODE == 1
    if (mMediaPlayer->error() != QMediaPlayer::NoError)
    {
        MSG_ERROR("Error playing \"" << file << "\": " << mMediaPlayer->errorString().toStdString());
    }
    else
        __success = true;

    setAllDone();
#endif
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (mMediaPlayer)
        mMediaPlayer->setMuted(state);
#else
    if (mAudioOutput)
        mAudioOutput->setMuted(state);
#endif
#if TESTMODE == 1
    __success = true;
    setAllDone();
#endif
}

void MainWindow::setVolume(int volume)
{
    DECL_TRACER("MainWindow::setVolume(int volume)");

    if (!mMediaPlayer)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mMediaPlayer->setVolume(calcVolume(volume));
#else   // QT_VERSION
    if (!mMediaPlayer || !mAudioOutput)
    {
#if TESTMODE == 1
        setAllDone();
#endif  // TESTMODE
        return;
    }

    mAudioOutput->setVolume(static_cast<float>(calcVolume(volume)));
#if TESTMODE == 1
    __success = true;
    setAllDone();
#endif  // TESTMODE
#endif  // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
void MainWindow::onPlayingChanged(bool playing)
{
    DECL_TRACER("MainWindow::onPlayingChanged(bool playing)");

    // If playing stopped for whatever reason, we rewind the track
    if (!playing && mMediaPlayer)
    {
        mMediaPlayer->setPosition(0);
        MSG_DEBUG("Track was rewound.");
    }
}
#endif

void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    DECL_TRACER("MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)");

    switch(status)
    {
        case QMediaPlayer::NoMedia:         MSG_WARNING("There is no current media."); break;
        case QMediaPlayer::LoadingMedia:    MSG_INFO("The current media is being loaded."); break;
        case QMediaPlayer::LoadedMedia:     MSG_INFO("The current media has been loaded."); break;
        case QMediaPlayer::StalledMedia:    MSG_WARNING("Playback of the current media has stalled due to insufficient buffering or some other temporary interruption."); break;
        case QMediaPlayer::BufferingMedia:  MSG_INFO("The player is buffering data but has enough data buffered for playback to continue for the immediate future."); break;
        case QMediaPlayer::BufferedMedia:   MSG_INFO("The player has fully buffered the current media."); break;
        case QMediaPlayer::EndOfMedia:      MSG_INFO("Playback has reached the end of the current media."); break;
        case QMediaPlayer::InvalidMedia:    MSG_WARNING("The current media cannot be played.");
    }
}

void MainWindow::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    DECL_TRACER("MainWindow::onPlayerError(QMediaPlayer::Error error, const QString &errorString)");

    if (error == QMediaPlayer::NoError)
        return;

    MSG_ERROR("Media player error (" << error << "): " << errorString.toStdString());
}

void MainWindow::initializeIntercom(INTERCOM_t ic)
{
    DECL_TRACER("MainWindow::initializeIntercom(INTERCOM_t ic)");

    mIntercom.setIntercom(ic);
}

void MainWindow::intercomStart()
{
    DECL_TRACER("MainWindow::intercomStart()");

    mIntercom.start();
}

void MainWindow::intercomStop()
{
    DECL_TRACER("MainWindow::intercomStop()");

    mIntercom.stop();
}

void MainWindow::intercomMicLevel(int level)
{
    DECL_TRACER("MainWindow::intercomMicLevel(int level)");

    mIntercom.setMicrophoneLevel(level);
}

void MainWindow::intercomSpkLevel(int level)
{
    DECL_TRACER("MainWindow::intercomSpkLevel(int level)");

    mIntercom.setSpeakerLevel(level);
}

void MainWindow::intercomMicMute(bool mute)
{
    DECL_TRACER("MainWindow::intercomMicMute(bool mute)");

    mIntercom.setMute(mute);
}

/*
void MainWindow::playShowList()
{
    DECL_TRACER("MainWindow::playShowList()");

    _EMIT_TYPE_t etype = getNextType();

    while (etype != ET_NONE)
    {
        ulong handle = 0;
        ulong parent = 0;
        int left = 0;
        int top = 0;
        int width = 0;
        int height = 0;
        int frame = 0;
        TBitmap image;
        ulong color = 0;
        int space{0};
        bool vertical = false;
        TColor::COLOR_T fillColor;
        bool pth = false;
#ifndef _OPAQUE_SKIA_
        int opacity = 255;
#endif
        ANIMATION_t animate;
        std::string url;
        std::string user;
        std::string pw;
        Button::TButton *button;
        Button::BITMAP_t bm;

        switch(etype)
        {
            case ET_BACKGROUND:
#ifdef _OPAQUE_SKIA_
                if (getBackground(&handle, &image, &width, &height, &color))
#else
                if (getBackground(&handle, &image, &width, &height, &color, &opacity))
#endif
                {
                    MSG_PROTOCOL("Replay: BACKGROUND of object " << handleToString(handle));
#ifdef _OPAQUE_SKIA_
                    emit sigSetBackground(handle, image, width, height, color);
#else
                    emit sigSetBackground(handle, image, width, height, color, opacity);
#endif
                }
            break;

            case ET_BUTTON:
                if (getButton(&handle, &parent, &image, &left, &top, &width, &height, &pth))
                {
                    MSG_PROTOCOL("Replay: BUTTON object " << handleToString(handle));
                    emit sigDisplayButton(handle, parent, image, width, height, left, top, pth);
                }
            break;

            case ET_INTEXT:
                if (getInText(&handle, &button, &bm, &frame))
                {
                    QByteArray buf;

                    if (bm.buffer && bm.rowBytes > 0)
                    {
                        size_t s = (size_t)bm.width * (size_t)bm.height * (size_t)(bm.rowBytes / bm.width);
                        buf.insert(0, (const char *)bm.buffer, s);
                    }

                    MSG_PROTOCOL("Replay: INTEXT object " << handleToString(handle));
                    emit sigInputText(button, buf, bm.width, bm.height, (int)bm.rowBytes, frame);
                }
            break;

            case ET_PAGE:
                if (getPage(&handle, &width, &height))
                {
                    MSG_PROTOCOL("Replay: PAGE object " << handleToString(handle));

                    if (isDeleted())
                        emit sigDropPage(handle);
                    else
                        emit sigSetPage(handle, width, height);
                }
            break;

            case ET_SUBPAGE:
                if (getSubPage(&handle, &parent, &left, &top, &width, &height, &animate))
                {
                    MSG_PROTOCOL("Replay: SUBPAGE object " << handleToString(handle));

                    if (isDeleted())
                        emit sigDropSubPage(handle, parent);
                    else
                        emit sigSetSubPage(handle, parent, left, top, width, height, animate);
                }
            break;

            case ET_SUBVIEW:
                if (getViewButton(&handle, &parent, &vertical, &image, &left, &top, &width, &height, &space, &fillColor))
                {
                    MSG_PROTOCOL("Replay: SUBVIEW object " << handleToString(handle));
                    emit sigDisplayViewButton(handle, parent, vertical, image, width, height, left, top, space, fillColor);
                }
            break;

            case ET_VIDEO:
                if (getVideo(&handle, &parent, &left, &top, &width, &height, &url, &user, &pw))
                {
                    MSG_PROTOCOL("Replay: VIDEO object " << handleToString(handle));
                    emit sigPlayVideo(handle, parent, left, top, width, height, url, user, pw);
                }
            break;

            default:
                MSG_WARNING("Type " << etype << " is currently not supported!");
        }

        dropType(etype);
        etype = getNextType();
    }
}
*/

int MainWindow::scale(int value)
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    double s = gScale;
#else
    double s = mScaleFactor;
#endif
    if (value <= 0 || s == 1.0 || s < 0.0)
        return value;

    return static_cast<int>(static_cast<double>(value) * s);
}

bool MainWindow::isScaled()
{
    DECL_TRACER("MainWindow::isScaled()");

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    double s = gScale;
#else
    double s = mScaleFactor;
#endif
    if (s > 0.0 && s != 1.0 && gPageManager && TConfig::getScale())
        return true;

    return false;
}

bool MainWindow::startAnimation(TObject::OBJECT_t* obj, ANIMATION_t& ani, bool in)
{
    DECL_TRACER("MainWindow::startAnimation(OBJECT_t* obj, ANIMATION_t& ani)");

    if (!obj)
    {
        MSG_ERROR("Got no object to start the animation!");
        return false;
    }

    int scLeft = obj->left;
    int scTop = obj->top;
    int scWidth = obj->width;
    int scHeight = obj->height;
    int duration = (in ? ani.showTime : ani.hideTime);
    SHOWEFFECT_t effect = (in ? ani.showEffect : ani.hideEffect);
    int offset = 0;

    if (TTPInit::isG5())
        offset = ani.offset;

    mLastObject = nullptr;

    if (effect == SE_NONE || duration <= 0 || (obj->type != OBJ_SUBPAGE && obj->type != OBJ_PAGE))
        return false;

    if (!obj->object.widget)
    {
        MSG_WARNING("Object " << handleToString(obj->handle) << " has no widget defined! Ignoring fade effect.");
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    if (effect == SE_FADE)
    {
        MSG_DEBUG("Fading object " << handleToString(obj->handle) << (in ? " IN" : " OUT"));
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(obj->object.widget);

        if (effect)
        {
            obj->object.widget->setGraphicsEffect(effect);
            obj->animation = new QPropertyAnimation(effect, "opacity");
        }
    }
    else
    {
        MSG_DEBUG("Moving object " << handleToString(obj->handle) << (in ? " IN" : " OUT"));
        obj->animation = new QPropertyAnimation(obj->object.widget);
        obj->animation->setTargetObject(obj->object.widget);
    }

    obj->animation->setDuration(duration * 100);    // convert 10th of seconds into milliseconds
    MSG_DEBUG("Processing animation effect " << effect << " with a duration of " << obj->animation->duration() << "ms");

    switch(effect)
    {
        case SE_SLIDE_BOTTOM_FADE:
        case SE_SLIDE_BOTTOM:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft, scTop + (scHeight * 2), scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationInFinished);
                obj->object.widget->show();
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop + (scHeight * 2) - offset, scWidth, scHeight));
                obj->remove = true;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            mLastObject = obj;
            mAnimObjects.insert(pair<ulong, OBJECT_t *>(obj->handle, obj));
            obj->animation->start();
            MSG_DEBUG("Animation SLIDE BOTTOM started.");
        break;

        case SE_SLIDE_LEFT_FADE:
        case SE_SLIDE_LEFT:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft - scWidth, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationInFinished);
                obj->object.widget->show();
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft - scWidth + offset, scTop, scWidth, scHeight));
                obj->remove = true;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            mLastObject = obj;
            mAnimObjects.insert(pair<ulong, OBJECT_t *>(obj->handle, obj));
            obj->animation->start();
        break;

        case SE_SLIDE_RIGHT_FADE:
        case SE_SLIDE_RIGHT:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft + scWidth, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationInFinished);
                obj->object.widget->show();
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft + scWidth - offset, scTop, scWidth, scHeight));
                obj->remove = true;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            mLastObject = obj;
            mAnimObjects.insert(pair<ulong, OBJECT_t *>(obj->handle, obj));
            obj->animation->start();
        break;

        case SE_SLIDE_TOP_FADE:
        case SE_SLIDE_TOP:
            obj->animation->setPropertyName("geometry");

            if (in)
            {
                obj->animation->setStartValue(QRect(scLeft, scTop - scHeight, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop, scWidth, scHeight));
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationInFinished);
                obj->object.widget->show();
            }
            else
            {
                obj->animation->setStartValue(QRect(scLeft, scTop, scWidth, scHeight));
                obj->animation->setEndValue(QRect(scLeft, scTop - scHeight + offset, scWidth, scHeight));
                obj->remove = true;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            mLastObject = obj;
            mAnimObjects.insert(pair<ulong, OBJECT_t *>(obj->handle, obj));
            obj->animation->start();
        break;

        case SE_FADE:
            if (in)
            {
                if (obj->object.widget)
                {
                    obj->object.widget->setWindowOpacity(0.0);
                    obj->object.widget->show();
                }

                obj->animation->setStartValue(0.0);
                obj->animation->setEndValue(1.0);
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationInFinished);
                obj->object.widget->show();
            }
            else
            {
                obj->animation->setStartValue(1.0);
                obj->animation->setEndValue(0.0);
                obj->remove = true;
                connect(obj->animation, &QPropertyAnimation::finished, this, &MainWindow::animationFinished);
            }

            mLastObject = obj;
            mAnimObjects.insert(pair<ulong, OBJECT_t *>(obj->handle, obj));
            obj->animation->setEasingCurve(QEasingCurve::Linear);
            obj->animation->start();
        break;

        default:
            MSG_WARNING("Subpage effect " << ani.showEffect << " is not supported.");
            delete obj->animation;
            obj->animation = nullptr;
            return false;
    }

    return true;
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

void MainWindow::onSubViewItemClicked(ulong handle, int x, int y, bool pressed)
{
    DECL_TRACER("MainWindow::onSubViewItemClicked(ulong handle, int x, int y, bool pressed)");

    if (!handle)
        return;

    // Create a thread and call the base program function.
    // We create a thread to not interrupt the QT framework longer then
    // necessary.
    if (gPageManager)
        gPageManager->mouseEvent(handle, x, y, pressed);
}

void MainWindow::onInputChanged(ulong handle, string& text)
{
    DECL_TRACER("MainWindow::onInputChanged(ulong handle, string& text)");

    try
    {
        std::thread thr = std::thread([=] {
            if (gPageManager)
                gPageManager->inputButtonFinished(handle, text);
        });

        thr.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting a thread to handle input line finish: " << e.what());
    }
}

void MainWindow::onFocusChanged(ulong handle, bool in)
{
    DECL_TRACER("MainWindow::onFocusChanged(ulong handle, bool in)");

    try
    {
        std::thread thr = std::thread([=] {
            if (gPageManager)
                gPageManager->inputFocusChanged(handle, in);
        });

        thr.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting a thread to handle input line finish: " << e.what());
    }
}

void MainWindow::onCursorChanged(ulong handle, int oldPos, int newPos)
{
    DECL_TRACER("MainWindow::onCursorChanged(ulong handle, int oldPos, int newPos)");

    try
    {
        std::thread thr = std::thread([=] {
            if (gPageManager)
                gPageManager->inputCursorPositionChanged(handle, oldPos, newPos);
        });

        thr.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting a thread to handle input line finish: " << e.what());
    }
}

void MainWindow::onGestureEvent(QObject *obj, QGestureEvent *event)
{
    DECL_TRACER("MainWindow::onGestureEvent(QObject *obj, QGestureEvent *event)");

    Q_UNUSED(obj);
    gestureEvent(event);
}

QPixmap MainWindow::scaleImage(QPixmap& pix)
{
    DECL_TRACER("MainWindow::scaleImage(QPixmap& pix)");

    int width = scale(pix.width());
    int height = scale(pix.height());

    return pix.scaled(width, height);
}

QPixmap MainWindow::scaleImage(unsigned char* buffer, int width, int height, int pixline)
{
    DECL_TRACER("MainWindow::scaleImage(unsigned char* buffer, int width, int height, int pixline)");

    if (!buffer || width < 1 || height < 1 || pixline < (width * 4))
    {
        MSG_ERROR("Invalid image for scaling!");
        return QPixmap();
    }

    QImage img(buffer, width, height, pixline, QImage::Format_ARGB32);  // Original size

    if (img.isNull() || !img.valid(width-1, height-1))
    {
        MSG_ERROR("Unable to create a valid image!");
        return QPixmap();
    }

    QSize size(scale(width), scale(height));
    QPixmap pixmap;
    bool ret = false;

    if (isScaled())
        ret = pixmap.convertFromImage(img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)); // Scaled size
    else
        ret = pixmap.convertFromImage(img);

    if (!ret || pixmap.isNull())
    {
        MSG_ERROR("Unable to create a pixmap out of an image!");
    }

    return pixmap;
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

/*
 * Copyright (C) 2022, 2023 by Andreas Theofilu <andreas@theosys.at>
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

#include <iostream>

#include "QASettings.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>
#import <OSLog/OSLog.h>
#include <QtCore>

#include "tconfig.h"
#include "terror.h"

@interface SetupController : NSObject

@property(getter=isInitialized) BOOL Initialized;

- (void)fetchSettingBundleData:(NSNotification *)notification;

@end

@implementation SetupController

- (id)init
{
    std::cout << "[SetupController init]" << std::endl;
    [self setInitialized:NO];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(fetchSettingBundleData:) name:NSUserDefaultsDidChangeNotification object:nil];
    return self;
}

- (void)fetchSettingBundleData:(NSNotification *)notification
{
    if ([self isInitialized] == NO)
    {
        std::cout << "[SetupController fetchSettingBundleData]: Registering defaults." << std::endl;
        [self registerDefaults];
        return;
    }

    // Check here for all potential changed values who could be executed on the fly
    std::cout << "[SetupController fetchSettingBundleData]: Checking for changed values." << std::endl;
    unsigned int logLevel = 0;

    bool logInfo = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_info"];
    bool logWarn = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_warning"];
    bool logError = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_error"];
    bool logTrace = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_trace"];
    bool logDebug = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_debug"];

    if (logInfo)
        logLevel |= HLOG_INFO;

    if (logWarn)
        logLevel |= HLOG_WARNING;

    if (logError)
        logLevel |= HLOG_ERROR;

    if (logTrace)
        logLevel |= HLOG_TRACE;

    if (logDebug)
        logLevel |= HLOG_DEBUG;

    if (logLevel != TConfig::getLogLevelBits())
    {
        TConfig::saveLogLevel(logLevel);
        TStreamError::setLogLevel(logLevel);
    }

    TConfig::saveProfiling([[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_profile"]);
    TConfig::saveFormat([[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_long_format"]);
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_system"];
    TConfig::saveSystemSoundFile(QString::fromNSString(str).toStdString());
    str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_single"];
    TConfig::saveSingleBeepFile(QString::fromNSString(str).toStdString());
    str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_double"];
    TConfig::saveDoubleBeepFile(QString::fromNSString(str).toStdString());
    TConfig::saveSystemSoundState([[ NSUserDefaults standardUserDefaults] boolForKey:@"sound_enable"]);
    TConfig::saveSystemVolume((int)[[ NSUserDefaults standardUserDefaults] integerForKey:@"sound_volume"]);
    TConfig::saveSystemGain((int)[[ NSUserDefaults standardUserDefaults] integerForKey:@"sound_gain"]);
    str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_1"];
    TConfig::savePassword1(QString::fromNSString(str).toStdString());
    str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_2"];
    TConfig::savePassword2(QString::fromNSString(str).toStdString());
    str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_3"];
    TConfig::savePassword3(QString::fromNSString(str).toStdString());
    str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_4"];
    TConfig::savePassword4(QString::fromNSString(str).toStdString());
}

- (void)registerDefaults
{
    if ([self isInitialized] == YES)
        return;

    [self setInitialized:YES];

    std::cout << "[SetupController registerDefaults]" << std::endl;

    NSDictionary* appDefaultsRoot = [NSDictionary dictionaryWithObject:@"Root" forKey:@"StringsTable"];
    NSDictionary* appDefaultsSip = [NSDictionary dictionaryWithObject:@"SIP" forKey:@"StringsTable"];
    NSDictionary* appDefaultsView = [NSDictionary dictionaryWithObject:@"View" forKey:@"StringsTable"];
    NSDictionary* appDefaultsSound = [NSDictionary dictionaryWithObject:@"Sound" forKey:@"StringsTable"];
    NSDictionary* appDefaultsLog = [NSDictionary dictionaryWithObject:@"Logging" forKey:@"StringsTable"];
    NSDictionary* appDefaultsPasswd = [NSDictionary dictionaryWithObject:@"Passwords" forKey:@"StringsTable"];

    if (appDefaultsRoot)
    {
        // the default value was found in the dictionary.  Register it.
        // wrapping this in an if.. clause prevents overwriting a user entered
        // value with the default value
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaultsRoot];
    }

    if (appDefaultsSip)
    {
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaultsSip];
    }

    if (appDefaultsView)
    {
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaultsView];
    }

    if (appDefaultsSound)
    {
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaultsSound];
    }

    if (appDefaultsLog)
    {
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaultsLog];
    }

    if (appDefaultsPasswd)
    {
        [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaultsPasswd];
    }
}

@end

// -----------------------------------------------------------------------------
// ---- C++ part starts here
// -----------------------------------------------------------------------------

std::string QASettings::oldNetlinx;
int QASettings::oldPort{0};
int QASettings::oldChannelID{0};
std::string QASettings::oldSurface;
bool QASettings::oldToolbarSuppress{false};
bool QASettings::oldToolbarForce{false};

SetupController *mSetup = nil;
os_log_t TPANEL_LOG = nil;

QASettings::QASettings()
{
    mSetup = [[SetupController alloc] init];
}

QASettings::~QASettings()
{
    if (TPANEL_LOG != nil)
        [TPANEL_LOG release];
}

void QASettings::registerDefaultPrefs()
{
    if (mSetup)
        [mSetup registerDefaults];
}

void QASettings::unregisterPrefs()
{
    if (mSetup)
        [mSetup release];

    mSetup = nil;
}

QString QASettings::getNetlinxIP()
{
    NSString* netlinx_ip = [[NSUserDefaults standardUserDefaults] stringForKey:@"netlinx_ip"];

    if (!netlinx_ip)
        netlinx_ip = @"0.0.0.0";

    return QString::fromNSString(netlinx_ip);
}

int QASettings::getNetlinxPort()
{
    return getDefaultNumber((char *)"netlinx_port", 1319);
}

int QASettings::getNetlinxChannel()
{
    return getDefaultNumber((char *)"netlinx_channel", 10001);
}

QString QASettings::getNetlinxPanelType()
{
    NSString *netlinx_type = [[ NSUserDefaults standardUserDefaults] stringForKey:@"netlinx_type"];

    if (!netlinx_type)
        netlinx_type = @"MVP-5200i";

    return QString::fromNSString(netlinx_type);
}

QString QASettings::getFTPUser()
{
    NSString *netlinx_ftp_user = [[ NSUserDefaults standardUserDefaults] stringForKey:@"netlinx_ftp_user"];

    if (!netlinx_ftp_user)
        netlinx_ftp_user = @"administrator";

    return QString::fromNSString(netlinx_ftp_user);
}

QString QASettings::getFTPPassword()
{
    NSString *netlinx_ftp_password = [[ NSUserDefaults standardUserDefaults] stringForKey:@"netlinx_ftp_password"];

    if (!netlinx_ftp_password)
        netlinx_ftp_password = @"password";

    return QString::fromNSString(netlinx_ftp_password);
}

QString QASettings::getNetlinxSurface()
{
    NSString *netlinx_surface = [[ NSUserDefaults standardUserDefaults] stringForKey:@"netlinx_surface"];

    if (!netlinx_surface)
        netlinx_surface = @"tpanel.TP4";

    return QString::fromNSString(netlinx_surface);
}

bool QASettings::getFTPPassive()
{
    return getDefaultBool((char *)"netlinx_ftp_passive", true);
}

// Settings for SIP

QString QASettings::getSipProxy(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sip_proxy"];
    return QString::fromNSString(str);
}

int QASettings::getSipNetworkPort(void)
{
    return getDefaultNumber((char *)"sip_port", 5060);
}

int QASettings::getSipNetworkTlsPort(void)
{
    NSInteger number = [[ NSUserDefaults standardUserDefaults] integerForKey:@"sip_tsl_port"];
    return (int)number;
}

QString QASettings::getSipStun(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sip_stun"];
    return QString::fromNSString(str);
}

QString QASettings::getSipDomain(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sip_domain"];
    return QString::fromNSString(str);
}

QString QASettings::getSipUser(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sip_user"];
    return QString::fromNSString(str);
}

QString QASettings::getSipPassword(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sip_password"];
    return QString::fromNSString(str);
}

bool QASettings::getSipNetworkIpv4(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"sip_ipv4"];
    return boolean;
}

bool QASettings::getSipNetworkIpv6(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"sip_ipv6"];
    return boolean;
}

bool QASettings::getSipEnabled(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"sip_enabled"];
    return boolean;
}
bool QASettings::getSipIntegratedPhone(void)
{
    return getDefaultBool((char *)"sip_internal_phone", true);
}

// Settings for View

bool QASettings::getViewScale(void)
{
    return getDefaultBool((char *)"view_scale", true);
}

bool QASettings::getViewToolbarVisible(void)
{
    return getDefaultBool((char *)"view_toolbar", true);
}

bool QASettings::getViewToolbarForce(void)
{
    return getDefaultBool((char *)"view_toolbar_force", true);
}

bool QASettings::getViewRotation(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"view_rotation"];
    return boolean;
}

// Settings for sound

QString QASettings::getSoundSystem(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_system"];

    if (!str)
        return "singleBeep.wav";

    return QString::fromNSString(str);
}

QString QASettings::getSoundSingleBeep(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_single"];

    if (!str)
        return "singleBeep.wav";

    return QString::fromNSString(str);
}

QString QASettings::getSoundDoubleBeep(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_double"];

    if (!str)
        return "doubleBeep.wav";

    return QString::fromNSString(str);
}

bool QASettings::getSoundEnabled(void)
{
    return getDefaultBool((char *)"sound_enable", true);
}

int QASettings::getSoundVolume(void)
{
    return getDefaultNumber((char *)"sound_volume", 100);
}

int QASettings::getSoundGain(void)
{
    return getDefaultNumber((char *)"sound_gain", 100);
}

// Settings for logging

bool QASettings::getLoggingInfo()
{
    NSInteger log_info = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_info"];
    return log_info;
}

bool QASettings::getLoggingWarning()
{
    NSInteger log_warning = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_warning"];
    return log_warning;
}

bool QASettings::getLoggingError()
{
    NSInteger log_error = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_error"];
    return log_error;
}

bool QASettings::getLoggingTrace()
{
    NSInteger log_trace = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_trace"];
    return log_trace;
}

bool QASettings::getLoggingDebug()
{
    NSInteger log_debug = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_debug"];
    return log_debug;
}

bool QASettings::getLoggingProfile()
{
    NSInteger log_profile = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_profile"];
    return log_profile;
}

bool QASettings::getLoggingLogFormat()
{
    NSInteger log_format = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_long_format"];
    return log_format;
}

bool QASettings::getLoggingLogfileEnabled()
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"logging_logfile_enabled"];
    return boolean;
}

QString QASettings::getLoggingLogfile()
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"logging_logfile"];

    if (!str)
        return "tpanel.log";

    return QString::fromNSString(str);
}

QString QASettings::getPassword1()
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_1"];

    if (!str)
        return "1988";

    return QString::fromNSString(str);
}

QString QASettings::getPassword2()
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_2"];

    if (!str)
        return "1988";

    return QString::fromNSString(str);
}

QString QASettings::getPassword3()
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_3"];

    if (!str)
        return "1988";

    return QString::fromNSString(str);
}

QString QASettings::getPassword4()
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"password_4"];

    if (!str)
        return "1988";

    return QString::fromNSString(str);
}

// Static methods
QString QASettings::getLibraryPath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *libraryDirectory = [paths objectAtIndex:0];
    return QString::fromNSString(libraryDirectory);
}

QString QASettings::getDocumentPath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentDirectory = [paths objectAtIndex:0];
    return QString::fromNSString(documentDirectory);
}

QMargins QASettings::getNotchSize()
{
    DECL_TRACER("QASettings::getNotchSize()");

    NSArray *scenes = [[[UIApplication sharedApplication] connectedScenes] allObjects];

    if (scenes == nil || scenes.count <= 0)
    {
        MSG_WARNING("Found no scenes. Can't get the size of the notch, if there is any.");
        return QMargins();
    }

    NSArray<UIWindow *> *windows = [[scenes objectAtIndex:0] windows];

    if (windows == nil || windows.count <= 0)
    {
        MSG_WARNING("Found no windows! Can't calculate notch size.");
        return QMargins();
    }

    UIWindow* window = nil;

    for (UIWindow *w in windows)
    {
        if (w.isKeyWindow)
        {
            window = w;
            break;
        }
    }

    if (window == nil)
    {
        MSG_WARNING("No key window detected! Will take 1st window instead ...");
        window = [windows objectAtIndex:0];

        if (window == nil)
        {
            MSG_WARNING("No window found! Can't get the notch size, if any.");
            return QMargins();
        }

        MSG_DEBUG("Found first window to be a valid one.");
    }

    QMargins rect;

    float reservedTop = window.safeAreaInsets.top;
    float reservedBottom = window.safeAreaInsets.bottom;
    float reservedLeft = window.safeAreaInsets.left;
    float reservedRight = window.safeAreaInsets.right;

    rect.setLeft(reservedLeft);
    rect.setTop(reservedTop);
    rect.setBottom(reservedBottom);
    rect.setRight(reservedRight);
    return rect;
}

bool QASettings::getDefaultBool(char *key, bool def)
{
    NSString *nsKey = [[NSString alloc] initWithUTF8String: key];

    if (!nsKey)
        return def;

    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    id dataExists = [userDefaults objectForKey:nsKey];

    if (dataExists != nil)
    {
        bool ret = [[NSUserDefaults standardUserDefaults] boolForKey:nsKey];
        [nsKey release];
        return ret;
    }

    [nsKey release];
    return def;
}

int QASettings::getDefaultNumber(char *key, int def)
{
    NSString *nsKey = [[NSString alloc] initWithUTF8String: key];

    if (!nsKey)
        return def;

    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    id dataExists = [userDefaults objectForKey:nsKey];

    if (dataExists != nil)
    {
        long ret = [[NSUserDefaults standardUserDefaults] integerForKey:nsKey];
        [nsKey release];
        return (int)ret;
    }

    [nsKey release];
    return def;
}

void QASettings::openSettings()
{
    NSURL *url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];

    if (!url)
        return;

    // Save current settings
    oldNetlinx = TConfig::getController();
    oldPort = TConfig::getPort();
    oldChannelID = TConfig::getChannel();
    oldSurface = TConfig::getFtpSurface();
    oldToolbarSuppress = TConfig::getToolbarSuppress();
    oldToolbarForce = TConfig::getToolbarForce();
    // Launch the setup
    UIApplication *application = [UIApplication sharedApplication];
    [application openURL:url options:@{} completionHandler:nil];
}

void QASettings::writeLog(int type, const std::string& msg)
{
    os_log_type_t dbType = OS_LOG_TYPE_DEBUG;

    switch (type)
    {
        case TERRINFO:      dbType = OS_LOG_TYPE_INFO; break;
        case TERRWARNING:   dbType = OS_LOG_TYPE_FAULT; break;
        case TERRERROR:     dbType = OS_LOG_TYPE_ERROR; break;
        case TERRDEBUG:     dbType = OS_LOG_TYPE_DEBUG; break;
        default:
            dbType = OS_LOG_TYPE_DEFAULT;
    }

    if (TPANEL_LOG == nil)
        TPANEL_LOG = os_log_create("at.theosys.tpanel", "tpanel");

    if (os_log_type_enabled(TPANEL_LOG, dbType))
        os_log_with_type(TPANEL_LOG, dbType, "%{public}s", msg.c_str());
    else
        NSLog(@"(tpanel) %s", msg.c_str());

}

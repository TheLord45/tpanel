/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include "QASettings.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <QtCore>
#include "terror.h"

void QASettings::registerDefaultPrefs()
{
    NSDictionary* appDefaultsRoot = [NSDictionary dictionaryWithObject:@"Root" forKey:@"StringsTable"];
    NSDictionary* appDefaultsSip = [NSDictionary dictionaryWithObject:@"SIP" forKey:@"StringsTable"];
    NSDictionary* appDefaultsView = [NSDictionary dictionaryWithObject:@"View" forKey:@"StringsTable"];
    NSDictionary* appDefaultsSound = [NSDictionary dictionaryWithObject:@"Sound" forKey:@"StringsTable"];
    NSDictionary* appDefaultsLog = [NSDictionary dictionaryWithObject:@"Logging" forKey:@"StringsTable"];

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
    NSInteger netlinx_port = [[ NSUserDefaults standardUserDefaults] integerForKey:@"netlinx_port"];

    if (netlinx_port == 0)
        netlinx_port = 1319;

    return netlinx_port;
}

int QASettings::getNetlinxChannel()
{
    NSInteger netlinx_channel = [[ NSUserDefaults standardUserDefaults] integerForKey:@"netlinx_channel"];

    if (netlinx_channel == 0)
        netlinx_channel = 10001;

    return netlinx_channel;
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
    NSInteger ftp_passive = [[ NSUserDefaults standardUserDefaults] boolForKey:@"netlinx_ftp_passive"];
    return ftp_passive;
}

// Settings for SIP

QString QASettings::getSipProxy(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sip_proxy"];
    return QString::fromNSString(str);
}

int QASettings::getSipNetworkPort(void)
{
    NSInteger number = [[ NSUserDefaults standardUserDefaults] integerForKey:@"sip_port"];

    if (number <= 0)
        number = 5060;

    return number;
}

int QASettings::getSipNetworkTlsPort(void)
{
    NSInteger number = [[ NSUserDefaults standardUserDefaults] integerForKey:@"sip_tsl_port"];
    return number;
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
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"sip_internal_phone"];
    return boolean;
}

// Settings for View

bool QASettings::getViewScale(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"view_scale"];
    return boolean;
}

bool QASettings::getViewToolbarVisible(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"view_toolbar"];
    return boolean;
}

bool QASettings::getViewToolbarForce(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"view_toolbar_force"];
    return boolean;
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
    return QString::fromNSString(str);
}

QString QASettings::getSoundSingleBeep(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_single"];
    return QString::fromNSString(str);
}

QString QASettings::getSoundDoubleBeep(void)
{
    NSString *str = [[ NSUserDefaults standardUserDefaults] stringForKey:@"sound_double"];
    return QString::fromNSString(str);
}

bool QASettings::getSoundEnabled(void)
{
    NSInteger boolean = [[ NSUserDefaults standardUserDefaults] boolForKey:@"sound_enable"];
    return boolean;
}

int QASettings::getSoundVolume(void)
{
    NSInteger number = [[ NSUserDefaults standardUserDefaults] integerForKey:@"sound_volume"];
    return number;
}

int QASettings::getSoundGain(void)
{
    NSInteger number = [[ NSUserDefaults standardUserDefaults] integerForKey:@"sound_gain"];
    return number;
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

QRect QASettings::getNotchSize()
{
    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
    QRect rect;

    float reservedTop = window.safeAreaInsets.top;
    float reservedBottom = [[[UIApplication sharedApplication] keyWindow] safeAreaInsets].bottom;
    float reservedLeft = window.safeAreaInsets.left;
    float reservedRight = window.safeAreaInsets.right;
MSG_DEBUG("Notch top: " << reservedTop << ", bottom: " << reservedBottom << ", left: " << reservedLeft << ", right: " << reservedRight);
    rect.setLeft(reservedLeft);
    rect.setTop(reservedTop);
    rect.setBottom(reservedBottom);
    rect.setRight(reservedRight);
    return rect;
}

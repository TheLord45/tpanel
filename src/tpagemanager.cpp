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

#include <QtGlobal>

#include <vector>
#include <thread>
#include <mutex>

#ifdef __ANDROID__
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#       include <QtAndroidExtras/QAndroidJniObject>
#       include <QtAndroidExtras/QtAndroid>
#   else
#       include <QJniObject>
#       include <QCoreApplication>
#   endif
#   include <android/log.h>
#endif
#include <unistd.h>
#ifndef __ANDROID__
#   include <fstab.h>
#endif

#if __cplusplus < 201402L
#   error "This module requires at least C++14 standard!"
#else
#   if __cplusplus < 201703L
#       include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#       warning "Support for C++14 and experimental filesystem will be removed in a future version!"
#   else
#       include <filesystem>
#       ifdef __ANDROID__
namespace fs = std::__fs::filesystem;
#       else
namespace fs = std::filesystem;
#       endif
#   endif
#endif

#ifdef __APPLE__
#   include <unistd.h>
#   ifndef HOST_NAME_MAX
#       ifdef MAXHOSTNAMELEN
#           define HOST_NAME_MAX    MAXHOSTNAMELEN
#       else
#           define HOST_NAME_MAX    64
#       endif   // MAXHOSTNAMELEN
#   endif       // HOST_NAME_MAX
#endif          // __APPLE__

#include "tpagemanager.h"
#include "tcolor.h"
#include "terror.h"
#include "ticons.h"
#include "tbutton.h"
#include "tprjresources.h"
#include "tresources.h"
#include "tresources.h"
#include "tsystemsound.h"
#include "tvalidatefile.h"
#include "ttpinit.h"
#include "tconfig.h"
#include "tlock.h"
#include "tintborder.h"
#ifdef Q_OS_IOS
#include "ios/tiosbattery.h"
#endif
#if TESTMODE == 1
#include "testmode.h"
#endif

using std::vector;
using std::string;
using std::map;
using std::pair;
using std::to_string;
using std::thread;
using std::atomic;
using std::mutex;
using std::bind;

TIcons *gIcons = nullptr;
TPrjResources *gPrjResources = nullptr;
TPageManager *gPageManager = nullptr;
//std::vector<amx::ANET_COMMAND> TPageManager::mCommands;

extern amx::TAmxNet *gAmxNet;
extern std::atomic<bool> _netRunning;
extern bool _restart_;                          //!< If this is set to true then the whole program will start over.

bool prg_stopped = false;

#ifdef __ANDROID__
string javaJStringToString(JNIEnv *env, jstring str)
{
    if (!str)
        return string();

    const jclass stringClass = env->GetObjectClass(str);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(str, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_BatteryState_informBatteryStatus(JNIEnv *, jclass, jint level, jboolean charging, jint chargeType)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_BatteryState_informBatteryStatus(JNIEnv *, jclass, jint level, jboolean charging, jint chargeType)");

    if (gPageManager)
        gPageManager->informBatteryStatus(level, charging, chargeType);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_NetworkStatus_informTPanelNetwork(JNIEnv */*env*/, jclass /*clazz*/, jboolean conn, jint level, jint type)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_NetworkStatus_informTPanelNetwork(JNIEnv */*env*/, jclass /*clazz*/, jboolean conn, jint level, jint type)");

    //Call native side conterpart
    if (gPageManager)
        gPageManager->informTPanelNetwork(conn, level, type);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_PhoneCallState_informPhoneState(JNIEnv *env, jclass, jboolean call, jstring pnumber)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_PhoneCallState_informPhoneState(JNIEnv *env, jclass, jboolean call, jstring pnumber)");

    string phoneNumber;

    if (pnumber)
        phoneNumber = javaJStringToString(env, pnumber);

    if (gPageManager)
        gPageManager->informPhoneState(call, phoneNumber);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_Logger_logger(JNIEnv *env, jclass, jint mode, jstring msg)
{
    if (!msg)
        return;

    string ret = javaJStringToString(env, msg);

    try
    {
        if (TStreamError::checkFilter(mode))
        {
            *TError::Current()->getStream() << TError::append(mode, __LINE__, __FILE__) << ret << std::endl;
            TStreamError::resetFlags();
        }
    }
    catch (std::exception& e)
    {
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "%s", e.what());
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_Orientation_informTPanelOrientation(JNIEnv */*env*/, jclass /*clazz*/, jint orientation)
{
    DECL_TRACER("Java_org_qtproject_theosys_Orientation_informTPanelOrientation(JNIEnv */*env*/, jclass /*clazz*/, jint orientation)");

    if (!gPageManager)
        return;

    if (gPageManager->onOrientationChange())
        gPageManager->onOrientationChange()(orientation);

    gPageManager->setOrientation(orientation);

    if (gPageManager->getInformOrientation())
        gPageManager->sendOrientation();
}

/* -------- Settings -------- */

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_saveSettings(JNIEnv *env, jclass clazz)
{
    DECL_TRACER("Java_org_qtproject_theosys_SettingsActivity_saveSettings(JNIEnv *env, jclass clazz)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    TConfig::setTemporary(true);
    string oldNetlinx = TConfig::getController();
    int oldPort = TConfig::getPort();
    int oldChannelID = TConfig::getChannel();
    string oldSurface = TConfig::getFtpSurface();
    bool oldToolbarSuppress = TConfig::getToolbarSuppress();
    bool oldToolbarForce = TConfig::getToolbarForce();
    TConfig::setTemporary(false);
    MSG_DEBUG("Old values:\n" <<
              "   NetLinx: " << oldNetlinx << "\n" <<
              "   Port:    " << oldPort << "\n" <<
              "   Channel: " << oldChannelID << "\n" <<
              "   Surface: " << oldSurface << "\n" <<
              "   TB suppr:" << oldToolbarSuppress << "\n" <<
              "   TB force:" << oldToolbarForce);
    TConfig::saveSettings();

    MSG_DEBUG("New values:\n" <<
              "   NetLinx: " << TConfig::getController() << "\n" <<
              "   Port:    " << TConfig::getPort() << "\n" <<
              "   Channel: " << TConfig::getChannel() << "\n" <<
              "   Surface: " << TConfig::getFtpSurface() << "\n" <<
              "   TB suppr:" << TConfig::getToolbarSuppress() << "\n" <<
              "   TB force:" << TConfig::getToolbarForce());

    if (gPageManager && gPageManager->onSettingsChanged())
        gPageManager->onSettingsChanged()(oldNetlinx, oldPort, oldChannelID, oldSurface, oldToolbarSuppress, oldToolbarForce);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxIp(JNIEnv *env, jclass clazz, jstring ip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxIp(JNIEnv *env, jclass clazz, jstring ip)");

    Q_UNUSED(clazz);

    string netlinxIp = javaJStringToString(env, ip);

    if (TConfig::getController() != netlinxIp)
        TConfig::saveController(netlinxIp);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxPort(JNIEnv *env, jclass clazz, jint port)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxPort(JNIEnv *env, jclass clazz, jint port)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (port > 0 && port < 65535 && TConfig::getPort() != port)
        TConfig::savePort(port);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxChannel(JNIEnv *env, jclass clazz, jint channel)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxChannel(JNIEnv *env, jclass clazz, jint channel)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (channel >= 10000 && channel < 20000 && TConfig::getChannel() != channel)
        TConfig::saveChannel(channel);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxType(JNIEnv *env, jclass clazz, jstring type)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxType(JNIEnv *env, jclass clazz, jstring type)");

    Q_UNUSED(clazz);

    string netlinxType = javaJStringToString(env, type);

    if (TConfig::getPanelType() != netlinxType)
        TConfig::savePanelType(netlinxType);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxFtpUser(JNIEnv *env, jclass clazz, jstring user)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxFtpUser(JNIEnv *env, jclass clazz, jstring user)");

    Q_UNUSED(clazz);

    string netlinxFtpUser = javaJStringToString(env, user);

    if (TConfig::getFtpUser() != netlinxFtpUser)
        TConfig::saveFtpUser(netlinxFtpUser);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxFtpPassword(JNIEnv *env, jclass clazz, jstring pw)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxFtpPassword(JNIEnv *env, jclass clazz, jstring pw)");

    Q_UNUSED(clazz);

    string netlinxPw = javaJStringToString(env, pw);

    if (TConfig::getFtpPassword() != netlinxPw)
        TConfig::saveFtpPassword(netlinxPw);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxSurface(JNIEnv *env, jclass clazz, jstring surface)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxIp(JNIEnv *env, jclass clazz, jstring surface)");

    Q_UNUSED(clazz);

    string netlinxSurface = javaJStringToString(env, surface);

    if (TConfig::getFtpSurface() != netlinxSurface)
        TConfig::saveFtpSurface(netlinxSurface);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setNetlinxFtpPassive(JNIEnv *env, jclass clazz, jboolean passive)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setNetlinxIp(JNIEnv *env, jclass clazz, jboolean passive)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getFtpPassive() != passive)
        TConfig::saveFtpPassive(passive);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewScale(JNIEnv *env, jclass clazz, jboolean scale)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setViewScale(JNIEnv *env, jclass clazz, jboolean scale)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getScale() != scale)
        TConfig::saveScale(scale);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewToolbar(JNIEnv *env, jclass clazz, jboolean bar)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setViewToolbar(JNIEnv *env, jclass clazz, jboolean bar)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getToolbarSuppress() == bar)
        TConfig::saveToolbarSuppress(!bar);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewToolbarForce(JNIEnv *env, jclass clazz, jboolean bar)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setViewToolbarForce(JNIEnv *env, jclass clazz, jboolean bar)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getToolbarForce() != bar)
        TConfig::saveToolbarForce(bar);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setViewRotation(JNIEnv *env, jclass clazz, jboolean rotate)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setViewRotation(JNIEnv *env, jclass clazz, jboolean rotate)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getRotationFixed() != rotate)
        TConfig::setRotationFixed(rotate);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundSystem(JNIEnv *env, jclass clazz, jstring sound)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSoundSystem(JNIEnv *env, jclass clazz, jstring sound)");

    Q_UNUSED(clazz);

    string s = javaJStringToString(env, sound);

    if (TConfig::getSystemSound() != s)
        TConfig::saveSystemSoundFile(s);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundSingle(JNIEnv *env, jclass clazz, jstring sound)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSoundSingle(JNIEnv *env, jclass clazz, jstring sound)");

    Q_UNUSED(clazz);

    string s = javaJStringToString(env, sound);

    if (TConfig::getSingleBeepSound() != s)
        TConfig::saveSingleBeepFile(s);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundDouble(JNIEnv *env, jclass clazz, jstring sound)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSoundDouble(JNIEnv *env, jclass clazz, jstring sound)");

    Q_UNUSED(clazz);

    string s = javaJStringToString(env, sound);

    if (TConfig::getDoubleBeepSound() != s)
        TConfig::saveDoubleBeepFile(s);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundEnable(JNIEnv *env, jclass clazz, jboolean sound)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSoundEnable(JNIEnv *env, jclass clazz, jboolean sound)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSystemSoundState() != sound)
        TConfig::saveSystemSoundState(sound);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundVolume(JNIEnv *env, jclass clazz, jint sound)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSoundVolume(JNIEnv *env, jclass clazz, jint sound)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSystemVolume() != sound)
        TConfig::saveSystemVolume(sound);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSoundGain(JNIEnv *env, jclass clazz, jint sound)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSoundGain(JNIEnv *env, jclass clazz, jint sound)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSystemGain() != sound)
        TConfig::saveSystemGain(sound);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipProxy(JNIEnv *env, jclass clazz, jstring sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipProxy(JNIEnv *env, jclass clazz, jstring sip)");

    Q_UNUSED(clazz);

    string sipStr = javaJStringToString(env, sip);

    if (TConfig::getSIPproxy() != sipStr)
        TConfig::setSIPproxy(sipStr);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipPort(JNIEnv *env, jclass clazz, jint sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipPort(JNIEnv *env, jclass clazz, jint sip)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSIPport() != sip)
        TConfig::setSIPport(sip);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipTlsPort(JNIEnv *env, jclass clazz, jint sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipTlsPort(JNIEnv *env, jclass clazz, jint sip)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSIPportTLS() != sip)
        TConfig::setSIPportTLS(sip);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipStun(JNIEnv *env, jclass clazz, jstring sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipStun(JNIEnv *env, jclass clazz, jstring sip)");

    Q_UNUSED(clazz);

    string sipStr = javaJStringToString(env, sip);

    if (TConfig::getSIPstun() != sipStr)
        TConfig::setSIPstun(sipStr);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipDomain(JNIEnv *env, jclass clazz, jstring sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipDomain(JNIEnv *env, jclass clazz, jstring sip)");

    Q_UNUSED(clazz);

    string sipStr = javaJStringToString(env, sip);

    if (TConfig::getSIPdomain() != sipStr)
        TConfig::setSIPdomain(sipStr);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipUser(JNIEnv *env, jclass clazz, jstring sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipUser(JNIEnv *env, jclass clazz, jstring sip)");

    Q_UNUSED(clazz);

    string sipStr = javaJStringToString(env, sip);

    if (TConfig::getSIPuser() != sipStr)
        TConfig::setSIPuser(sipStr);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipPassword(JNIEnv *env, jclass clazz, jstring sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipPassword(JNIEnv *env, jclass clazz, jstring sip)");

    Q_UNUSED(clazz);

    string sipStr = javaJStringToString(env, sip);

    if (TConfig::getSIPpassword() != sipStr)
        TConfig::setSIPpassword(sipStr);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipIpv4(JNIEnv *env, jclass clazz, jboolean sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipIpv4(JNIEnv *env, jclass clazz, jboolean sip)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSIPnetworkIPv4() != sip)
        TConfig::setSIPnetworkIPv4(sip);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipIpv6(JNIEnv *env, jclass clazz, jboolean sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipIpv6(JNIEnv *env, jclass clazz, jboolean sip)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSIPnetworkIPv6() != sip)
        TConfig::setSIPnetworkIPv6(sip);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipEnabled(JNIEnv *env, jclass clazz, jboolean sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipEnabled(JNIEnv *env, jclass clazz, jboolean sip)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSIPstatus() != sip)
        TConfig::setSIPstatus(sip);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setSipIphone(JNIEnv *env, jclass clazz, jboolean sip)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setSipIphone(JNIEnv *env, jclass clazz, jboolean sip)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getSIPiphone() != sip)
        TConfig::setSIPiphone(sip);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogInfo(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogInfo(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    uint logSwitch = (log ? HLOG_INFO : 0);

    if ((TConfig::getLogLevelBits() & HLOG_INFO) != logSwitch)
    {
        if (!(TConfig::getLogLevelBits() & HLOG_INFO))
            TConfig::saveLogLevel(TConfig::getLogLevelBits() | HLOG_INFO);
        else
            TConfig::saveLogLevel(TConfig::getLogLevelBits() ^ HLOG_INFO);
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogWarning(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogWarning(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    uint logSwitch = (log ? HLOG_WARNING : 0);

    if ((TConfig::getLogLevelBits() & HLOG_WARNING) != logSwitch)
    {
        if (!(TConfig::getLogLevelBits() & HLOG_INFO))
            TConfig::saveLogLevel(TConfig::getLogLevelBits() | HLOG_WARNING);
        else
            TConfig::saveLogLevel(TConfig::getLogLevelBits() ^ HLOG_WARNING);
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogError(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogError(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    uint logSwitch = (log ? HLOG_ERROR : 0);

    if ((TConfig::getLogLevelBits() & HLOG_ERROR) != logSwitch)
    {
        if (!(TConfig::getLogLevelBits() & HLOG_ERROR))
            TConfig::saveLogLevel(TConfig::getLogLevelBits() | HLOG_ERROR);
        else
            TConfig::saveLogLevel(TConfig::getLogLevelBits() ^ HLOG_ERROR);
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogTrace(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogTrace(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    uint logSwitch = (log ? HLOG_TRACE : 0);

    if ((TConfig::getLogLevelBits() & HLOG_TRACE) != logSwitch)
    {
        if (!(TConfig::getLogLevelBits() & HLOG_TRACE))
            TConfig::saveLogLevel(TConfig::getLogLevelBits() | HLOG_TRACE);
        else
            TConfig::saveLogLevel(TConfig::getLogLevelBits() ^ HLOG_TRACE);
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogDebug(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogDebug(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    uint logSwitch = (log ? HLOG_DEBUG : 0);

    if ((TConfig::getLogLevelBits() & HLOG_DEBUG) != logSwitch)
    {
        if (!(TConfig::getLogLevelBits() & HLOG_DEBUG))
            TConfig::saveLogLevel(TConfig::getLogLevelBits() | HLOG_DEBUG);
        else
            TConfig::saveLogLevel(TConfig::getLogLevelBits() ^ HLOG_DEBUG);
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogProfile(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogProfile(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::getProfiling() != log)
        TConfig::saveProfiling(log);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogLongFormat(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogLongFormat(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    if (TConfig::isLongFormat() != log)
        TConfig::saveFormat(log);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogEnableFile(JNIEnv *env, jclass clazz, jboolean log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogEnableFile(JNIEnv *env, jclass clazz, jboolean log)");

    Q_UNUSED(env);
    Q_UNUSED(clazz);

    TConfig::setLogFileEnabled(log);
    TStreamError::setLogFileEnabled(log);
    string logFile = TConfig::getLogFile();

    if (log && !logFile.empty() && fs::is_regular_file(logFile))
        TStreamError::setLogFile(logFile);
    else if (!log)
        TStreamError::setLogFile("");

    __android_log_print(ANDROID_LOG_DEBUG, "tpanel", "JAVA::setLogEnableFile: Logfile was %s", (log ? "ENABLED" : "DISABLED"));
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setLogFile(JNIEnv *env, jclass clazz, jstring log)
{
    DECL_TRACER("Java_org_qtproject_theosys_Settings_setLogFile(JNIEnv *env, jclass clazz, jstring sip)");

    Q_UNUSED(clazz);

    string logStr = javaJStringToString(env, log);

    if (TConfig::getLogFile() != logStr)
    {
        TConfig::saveLogFile(logStr);
        __android_log_print(ANDROID_LOG_DEBUG, "tpanel", "JAVA::setLogFile: Logfile set to: %s", logStr.c_str());

        if (fs::is_regular_file(logStr))
            TStreamError::setLogFile(logStr);
        else
        {
            TStreamError::setLogFile("");
            __android_log_print(ANDROID_LOG_WARN, "tpanel", "JAVA::setLogFile: Logfile \"%s\" is not accessible!", logStr.c_str());
        }
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword1(JNIEnv *env, jclass clazz, jstring pw)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword1(JNIEnv *env, jclass clazz, jstring pw)");

    Q_UNUSED(clazz);

    string password = javaJStringToString(env, pw);
    TConfig::savePassword1(password);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword2(JNIEnv *env, jclass clazz, jstring pw)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword2(JNIEnv *env, jclass clazz, jstring pw)");

    Q_UNUSED(clazz);

    string password = javaJStringToString(env, pw);
    TConfig::savePassword2(password);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword3(JNIEnv *env, jclass clazz, jstring pw)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword3(JNIEnv *env, jclass clazz, jstring pw)");

    Q_UNUSED(clazz);

    string password = javaJStringToString(env, pw);
    TConfig::savePassword3(password);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword4(JNIEnv *env, jclass clazz, jstring pw)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_SettingsActivity_setPassword4(JNIEnv *env, jclass clazz, jstring pw)");

    Q_UNUSED(clazz);

    string password = javaJStringToString(env, pw);
    TConfig::savePassword4(password);
}
#endif

TPageManager::TPageManager()
{
    TLOCKER(surface_mutex);
    DECL_TRACER("TPageManager::TPageManager()");

    gPageManager = this;
    TTPInit *tinit = new TTPInit;
    string projectPath = TConfig::getProjectPath();
    string pp = projectPath + "/prj.xma";

    tinit->setPath(projectPath);
    bool haveSurface = false;

    if (tinit->isVirgin())
        haveSurface = tinit->loadSurfaceFromController();
    else
        haveSurface = true;

    if (!haveSurface)
    {
        if (!isValidFile(pp))
            tinit->reinitialize();
    }
    else
        tinit->makeSystemFiles();

    delete tinit;

    // Read the AMX panel settings.
    mTSettings = new TSettings(projectPath);

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_ERROR("Settings were not read successfull!");
        return;
    }

    // Read the application file if it is G5
    if (mTSettings->isG5())
    {
        TTPInit::setG5(true);
        mApps = new TApps;
        mApps->parseApps();

        if (TError::isError())
        {
            PRINT_LAST_ERROR();
            MSG_WARNING("Apps list was not read successfully!");
        }
    }

    // Set the panel type from the project information
    TConfig::savePanelType(mTSettings->getPanelType());

    readMap(mTSettings->isG5());  // Start the initialisation of the AMX part.

    gPrjResources = new TPrjResources(mTSettings->getResourcesList());
    mPalette = new TPalette(mTSettings->isG5());
    vector<PALETTE_SETUP> pal = mTSettings->getSettings().palettes;

    if (pal.size() > 0)
    {
        vector<PALETTE_SETUP>::iterator iterPal;

        for (iterPal = pal.begin(); iterPal != pal.end(); ++iterPal)
            mPalette->initialize(iterPal->file);
    }

    if (!TError::isError())
        TColor::setPalette(mPalette);

    mFonts = new TFont(mTSettings->getFontFileName(), mTSettings->isG5());

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_ERROR("Initializing fonts was not successfull!");
    }

    if (!mTSettings->isG5())
    {
        gIcons = new TIcons();

        if (TError::isError())
        {
            PRINT_LAST_ERROR();
            MSG_ERROR("Initializing icons was not successfull!");
        }
    }

    mPageList = new TPageList();
    mExternal = new TExternal();
    PAGELIST_T page;

    if (!mTSettings->getSettings().powerUpPage.empty())
    {
        if (readPage(mTSettings->getSettings().powerUpPage))
        {
            MSG_TRACE("Found power up page " << mTSettings->getSettings().powerUpPage);
            page = findPage(mTSettings->getSettings().powerUpPage);
            mActualPage = page.pageID;
        }
    }
    else
    {
        MSG_WARNING("No power up page defined! Setting default page to 1.");
        mActualPage = 1;
    }

    TPage *pg = getPage(mActualPage);

    vector<string> popups = mTSettings->getSettings().powerUpPopup;

    if (popups.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = popups.begin(); iter != popups.end(); ++iter)
        {
            if (readSubPage(*iter))
            {
                MSG_TRACE("Found power up popup " << *iter);

                if (pg)
                {
                    TSubPage *spage = getSubPage(*iter);
                    spage->setParent(pg->getHandle());
                    pg->addSubPage(spage);
                }
            }
        }
    }

    // Here we initialize the system resources like borders, cursors, sliders, ...
    mSystemDraw = new TSystemDraw(TConfig::getSystemPath(TConfig::BASE));

    // Here are the commands supported by this emulation.
    MSG_INFO("Registering commands ...");
    REG_CMD(doLEVON, "LEVON");  // Enable device to send level changes to the master.
    REG_CMD(doLEVOF, "LEVOF");  // Disable the device from sending level changes to the master.
    REG_CMD(doRXON, "RXON");    // Enable device to send STRING changes to the master.
    REG_CMD(doRXOF, "RXOF");    // Disable the device from sending STRING changes to the master.
    REG_CMD(doAFP, "@AFP");     // Flips to a page with the specified page name using an animated transition.
    REG_CMD(doAFP, "^AFP");     // Flips to a page with the specified page name using an animated transition.
    REG_CMD(doAPG, "@APG");     // Add a specific popup page to a specified popup group.
    REG_CMD(doCPG, "@CPG");     // Clear all popup pages from specified popup group.
    REG_CMD(doDPG, "@DPG");     // Delete a specific popup page from specified popup group if it exists
//    REG_CMD(doPDR, "@PDR");     // Set the popup location reset flag.
    REG_CMD(doPHE, "@PHE");     // Set the hide effect for the specified popup page to the named hide effect.
    REG_CMD(doPHP, "@PHP");     // Set the hide effect position.
    REG_CMD(doPHT, "@PHT");     // Set the hide effect time for the specified popup page.
    REG_CMD(doPPA, "@PPA");     // Close all popups on a specified page.
    REG_CMD(doPPA, "^PPA");     // G5: Close all popups on a specified page.
    REG_CMD(doPPF, "@PPF");     // Deactivate a specific popup page on either a specified page or the current page.
    REG_CMD(doPPF, "^PPF");     // G5: Deactivate a specific popup page on either a specified page or the current page.
    REG_CMD(doPPF, "PPOF");     // Deactivate a specific popup page on either a specified page or the current page
    REG_CMD(doPPG, "@PPG");     // Toggle a specific popup page on either a specified page or the current page.
    REG_CMD(doPPG, "^PPG");     // G5: Toggle a specific popup page on either a specified page or the current page.
    REG_CMD(doPPG, "PPOG");     // Toggle a specific popup page on either a specified page or the current page.
    REG_CMD(doPPK, "@PPK");     // Kill a specific popup page from all pages.
    REG_CMD(doPPK, "^PPK");     // G5: Kill a specific popup page from all pages.
    REG_CMD(doPPM, "@PPM");     // Set the modality of a specific popup page to Modal or NonModal.
    REG_CMD(doPPM, "^PPM");     // G5: Set the modality of a specific popup page to Modal or NonModal.
    REG_CMD(doPPN, "@PPN");     // Activate a specific popup page to launch on either a specified page or the current page.
    REG_CMD(doPPN, "^PPN");     // G5: Activate a specific popup page to launch on either a specified page or the current page.
    REG_CMD(doPPN, "PPON");     // Activate a specific popup page to launch on either a specified page or the current page.
    REG_CMD(doPPT, "@PPT");     // Set a specific popup page to timeout within a specified time.
    REG_CMD(doPPT, "^PPT");     // G5: Set a specific popup page to timeout within a specified time.
    REG_CMD(doPPX, "@PPX");     // Close all popups on all pages.
    REG_CMD(doPPX, "^PPX");     // G5: Close all popups on all pages.
    REG_CMD(doPSE, "@PSE");     // Set the show effect for the specified popup page to the named show effect.
    REG_CMD(doPSP, "@PSP");     // Set the show effect position.
    REG_CMD(doPST, "@PST");     // Set the show effect time for the specified popup page.
    REG_CMD(doPAGE, "PAGE");    // Flip to a specified page.
    REG_CMD(doPAGE, "^PGE");    // G5: Flip to a specified page.
    REG_CMD(doPCL, "^PCL");     // G5: Collapse Collapsible Popup Command
    REG_CMD(doPCT, "^PCT");     // G5: Collapsible Popup Custom Toggle Command
    REG_CMD(doPTC, "^PTC");     // G5: Toggle Collapsible Popup Collapsed Command
    REG_CMD(doPTO, "^PTO");     // G5: Toggle Collapsed Popup Open Command

    REG_CMD(doANI, "^ANI");     // Run a button animation (in 1/10 second).
    REG_CMD(doAPF, "^APF");     // Add page flip action to a button if it does not already exist.
    REG_CMD(doBAT, "^BAT");     // Append non-unicode text.
    REG_CMD(doBAU, "^BAU");     // Append unicode text. Same format as ^UNI.
    REG_CMD(doBCB, "^BCB");     // Set the border color.
    REG_CMD(getBCB, "?BCB");    // Get the current border color.
    REG_CMD(doBCF, "^BCF");     // Set the fill color to the specified color.
    REG_CMD(getBCF, "?BCF");    // Get the current fill color.
    REG_CMD(doBCT, "^BCT");     // Set the text color to the specified color.
    REG_CMD(getBCT, "?BCT");    // Get the current text color.
    REG_CMD(doBDO, "^BDO");     // Set the button draw order
    REG_CMD(doBFB, "^BFB");     // Set the feedback type of the button.
    REG_CMD(doBIM, "^BIM");     // Set the input mask for the specified address
    REG_CMD(doBMC, "^BMC");     // Button copy command.
    REG_CMD(doBMF, "^BMF");     // Button Modify Command - Set any/all button parameters by sending embedded codes and data.
//    REG_CMD(doBMI, "^BMI");    // Set the button mask image.
    REG_CMD(doBML, "^BML");     // Set the maximum length of the text area button.
    REG_CMD(doBMP, "^BMP");     // Assign a picture to those buttons with a defined addressrange.
    REG_CMD(getBMP, "?BMP");    // Get the current bitmap name.
    REG_CMD(doBOP, "^BOP");     // Set the button opacity.
    REG_CMD(getBOP, "?BOP");    // Get the button opacity.
    REG_CMD(doBOR, "^BOR");     // Set a border to a specific border style.
    REG_CMD(doBOS, "^BOS");     // Set the button to display either a Video or Non-Video window.
    REG_CMD(doBRD, "^BRD");     // Set the border of a button state/states.
    REG_CMD(getBRD, "?BRD");    // Get the border of a button state/states.
//    REG_CMD(doBSF, "^BSF");     // Set the focus to the text area.
    REG_CMD(doBSP, "^BSP");     // Set the button size and position.
    REG_CMD(doBSM, "^BSM");     // Submit text for text area buttons.
    REG_CMD(doBSO, "^BSO");     // Set the sound played when a button is pressed.
    REG_CMD(doBWW, "^BWW");     // Set the button word wrap feature to those buttons with a defined address range.
    REG_CMD(getBWW, "?BWW");    // Get the button word wrap feature to those buttons with a defined address range.
    REG_CMD(doCPF, "^CPF");     // Clear all page flips from a button.
    REG_CMD(doDPF, "^DPF");     // Delete page flips from button if it already exists.
    REG_CMD(doENA, "^ENA");     // Enable or disable buttons with a set variable text range.
    REG_CMD(doFON, "^FON");     // Set a font to a specific Font ID value for those buttons with a range.
    REG_CMD(getFON, "?FON");    // Get the current font index.
    REG_CMD(doGDI, "^GDI");     // Change the bargraph drag increment.
    REG_CMD(doGIV, "^GIV");     // Invert the joystick axis to move the origin to another corner.
    REG_CMD(doGLH, "^GLH");     // Change the bargraph upper limit.
    REG_CMD(doGLL, "^GLL");     // Change the bargraph lower limit.
    REG_CMD(doGRD, "^GRD");     // Change the bargraph ramp down time.
    REG_CMD(doGRU, "^GRU");     // Change the bargraph ramp up time.
    REG_CMD(doGSN, "^GSN");     // Set slider/cursor name.
    REG_CMD(doGSC, "^GSC");     // Change the bargraph slider color or joystick cursor color.
    REG_CMD(doICO, "^ICO");     // Set the icon to a button.
    REG_CMD(getICO, "?ICO");    // Get the current icon index.
    REG_CMD(doJSB, "^JSB");     // Set bitmap/picture alignment using a numeric keypad layout for those buttons with a defined address range.
    REG_CMD(getJSB, "?JSB");    // Get the current bitmap justification.
    REG_CMD(doJSI, "^JSI");     // Set icon alignment using a numeric keypad layout for those buttons with a defined address range.
    REG_CMD(getJSI, "?JSI");    // Get the current icon justification.
    REG_CMD(doJST, "^JST");     // Set text alignment using a numeric keypad layout for those buttons with a defined address range.
    REG_CMD(getJST, "?JST");    // Get the current text justification.
    REG_CMD(doMSP, "^MSP");     // Set marquee line speed.
    REG_CMD(doSHO, "^SHO");     // Show or hide a button with a set variable text range.
    REG_CMD(doTEC, "^TEC");     // Set the text effect color for the specified addresses/states to the specified color.
    REG_CMD(getTEC, "?TEC");    // Get the current text effect color.
    REG_CMD(doTEF, "^TEF");     // Set the text effect. The Text Effect is specified by name and can be found in TPD4.
    REG_CMD(getTEF, "?TEF");    // Get the current text effect name.
//    REG_CMD(doTOP, "^TOP");     // Send events to the Master as string events.
    REG_CMD(doTXT, "^TXT");     // Assign a text string to those buttons with a defined address range.
    REG_CMD(getTXT, "?TXT");    // Get the current text information.
    REG_CMD(doUNI, "^UNI");     // Set Unicode text.
    REG_CMD(doUTF, "^UTF");     // G5: Set button state text using UTF-8 text command.
    REG_CMD(doVTP, "^VTP");     // Simulates a touch/release/pulse at the given coordinate

    REG_CMD(doLPB, "^LPB");     // Assigns a user name to a button.
    REG_CMD(doLPC, "^LPC");     // Clear all users from the User Access Passwords list on the Password Setup page.
    REG_CMD(doLPR, "^LPR");     // Remove a given user from the User Access Passwords list on the Password Setup page.
    REG_CMD(doLPS, "^LPS");     // Set the user name and password.

    REG_CMD(doKPS, "^KPS");     // Set the keyboard passthru.
    REG_CMD(doVKS, "^VKS");     // Send one or more virtual key strokes to the G4 application.

    REG_CMD(doAPWD, "@PWD");    // Set the page flip password.
    REG_CMD(doPWD, "^PWD");     // Set the page flip password. Password level is required and must be 1 - 4.

    REG_CMD(doBBR, "^BBR");     // Set the bitmap of a button to use a particular resource.
    REG_CMD(doRAF, "^RAF");     // Add new resources
    REG_CMD(doRFR, "^RFR");     // Force a refresh for a given resource.
    REG_CMD(doRMF, "^RMF");     // Modify an existing resource.
    REG_CMD(doRSR, "^RSR");     // Change the refresh rate for a given resource.

    REG_CMD(doABEEP, "ABEEP");  // Output a single beep even if beep is Off.
    REG_CMD(doADBEEP, "ADBEEP");// Output a double beep even if beep is Off.
    REG_CMD(doAKB, "@AKB");     // Pop up the keyboard icon and initialize the text string to that specified.
    REG_CMD(doAKEYB, "AKEYB");  // Pop up the keyboard icon and initialize the text string to that specified.
    REG_CMD(doAKP, "@AKP");     // Pop up the keypad icon and initialize the text string to that specified.
    REG_CMD(doAKEYP, "AKEYP");  // Pop up the keypad icon and initialize the text string to that specified.
    REG_CMD(doAKEYR, "AKEYR");  // Remove the Keyboard/Keypad.
    REG_CMD(doAKR, "@AKR");     // Remove the Keyboard/Keypad.
    REG_CMD(doBEEP, "BEEP");    // Play a single beep.
    REG_CMD(doBEEP, "^ABP");    // G5: Play a single beep.
    REG_CMD(doDBEEP, "DBEEP");  // Play a double beep.
    REG_CMD(doDBEEP, "^ADB");   // G5: Play a double beep.
    REG_CMD(doEKP, "@EKP");     // Pop up the keypad icon and initialize the text string to that specified.
    REG_CMD(doPKP, "@PKB");     // Present a private keyboard.
    REG_CMD(doPKP, "PKEYP");    // Present a private keypad.
    REG_CMD(doPKP, "@PKP");     // Present a private keypad.
    REG_CMD(doRPP, "^RPP");     // Reset protected password command
    REG_CMD(doSetup, "SETUP");  // Send panel to SETUP page.
    REG_CMD(doSetup, "^STP");   // G5: Open setup page.
    REG_CMD(doShutdown, "SHUTDOWN");// Shut down the App
    REG_CMD(doSOU, "@SOU");     // Play a sound file.
    REG_CMD(doSOU, "^SOU");     // G5: Play a sound file.
    REG_CMD(doMUT, "^MUT");     // G5: Panel Volume Mute
    REG_CMD(doTKP, "@TKP");     // Present a telephone keypad.
    REG_CMD(doTKP, "^TKP");     // G5: Bring up a telephone keypad.
    REG_CMD(doTKP, "@VKB");     // Present a virtual keyboard
    REG_CMD(doTKP, "^VKB");     // G5: Bring up a virtual keyboard.
    // Audio communication
    REG_CMD(getMODEL, "^MODEL?"); // Panel model name.
    REG_CMD(doICS, "^ICS");     // Intercom start
    REG_CMD(doICE, "^ICE");     // Intercom end
    REG_CMD(doICM, "^ICM");     // Intercom modify command
#ifndef _NOSIP_
    // Here the SIP commands will take place
    REG_CMD(doPHN, "^PHN");     // SIP commands
    REG_CMD(getPHN, "?PHN");    // SIP state commands
#endif
    // SubView commands
//    REG_CMD(doEPR, "^EPR");     // Execute Push on Release.
    REG_CMD(doPOP, "^POP");     // Open Collapsible Popup Command
    REG_CMD(doSCE, "^SCE");     // Configures subpage custom events.
//    REG_CMD(doSDR, "^SDR");     // Enabling subpage dynamic reordering.
    REG_CMD(doSHA, "^SHA");     // Subpage Hide All Command
    REG_CMD(doSHD, "^SHD");     // Hides subpage
    REG_CMD(doSPD, "^SPD");     //  Set the padding between subpages on a subpage viewer button
    REG_CMD(doSSH, "^SSH");     // Subpage show command.
    REG_CMD(doSTG, "^STG");     // Subpage toggle command

    // ListView commands (G5)
    REG_CMD(doLVD, "^LVD");     // G5: Set Listview Data Source
    REG_CMD(doLVE, "^LVE");     // G5: Set ListView custom event number
    REG_CMD(doLVF, "^LVF");     // G5: Listview Filter
    REG_CMD(doLVL, "^LVL");     // G5: ListView layout
    REG_CMD(doLVM, "^LVM");     // G5: ListView map fields
    REG_CMD(doLVN, "^LVN");     // G5: ListView navigate
    REG_CMD(doLVR, "^LVR");     // G5: ListView refresh data
    REG_CMD(doLVS, "^LVS");     // G5: ListView sort data

    // State commands
    REG_CMD(doON, "ON");
    REG_CMD(doOFF, "OFF");
    REG_CMD(doLEVEL, "LEVEL");
    REG_CMD(doBLINK, "BLINK");
    REG_CMD(doVER, "^VER?");    // Return version string to master
#ifndef _NOSIP_
    REG_CMD(doWCN, "^WCN?");    // Return SIP phone number
#endif
    // TPControl commands
    REG_CMD(doTPCCMD, "TPCCMD");    // Profile related options
    REG_CMD(doTPCACC, "TPCACC");    // Device orientation
#ifndef _NOSIP_
    REG_CMD(doTPCSIP, "TPCSIP");    // Show the built in SIP phone
#endif
    // Virtual internal commands
    REG_CMD(doFTR, "#FTR");     // File transfer (virtual internal command)

    // At least we must add the SIP client
#ifndef _NOSIP_
    mSIPClient = new TSIPClient;

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_ERROR("Error initializing the SIP client!");
        TConfig::setSIPstatus(false);
    }
#endif
    TError::clear();
    runClickQueue();
    runUpdateSubViewItem();
}

TPageManager::~TPageManager()
{
    DECL_TRACER("TPageManager::~TPageManager()");
#ifndef _NOSIP_
    if (mSIPClient)
    {
        delete mSIPClient;
        mSIPClient = nullptr;
    }
#endif
    PCHAIN_T *p = mPchain;
    PCHAIN_T *next = nullptr;
#ifdef __ANDROID__
    stopNetworkState();
#endif
    try
    {
        while (p)
        {
            next = p->next;

            if (p->page)
                delete p->page;

            delete p;
            p = next;
        }

        SPCHAIN_T *sp = mSPchain;
        SPCHAIN_T *snext = nullptr;

        while (sp)
        {
            snext = sp->next;

            if (sp->page)
                delete sp->page;

            delete sp;
            sp = snext;
        }

        mPchain = nullptr;
        mSPchain = nullptr;
        setPChain(mPchain);
        setSPChain(mSPchain);

        if (mAmxNet)
        {
            delete mAmxNet;
            mAmxNet = nullptr;
        }

        if (mTSettings)
        {
            delete mTSettings;
            mTSettings = nullptr;
        }

        if (mPageList)
        {
            delete mPageList;
            mPageList = nullptr;
        }

        if (mPalette)
        {
            delete mPalette;
            mPalette = nullptr;
        }

        if (mFonts)
        {
            delete mFonts;
            mFonts = nullptr;
        }

        if (gIcons)
        {
            delete gIcons;
            gIcons = nullptr;
        }

        if (gPrjResources)
        {
            delete gPrjResources;
            gPrjResources = nullptr;
        }

        if (mExternal)
        {
            delete mExternal;
            mExternal = nullptr;
        }

        if (!mButtonStates.empty())
        {
            vector<TButtonStates *>::iterator iter;

            for (iter = mButtonStates.begin(); iter != mButtonStates.end(); ++iter)
                delete *iter;

            mButtonStates.clear();
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Memory error: " << e.what());
    }

    gPageManager = nullptr;
}

void TPageManager::initialize()
{
    DECL_TRACER("TPageManager::initialize()");

    surface_mutex.lock();
    dropAllSubPages();
    dropAllPages();

    string projectPath = TConfig::getProjectPath();

    if (!fs::exists(projectPath + "/prj.xma"))
        projectPath += "/__system";

    if (mAmxNet && mAmxNet->isConnected())
        mAmxNet->close();

    if (mTSettings)
        mTSettings->loadSettings();
    else
        mTSettings = new TSettings(projectPath);

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        surface_mutex.unlock();
        return;
    }

    // Set the panel type from the project information
    TConfig::savePanelType(mTSettings->getPanelType());

    if (gPrjResources)
        delete gPrjResources;

    gPrjResources = new TPrjResources(mTSettings->getResourcesList());

    if (mPalette)
        delete mPalette;

    mPalette = new TPalette();

    vector<PALETTE_SETUP> pal = mTSettings->getSettings().palettes;

    if (pal.size() > 0)
    {
        vector<PALETTE_SETUP>::iterator iterPal;

        for (iterPal = pal.begin(); iterPal != pal.end(); ++iterPal)
            mPalette->initialize(iterPal->file);
    }

    if (!TError::isError())
        TColor::setPalette(mPalette);

    if (mFonts)
        delete mFonts;

    mFonts = new TFont(mTSettings->getFontFileName(), mTSettings->isG5());

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_ERROR("Initializing fonts was not successfull!");
        surface_mutex.unlock();
        return;
    }

    if (gIcons)
        delete gIcons;

    gIcons = new TIcons();

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        MSG_ERROR("Initializing icons was not successfull!");
        surface_mutex.unlock();
        return;
    }

    if (mPageList)
        delete mPageList;

    mPageList = new TPageList();

    if (mExternal)
        delete mExternal;

    mExternal = new TExternal();

    PAGELIST_T page;

    if (!mTSettings->getSettings().powerUpPage.empty())
    {
        if (readPage(mTSettings->getSettings().powerUpPage))
        {
            MSG_TRACE("Found power up page " << mTSettings->getSettings().powerUpPage);
            page = findPage(mTSettings->getSettings().powerUpPage);
            mActualPage = page.pageID;
        }
    }

    TPage *pg = getPage(mActualPage);

    vector<string> popups = mTSettings->getSettings().powerUpPopup;

    if (popups.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = popups.begin(); iter != popups.end(); ++iter)
        {
            if (readSubPage(*iter))
            {
                MSG_TRACE("Found power up popup " << *iter);

                if (pg)
                {
                    TSubPage *spage = getSubPage(*iter);
                    spage->setParent(pg->getHandle());
                    pg->addSubPage(spage);
                }
            }
        }
    }

    // Here we initialize the system resources like borders, cursors, sliders, ...
    if (mSystemDraw)
        delete mSystemDraw;

    mSystemDraw = new TSystemDraw(TConfig::getSystemPath(TConfig::BASE));

    TError::clear();        // Clear all errors who may be occured until here

    // Start the thread
    startComm();

    surface_mutex.unlock();
}

bool TPageManager::startComm()
{
    DECL_TRACER("TPageManager::startComm()");

    if (mAmxNet && mAmxNet->isNetRun())
        return true;

    try
    {
        if (!mAmxNet)
        {
            if (_netRunning)
            {
                // Wait until previous connection thread ended
                while (_netRunning)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            mAmxNet = new amx::TAmxNet();
            mAmxNet->setCallback(bind(&TPageManager::doCommand, this, std::placeholders::_1));
            mAmxNet->setPanelID(TConfig::getChannel());
            mAmxNet->setSerialNum(V_SERIAL);
        }

        if (!mAmxNet->isNetRun())
            mAmxNet->Run();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting the AmxNet thread: " << e.what());
        return false;
    }

    return true;
}

void TPageManager::startUp()
{
    DECL_TRACER("TPageManager::startUp()");

    if (mAmxNet)
    {
        MSG_WARNING("Communication with controller already initialized!");
        return;
    }

    if (!startComm())
        return;

#ifdef __ANDROID__
    initOrientation();
    initNetworkState();
#endif
}

void TPageManager::reset()
{
    DECL_TRACER("TPageManager::reset()");

    // Freshly initialize everything.
    initialize();
}

void TPageManager::runCommands()
{
    DECL_TRACER("TPageManager::runCommands()");

    if (cmdLoop_busy)
        return;

    try
    {
        mThreadCommand = std::thread([=] { this->commandLoop(); });
        mThreadCommand.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting thread for command loop: " << e.what());
        _netRunning = false;
    }
}

void TPageManager::showSetup()
{
    DECL_TRACER("TPageManager::showSetup()");
#ifdef Q_OS_ANDROID
    // Scan Netlinx for TP4/TP5 files and update the list of setup.
    if (TConfig::getController().compare("0.0.0.0") != 0)
    {
        if (_startWait)
            _startWait(string("Please wait while I try to load the list of surface files from Netlinx (") + TConfig::getController() + ")");

        TTPInit tpinit;
        std::vector<TTPInit::FILELIST_t> fileList;
        tpinit.setPath(TConfig::getProjectPath());
        fileList = tpinit.getFileList(".tp4|.tp5");

        if (fileList.size() > 0)
        {
            vector<TTPInit::FILELIST_t>::iterator iter;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "clearSurfaces");
#else
            QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "clearSurfaces");
#endif
            for (iter = fileList.begin(); iter != fileList.end(); ++iter)
            {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                QAndroidJniObject str = QAndroidJniObject::fromString(iter->fname.c_str());
                QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "addSurface", "(Ljava/lang/String;)V", str.object<jstring>());
#else
                QJniObject str = QJniObject::fromString(iter->fname.c_str());
                QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "addSurface", "(Ljava/lang/String;)V", str.object<jstring>());
#endif
            }
        }

        if (_stopWait)
            _stopWait();
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setLogLevel", "(Ljava/lang/Integer;)V", TConfig::getLogLevelBits());
    QAndroidJniObject strPath = QAndroidJniObject::fromString(TConfig::getLogFile().c_str());
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setLogEnableFile", "(Z)V", TConfig::getLogFileEnabled());
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setLogPath", "(Ljava/lang/String;)V", strPath.object<jstring>());

    QAndroidJniObject pw1 = QAndroidJniObject::fromString(TConfig::getPassword1().c_str());
    QAndroidJniObject pw2 = QAndroidJniObject::fromString(TConfig::getPassword2().c_str());
    QAndroidJniObject pw3 = QAndroidJniObject::fromString(TConfig::getPassword3().c_str());
    QAndroidJniObject pw4 = QAndroidJniObject::fromString(TConfig::getPassword4().c_str());
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 1, pw1.object<jstring>());
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 2, pw2.object<jstring>());
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 3, pw3.object<jstring>());
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 4, pw4.object<jstring>());
#else
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setLogLevel", "(I)V", TConfig::getLogLevelBits());
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setLogEnableFile", "(I)V", (TConfig::getLogFileEnabled() ? 1 : 0));
    QJniObject strPath = QJniObject::fromString(TConfig::getLogFile().c_str());
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setLogPath", "(Ljava/lang/String;)V", strPath.object<jstring>());

    QJniObject pw1 = QJniObject::fromString(TConfig::getPassword1().c_str());
    QJniObject pw2 = QJniObject::fromString(TConfig::getPassword2().c_str());
    QJniObject pw3 = QJniObject::fromString(TConfig::getPassword3().c_str());
    QJniObject pw4 = QJniObject::fromString(TConfig::getPassword4().c_str());
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 1, pw1.object<jstring>());
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 2, pw2.object<jstring>());
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 3, pw3.object<jstring>());
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "setPassword", "(ILjava/lang/String;)V", 4, pw4.object<jstring>());
#endif

    enterSetup();
#else
        if (_callShowSetup)
            _callShowSetup();
#endif
}

int TPageManager::getSelectedRow(ulong handle)
{
    DECL_TRACER("TPageManager::getSelectedRow(ulong handle)");

    int nPage = (handle >> 16) & 0x0000ffff;

    if ((nPage && TPage::isRegularPage(nPage)) || TPage::isSystemPage(nPage)) // Do we have a page?
    {                                                   // Yes, then look on page
        TPage *pg = getPage(nPage);

        if (!pg)
            return -1;

        return pg->getSelectedRow(handle);
    }
    else if (TPage::isRegularSubPage(nPage) || TPage::isSystemSubPage(nPage))
    {
        TSubPage *subPg = getSubPage(nPage);

        if (!subPg)
            return -1;

        return subPg->getSelectedRow(handle);
    }

    MSG_WARNING("Invalid handle " << handleToString(handle) << " detected!");
    return -1;
}

string TPageManager::getSelectedItem(ulong handle)
{
    DECL_TRACER("TPageManager::getSelectedItem(ulong handle)");

    int nPage = (handle >> 16) & 0x0000ffff;

    if ((nPage && TPage::isRegularPage(nPage)) || TPage::isSystemPage(nPage)) // Do we have a page?
    {                                                   // Yes, then look on page
        TPage *pg = getPage(nPage);

        if (!pg)
            return string();

        return pg->getSelectedItem(handle);
    }
    else if (TPage::isRegularSubPage(nPage) || TPage::isSystemSubPage(nPage))
    {
        TSubPage *subPg = getSubPage(nPage);

        if (!subPg)
            return string();

        return subPg->getSelectedItem(handle);
    }

    MSG_WARNING("Invalid handle " << handleToString(handle) << " detected!");
    return string();
}

void TPageManager::setSelectedRow(ulong handle, int row, const std::string& text)
{
    DECL_TRACER("TPageManager::setSelectedRow(ulong handle, int row)");

    int nPage = (handle >> 16) & 0x0000ffff;

    if (TPage::isRegularPage(nPage) || TPage::isSystemPage(nPage)) // Do we have a page?
    {                                                   // Yes, then look on page
        TPage *pg = getPage(nPage);

        if (!pg)
            return;

        pg->setSelectedRow(handle, row);
    }
    else if (TPage::isRegularSubPage(nPage) || TPage::isSystemSubPage(nPage))   // Do we have a subpage?
    {                                                   // Yes, then look on subpage
        TSubPage *subPg = getSubPage(nPage);

        if (!subPg)
            return;

        subPg->setSelectedRow(handle, row);
        // Check if this is a system list. If so we must set the selected
        // text to the input line or "label".
        TPage *mainPage = nullptr;

        if (nPage >= SYSTEM_SUBPAGE_START)  // System subpage?
        {
            switch(nPage)
            {
                case SYSTEM_SUBPAGE_SYSTEMSOUND:
                case SYSTEM_SUBPAGE_SINGLEBEEP:
                case SYSTEM_SUBPAGE_DOUBLEBEEP:
                    mainPage = getPage(SYSTEM_PAGE_SOUND);
                break;

                case SYSTEM_SUBPAGE_SURFACE:
                    mainPage = getPage(SYSTEM_PAGE_CONTROLLER);
                break;
            }
        }

        if (mainPage)
        {
            if (nPage == SYSTEM_SUBPAGE_SYSTEMSOUND)  // System sound beep
            {
                Button::TButton *bt = mainPage->getButton(SYSTEM_PAGE_SOUND_TXSYSSOUND);

                if (bt)
                {
                    bt->setText(text, -1);
                    TConfig::setTemporary(true);
                    TConfig::saveSystemSoundFile(text);
                }
            }
            else if (nPage == SYSTEM_SUBPAGE_SINGLEBEEP) // System sound single beep
            {
                Button::TButton *bt = mainPage->getButton(SYSTEM_PAGE_SOUND_TXSINGLEBEEP);

                if (bt)
                {
                    bt->setText(text, -1);
                    TConfig::setTemporary(true);
                    TConfig::saveSingleBeepFile(text);
                }
            }
            else if (nPage == SYSTEM_SUBPAGE_DOUBLEBEEP) // System sound double beep
            {
                Button::TButton *bt = mainPage->getButton(SYSTEM_PAGE_SOUND_TXDOUBLEBEEP);

                if (bt)
                {
                    bt->setText(text, -1);
                    TConfig::setTemporary(true);
                    TConfig::saveDoubleBeepFile(text);
                }
            }
            else if (nPage == SYSTEM_SUBPAGE_SURFACE)   // System TP4 files (surface files)
            {
                Button::TButton *bt = mainPage->getButton(SYSTEM_PAGE_CTRL_SURFACE);

                if (bt)
                {
                    MSG_DEBUG("Setting text: " << text);
                    bt->setText(text, -1);
                    TConfig::setTemporary(true);
                    TConfig::saveFtpSurface(text);
                }
            }

            // Close the list subpage
            subPg->drop();
        }
    }
}

void TPageManager::redrawObject(ulong handle)
{
    DECL_TRACER("TPageManager::redrawObject(ulong handle)");

    int pnumber = (int)((handle >> 16) & 0x0000ffff);
    int btnumber = (int)(handle & 0x0000ffff);

    if (pnumber < REGULAR_SUBPAGE_START)    // Is it a page?
    {
        TPage *page = getPage(pnumber);

        if (!page)
        {
            MSG_WARNING("Page " << pnumber << " not found!");
            return;
        }

        if (!page->isVisilble())
            return;

        if (btnumber == 0)
        {
            page->show();
            return;
        }

        Button::TButton *button = page->getButton(btnumber);

        if (!button)
        {
            MSG_WARNING("Button " << btnumber << " on page " << pnumber << " not found!");
            return;
        }

        button->showLastButton();
    }
    else if (pnumber >= REGULAR_SUBPAGE_START && pnumber < SYSTEM_PAGE_START)
    {
        TSubPage *spage = getSubPage(pnumber);

        if (!spage)
        {
            MSG_WARNING("Subpage " << pnumber << " not found!");
            return;
        }

        if (!spage->isVisible())
            return;

        if (btnumber == 0)
        {
            spage->show();
            return;
        }

        Button::TButton *button = spage->getButton(btnumber);

        if (!button)
        {
            MSG_WARNING("Button " << btnumber << " on subpage " << pnumber << " not found!");
            return;
        }

        button->showLastButton();
    }
    else
    {
        MSG_WARNING("System pages are not handled by redraw method! Ignoring page " << pnumber << ".");
    }
}

#ifdef _SCALE_SKIA_
void TPageManager::setSetupScaleFactor(double scale, double sw, double sh)
{
    DECL_TRACER("TPageManager::setSetupScaleFactor(double scale, double sw, double sh)");

    mScaleSystem = scale;
    mScaleSystemWidth = sw;
    mScaleSystemHeight = sh;
}
#endif

/*
 * The following method is called by the class TAmxNet whenever an event from
 * the Netlinx occured.
 */
void TPageManager::doCommand(const amx::ANET_COMMAND& cmd)
{
    DECL_TRACER("TPageManager::doCommand(const amx::ANET_COMMAND& cmd)");

    if (!cmdLoop_busy)
        runCommands();

    mCommands.push_back(cmd);
}

void TPageManager::commandLoop()
{
    DECL_TRACER("TPageManager::commandLoop()");

    if (cmdLoop_busy)
        return;

    cmdLoop_busy = true;
    string com;

    while (cmdLoop_busy && !killed && !_restart_)
    {
        while (mCommands.size() > 0)
        {
            amx::ANET_COMMAND bef = mCommands.at(0);
            mCommands.erase(mCommands.begin());

            switch (bef.MC)
            {
                case 0x0006:
                case 0x0018:	// feedback channel on
                    com.assign("ON-");
                    com.append(to_string(bef.data.chan_state.channel));
                    parseCommand(bef.device1, bef.data.chan_state.port, com);
                break;

                case 0x0007:
                case 0x0019:	// feedback channel off
                    com.assign("OFF-");
                    com.append(to_string(bef.data.chan_state.channel));
                    parseCommand(bef.device1, bef.data.chan_state.port, com);
                break;

                case 0x000a:	// level value change
                    com = "LEVEL-";
                    com += to_string(bef.data.message_value.value);
                    com += ",";

                    switch (bef.data.message_value.type)
                    {
                        case 0x10: com += to_string(bef.data.message_value.content.byte); break;
                        case 0x11: com += to_string(bef.data.message_value.content.ch); break;
                        case 0x20: com += to_string(bef.data.message_value.content.integer); break;
                        case 0x21: com += to_string(bef.data.message_value.content.sinteger); break;
                        case 0x40: com += to_string(bef.data.message_value.content.dword); break;
                        case 0x41: com += to_string(bef.data.message_value.content.sdword); break;
                        case 0x4f: com += to_string(bef.data.message_value.content.fvalue); break;
                        case 0x8f: com += to_string(bef.data.message_value.content.dvalue); break;
                    }

                    parseCommand(bef.device1, bef.data.message_value.port, com);
                break;

                case 0x000c:	// Command string
                {
                    amx::ANET_MSG_STRING msg = bef.data.message_string;

                    if (msg.length < strlen((char *)&msg.content))
                    {
                        mCmdBuffer.append((char *)&msg.content);
                        break;
                    }
                    else if (mCmdBuffer.length() > 0)
                    {
                        mCmdBuffer.append((char *)&msg.content);
                        size_t len = (mCmdBuffer.length() >= sizeof(msg.content)) ? (sizeof(msg.content)-1) : mCmdBuffer.length();
                        strncpy((char *)&msg.content, mCmdBuffer.c_str(), len);
                        msg.content[len] = 0;
                    }

                    if (getCommand((char *)msg.content) == "^UTF" || bef.intern /*|| TTPInit::isG5()*/)  // This is already UTF8!
                        com.assign((char *)msg.content);
                    else
                        com.assign(cp1250ToUTF8((char *)&msg.content));

                    parseCommand(bef.device1, msg.port, com);
                    mCmdBuffer.clear();
                }
                break;

                case 0x0502:    // Blink message (contains date and time)
                    com = "BLINK-" + to_string(bef.data.blinkMessage.hour) + ":";
                    com += to_string(bef.data.blinkMessage.minute) + ":";
                    com += to_string(bef.data.blinkMessage.second) + ",";
                    com += to_string(bef.data.blinkMessage.year) + "-";
                    com += to_string(bef.data.blinkMessage.month) + "-";
                    com += to_string(bef.data.blinkMessage.day) + ",";
                    com += to_string(bef.data.blinkMessage.weekday) + ",";
                    com += ((bef.data.blinkMessage.LED & 0x0001) ? "ON" : "OFF");
                    parseCommand(0, 0, com);
                break;

                case 0x1000:	// Filetransfer
                {
                    amx::ANET_FILETRANSFER ftr = bef.data.filetransfer;

                    if (ftr.ftype == 0)
                    {
                        switch(ftr.function)
                        {
                            case 0x0100:	// Syncing directory
                                com = "#FTR-SYNC:0:";
                                com.append((char*)&ftr.data[0]);
                                parseCommand(bef.device1, bef.port1, com);
                            break;

                            case 0x0104:	// Delete file
                                com = "#FTR-SYNC:"+to_string(bef.count)+":Deleting files ... ("+to_string(bef.count)+"%)";
                                parseCommand(bef.device1, bef.port1, com);
                            break;

                            case 0x0105:	// start filetransfer
                                com = "#FTR-START";
                                parseCommand(bef.device1, bef.port1, com);
                            break;
                        }
                    }
                    else
                    {
                        switch(ftr.function)
                        {
                            case 0x0003:	// Received part of file
                            case 0x0004:	// End of file
                                com = "#FTR-FTRPART:"+to_string(bef.count)+":"+to_string(ftr.info1);
                                parseCommand(bef.device1, bef.port1, com);
                            break;

                            case 0x0007:	// End of file transfer
                            {
                                com = "#FTR-END";
                                parseCommand(bef.device1, bef.port1, com);
                            }
                            break;

                            case 0x0102:	// Receiving file
                                com = "#FTR-FTRSTART:"+to_string(bef.count)+":"+to_string(ftr.info1)+":";
                                com.append((char*)&ftr.data[0]);
                                parseCommand(bef.device1, bef.port1, com);
                            break;
                        }
                    }
                }
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    cmdLoop_busy = false;
}

void TPageManager::deployCallbacks()
{
    DECL_TRACER("TPageManager::deployCallbacks()");

    PCHAIN_T *p = mPchain;

    while (p)
    {
        if (p->page)
        {
            if (_setBackground)
                p->page->registerCallback(_setBackground);

            if (_callPlayVideo)
                p->page->regCallPlayVideo(_callPlayVideo);
        }

        p = p->next;
    }

    SPCHAIN_T *sp = mSPchain;

    while (sp)
    {
        if (sp->page)
        {
            if (_setBackground)
                sp->page->registerCallback(_setBackground);

            if (_callPlayVideo)
                sp->page->regCallPlayVideo(_callPlayVideo);
        }

        sp = sp->next;
    }
}

void TPageManager::regCallbackNetState(std::function<void (int)> callNetState, ulong handle)
{
    DECL_TRACER("TPageManager::regCallbackNetState(std::function<void (int)> callNetState, ulong handle)");

    if (handle == 0)
        return;

    mNetCalls.insert(std::pair<int, std::function<void (int)> >(handle, callNetState));
}

void TPageManager::unregCallbackNetState(ulong handle)
{
    DECL_TRACER("TPageManager::unregCallbackNetState(ulong handle)");

    if (mNetCalls.size() == 0)
        return;

    std::map<int, std::function<void (int)> >::iterator iter = mNetCalls.find((int)handle);

    if (iter != mNetCalls.end())
        mNetCalls.erase(iter);
}
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#ifdef Q_OS_ANDROID
void TPageManager::regCallbackBatteryState(std::function<void (int, bool, int)> callBatteryState, ulong handle)
{
    DECL_TRACER("TPageManager::regCallbackBatteryState(std::function<void (int, bool, int)> callBatteryState, ulong handle)");

    if (handle == 0)
        return;

    mBatteryCalls.insert(std::pair<int, std::function<void (int, bool, int)> >(handle, callBatteryState));
}
#endif
#ifdef Q_OS_IOS
void TPageManager::regCallbackBatteryState(std::function<void (int, int)> callBatteryState, ulong handle)
{
    DECL_TRACER("TPageManager::regCallbackBatteryState(std::function<void (int, int)> callBatteryState, ulong handle)");

    if (handle == 0)
        return;

    mBatteryCalls.insert(std::pair<int, std::function<void (int, int)> >(handle, callBatteryState));
#ifdef Q_OS_IOS
    mLastBatteryLevel = TIOSBattery::getBatteryLeft();
    mLastBatteryState = TIOSBattery::getBatteryState();

#endif
    if (mLastBatteryLevel > 0 || mLastBatteryState > 0)
        informBatteryStatus(mLastBatteryLevel, mLastBatteryState);
}
#endif
void TPageManager::unregCallbackBatteryState(ulong handle)
{
    DECL_TRACER("TPageManager::unregCallbackBatteryState(ulong handle)");

    if (mBatteryCalls.size() == 0)
        return;
#ifdef Q_OS_ANDROID
    std::map<int, std::function<void (int, bool, int)> >::iterator iter = mBatteryCalls.find(handle);
#endif
#ifdef Q_OS_IOS
    std::map<int, std::function<void (int, int)> >::iterator iter = mBatteryCalls.find((int)handle);
#endif
    if (iter != mBatteryCalls.end())
        mBatteryCalls.erase(iter);
}
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
/*
 * The following function must be called to start the "panel".
 */
bool TPageManager::run()
{
    DECL_TRACER("TPageManager::run()");

    if (mActualPage <= 0)
        return false;

    TPage *pg = getPage(mActualPage);

    if (!pg || !_setPage || !mTSettings)
        return false;

    surface_mutex.lock();
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->regCallPlayVideo(_callPlayVideo);

    int width, height;
    width = mTSettings->getWidth();
    height = mTSettings->getHeight();
#ifdef _SCALE_SKIA_
    if (mScaleFactor != 1.0)
    {
        width = (int)((double)width * mScaleFactor);
        height = (int)((double)height * mScaleFactor);
    }
#endif
    _setPage((pg->getNumber() << 16) & 0xffff0000, width, height);
    pg->show();

    TSubPage *subPg = pg->getFirstSubPage();

    while (subPg)
    {
        subPg->setFonts(mFonts);
        subPg->registerCallback(_setBackground);
        subPg->registerCallbackDB(_displayButton);
        subPg->regCallDropSubPage(_callDropSubPage);
        subPg->regCallPlayVideo(_callPlayVideo);

        if (_setSubPage)
        {
            MSG_DEBUG("Drawing page " << subPg->getNumber() << ": " << subPg->getName() << "...");
            width = subPg->getWidth();
            height = subPg->getHeight();
            int left = subPg->getLeft();
            int top = subPg->getTop();
#ifdef _SCALE_SKIA_
            if (mScaleFactor != 1.0)
            {
                width = (int)((double)width * mScaleFactor);
                height = (int)((double)height * mScaleFactor);
                left = (int)((double)left * mScaleFactor);
                top = (int)((double)top * mScaleFactor);
            }
#endif
            ANIMATION_t ani;
            ani.showEffect = subPg->getShowEffect();
            ani.showTime = subPg->getShowTime();
            ani.hideEffect = subPg->getHideEffect();
            ani.hideTime = subPg->getHideTime();

            subPg->setZOrder(pg->getNextZOrder());
            _setSubPage(subPg->getHandle(), pg->getHandle(), left, top, width, height, ani, subPg->isModal(), subPg->isCollapsible());
            subPg->show();
        }

        subPg = pg->getNextSubPage();
    }

    surface_mutex.unlock();
    return true;
}

TPage *TPageManager::getPage(int pageID)
{
    DECL_TRACER("TPageManager::getPage(int pageID)");

    if (pageID <= 0)
        return nullptr;

    PCHAIN_T *p = mPchain;

    while (p)
    {
        if (p->page && p->page->getNumber() == pageID)
            return p->page;

        p = p->next;
    }

    return nullptr;
}

TPage *TPageManager::getPage(const string& name)
{
    DECL_TRACER("TPageManager::getPage(const string& name)");

    if (name.empty())
        return nullptr;

    PCHAIN_T *p = mPchain;

    while (p)
    {
        if (p->page && p->page->getName().compare(name) == 0)
            return p->page;

        p = p->next;
    }

    return nullptr;
}

TPage *TPageManager::loadPage(PAGELIST_T& pl, bool *refresh)
{
    DECL_TRACER("TPageManager::loadPage(PAGELIST_T& pl, bool *refresh)");

    if (refresh)
        *refresh = false;

    if (!pl.isValid)
        return nullptr;

    TPage *pg = getPage(pl.pageID);

    if (!pg)
    {
        if (!readPage(pl.pageID))
            return nullptr;

        pg = getPage(pl.pageID);

        if (!pg)
        {
            MSG_ERROR("Error loading page " << pl.pageID << ", " << pl.name << " from file " << pl.file << "!");
            return nullptr;
        }

        if (refresh)
            *refresh = true;        // Indicate that the page was freshly loaded
    }

    return pg;
}

void TPageManager::reloadSystemPage(TPage *page)
{
    DECL_TRACER("TPageManager::reloadSystemPage(TPage *page)");

    if (!page)
        return;

    vector<Button::TButton *> buttons = page->getAllButtons();
    vector<Button::TButton *>::iterator iter;
    TConfig::setTemporary(false);

    for (iter = buttons.begin(); iter != buttons.end(); ++iter)
    {
        Button::TButton *bt = *iter;

        if (bt->getAddressPort() == 0 && bt->getAddressChannel() > 0)
        {
            switch(bt->getAddressChannel())
            {
                case SYSTEM_ITEM_LOGLOGFILE:        bt->setTextOnly(TConfig::getLogFile(), -1); break;

                case SYSTEM_ITEM_NETLINX_IP:        bt->setTextOnly(TConfig::getController(), -1); break;
                case SYSTEM_ITEM_NETLINX_PORT:      bt->setTextOnly(std::to_string(TConfig::getPort()), -1); break;
                case SYSTEM_ITEM_NETLINX_CHANNEL:   bt->setTextOnly(std::to_string(TConfig::getChannel()), -1); break;
                case SYSTEM_ITEM_NETLINX_PTYPE:     bt->setTextOnly(TConfig::getPanelType(), -1); break;
                case SYSTEM_ITEM_FTPUSER:           bt->setTextOnly(TConfig::getFtpUser(), -1); break;
                case SYSTEM_ITEM_FTPPASSWORD:       bt->setTextOnly(TConfig::getFtpPassword(), -1); break;
                case SYSTEM_ITEM_FTPSURFACE:        bt->setTextOnly(TConfig::getFtpSurface(), -1); break;

                case SYSTEM_ITEM_SIPPROXY:          bt->setTextOnly(TConfig::getSIPproxy(), -1); break;
                case SYSTEM_ITEM_SIPPORT:           bt->setTextOnly(std::to_string(TConfig::getSIPport()), -1); break;
                case SYSTEM_ITEM_SIPSTUN:           bt->setTextOnly(TConfig::getSIPstun(), -1); break;
                case SYSTEM_ITEM_SIPDOMAIN:         bt->setTextOnly(TConfig::getSIPdomain(), -1); break;
                case SYSTEM_ITEM_SIPUSER:           bt->setTextOnly(TConfig::getSIPuser(), -1); break;
                case SYSTEM_ITEM_SIPPASSWORD:       bt->setTextOnly(TConfig::getSIPpassword(), -1); break;

                case SYSTEM_ITEM_SYSTEMSOUND:       bt->setTextOnly(TConfig::getSystemSound(), -1); break;
                case SYSTEM_ITEM_SINGLEBEEP:        bt->setTextOnly(TConfig::getSingleBeepSound(), -1); break;
                case SYSTEM_ITEM_DOUBLEBEEP:        bt->setTextOnly(TConfig::getDoubleBeepSound(), -1); break;
            }
        }
        else if (bt->getChannelPort() == 0 && bt->getChannelNumber() > 0)
        {
            switch(bt->getChannelNumber())
            {
                case SYSTEM_ITEM_DEBUGINFO:         bt->setActiveInstance(IS_LOG_INFO() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGWARNING:      bt->setActiveInstance(IS_LOG_WARNING() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGERROR:        bt->setActiveInstance(IS_LOG_ERROR() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGTRACE:        bt->setActiveInstance(IS_LOG_TRACE() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGDEBUG:        bt->setActiveInstance(IS_LOG_DEBUG() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGPROTOCOL:     bt->setActiveInstance(IS_LOG_PROTOCOL() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGALL:          bt->setActiveInstance(IS_LOG_ALL() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGLONG:         bt->setActiveInstance(TConfig::isLongFormat() ? 1 : 0); break;
                case SYSTEM_ITEM_DEBUGPROFILE:      bt->setActiveInstance(TConfig::getProfiling() ? 1 : 0); break;

                case SYSTEM_ITEM_FTPPASSIVE:        bt->setActiveInstance(TConfig::getFtpPassive() ? 1 : 0); break;

                case SYSTEM_ITEM_SIPIPV4:           bt->setActiveInstance(TConfig::getSIPnetworkIPv4() ? 1 : 0); break;
                case SYSTEM_ITEM_SIPIPV6:           bt->setActiveInstance(TConfig::getSIPnetworkIPv6() ? 1 : 0); break;
                case SYSTEM_ITEM_SIPENABLE:         bt->setActiveInstance(TConfig::getSIPstatus() ? 1 : 0); break;
                case SYSTEM_ITEM_SIPIPHONE:         bt->setActiveInstance(TConfig::getSIPiphone() ? 1 : 0); break;

                case SYSTEM_ITEM_SOUNDSWITCH:       bt->setActiveInstance(TConfig::getSystemSoundState() ? 1 : 0); break;

                case SYSTEM_ITEM_VIEWSCALEFIT:      bt->setActiveInstance(TConfig::getScale() ? 1 : 0); break;
                case SYSTEM_ITEM_VIEWBANNER:        bt->setActiveInstance(TConfig::showBanner() ? 1 : 0); break;
                case SYSTEM_ITEM_VIEWNOTOOLBAR:     bt->setActiveInstance(TConfig::getToolbarSuppress() ? 1 : 0); break;
                case SYSTEM_ITEM_VIEWTOOLBAR:       bt->setActiveInstance(TConfig::getToolbarForce() ? 1 : 0); break;
                case SYSTEM_ITEM_VIEWROTATE:        bt->setActiveInstance(TConfig::getRotationFixed() ? 1 : 0); break;
            }
        }
        else if (bt->getLevelPort() == 0 && bt->getLevelChannel() > 0)
        {
            switch(bt->getLevelChannel())
            {
                case SYSTEM_ITEM_SYSVOLUME:         bt->drawBargraph(0, TConfig::getSystemVolume(), false); break;
                case SYSTEM_ITEM_SYSGAIN:           bt->drawBargraph(0, TConfig::getSystemGain(), false); break;
            }
        }
    }
}

bool TPageManager::setPage(int PageID, bool forget)
{
    DECL_TRACER("TPageManager::setPage(int PageID, bool forget)");

    return _setPageDo(PageID, "", forget);
}

bool TPageManager::setPage(const string& name, bool forget)
{
    DECL_TRACER("TPageManager::setPage(const string& name, bool forget)");

    return _setPageDo(0, name, forget);
}

bool TPageManager::_setPageDo(int pageID, const string& name, bool forget)
{
    DECL_TRACER("TPageManager::_setPageDo(int pageID, const string& name, bool forget)");

    TPage *pg = nullptr;

    if (pageID > 0 && mActualPage == pageID)
    {
#if TESTMODE == 1
        __success = true;
        setScreenDone();
#endif
        return true;
    }
    else if (!name.empty())
    {
        pg = getPage(mActualPage);

        if (pg && pg->getName().compare(name) == 0)
        {
#if TESTMODE == 1
            __success = true;
            setScreenDone();
#endif
            return true;
        }
    }
    else if (pageID > 0)
        pg = getPage(mActualPage);
    else
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    // FIXME: Make this a vector array to hold a larger history!
    if (!forget)
        mPreviousPage = mActualPage;    // Necessary to be able to jump back to at least the last previous page

    if (pg)
        pg->drop();

    mActualPage = 0;
    PAGELIST_T listPg;

    if (pageID > 0)
        listPg = findPage(pageID);
    else
        listPg = findPage(name);

    bool refresh = false;

    if ((pg = loadPage(listPg, &refresh)) == nullptr)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return false;
    }

    mActualPage = pg->getNumber();

    if (mActualPage >= SYSTEM_PAGE_START && !refresh)
        reloadSystemPage(pg);

    int width = mTSettings->getWidth();
    int height = mTSettings->getHeight();

    if (_setPage)
        _setPage((mActualPage << 16) & 0xffff0000, width, height);

    pg->show();
    return true;
}


TSubPage *TPageManager::getSubPage(int pageID)
{
    DECL_TRACER("TPageManager::getSubPage(int pageID)");

    if (pageID < REGULAR_SUBPAGE_START)
        return nullptr;

    SPCHAIN_T *p = mSPchain;

    while(p)
    {
        if (p->page->getNumber() == pageID)
            return p->page;

        p = p->next;
    }

    return nullptr;
}

TSubPage *TPageManager::getSubPage(const std::string& name)
{
    DECL_TRACER("TPageManager::getSubPage(const std::string& name)");

    SPCHAIN_T *p = mSPchain;

    while (p)
    {
        if (p->page->getName().compare(name) == 0)
            return p->page;

        p = p->next;
    }

    MSG_DEBUG("Page " << name << " not found in cache.");
    return nullptr;
}

TSubPage *TPageManager::deliverSubPage(const string& name, TPage **pg)
{
    DECL_TRACER("TPageManager::deliverSubPage(const string& name, TPage **pg)");

    TPage *page = getActualPage();

    if (!page)
    {
        MSG_ERROR("No actual page loaded!");
        return nullptr;
    }

    if (pg)
        *pg = page;

    TSubPage *subPage = getSubPage(name);

    if (!subPage)
    {
        if (!readSubPage(name))
        {
            MSG_ERROR("Error reading subpage " << name);
            return nullptr;
        }

        subPage = getSubPage(name);

        if (!subPage)
        {
            MSG_ERROR("Fatal: A page with name " << name << " does not exist!");
            return nullptr;
        }

        subPage->setParent(page->getHandle());
    }

    return subPage;
}

TSubPage *TPageManager::deliverSubPage(int number, TPage **pg)
{
    DECL_TRACER("TPageManager::deliverSubPage(int number, TPage **pg)");

    TPage *page = getActualPage();

    if (!page)
    {
        MSG_ERROR("No actual page loaded!");
        return nullptr;
    }

    if (pg)
        *pg = page;

    TSubPage *subPage = getSubPage(number);

    if (!subPage)
    {
        if (!readSubPage(number))
        {
            MSG_ERROR("Error reading subpage " << number);
            return nullptr;
        }

        subPage = getSubPage(number);

        if (!subPage)
        {
            MSG_ERROR("Fatal: A page with name " << number << " does not exist!");
            return nullptr;
        }

        subPage->setParent(page->getHandle());
    }

    return subPage;
}

bool TPageManager::readPages()
{
    DECL_TRACER("TPageManager::readPages()");

    if (!mPageList)
    {
        MSG_ERROR("Page list is not initialized!");
        TError::SetError();
        return false;
    }

    // Read all pages
    vector<PAGELIST_T> pageList = mPageList->getPagelist();

    if (pageList.size() > 0)
    {
        vector<PAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            TPage *page = new TPage(pgIter->name+".xml");

            if (TError::isError())
            {
                PRINT_LAST_ERROR();
                delete page;
                return false;
            }

            page->setPalette(mPalette);
            page->setFonts(mFonts);
            page->registerCallback(_setBackground);
            page->registerCallbackDB(_displayButton);
            page->regCallPlayVideo(_callPlayVideo);

            if (!addPage(page))
                return false;
        }
    }

    vector<SUBPAGELIST_T> subPageList = mPageList->getSubPageList();

    if (subPageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator spgIter;

        for (spgIter = subPageList.begin(); spgIter != subPageList.end(); ++spgIter)
        {
            TSubPage *page = new TSubPage(spgIter->name+".xml");

            if (TError::isError())
            {
                PRINT_LAST_ERROR();
                delete page;
                return false;
            }

            page->setPalette(mPalette);
            page->setFonts(mFonts);
            page->registerCallback(_setBackground);
            page->registerCallbackDB(_displayButton);
            page->regCallDropSubPage(_callDropSubPage);
            page->regCallPlayVideo(_callPlayVideo);
            page->setGroup(spgIter->group);

            if (!addSubPage(page))
                return false;
        }
    }

    return true;
}

bool TPageManager::readPage(const std::string& name)
{
    DECL_TRACER("TPageManager::readPage(const std::string& name)");

    PAGELIST_T page = findPage(name);

    if ((page.pageID <= 0 || page.pageID >= MAX_PAGE_ID) && page.pageID < SYSTEM_PAGE_START && page.pageID >= SYSTEM_SUBPAGE_START)
    {
        MSG_ERROR("Page " << name << " not found!");
        return false;
    }

    TPage *pg;

    if (name.compare("_progress") == 0)
        pg = new TPage(name);
    else
        pg = new TPage(page.name+".xml");

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallPlayVideo(_callPlayVideo);

    if (!addPage(pg))
        return false;

    return true;
}

bool TPageManager::readPage(int ID)
{
    DECL_TRACER("TPageManager::readPage(int ID)");

    TError::clear();
    PAGELIST_T page = findPage(ID);

    if (page.pageID <= 0)
    {
        MSG_ERROR("Page with ID " << ID << " not found!");
        return false;
    }

    TPage *pg;

    if (ID == 300)      // Progress page of system?
        pg = new TPage("_progress");
    else
        pg = new TPage(page.name+".xml");

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallPlayVideo(_callPlayVideo);

    if (!addPage(pg))
        return false;

    return true;
}

bool TPageManager::readSubPage(const std::string& name)
{
    DECL_TRACER("TPageManager::readSubPage(const std::string& name)");

    TError::clear();
    SUBPAGELIST_T page = findSubPage(name);

    if (page.pageID < MAX_PAGE_ID || (page.pageID >= SYSTEM_PAGE_START && page.pageID < SYSTEM_SUBPAGE_START))
    {
        MSG_ERROR("Subpage " << name << " not found!");
        return false;
    }

    if (haveSubPage(name))
        return true;

    TSubPage *pg = new TSubPage(page.name+".xml");

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallDropSubPage(_callDropSubPage);
    pg->regCallPlayVideo(_callPlayVideo);
    pg->setGroup(page.group);

    if (!addSubPage(pg))
    {
        delete pg;
        return false;
    }

    return true;
}

bool TPageManager::readSubPage(int ID)
{
    DECL_TRACER("TPageManager::readSubPage(int ID)");

    TError::clear();
    SUBPAGELIST_T page = findSubPage(ID);

    if (page.pageID <= MAX_PAGE_ID)
    {
        MSG_ERROR("Subpage with ID " << ID << " not found!");
        return false;
    }

    TSubPage *pg = new TSubPage(page.name+".xml");

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallDropSubPage(_callDropSubPage);
    pg->regCallPlayVideo(_callPlayVideo);
    pg->setGroup(page.group);

    if (!addSubPage(pg))
        return false;

    return true;
}

vector<TSubPage *> TPageManager::createSubViewList(int id)
{
    DECL_TRACER("TPageManager::createSubViewList(int id)");

    vector<TSubPage *> subviews;

    if (id <= 0)
        return subviews;

    if (!mPageList)
    {
        MSG_WARNING("Missing page list and because of this can't make a subview list!");
        return subviews;
    }

    SUBVIEWLIST_T slist = mPageList->findSubViewList(id);

    if (slist.id <= 0 || slist.items.empty())
    {
        if (slist.id <= 0)
        {
            MSG_WARNING("Found no subview list with ID " << id);
        }
        else
        {
            MSG_WARNING("Subview list " << id << " has no items!");
        }

        return subviews;
    }

    vector<SUBVIEWITEM_T>::iterator iter;

    for (iter = slist.items.begin(); iter != slist.items.end(); ++iter)
    {
        if (!haveSubPage(iter->pageID))
        {
            if (!readSubPage(iter->pageID))
                return vector<TSubPage *>();
        }

        TSubPage *pg = getSubPage(iter->pageID);

        if (pg)
            subviews.push_back(pg);
        else
        {
            MSG_DEBUG("No subpage with ID " << id);
        }
    }

    MSG_DEBUG("Found " << subviews.size() << " subview items.");
    return subviews;
}

void TPageManager::showSubViewList(int id, Button::TButton *bt)
{
    DECL_TRACER("TPageManager::showSubViewList(int id, Button::TButton *bt)");

    vector<TSubPage *> subviews = createSubViewList(id);

    if (subviews.empty() || !_addViewButtonItems || !bt)
    {
        MSG_DEBUG("Number views: " << subviews.size() << (_addViewButtonItems ? ", addView" : ", NO addView") << (_displayViewButton ? " display" : " NO display"));
        return;
    }

    ulong btHandle = bt->getHandle();
    MSG_DEBUG("Working on button " << handleToString(btHandle) << " (" << bt->getName() << ") with " << subviews.size() << " pages.");
    TBitmap bm = bt->getLastBitmap();
    TColor::COLOR_T fillColor = TColor::getAMXColor(bt->getFillColor());

    _displayViewButton(btHandle, bt->getParent(), bt->isSubViewVertical(), bm, bt->getWidth(), bt->getHeight(), bt->getLeftPosition(), bt->getTopPosition(), bt->getSubViewSpace(), fillColor);

    vector<PGSUBVIEWITEM_T> items;
    PGSUBVIEWITEM_T svItem;
    PGSUBVIEWATOM_T svAtom;
    vector<TSubPage *>::iterator iter;

    for (iter = subviews.begin(); iter != subviews.end(); ++iter)
    {
        TSubPage *sub = *iter;
        sub->setParent(btHandle);

        svItem.clear();
        Button::TButton *button = sub->getFirstButton();
        SkBitmap bitmap = sub->getBgImage();

        svItem.handle = sub->getHandle();
        svItem.parent = btHandle;
        svItem.width = sub->getWidth();
        svItem.height = sub->getHeight();
        svItem.bgcolor = TColor::getAMXColor(sub->getFillColor());
        svItem.scrollbar = bt->getSubViewScrollbar();
        svItem.scrollbarOffset = bt->getSubViewScrollbarOffset();
        svItem.position = bt->getSubViewAnchor();
        svItem.wrap = bt->getWrapSubViewPages();
        svItem.show = bt->showSubviewItems();
        svItem.dynamic = bt->isSubViewOrderingDynamic();

        if (!bitmap.empty())
            svItem.image.setBitmap((unsigned char *)bitmap.getPixels(), bitmap.info().width(), bitmap.info().height(), bitmap.info().bytesPerPixel());

        while (button)
        {
            button->drawButton(0, false, true);
            svAtom.clear();
            svAtom.handle = button->getHandle();
            svAtom.parent = sub->getHandle();
            svAtom.width = button->getWidth();
            svAtom.height = button->getHeight();
            svAtom.left = button->getLeftPosition();
            svAtom.top = button->getTopPosition();
            svAtom.bgcolor = TColor::getAMXColor(button->getFillColor(button->getActiveInstance()));
            svAtom.bounding = button->getBounding();
            Button::BITMAP_t bmap = button->getLastImage();

            if (bmap.buffer)
                svAtom.image.setBitmap(bmap.buffer, bmap.width, bmap.height, (int)(bmap.rowBytes / bmap.width));

            svItem.atoms.push_back(svAtom);
            button = sub->getNextButton();
        }

        items.push_back(svItem);
    }

    _addViewButtonItems(bt->getHandle(), items);

    if (_pageFinished)
        _pageFinished(bt->getHandle());
}

void TPageManager::updateSubViewItem(Button::TButton *bt)
{
    DECL_TRACER("TPageManager::updateSubViewItem(Button::TButton *bt)");

    if (!bt)
        return;

    updview_mutex.lock();
    mUpdateViews.push_back(bt);
    updview_mutex.unlock();
}

void TPageManager::_updateSubViewItem(Button::TButton *bt)
{
    DECL_TRACER("TPageManager::_updateSubViewItem(Button::TButton *bt)");

    if (!mPageList || !_updateViewButtonItem)
        return;

    // The parent of this kind of button is always the button of type subview.
    // If we take the parent handle and extract the page ID (upper 16 bits)
    // we get the page ID of the subpage or page ID of the page the button is
    // ordered to.
    int pageID = (bt->getParent() >> 16) & 0x0000ffff;
    ulong parent = 0;
    Button::TButton *button = nullptr;
    PGSUBVIEWITEM_T item;
    PGSUBVIEWATOM_T atom;
    SkBitmap bitmap;
    TPage *pg = nullptr;
    TSubPage *sub = nullptr;

    if (pageID < REGULAR_SUBPAGE_START)     // Is it a page?
    {
        pg = getPage(pageID);

        if (!pg)
        {
            MSG_WARNING("Invalid page " << pageID << "!");
            return;
        }

        button = pg->getFirstButton();
        bitmap = pg->getBgImage();

        item.handle = pg->getHandle();
        item.parent = bt->getParent();
        item.width = pg->getWidth();
        item.height = pg->getHeight();
        item.bgcolor = TColor::getAMXColor(pg->getFillColor());
    }
    else
    {
        sub = getSubPage(pageID);

        if (!sub)
        {
            MSG_WARNING("Couldn't find the subpage " << pageID << "!");
            return;
        }

        parent = sub->getParent();
        button = sub->getFirstButton();
        bitmap = sub->getBgImage();

        item.handle = sub->getHandle();
        item.parent = bt->getParent();
        item.width = sub->getWidth();
        item.height = sub->getHeight();
        item.position = bt->getSubViewAnchor();
        item.bgcolor = TColor::getAMXColor(sub->getFillColor());
    }


    if (!bitmap.empty())
        item.image.setBitmap((unsigned char *)bitmap.getPixels(), bitmap.info().width(), bitmap.info().height(), bitmap.info().bytesPerPixel());

    while (button)
    {
        atom.clear();
        atom.handle = button->getHandle();
        atom.parent = item.handle;
        atom.width = button->getWidth();
        atom.height = button->getHeight();
        atom.left = button->getLeftPosition();
        atom.top = button->getTopPosition();
        atom.bgcolor = TColor::getAMXColor(button->getFillColor(button->getActiveInstance()));
        atom.bounding = button->getBounding();
        Button::BITMAP_t bmap = button->getLastImage();

        if (bmap.buffer)
            atom.image.setBitmap(bmap.buffer, bmap.width, bmap.height, (int)(bmap.rowBytes / bmap.width));

        item.atoms.push_back(atom);
        button = (pg ? pg->getNextButton() : sub->getNextButton());
    }

    _updateViewButtonItem(item, parent);
}

void TPageManager::updateActualPage()
{
    DECL_TRACER("TPageManager::updateActualPage()");

    if (!mActualPage)
        return;

    TPage *pg = getPage(mActualPage);
    Button::TButton *bt = pg->getFirstButton();

    while (bt)
    {
        bt->refresh();
        bt = pg->getNextButton();
    }
}

void TPageManager::updateSubpage(int ID)
{
    DECL_TRACER("TPageManager::updateSubpage(int ID)");

    TSubPage *pg = getSubPage(ID);

    if (!pg)
        return;

    vector<Button::TButton *> blist = pg->getAllButtons();
    vector<Button::TButton *>::iterator iter;

    if (blist.empty())
        return;

    for (iter = blist.begin(); iter != blist.end(); ++iter)
    {
        Button::TButton *bt = *iter;
        bt->refresh();
    }
}

void TPageManager::updateSubpage(const std::string &name)
{
    DECL_TRACER("TPageManager::updateSubpage(const std::string &name)");

    TSubPage *pg = getSubPage(name);

    if (!pg)
        return;

    vector<Button::TButton *> blist = pg->getAllButtons();
    vector<Button::TButton *>::iterator iter;

    if (blist.empty())
        return;

    for (iter = blist.begin(); iter != blist.end(); ++iter)
    {
        Button::TButton *bt = *iter;
        bt->refresh();
    }
}

/******************** Internal private methods *********************/

PAGELIST_T TPageManager::findPage(const std::string& name)
{
    DECL_TRACER("TPageManager::findPage(const std::string& name)");

    vector<PAGELIST_T> pageList;

    pageList = mPageList->getPagelist();

    if (pageList.size() > 0)
    {
        vector<PAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->name.compare(name) == 0)
                return *pgIter;
        }
    }

    MSG_WARNING("Page " << name << " not found!");
    return PAGELIST_T();
}

PAGELIST_T TPageManager::findPage(int ID)
{
    DECL_TRACER("TPageManager::findPage(int ID)");

    vector<PAGELIST_T> pageList = (ID < SYSTEM_PAGE_START ? mPageList->getPagelist() : mPageList->getSystemPagelist());

    if (pageList.size() > 0)
    {
        vector<PAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->pageID == ID)
                return *pgIter;
        }
    }

    return PAGELIST_T();
}

SUBPAGELIST_T TPageManager::findSubPage(const std::string& name)
{
    DECL_TRACER("TPageManager::findSubPage(const std::string& name)");

    vector<SUBPAGELIST_T> pageList = mPageList->getSubPageList();

    if (pageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->name.compare(name) == 0)
                return *pgIter;
        }
    }

    return SUBPAGELIST_T();
}

SUBPAGELIST_T TPageManager::findSubPage(int ID)
{
    DECL_TRACER("TPageManager::findSubPage(int ID)");

    vector<SUBPAGELIST_T> pageList = (ID < SYSTEM_PAGE_START ? mPageList->getSubPageList() : mPageList->getSystemSupPageList());

    if (pageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->pageID == ID)
                return *pgIter;
        }
    }

    return SUBPAGELIST_T();
}

bool TPageManager::addPage(TPage* pg)
{
    DECL_TRACER("TPageManager::addPage(TPage* pg)");

    if (!pg)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::SetError();
        return false;
    }

    PCHAIN_T *chain = new PCHAIN_T;
    chain->page = pg;
    chain->next = nullptr;

    if (mPchain)
    {
        PCHAIN_T *p = mPchain;

        while (p->next)
            p = p->next;

        p->next = chain;
    }
    else
    {
        mPchain = chain;
        setPChain(mPchain);
    }

//    MSG_DEBUG("Added page " << chain->page->getName());
    return true;
}

bool TPageManager::addSubPage(TSubPage* pg)
{
    DECL_TRACER("TPageManager::addSubPage(TSubPage* pg)");

    if (!pg)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::SetError();
        return false;
    }

    if (haveSubPage(pg->getNumber()))
    {
        MSG_ERROR("Subpage " << pg->getNumber() << ", " << pg->getName() << " is already in chain!");
        return false;
    }

    SPCHAIN_T *chain = new SPCHAIN_T;
    chain->page = pg;
    chain->next = nullptr;

    if (mSPchain)
    {
        SPCHAIN_T *p = mSPchain;

        while (p->next)
            p = p->next;

        p->next = chain;
    }
    else
    {
        mSPchain = chain;
        setSPChain(mSPchain);
    }

    return true;
}

void TPageManager::dropAllPages()
{
    DECL_TRACER("TPageManager::dropAllPages()");

    PCHAIN_T *pg = mPchain;
    PCHAIN_T *next = nullptr;

    while (pg)
    {
        next = pg->next;

        if (pg->page)
        {
            if (_callDropPage)
                _callDropPage((pg->page->getNumber() << 16) & 0xffff0000);

            delete pg->page;
        }

        delete pg;
        pg = next;
    }

    mPchain = nullptr;
    setPChain(mPchain);
}

void TPageManager::dropAllSubPages()
{
    DECL_TRACER("TPageManager::dropAllSubPages()");

    SPCHAIN_T *spg = mSPchain;
    SPCHAIN_T *next;

    while (spg)
    {
        next = spg->next;

        if (spg->page)
        {
            if (_callDropSubPage)
                _callDropSubPage((spg->page->getNumber() << 16) & 0xffff0000, spg->page->getParent());

            delete spg->page;
        }

        delete spg;
        spg = next;
    }

    mSPchain = nullptr;
    setSPChain(mSPchain);
}

bool TPageManager::destroyAll()
{
    DECL_TRACER("TPageManager::destroyAll()");

    dropAllSubPages();
    dropAllPages();
    mActualPage = 0;
    mPreviousPage = 0;
    mActualGroupName.clear();

    if (mPageList)
    {
        delete mPageList;
        mPageList = nullptr;
    }

    if (mTSettings)
    {
        delete mTSettings;
        mTSettings = nullptr;
    }

    if (mPalette)
    {
        delete mPalette;
        mPalette = nullptr;
    }

    if (mFonts)
    {
        delete mFonts;
        mFonts = nullptr;
    }

    if (mExternal)
    {
        delete mExternal;
        mExternal = nullptr;
    }

    if (gPrjResources)
    {
        delete gPrjResources;
        gPrjResources = nullptr;
    }

    if (gIcons)
    {
        delete gIcons;
        gIcons = nullptr;
    }

    if (TError::isError())
        return false;

    return true;
}

bool TPageManager::overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    DECL_TRACER("TPageManager::overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)");

    struct point
    {
        int x;
        int y;
    };

    struct point l1, r1, l2, r2;

    l1.x = x1;
    l1.y = y1;
    r1.x = x1 + w1;
    r1.y = y1 + h1;

    l2.x = x2;
    l2.y = y2;
    r2.x = x2 + w2;
    r2.y = y2 + h2;

    if (l1.x == r1.x || l1.y == r1.y || l2.x == r2.x || l2.y == r2.y)
    {
        // the line cannot have positive overlap
        return false;
    }

    return std::max(l1.x, l2.x) < std::min(r1.x, r2.x) &&
           std::max(l1.y, l2.y) < std::min(r1.y, r2.y);
}

Button::TButton *TPageManager::findButton(ulong handle)
{
    DECL_TRACER("TPageManager::findButton(ulong handle)");

    if (!handle)
        return nullptr;

    TPage *pg = getPage(mActualPage);

    if (!pg)
        return nullptr;

    vector<Button::TButton *> pgBtList = pg->getAllButtons();
    vector<Button::TButton *>::iterator iter;

    if (pgBtList.size() > 0)
    {
        // First we look into the elements of the page
        for (iter = pgBtList.begin(); iter != pgBtList.end(); ++iter)
        {
            Button::TButton *bt = *iter;

            if (bt->getHandle() == handle)
                return bt;
        }
    }

    // We've not found the wanted element in the elements of the page. So
    // we're looking at the elements of the subpages.
    TSubPage *sp = pg->getFirstSubPage();

    if (!sp)
        return nullptr;

    while (sp)
    {
        vector<Button::TButton *> spBtList = sp->getAllButtons();

        if (spBtList.size() > 0)
        {
            for (iter = spBtList.begin(); iter != spBtList.end(); ++iter)
            {
                Button::TButton *bt = *iter;

                if (bt->getHandle() == handle)
                    return bt;
            }
        }

        sp = pg->getNextSubPage();
    }

    return nullptr;
}

/**
 * @brief Find a bargraph
 * The method can search for a bargraph on a particular page or subpage defined
 * by \b parent. if no page or subpage defined, it searches on the actual page
 * or subpage.
 * If it finds a bargraph it returns the pointer to it. Otherwise a nullptr is
 * returned.
 *
 * @param lp        The brgraph port number.
 * @param lv        The bargraph code number.
 * @param parent    The parent handle of the bargraph.
 *
 * @return If the wanted bargraph with the parent handle exists, the pointer to
 * it is returned. Otherwise it return a \b nullptr.
 */
Button::TButton *TPageManager::findBargraph(int lp, int lv, ulong parent)
{
    DECL_TRACER("TPageManager::findBargraph(int lp, int lv, ulong parent)");

    int page = (parent >> 16) & 0x0000ffff;
    vector<Button::TButton *>::iterator iter;

    if (!page)
    {
        page = mActualPage;

        if (!page)
        {
            MSG_WARNING("No valid active page!");
            return nullptr;
        }
    }

    MSG_DEBUG("Searching for bargraph " << lp << ":" << lv << " on page " << page);

    if (page < REGULAR_SUBPAGE_START)
    {
        TPage *pg = getPage(mActualPage);

        if (!pg)
            return nullptr;

        vector<Button::TButton *> pgBtList = pg->getAllButtons();
        MSG_DEBUG("Found " << pgBtList.size() << " buttons.");

        if (pgBtList.size() > 0)
        {
            // First we look into the elements of the page
            for (iter = pgBtList.begin(); iter != pgBtList.end(); ++iter)
            {
                Button::TButton *bt = *iter;

                if (bt->getButtonType() == BARGRAPH && bt->getLevelPort() == lp && bt->getLevelChannel() == lv && bt->getParent() == parent)
                {
                    MSG_DEBUG("Found bargraph LP:" << lp << ", LV:" << lv << " on page " << page);
                    return bt;
                }
            }
        }

        MSG_WARNING("No bargraph " << lp << ":" << lv << " on page " << page);
        return nullptr;
    }

    // We've not found the wanted element in the elements of the page. So
    // we're looking at the elements of the subpage.
    TSubPage *sp = getSubPage(page);

    if (!sp)
    {
        MSG_WARNING("Found no subpage " << page);
        return nullptr;
    }

    vector<Button::TButton *> spBtList = sp->getAllButtons();
    MSG_DEBUG("Found " << spBtList.size() << " buttons.");

    if (spBtList.size() > 0)
    {
        for (iter = spBtList.begin(); iter != spBtList.end(); ++iter)
        {
            Button::TButton *bt = *iter;

            if (bt->getButtonType() == BARGRAPH && bt->getLevelPort() == lp && bt->getLevelChannel() == lv && bt->getParent() == parent)
            {
                MSG_DEBUG("Found bargraph LP:" << lp << ", LV:" << lv << " on subpage " << page);
                return bt;
            }
        }
    }

    MSG_WARNING("No bargraph " << lp << ":" << lv << " on subpage " << page);
    return nullptr;
}

TPage *TPageManager::getActualPage()
{
    DECL_TRACER("TPageManager::getActualPage()");

    return getPage(mActualPage);
}

TSubPage *TPageManager::getFirstSubPage()
{
    DECL_TRACER("TPageManager::getFirstSubPage()");

    mLastSubPage = 0;
    TPage *pg = getPage(mActualPage);

    if (!pg)
        return nullptr;

    map<int, TSubPage *> sp = pg->getSortedSubpages(true);

    if (!sp.empty())
    {
        map<int, TSubPage *>::iterator iter = sp.begin();
        mLastSubPage = iter->first;
        return iter->second;
    }

    return nullptr;
}

TSubPage *TPageManager::getNextSubPage()
{
    DECL_TRACER("TPageManager::getNextSubPage()");

    TPage *pg = getPage(mActualPage);

    if (pg)
    {
        map<int, TSubPage *> sp = pg->getSortedSubpages();

        if (sp.empty())
        {
            mLastSubPage = 0;
            return nullptr;
        }
        else
        {
            map<int, TSubPage *>::iterator iter = sp.find(mLastSubPage);

            if (iter != sp.end())
            {
                iter++;

                if (iter != sp.end())
                {
                    mLastSubPage = iter->first;
                    return iter->second;
                }
            }
        }
    }

    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPageManager::getPrevSubPage()
{
    DECL_TRACER("TPageManager::getPrevSubPage()");

    TPage *pg = getPage(mActualPage);

    if (pg)
    {
        map<int, TSubPage *> sp = pg->getSortedSubpages();

        if (sp.empty())
        {
            mLastSubPage = 0;
            return nullptr;
        }
        else
        {
            map<int, TSubPage *>::iterator iter = sp.find(mLastSubPage);

            if (iter != sp.end() && iter != sp.begin())
            {
                iter--;
                mLastSubPage = iter->first;
                return iter->second;
            }

            MSG_DEBUG("Page " << mLastSubPage << " not found!");
        }
    }

    mLastSubPage = 0;
    return nullptr;
}

TSubPage *TPageManager::getLastSubPage()
{
    DECL_TRACER("TPageManager::getLastSubPage()");

    mLastSubPage = 0;
    TPage *pg = getPage(mActualPage);

    if (pg)
    {
        map<int, TSubPage *> sp = pg->getSortedSubpages(true);

        if (sp.empty())
            return nullptr;
        else
        {
            map<int, TSubPage *>::iterator iter = sp.end();
            iter--;
            mLastSubPage = iter->first;
            return iter->second;
        }
    }
    else
    {
        MSG_WARNING("Actual page " << mActualPage << " not found!");
    }

    return nullptr;
}

TSubPage *TPageManager::getFirstSubPageGroup(const string& group)
{
    DECL_TRACER("TPageManager::getFirstSubPageGroup(const string& group)");

    if (group.empty())
    {
        MSG_WARNING("Empty group name is invalid. Ignoring it!");
        mActualGroupName.clear();
        mActualGroupPage = nullptr;
        return nullptr;
    }

    mActualGroupName = group;
    TSubPage *pg = getFirstSubPage();

    while (pg)
    {
        MSG_DEBUG("Evaluating group " << pg->getGroupName() << " with " << group);

        if (pg->getGroupName().compare(group) == 0)
        {
            mActualGroupPage = pg;
            return pg;
        }

        pg = getNextSubPage();
    }

    mActualGroupName.clear();
    mActualGroupPage = nullptr;
    return nullptr;
}

TSubPage *TPageManager::getNextSubPageGroup()
{
    DECL_TRACER("TPageManager::getNextSubPageGroup()");

    if (mActualGroupName.empty())
        return nullptr;

    TSubPage *pg = getFirstSubPage();
    bool found = false;

    while (pg)
    {
        MSG_DEBUG("Evaluating group " << pg->getGroupName() << " with " << mActualGroupName);

        if (!found && pg == mActualGroupPage)
        {
            pg = getNextSubPage();
            found = true;
            continue;
        }

        if (found && pg->getGroupName().compare(mActualGroupName) == 0)
        {
            mActualGroupPage = pg;
            return pg;
        }

        pg = getNextSubPage();
    }

    mActualGroupName.clear();
    mActualGroupPage = nullptr;
    return nullptr;
}

TSubPage *TPageManager::getNextSubPageGroup(const string& group, TSubPage* pg)
{
    DECL_TRACER("TPageManager::getNextSubPageGroup(const string& group, TSubPage* pg)");

    if (group.empty() || !pg)
        return nullptr;

    TSubPage *page = getFirstSubPage();
    bool found = false;

    while (page)
    {
        MSG_DEBUG("Evaluating group " << pg->getGroupName() << " with " << group);

        if (!found && pg == page)
        {
            page = getNextSubPage();
            found = true;
            continue;
        }

        if (found && page->getGroupName().compare(group) == 0)
            return page;

        page = getNextSubPage();
    }

    return nullptr;
}

TSubPage *TPageManager::getTopPage()
{
    DECL_TRACER("TPageManager::getTopPage()");

    // Scan for all occupied regions
    vector<RECT_T> regions;

    TSubPage *pg = getFirstSubPage();

    while (pg)
    {
        RECT_T r = pg->getRegion();
        regions.push_back(r);
        pg = getNextSubPage();
    }

    // Now scan all pages against all regions to find the top most
    pg = getFirstSubPage();
    TSubPage *top = nullptr;
    int zPos = 0;

    while (pg)
    {
        RECT_T r = pg->getRegion();

        if (regions.size() > 0)
        {
            vector<RECT_T>::iterator iter;
            int zo = 0;

            for (iter = regions.begin(); iter != regions.end(); ++iter)
            {
                if (doOverlap(*iter, r) && zPos > zo)
                    top = pg;

                zo++;
            }
        }

        pg = getNextSubPage();
        zPos++;
    }

    return top;
}

TSubPage *TPageManager::getCoordMatch(int x, int y)
{
    DECL_TRACER("TPageManager::getCoordMatch(int x, int y)");

    int realX = x;
    int realY = y;

    // Reverse order of pages
    TSubPage *pg = getLastSubPage();

    // Iterate in reverse order through array
    while (pg)
    {
        if (!pg->isVisible() || pg->getZOrder() == ZORDER_INVALID)
        {
            pg = getPrevSubPage();
            continue;
        }

        MSG_DEBUG("Scanning subpage (Z: " << pg->getZOrder() << "): " << pg->getNumber() << ", " << pg->getName());
        RECT_T r = pg->getRegion();

        if (r.left <= realX && (r.left + r.width) >= realX &&
            r.top <= realY && (r.top + r.height) >= realY)
        {
            MSG_DEBUG("Click matches subpage " << pg->getNumber() << " (" << pg->getName() << ")");
            return pg;
        }

        pg = getPrevSubPage();
    }

    return nullptr;
}

Button::TButton *TPageManager::getCoordMatchPage(int x, int y)
{
    DECL_TRACER("TPageManager::getCoordMatchPage(int x, int y)");

    TPage *page = getActualPage();

    if (page)
    {
        Button::TButton *bt = page->getLastButton();

        while (bt)
        {
            bool clickable = bt->isClickable();
            MSG_DEBUG("Button: " << bt->getButtonIndex() << ", l: " << bt->getLeftPosition() << ", t: " << bt->getTopPosition() << ", r: " << (bt->getLeftPosition() + bt->getWidth()) << ", b: " << (bt->getTopPosition() + bt->getHeight()) << ", x: " << x << ", y: " << y << ", " << (clickable ? "CLICKABLE" : "NOT CLICKABLE"));

            if (!clickable)
            {
                bt = page->getPreviousButton();
                continue;
            }

            if (bt->getLeftPosition() <= x && (bt->getLeftPosition() + bt->getWidth()) >= x &&
                bt->getTopPosition() <= y && (bt->getTopPosition() + bt->getHeight()) >= y)
            {
                if (!bt->isClickable(x - bt->getLeftPosition(), y - bt->getTopPosition()))
                {
                    bt = page->getPreviousButton();
                    continue;
                }

                MSG_DEBUG("Click matches button " << bt->getButtonIndex() << " (" << bt->getButtonName() << ")");
                return bt;
            }

            bt = page->getPreviousButton();
        }
    }

    return nullptr;
}

bool TPageManager::doOverlap(RECT_T r1, RECT_T r2)
{
    DECL_TRACER("TPageManager::doOverlap(RECT_T r1, RECT_T r2)");

    // If one rectangle is on left side of other
    if (r1.left >= r2.left || r2.left >= r1.left)
        return false;

    // If one rectangle is above other
    if (r1.top <= r2.top || r2.top <= r1.top)
        return false;

    return true;
}

bool TPageManager::havePage(const string& name)
{
    DECL_TRACER("TPageManager::havePage(const string& name)");

    if (name.empty())
        return false;

    PCHAIN_T *pg = mPchain;

    while (pg)
    {
        if (pg->page && pg->page->getName().compare(name) == 0)
            return true;

        pg = pg->next;
    }

    return false;
}

bool TPageManager::haveSubPage(const string& name)
{
    DECL_TRACER("TPageManager::haveSubPage(const string& name)");

    if (name.empty())
        return false;

    SPCHAIN_T *pg = mSPchain;

    while (pg)
    {
        if (pg->page && pg->page->getName().compare(name) == 0)
        {
            MSG_DEBUG("Subpage " << pg->page->getNumber() << ", " << name << " found.");
            return true;
        }

        pg = pg->next;
    }

    MSG_DEBUG("Subpage " << name << " not found.");
    return false;
}

bool TPageManager::haveSubPage(int id)
{
    DECL_TRACER("TPageManager::haveSubPage(int id)");

    SPCHAIN_T *pg = mSPchain;

    while (pg)
    {
        if (pg->page && pg->page->getNumber() == id)
        {
            MSG_DEBUG("Subpage " << pg->page->getNumber() << ", " << pg->page->getName() << " found.");
            return true;
        }

        pg = pg->next;
    }

    MSG_DEBUG("Subpage " << id << " not found.");
    return false;
}

bool TPageManager::haveSubPage(const string& page, const string& name)
{
    DECL_TRACER("TPageManager::haveSubPage(const string& page, const string& name)");

    TPage *pg = getPage(page);

    if (!pg)
        return false;

    TSubPage *spg = pg->getFirstSubPage();

    while (spg)
    {
        if (spg->getName().compare(name) == 0)
        {
            MSG_DEBUG("Subpage " << spg->getNumber() << ", " << name << " found.");
            return true;
        }

        spg = pg->getNextSubPage();
    }

    MSG_DEBUG("Subpage " << name << " not found on page " << page << ".");
    return false;
}

bool TPageManager::haveSubPage(const string& page, int id)
{
    DECL_TRACER("TPageManager::haveSubPage(const string& page, int id)");

    TPage *pg = getPage(page);

    if (!pg)
        return false;

    TSubPage *spg = pg->getFirstSubPage();

    while (spg)
    {
        if (spg->getNumber() == id)
        {
            MSG_DEBUG("Subpage " << spg->getNumber() << ", " << spg->getName() << " found.");
            return true;
        }

        spg = pg->getNextSubPage();
    }

    MSG_DEBUG("Subpage " << id << " on page " << page << " not found.");
    return false;
}

void TPageManager::closeGroup(const string& group)
{
    DECL_TRACER("TPageManager::closeGroup(const string& group)");

    SPCHAIN_T *pg = mSPchain;

    while (pg)
    {
        if (pg->page->getGroupName().compare(group) == 0 && pg->page->isVisible())
        {
            pg->page->regCallDropSubPage(_callDropSubPage);
            pg->page->drop();
            break;
        }

        pg = pg->next;
    }
}

void TPageManager::showSubPage(const string& name)
{
    DECL_TRACER("TPageManager::showSubPage(const string& name)");

    if (name.empty())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    TPage *page = nullptr;
    TSubPage *pg = deliverSubPage(name, &page);

    if (!pg)
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    if (page)
    {
        pg->setParent(page->getHandle());
        page->addSubPage(pg);
    }

    string group = pg->getGroupName();

    if (!group.empty())
    {
        TSubPage *sub = getFirstSubPageGroup(group);

        while(sub)
        {
            if (sub->isVisible() && sub->getNumber() != pg->getNumber())
                sub->drop();

            sub = getNextSubPageGroup(group, sub);
        }
    }

    if (pg->isVisible())
    {
        MSG_DEBUG("Page " << pg->getName() << " is already visible but maybe not on top.");

        TSubPage *sub = getFirstSubPage();
        bool redraw = false;

        while (sub)
        {
            if (sub->isVisible() && pg->getZOrder() < sub->getZOrder() &&
                overlap(sub->getLeft(), sub->getTop(), sub->getWidth(), sub->getHeight(),
                pg->getLeft(), pg->getTop(), pg->getWidth(), pg->getHeight()))
            {
                MSG_DEBUG("Page " << sub->getName() << " is overlapping page " << pg->getName());
                redraw = true;
                break;
            }

            sub = getNextSubPage();
        }

        if (redraw && _toFront)
        {
            _toFront((uint)pg->getHandle());
            pg->setZOrder(page->getNextZOrder());
//            page->sortSubpages();
            MSG_DEBUG("Setting new Z-order " << page->getActZOrder() << " on subpage " << pg->getName());
        }
        else if (redraw && !_toFront)
            pg->drop();
    }

    if (!pg->isVisible())
    {
        if (!page)
        {
            page = getPage(mActualPage);

            if (!page)
            {
                MSG_ERROR("No active page found! Internal error.");
                return;
            }
        }

        if (!haveSubPage(pg->getNumber()) && !page->addSubPage(pg))
            return;

        pg->setZOrder(page->getNextZOrder());

        if (_setSubPage)
        {
            int left = pg->getLeft();
            int top = pg->getTop();
            int width = pg->getWidth();
            int height = pg->getHeight();
#ifdef _SCALE_SKIA_
            if (mScaleFactor != 1.0)
            {
                left = (int)((double)left * mScaleFactor);
                top = (int)((double)top * mScaleFactor);
                width = (int)((double)width * mScaleFactor);
                height = (int)((double)height * mScaleFactor);
                MSG_DEBUG("Scaled subpage: left=" << left << ", top=" << top << ", width=" << width << ", height=" << height);
            }
#endif
            ANIMATION_t ani;
            pg->initAnimation(pg, &ani);

            // Test for a timer on the page
            if (pg->getTimeout() > 0)
                pg->startTimer();

            _setSubPage(pg->getHandle(), page->getHandle(), left, top, width, height, ani, pg->isModal(), pg->isCollapsible());
        }

        pg->show();
        // If the page requires other pages to show or hide we do this here.
        // The loop is calling this method recursively.
        if (TTPInit::isG5())
        {
            PAGE_T page = pg->getSubPage();

            if (!page.eventShow.empty())
            {
                vector<EVENT_t>::iterator iter;

                for (iter = page.eventShow.begin(); iter != page.eventShow.end(); ++iter)
                {
                    if (iter->evType == EV_PGFLIP)
                        showSubPage(iter->name);
                }
            }

            if (!page.eventHide.empty())
            {
                vector<EVENT_t>::iterator iter;

                for (iter = page.eventHide.begin(); iter != page.eventHide.end(); ++iter)
                {
                    if (iter->evType == EV_PGFLIP)
                        hideSubPage(iter->name);
                }
            }
        }
    }
}

void TPageManager::showSubPage(int number, bool force)
{
    DECL_TRACER("TPageManager::showSubPage(int number, bool force)");

    if (number <= 0)
        return;

    TPage *page = nullptr;
    TSubPage *pg = deliverSubPage(number, &page);

    if (!pg)
        return;

    if (page)
    {
        pg->setParent(page->getHandle());
        page->addSubPage(pg);
    }

    string group = pg->getGroupName();

    if (!group.empty())
    {
        TSubPage *sub = getFirstSubPageGroup(group);

        while(sub)
        {
            if (sub->isVisible() && sub->getNumber() != pg->getNumber())
                sub->drop();

            sub = getNextSubPageGroup(group, sub);
        }
    }

    if (pg->isVisible() && !force)
    {
        MSG_DEBUG("Page " << pg->getName() << " is already visible but maybe not on top.");

        TSubPage *sub = getFirstSubPage();
        bool redraw = false;

        while (sub)
        {
            if (sub->isVisible() && pg->getZOrder() < sub->getZOrder() &&
                overlap(sub->getLeft(), sub->getTop(), sub->getWidth(), sub->getHeight(),
                        pg->getLeft(), pg->getTop(), pg->getWidth(), pg->getHeight()))
            {
                MSG_DEBUG("Page " << sub->getName() << " is overlapping page " << pg->getName());
                redraw = true;
                break;
            }

            sub = getNextSubPage();
        }

        if (redraw && _toFront)
        {
            _toFront((uint)pg->getHandle());
            pg->setZOrder(page->getNextZOrder());
            page->sortSubpages();
            MSG_DEBUG("Setting new Z-order " << page->getActZOrder() << " on subpage " << pg->getName());
        }
        else if (redraw && !_toFront)
            pg->drop();
    }

    if (!pg->isVisible() || force)
    {
        if (!page)
        {
            MSG_ERROR("No active page found! Internal error.");
            return;
        }

        if (!haveSubPage(pg->getNumber()) && !page->addSubPage(pg))
            return;

        if (!pg->isVisible())
            pg->setZOrder(page->getNextZOrder());

        if (_setSubPage)
        {
            int left = pg->getLeft();
            int top = pg->getTop();
            int width = pg->getWidth();
            int height = pg->getHeight();
#ifdef _SCALE_SKIA_
            if (mScaleFactor != 1.0)
            {
                left = (int)((double)left * mScaleFactor);
                top = (int)((double)top * mScaleFactor);
                width = (int)((double)width * mScaleFactor);
                height = (int)((double)height * mScaleFactor);
                MSG_DEBUG("Scaled subpage: left=" << left << ", top=" << top << ", width=" << width << ", height=" << height);
            }
#endif
            ANIMATION_t ani;
            pg->initAnimation(pg, &ani);
            // Test for a timer on the page
            if (pg->getTimeout() > 0)
                pg->startTimer();

            _setSubPage(pg->getHandle(), page->getHandle(), left, top, width, height, ani, pg->isModal(), pg->isCollapsible());
        }
    }

    pg->show();
}

void TPageManager::hideSubPage(const string& name)
{
    DECL_TRACER("TPageManager::hideSubPage(const string& name)");

    if (name.empty())
    {
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
#if TESTMODE == 1
        setScreenDone();
#endif
        return;
    }

    TSubPage *pg = getSubPage(name);

    if (pg)
    {
        pg->drop();
        page->decZOrder();
    }
}

TSubPage *TPageManager::loadSubPage(const string& name)
{
    DECL_TRACER("TPageManager::loadSubPage(const string& name)");

    if (name.empty())
    {
        MSG_WARNING("Got no name to load a popup!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return nullptr;
    }

    TPage *page = nullptr;
    TSubPage *pg = deliverSubPage(name, &page);

    if (!pg)
    {
        MSG_WARNING("Subpage " << name << " has no parent!");
#if TESTMODE == 1
        setScreenDone();
#endif
        return nullptr;
    }

    if (page)
    {
        pg->setParent(page->getHandle());
        page->addSubPage(pg);
    }

    return pg;
}

/**
 * @brief TPageManager::runClickQueue - Processing mouse clicks
 * The following method is starting a thread which tests a queue containing
 * the mouse clicks. To not drain the CPU, it sleeps for a short time if there
 * are no more events in the queue.
 * If there is an entry in the queue, it copies it to a local struct and
 * deletes it from the queue. It take always the oldest antry (first entry)
 * and removes this entry from the queue until the queue is empty. This makes
 * it to a FIFO (first in, first out).
 * Depending on the state of the variable "coords" the method for mouse
 * coordinate click is executed or the method for a handle.
 * The thread runs as long as the variable "mClickQueueRun" is TRUE and the
 * variable "prg_stopped" is FALSE.
 */
void TPageManager::runClickQueue()
{
    DECL_TRACER("TPageManager::runClickQueue()");

    if (mClickQueueRun)
        return;

    mClickQueueRun = true;

    try
    {
        std::thread thr = std::thread([=] {
            MSG_PROTOCOL("Thread \"TPageManager::runClickQueue()\" was started.");

            while (mClickQueueRun && !prg_stopped)
            {
                while (!mClickQueue.empty())
                {
#ifdef QT_DEBUG
                    if (mClickQueue[0].coords)
                        MSG_TRACE("TPageManager::runClickQueue() -- executing: _mouseEvent(" << mClickQueue[0].x << ", " << mClickQueue[0].y << ", " << (mClickQueue[0].pressed ? "TRUE" : "FALSE") << ")")
                    else
                        MSG_TRACE("TPageManager::runClickQueue() -- executing: _mouseEvent(" << handleToString(mClickQueue[0].handle) << ", " << (mClickQueue[0].pressed ? "TRUE" : "FALSE") << ")")
#endif
                    if (mClickQueue[0].eventType == _EVENT_MOUSE_CLICK)
                    {
                        if (mClickQueue[0].coords)
                            _mouseEvent(mClickQueue[0].x, mClickQueue[0].y, mClickQueue[0].pressed);
                        else
                            _mouseEvent(mClickQueue[0].handle, mClickQueue[0].x, mClickQueue[0].y, mClickQueue[0].pressed);
                    }
                    else  if (mClickQueue[0].eventType == _EVENT_MOUSE_MOVE)
                        _mouseMoveEvent(mClickQueue[0].x, mClickQueue[0].y);

                    mClickQueue.erase(mClickQueue.begin()); // Remove first entry
                }

                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            mClickQueueRun = false;
            return;
        });

        thr.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting a thread to handle the click queue: " << e.what());
        mClickQueueRun = false;
    }
}

void TPageManager::runUpdateSubViewItem()
{
    DECL_TRACER("TPageManager::runUpdateSubViewItem()");

    if (mUpdateViewsRun)
        return;

    mUpdateViewsRun = true;

    try
    {
        std::thread thr = std::thread([=] {
            MSG_PROTOCOL("Thread \"TPageManager::runUpdateSubViewItem()\" was started.");

            while (mUpdateViewsRun && !prg_stopped)
            {
                while (!mUpdateViews.empty())
                {
                    _updateSubViewItem(mUpdateViews[0]);
                    mUpdateViews.erase(mUpdateViews.begin()); // Remove first entry
                }

                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            mUpdateViewsRun = false;
            return;
        });

        thr.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting a thread to handle the click queue: " << e.what());
        mUpdateViewsRun = false;
    }
}

/*
 * Catch the mouse presses and scan all pages and subpages for an element to
 * receive the klick.
 */
void TPageManager::mouseEvent(int x, int y, bool pressed)
{
    DECL_TRACER("TPageManager::mouseEvent(int x, int y, bool pressed)");

    TTRYLOCK(click_mutex);

    _CLICK_QUEUE_t cq;
    cq.eventType = _EVENT_MOUSE_CLICK;
    cq.x = x;
    cq.y = y;
    cq.pressed = pressed;
    cq.coords = true;
    mClickQueue.push_back(cq);
#if TESTMODE == 1
    setScreenDone();
#endif
}

void TPageManager::mouseMoveEvent(int x, int y)
{
    DECL_TRACER("TPageManager::mouseMoveEvent(int x, int y)");

    TTRYLOCK(click_mutex);

    _CLICK_QUEUE_t cq;
    cq.eventType = _EVENT_MOUSE_MOVE;
    cq.x = x;
    cq.y = y;
    cq.coords = true;
    mClickQueue.push_back(cq);
#if TESTMODE == 1
    setScreenDone();
#endif
}

void TPageManager::_mouseEvent(int x, int y, bool pressed)
{
    DECL_TRACER("TPageManager::_mouseEvent(int x, int y, bool pressed)");

    TError::clear();
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setMouseClick(x, y, pressed);
#endif
    int realX = x - mFirstLeftPixel;
    int realY = y - mFirstTopPixel;

    MSG_DEBUG("Mouse at " << realX << ", " << realY << ", state " << ((pressed) ? "PRESSED" : "RELEASED") << ", [ " << x << " | " << y << " ]");
#ifdef _SCALE_SKIA_
    if (mScaleFactor != 1.0 && mScaleFactor > 0.0)
    {
        realX = (int)((double)realX / mScaleFactor);
        realY = (int)((double)realY / mScaleFactor);
        MSG_DEBUG("Scaled coordinates: x=" << realX << ", y=" << realY);
    }
#endif

    TSubPage *subPage = nullptr;

    if (pressed)
        subPage = getCoordMatch(realX, realY);
    else if (mLastPagePush)
        subPage = getSubPage(mLastPagePush);
    else
        subPage = getCoordMatch(realX, realY);

    if (!subPage)
    {
        Button::TButton *bt = getCoordMatchPage(realX, realY);

        if (bt)
        {
            MSG_DEBUG("Button on page " << bt->getButtonIndex() << ": size: left=" << bt->getLeftPosition() << ", top=" << bt->getTopPosition() << ", width=" << bt->getWidth() << ", height=" << bt->getHeight());
            bt->doClick(x - bt->getLeftPosition(), y - bt->getTopPosition(), pressed);
        }

        if (pressed)
            mLastPagePush = getActualPageNumber();

        return;
    }

    MSG_DEBUG("Subpage " << subPage->getNumber() << " [" << subPage->getName() << "]: size: left=" << subPage->getLeft() << ", top=" << subPage->getTop() << ", width=" << subPage->getWidth() << ", height=" << subPage->getHeight());

    if (pressed)
        mLastPagePush = subPage->getNumber();
    else
        mLastPagePush = 0;

    subPage->doClick(realX - subPage->getLeft(), realY - subPage->getTop(), pressed);
}

void TPageManager::_mouseMoveEvent(int x, int y)
{
    DECL_TRACER("TPageManager::_mouseMoveEvent(int x, int y)");

    int realX = x - mFirstLeftPixel;
    int realY = y - mFirstTopPixel;

#ifdef _SCALE_SKIA_
    if (mScaleFactor != 1.0 && mScaleFactor > 0.0)
    {
        realX = (int)((double)realX / mScaleFactor);
        realY = (int)((double)realY / mScaleFactor);
        MSG_DEBUG("Scaled coordinates: x=" << realX << ", y=" << realY);
    }
#endif

    TSubPage *subPage = nullptr;
    subPage = getCoordMatch(realX, realY);

    if (!subPage)
    {
        Button::TButton *bt = getCoordMatchPage(realX, realY);

        if (bt)
        {
            if (bt->getButtonType() == BARGRAPH)
                bt->moveBargraphLevel(realX - bt->getLeftPosition(), realY - bt->getTopPosition());
            else if (bt->getButtonType() == JOYSTICK && !bt->getLevelFuction().empty())
            {
                bt->drawJoystick(realX - bt->getLeftPosition(), realY - bt->getTopPosition());
                // Send the levels
                bt->sendJoystickLevels();
            }
        }

        return;
    }

    subPage->moveMouse(realX - subPage->getLeft(), realY - subPage->getTop());
}

void TPageManager::mouseEvent(ulong handle, int x, int y, bool pressed)
{
    DECL_TRACER("TPageManager::mouseEvent(ulong handle, int x, int y, bool pressed)");

    if (!mClickQueue.empty() && mClickQueue.back().handle == handle && mClickQueue.back().pressed == pressed)
        return;

    TLOCKER(click_mutex);

    _CLICK_QUEUE_t cq;
    cq.handle = handle;
    cq.x = x;
    cq.y = y;
    cq.pressed = pressed;
    mClickQueue.push_back(cq);
    MSG_DEBUG("Queued click for handle " << handleToString(cq.handle) << " at coordinate " << x << "x" << y << ", state " << (cq.pressed ? "PRESSED" : "RELEASED"));
}

void TPageManager::_mouseEvent(ulong handle, int x, int y, bool pressed)
{
    DECL_TRACER("TPageManager::_mouseEvent(ulong handle, int x, int y, bool pressed)");

    MSG_DEBUG("Doing click for handle " << handleToString(handle) << " at coord " << x << "x" << y << ", state " << (pressed ? "PRESSED" : "RELEASED"));

    if (!handle)
        return;

    int pageID = (handle >> 16) & 0x0000ffff;
    int buttonID = (handle & 0x0000ffff);

    if (pageID < REGULAR_SUBPAGE_START || buttonID == 0)
        return;

    TSubPage *subPage = getSubPage(pageID);

    if (subPage)
    {
        Button::TButton *bt = subPage->getButton(buttonID);

        if (bt)
        {
            MSG_DEBUG("Button on subpage " << pageID << ": " << buttonID);

            if (x > 0 && y > 0)
                bt->doClick(x, y, pressed);
            else
                bt->doClick(bt->getLeftPosition() + bt->getWidth() / 2, bt->getTopPosition() + bt->getHeight() / 2, pressed);
        }
    }
}

void TPageManager::inputButtonFinished(ulong handle, const std::string &content)
{
    DECL_TRACER("TPageManager::inputButtonFinished(ulong handle, const std::string &content)");

    Button::TButton *bt = findButton(handle);

    if (!bt)
    {
        MSG_WARNING("Invalid button handle " << handleToString(handle));
        return;
    }

    bt->setTextOnly(content, -1);
}

void TPageManager::inputCursorPositionChanged(ulong handle, int oldPos, int newPos)
{
    DECL_TRACER("TPageManager::inputCursorPositionChanged(ulong handle, int oldPos, int newPos)");

    Button::TButton *bt = findButton(handle);

    if (!bt)
    {
        MSG_WARNING("Invalid button handle " << handleToString(handle));
        return;
    }

    ulong pageID = (bt->getHandle() >> 16) & 0x0000ffff;

    if (pageID < REGULAR_SUBPAGE_START)
    {
        TPage *pg = getPage((int)pageID);

        if (!pg)
            return;

        pg->setCursorPosition(handle, oldPos, newPos);
    }
    else
    {
        TSubPage *pg = getSubPage((int)pageID);

        if (!pg)
            return;

        pg->setCursorPosition(handle, oldPos, newPos);
    }
}

void TPageManager::inputFocusChanged(ulong handle, bool in)
{
    DECL_TRACER("TPageManager::inputFocusChanged(ulong handle, bool in)");

    Button::TButton *bt = findButton(handle);

    if (!bt)
    {
        MSG_WARNING("Invalid button handle " << handleToString(handle));
        return;
    }

    ulong pageID = (bt->getHandle() >> 16) & 0x0000ffff;
    MSG_DEBUG("Searching for page " << pageID);

    if (pageID < REGULAR_SUBPAGE_START)
    {
        TPage *pg = getPage((int)pageID);

        if (!pg)
            return;

        pg->setInputFocus(handle, in);
    }
    else
    {
        TSubPage *pg = getSubPage((int)pageID);

        if (!pg)
            return;

        pg->setInputFocus(handle, in);
    }
}

void TPageManager::setTextToButton(ulong handle, const string& txt, bool redraw)
{
    DECL_TRACER("TPageManager::setTextToButton(ulong handle, const string& txt, bool redraw)");

    // First we search for the button the handle points to
    Button::TButton *button = findButton(handle);

    if (!button)
    {
        MSG_ERROR("No button with handle " << handleToString(handle) << " found!");
        return;
    }

    // Now we search for all buttons with the same channel and port number
    vector<int> channels;
    channels.push_back(button->getAddressChannel());
    vector<TMap::MAP_T> map = findButtons(button->getAddressPort(), channels);

    if (TError::isError() || map.empty())
        return;

    // Here we load all buttons found.
    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;
        // Finaly we iterate through all found buttons and set the text
        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            if (redraw)
                bt->setText(txt, -1);
            else
                bt->setTextOnly(txt, -1);
        }
    }
}

vector<Button::TButton *> TPageManager::collectButtons(vector<TMap::MAP_T>& map)
{
    DECL_TRACER("TPageManager::collectButtons(vector<TMap::MAP_T>& map)");

    vector<Button::TButton *> buttons;

    if (map.size() == 0)
        return buttons;

    vector<TMap::MAP_T>::iterator iter;

    for (iter = map.begin(); iter != map.end(); ++iter)
    {
        if (iter->pg < REGULAR_SUBPAGE_START || (iter->pg >= SYSTEM_PAGE_START && iter->pg < SYSTEM_SUBPAGE_START))     // Main page?
        {
            TPage *page;

            if ((page = getPage(iter->pg)) == nullptr)
            {
                MSG_TRACE("Page " << iter->pg << ", " << iter->pn << " not found in memory. Reading it ...");

                if (!readPage(iter->pg))
                    return buttons;

                page = getPage(iter->pg);
            }

            Button::TButton *bt = page->getButton(iter->bt);

            if (bt)
                buttons.push_back(bt);
        }
        else
        {
            TSubPage *subpage;

            if ((subpage = getSubPage(iter->pg)) == nullptr)
            {
                MSG_TRACE("Subpage " << iter->pg << ", " << iter->pn << " not found in memory. Reading it ...");

                if (!readSubPage(iter->pg))
                    return buttons;

                subpage = getSubPage(iter->pg);
                TPage *page = getActualPage();

                if (!page)
                {
                    MSG_ERROR("No actual page loaded!");
                    return buttons;
                }
            }

            Button::TButton *bt = subpage->getButton(iter->bt);

            if (bt)
                buttons.push_back(bt);
        }
    }

    return buttons;
}

/****************************************************************************
 * Calls from a Java activity. This is only available for Android OS.
 ****************************************************************************/
#ifdef Q_OS_ANDROID
void TPageManager::initNetworkState()
{
    DECL_TRACER("TPageManager::initNetworkState()");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "Init", "(Landroid/app/Activity;)V", activity.object());
    activity.callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "InstallNetworkListener", "()V");
#else
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "Init", "(Landroid/app/Activity;)V", activity.object());
    activity.callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "InstallNetworkListener", "()V");
#endif
}

void TPageManager::stopNetworkState()
{
    DECL_TRACER("TPageManager::stopNetworkState()");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "destroyNetworkListener", "()V");
#else
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "destroyNetworkListener", "()V");
#endif
}

void TPageManager::initBatteryState()
{
    DECL_TRACER("TPageManager::initBatteryState()");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/BatteryState", "Init", "(Landroid/app/Activity;)V", activity.object());
#else
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/BatteryState", "Init", "(Landroid/app/Activity;)V", activity.object());
#endif
    activity.callStaticMethod<void>("org/qtproject/theosys/BatteryState", "InstallBatteryListener", "()V");
}

void TPageManager::initPhoneState()
{
    DECL_TRACER("TPageManager::initPhoneState()");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/PhoneCallState", "Init", "(Landroid/app/Activity;)V", activity.object());
#else
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/PhoneCallState", "Init", "(Landroid/app/Activity;)V", activity.object());
#endif
    activity.callStaticMethod<void>("org/qtproject/theosys/PhoneCallState", "InstallPhoneListener", "()V");
}

void TPageManager::stopBatteryState()
{
    DECL_TRACER("TPageManager::stopBatteryState()");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/BatteryState", "destroyBatteryListener", "()V");
#else
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/BatteryState", "destroyBatteryListener", "()V");
#endif
}

void TPageManager::informTPanelNetwork(jboolean conn, jint level, jint type)
{
    DECL_TRACER("TPageManager::informTPanelNetwork(jboolean conn, jint level, jint type)");

    int l = 0;
    string sType;

    switch (type)
    {
        case 1: sType = "Wifi"; break;
        case 2: sType = "Mobile"; break;

        default:
            sType = "Unknown"; break;
    }

    if (conn)
        l = level;

    if (mNetState && mNetState != type)     // Has the connection type changed?
    {
        if (gAmxNet)
            gAmxNet->reconnect();
    }

    mNetState = type;

    MSG_INFO("Connection status: " << (conn ? "Connected" : "Disconnected") << ", level: " << level << ", type: " << sType);

    if (mNetCalls.size() > 0)
    {
        std::map<int, std::function<void (int level)> >::iterator iter;

        for (iter = mNetCalls.begin(); iter != mNetCalls.end(); ++iter)
            iter->second(l);
    }
}

void TPageManager::informBatteryStatus(jint level, jboolean charging, jint chargeType)
{
    DECL_TRACER("TPageManager::informBatteryStatus(jint level, jboolean charging, jint chargeType)");

    MSG_INFO("Battery status: level: " << level << ", " << (charging ? "Charging" : "not charging") << ", type: " << chargeType << ", Elements: " << mBatteryCalls.size());

    if (mBatteryCalls.size() > 0)
    {
        std::map<int, std::function<void (int, bool, int)> >::iterator iter;

        for (iter = mBatteryCalls.begin(); iter != mBatteryCalls.end(); ++iter)
            iter->second(level, charging, chargeType);
    }
}

void TPageManager::informPhoneState(bool call, const string &pnumber)
{
    DECL_TRACER("TPageManager::informPhoneState(bool call, const string &pnumber)");

    MSG_INFO("Call state: " << (call ? "Call in progress" : "No call") << ", phone number: " << pnumber);

    if (!gAmxNet)
    {
        MSG_WARNING("The network manager for the AMX controller is not initialized!");
        return;
    }
}

void TPageManager::initOrientation()
{
    DECL_TRACER("TPageManager::initOrientation()");

    int rotate = getSettings()->getRotate();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "Init", "(Landroid/app/Activity;I)V", activity.object(), rotate);
#else
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "Init", "(Landroid/app/Activity;I)V", activity.object(), rotate);
#endif
    activity.callStaticMethod<void>("org/qtproject/theosys/Orientation", "InstallOrientationListener", "()V");
}

void TPageManager::enterSetup()
{
    DECL_TRACER("TPageManager::enterSetup()");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "callSettings", "(Landroid/app/Activity;)V", activity.object());
#else
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>("org/qtproject/theosys/Settings", "callSettings", "(Landroid/app/Activity;)V", activity.object());
#endif
}
#endif  // __ANDROID__
#ifdef Q_OS_IOS
void TPageManager::informBatteryStatus(int level, int state)
{
    DECL_TRACER("TPageManager::informBatteryStatus(int level, int state)");

    MSG_INFO("Battery status: level: " << level << ", " << state);

    if (mBatteryCalls.size() > 0)
    {
        std::map<int, std::function<void (int, int)> >::iterator iter;

        for (iter = mBatteryCalls.begin(); iter != mBatteryCalls.end(); ++iter)
            iter->second(level, state);
    }
}

void TPageManager::informTPanelNetwork(bool conn, int level, int type)
{
    DECL_TRACER("TPageManager::informTPanelNetwork(bool conn, int level, int type)");

    int l = 0;
    string sType;

    switch (type)
    {
        case 1: sType = "Ethernet"; break;
        case 2: sType = "Mobile"; break;
        case 3: sType = "WiFi"; break;
        case 4: sType = "Bluetooth"; break;

        default:
            sType = "Unknown"; break;
    }

    if (conn)
        l = level;

    if (mNetState && mNetState != type)     // Has the connection type changed?
    {
        if (gAmxNet)
            gAmxNet->reconnect();
    }

    mNetState = type;

    MSG_INFO("Connection status: " << (conn ? "Connected" : "Disconnected") << ", level: " << level << ", type: " << sType);

    if (mNetCalls.size() > 0)
    {
        std::map<int, std::function<void (int level)> >::iterator iter;

        for (iter = mNetCalls.begin(); iter != mNetCalls.end(); ++iter)
            iter->second(l);
    }
}

#endif

void TPageManager::setButtonCallbacks(Button::TButton *bt)
{
    DECL_TRACER("TPageManager::setButtonCallbacks(Button::TButton *bt)");

    if (!bt)
        return;

    bt->registerCallback(_displayButton);
    bt->regCallPlayVideo(_callPlayVideo);
    bt->setFonts(mFonts);
    bt->setPalette(mPalette);
}

void TPageManager::externalButton(extButtons_t bt, bool checked)
{
    DECL_TRACER("TPageManager::externalButton(extButtons_t bt)");

    if (!mExternal)
        return;

    EXTBUTTON_t button = mExternal->getButton(bt);

    if (button.type == EXT_NOBUTTON)
        return;

    if (button.cp && button.ch)
    {
        amx::ANET_SEND scmd;

        scmd.device = TConfig::getChannel();
        scmd.port = button.cp;
        scmd.channel = button.ch;

        if (checked)
            scmd.MC = 0x0084;   // push button
        else
            scmd.MC = 0x0085;   // release button

        MSG_DEBUG("Sending to device <" << scmd.device << ":" << scmd.port << ":0> channel " << scmd.channel << " value 0x" << std::setw(2) << std::setfill('0') << std::hex << scmd.MC << " (" << (checked?"PUSH":"RELEASE") << ")");

        if (gAmxNet)
            gAmxNet->sendCommand(scmd);
        else
        {
            MSG_WARNING("Missing global class TAmxNet. Can't send a message!");
        }
    }
}

void TPageManager::sendKeyboard(const std::string& text)
{
    DECL_TRACER("TPageManager::sendKeyboard(const std::string& text)");

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg = UTF8ToCp1250(text);
    scmd.MC = 0x008b;
    MSG_DEBUG("Sending keyboard: " << text);

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendKeypad(const std::string& text)
{
    DECL_TRACER("TPageManager::sendKeypad(const std::string& text)");

    sendKeyboard(text);
}

void TPageManager::sendString(uint handle, const std::string& text)
{
    DECL_TRACER("TPageManager::sendString(uint handle, const std::string& text)");

    Button::TButton *bt = findButton(handle);

    if (!bt)
    {
        MSG_WARNING("Button " << handleToString(handle) << " not found!");
        return;
    }

    amx::ANET_SEND scmd;
    scmd.port = bt->getAddressPort();
    scmd.channel = bt->getAddressChannel();
    scmd.msg = UTF8ToCp1250(text);
    scmd.MC = 0x008b;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendGlobalString(const string& text)
{
    DECL_TRACER("TPageManager::sendGlobalString(const string& text)");

    if (text.empty() || text.find("-") == string::npos)
        return;

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg = text;
    scmd.MC = 0x008b;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendCommandString(int port, const string& cmd)
{
    DECL_TRACER("TPageManager::sendGlobalString(const string& text)");

    if (cmd.empty())
        return;

    amx::ANET_SEND scmd;
    scmd.port = port;
    scmd.channel = 0;
    scmd.msg = cmd;
    scmd.MC = 0x008c;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendLevel(int lp, int lv, int level)
{
    DECL_TRACER("TPageManager::sendLevel(int lp, int lv, int level)");

    if (!lv)
        return;

    amx::ANET_SEND scmd;
    scmd.device = TConfig::getChannel();
    scmd.port = lp;
    scmd.channel = lv;
    scmd.level = lv;
    scmd.MC = 0x008a;
    scmd.value = level;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendInternalLevel(int lp, int lv, int level)
{
    DECL_TRACER("TPageManager::sendInternalLevel(int lp, int lv, int level)");

    amx::ANET_COMMAND cmd;
    int channel = TConfig::getChannel();
    int system = TConfig::getSystem();

    cmd.MC = 0x000a;
    cmd.device1 = channel;
    cmd.port1 = lp;
    cmd.system = system;
    cmd.data.message_value.system = system;
    cmd.data.message_value.device = channel;
    cmd.data.message_value.port = lp;           // Must be the address port of bargraph
    cmd.data.message_value.value = lv;          // Must be the level channel of bargraph
    cmd.data.message_value.type = DTSZ_UINT;    // unsigned integer
    cmd.data.message_value.content.sinteger = level;
    doCommand(cmd);
}

void TPageManager::sendPHNcommand(const std::string& cmd)
{
    DECL_TRACER("TPageManager::sendPHNcommand(const std::string& cmd)");

    amx::ANET_SEND scmd;
    scmd.port = mTSettings->getSettings().voipCommandPort;
    scmd.channel = TConfig::getChannel();
    scmd.msg = "^PHN-" + cmd;
    scmd.MC = 0x008c;
    MSG_DEBUG("Sending PHN command: ^PHN-" << cmd);

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send ^PHN command!");
}

void TPageManager::sendKeyStroke(char key)
{
    DECL_TRACER("TPageManager::sendKeyStroke(char key)");

    if (!key)
        return;

    char msg[2];
    msg[0] = key;
    msg[1] = 0;

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg.assign(msg);
    scmd.MC = 0x008c;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

/**
 * Sending a custom event is identical in all cases. Because of this I
 * implemented this method to send a custom event. This is called in all cases
 * where a ?XXX command is received.
 *
 * @param value1    The instance of the button.
 * @param value2    The value of a numeric request or the length of the string.
 * @param value3    Always 0
 * @param msg       In case of a string this contains the string.
 * @param evType    This is the event type, a number between 1001 and 1099.
 * @param cp        Channel port of button.
 * @param cn        Channel number. of button.
 *
 * @return If all parameters are valid it returns TRUE.
 */
bool TPageManager::sendCustomEvent(int value1, int value2, int value3, const string& msg, int evType, int cp, int cn)
{
    DECL_TRACER("TPageManager::sendCustomEvent(int value1, int value2, int value3, const string& msg, int evType)");

    if (value1 < 1)
        return false;

    amx::ANET_SEND scmd;
    scmd.port = cp;
    scmd.channel = cn;
    scmd.ID = scmd.channel;
    scmd.flag = 0;
    scmd.type = evType;
    scmd.value1 = value1;   // instance
    scmd.value2 = value2;
    scmd.value3 = value3;
    scmd.msg = msg;

    if (!msg.empty())
        scmd.dtype = 0x0001;// Char array

    scmd.MC = 0x008d;       // custom event

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");

    return true;
}
#ifndef _NOSIP_
string TPageManager::sipStateToString(TSIPClient::SIP_STATE_t s)
{
    DECL_TRACER("TPageManager::sipStateToString(TSIPClient::SIP_STATE_t s)");

    switch(s)
    {
        case TSIPClient::SIP_CONNECTED:     return "CONNECTED";
        case TSIPClient::SIP_DISCONNECTED:  return "DISCONNECTED";
        case TSIPClient::SIP_HOLD:          return "HOLD";
        case TSIPClient::SIP_RINGING:       return "RINGING";
        case TSIPClient::SIP_TRYING:        return "TRYING";

        default:
            return "IDLE";
    }

    return "IDLE";
}
#endif
void TPageManager::sendOrientation()
{
    string ori;

    switch(mOrientation)
    {
        case O_PORTRAIT:            ori = "DeviceOrientationPortrait"; break;
        case O_REVERSE_PORTRAIT:    ori = "DeviceOrientationPortraitUpsideDown"; break;
        case O_LANDSCAPE:           ori = "DeviceOrientationLandscapeLeft"; break;
        case O_REVERSE_LANDSCAPE:   ori = "DeviceOrientationLandscapeRight"; break;
        case O_FACE_UP:             ori = "DeviceOrientationFaceUp"; break;
        case O_FACE_DOWN:           ori = "DeviceOrientationFaceDown"; break;
        default:
            return;
    }

    sendGlobalString("TPCACC-" + ori);
}

void TPageManager::callSetPassword(ulong handle, const string& pw, int x, int y)
{
    DECL_TRACER("TPageManager::callSetPassword(ulong handle, const string& pw, int x, int y)");

    Button::TButton *bt = findButton(handle);

    if (!bt)
    {
        MSG_WARNING("callSetPassword: Button " << handleToString(handle) << " not found!");
        return;
    }

    string pass = pw;

    if (pass.empty())
        pass = "\x01";

    bt->setPassword(pass);
    bt->doClick(x, y, true);
    bt->doClick(x, y, false);
}

TButtonStates *TPageManager::addButtonState(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv)
{
    DECL_TRACER("TPageManager::addButtonState(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv)");

    TButtonStates *pbs = new TButtonStates(t, rap, rad, rch, rcp, rlp, rlv);
    uint32_t id = pbs->getID();

    if (!mButtonStates.empty())
    {
        vector<TButtonStates *>::iterator iter;

        for (iter = mButtonStates.begin(); iter != mButtonStates.end(); ++iter)
        {
            TButtonStates *bs = *iter;

            if (bs->isButton(t, id))
            {
                delete pbs;
                return bs;
            }
        }
    }

    mButtonStates.push_back(pbs);
    return pbs;
}

TButtonStates *TPageManager::addButtonState(const TButtonStates& rbs)
{
    DECL_TRACER("TPageManager::addButtonState(const TButtonStates& rbs)");

    if (!mButtonStates.empty())
    {
        vector<TButtonStates *>::iterator iter;
        TButtonStates bs = rbs;
        BUTTONTYPE type = bs.getType();
        uint32_t id = bs.getID();

        for (iter = mButtonStates.begin(); iter != mButtonStates.end(); ++iter)
        {
            TButtonStates *pbs = *iter;

            if (pbs->isButton(type, id))
                return pbs;
        }
    }

    TButtonStates *pbs = new TButtonStates(rbs);
    mButtonStates.push_back(pbs);
    return pbs;
}

TButtonStates *TPageManager::getButtonState(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv)
{
    DECL_TRACER("TPageManager::getButtonState(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv)");

    if (mButtonStates.empty())
        return nullptr;

    vector<TButtonStates *>::iterator iter;
    MSG_DEBUG("Found " << mButtonStates.size() << " button states.");

    for (iter = mButtonStates.begin(); iter != mButtonStates.end(); ++iter)
    {
        TButtonStates *bs = *iter;

        if (bs->isButton(t, rap, rad, rch, rcp, rlp, rlv))
            return bs;
    }

    return nullptr;
}

TButtonStates *TPageManager::getButtonState(uint32_t id)
{
    DECL_TRACER("TPageManager::getButtonState(uint32_t id)");

    if (mButtonStates.empty())
        return nullptr;

    vector<TButtonStates *>::iterator iter;
    MSG_DEBUG("Found " << mButtonStates.size() << " button states.");

    for (iter = mButtonStates.begin(); iter != mButtonStates.end(); ++iter)
    {
        TButtonStates *bs = *iter;

        if (bs->isButton(id))
            return bs;
    }

    return nullptr;
}

TButtonStates *TPageManager::getButtonState(BUTTONTYPE t, uint32_t id)
{
    DECL_TRACER("TPageManager::getButtonState(BUTTONTYPE t, uint32_t id)");

    if (mButtonStates.empty())
        return nullptr;

    vector<TButtonStates *>::iterator iter;
    MSG_DEBUG("Found " << mButtonStates.size() << " button states.");

    for (iter = mButtonStates.begin(); iter != mButtonStates.end(); ++iter)
    {
        TButtonStates *bs = *iter;

        if (bs->isButton(t, id))
            return bs;
    }

    return nullptr;
}

void TPageManager::onSwipeEvent(TPageManager::SWIPES sw)
{
    DECL_TRACER("TPageManager::onSwipeEvent(TPageManager::SWIPES sw)");

    // Swipes are defined in "external".
    if (!mExternal)
        return;

    extButtons_t eBt;
    string dbg;

    switch(sw)
    {
        case SW_LEFT:   eBt = EXT_GESTURE_LEFT; dbg.assign("LEFT"); break;
        case SW_RIGHT:  eBt = EXT_GESTURE_RIGHT; dbg.assign("RIGHT"); break;
        case SW_UP:     eBt = EXT_GESTURE_UP; dbg.assign("UP"); break;
        case SW_DOWN:   eBt = EXT_GESTURE_DOWN; dbg.assign("DOWN"); break;

        default:
            return;
    }

    int pgNum = getActualPageNumber();
    EXTBUTTON_t bt = mExternal->getButton(pgNum, eBt);

    if (bt.bi == 0)
        return;

    MSG_DEBUG("Received swipe " << dbg << " event for page " << pgNum << " on button " << bt.bi << " \"" << bt.na << "\"");

    if (!bt.cm.empty() && bt.co == 0)           // Feed command to ourself?
    {                                           // Yes, then feed it into command queue.
        MSG_DEBUG("Button has a self feed command");

        int channel = TConfig::getChannel();
        int system = TConfig::getSystem();

        amx::ANET_COMMAND cmd;
        cmd.MC = 0x000c;
        cmd.device1 = channel;
        cmd.port1 = bt.ap;
        cmd.system = system;
        cmd.data.message_string.device = channel;
        cmd.data.message_string.port = bt.ap;  // Must be the address port of button
        cmd.data.message_string.system = system;
        cmd.data.message_string.type = 1;   // 8 bit char string

        vector<string>::iterator iter;

        for (iter = bt.cm.begin(); iter != bt.cm.end(); ++iter)
        {
            cmd.data.message_string.length = iter->length();
            memset(&cmd.data.message_string.content, 0, sizeof(cmd.data.message_string.content));
            strncpy((char *)&cmd.data.message_string.content, iter->c_str(), sizeof(cmd.data.message_string.content));
            doCommand(cmd);
        }
    }
    else if (!bt.cm.empty())
    {
        MSG_DEBUG("Button sends a command on port " << bt.co);

        vector<string>::iterator iter;

        for (iter = bt.cm.begin(); iter != bt.cm.end(); ++iter)
            sendCommandString(bt.co, *iter);
    }
}

/****************************************************************************
 * The following functions implements one of the commands the panel accepts.
 ****************************************************************************/

/**
 * This is a special function handling the progress bars when the files of the
 * panel are updated. Instead of simply displaying a ready page, it fakes one
 * with the actual dimensions of the main page. This is possible, because we've
 * always a main page even if the panel is started for the first time.
 */
void TPageManager::doFTR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doFTR(int, vector<int>&, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command #FTR needs at least 1 parameter! Ignoring command.");
        return;
    }

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        for (size_t i = 0; i < pars.size(); i++)
        {
            MSG_DEBUG("[" << i << "]: " << pars.at(i));
        }
    }

    if (pars.at(0).compare("START") == 0)
    {
        // Here we have to drop all pages and subpages first and then display
        // the faked page with the progress bars.
        MSG_DEBUG("Starting file transfer ...");
        doPPX(port, channels, pars);
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            if (!readPage("_progress"))
            {
                MSG_ERROR("Error creating the system page _progress!");
                return;
            }

            pg = getPage("_progress");

            if (!pg)
            {
                MSG_ERROR("Error getting system page _progress!");
                return;
            }
        }

        pg->setFonts(mFonts);
        pg->registerCallback(_setBackground);
        pg->regCallPlayVideo(_callPlayVideo);

        if (!pg || !_setPage || !mTSettings)
            return;

        int width, height;
        width = mTSettings->getWidth();
        height = mTSettings->getHeight();
#ifdef _SCALE_SKIA_
        if (mScaleFactor != 1.0)
        {
            width = (int)((double)width * mScaleFactor);
            height = (int)((double)height * mScaleFactor);
        }
#endif
        _setPage((pg->getNumber() << 16) & 0xffff0000, width, height);
        pg->show();
        MSG_DEBUG("Page _progress on screen");
    }
    else if (pars.at(0).compare("SYNC") == 0)
    {
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            MSG_ERROR("Page _progress not found!");
            return;
        }

        Button::TButton *bt = pg->getButton(1);   // Line 1

        if (!bt)
        {
            MSG_ERROR("Button 160 of page _progress not found!");
            return;
        }

        bt->setText(pars.at(2), 0);
        bt->show();
    }
    else if (pars.at(0).compare("FTRSTART") == 0)
    {
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            MSG_ERROR("Page _progress not found!");
            return;
        }

        Button::TButton *bt1 = pg->getButton(1);   // Line 1
        Button::TButton *bt2 = pg->getButton(2);   // Line 2
        Button::TButton *bt3 = pg->getButton(3);   // Bargraph 1
        Button::TButton *bt4 = pg->getButton(4);   // Bargraph 2

        if (!bt1 || !bt2 || !bt3 || !bt4)
        {
            MSG_ERROR("Buttons of page _progress not found!");
            return;
        }

        bt1->setText("Transfering files ...", 0);
        bt1->show();
        bt2->setText(pars.at(3), 0);
        bt2->show();
        bt3->drawBargraph(0, atoi(pars.at(1).c_str()), true);
        bt4->drawBargraph(0, atoi(pars.at(2).c_str()), true);
    }
    else if (pars.at(0).compare("FTRPART") == 0)
    {
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            MSG_ERROR("Page _progress not found!");
            return;
        }

        Button::TButton *bt = pg->getButton(4);   // Bargraph 2

        if (!bt)
        {
            MSG_ERROR("Buttons of page _progress not found!");
            return;
        }

        bt->drawBargraph(0, atoi(pars.at(2).c_str()), true);
    }
    else if (pars.at(0).compare("END") == 0)
    {
        MSG_TRACE("End of file transfer reached.");

        // To make sure the new surface will not be deleted and replaced by the
        // default build in surface, we must delete the "virgin" marker first.
        // This is a file called <project path>/.system.
        string virgin = TConfig::getProjectPath() + "/.system";
        remove(virgin.c_str());     // Because this file may not exist we don't care about the result code.

        if (_resetSurface)
            _resetSurface();
        else
        {
            MSG_WARNING("Missing callback function \"resetSurface\"!");
        }
    }
}

void TPageManager::doLEVON(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doLEVON(int, vector<int>&, vector<string>&)");

    mLevelSend = true;
#if TESTMODE == 1
    __success = true;
    setAllDone();
#endif
}

void TPageManager::doLEVOF(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doLEVOF(int, vector<int>&, vector<string>&)");

    mLevelSend = false;
#if TESTMODE == 1
    __success = true;
    setAllDone();
#endif
}

void TPageManager::doRXON(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doRXON(int, vector<int>&, vector<string>&)");

    mRxOn = true;
#if TESTMODE == 1
    __success = true;
    setAllDone();
#endif
}

void TPageManager::doRXOF(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doRXOF(int, vector<int>&, vector<string>&)");

    mRxOn = false;
#if TESTMODE == 1
    __success = true;
    setAllDone();
#endif
}

void TPageManager::doON(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doON(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command ON needs 1 parameter! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int c = atoi(pars[0].c_str());

    if (c <= 0)
    {
        MSG_WARNING("Invalid channel " << c << "! Ignoring command ON.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<int> chans = { c };
    vector<TMap::MAP_T> map = findButtons(port, chans, TMap::TYPE_CM);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            if (bt->getButtonType() == GENERAL)
            {
                bt->setActive(1);
#if TESTMODE == 1
                if (_gTestMode)
                    _gTestMode->setResult(intToString(bt->getActiveInstance() + 1));
#endif
            }
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::doOFF(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doOFF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command OFF needs 1 parameter! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int c = atoi(pars[0].c_str());

    if (c <= 0)
    {
        MSG_WARNING("Invalid channel " << c << "! Ignoring command OFF.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<int> chans = { c };
    vector<TMap::MAP_T> map = findButtons(port, chans, TMap::TYPE_CM);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            if (bt->getButtonType() == GENERAL)
                bt->setActive(0);
#if TESTMODE == 1
                if (_gTestMode)
                    _gTestMode->setResult(intToString(bt->getActiveInstance() + 1));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::doLEVEL(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doLEVEL(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_WARNING("Command LEVEL needs 2 parameters! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int c = atoi(pars[0].c_str());
    int level = atoi(pars[1].c_str());

    if (c <= 0)
    {
        MSG_WARNING("Invalid channel " << c << "! Ignoring command LEVEL.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<int> chans = { c };
    vector<TMap::MAP_T> map = findBargraphs(port, chans);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
        MSG_WARNING("No bargraphs found!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        MSG_DEBUG("Found " << buttons.size() << " buttons.");
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;
            MSG_DEBUG("Evaluating button " << handleToString(bt->getHandle()))

            if (bt->getButtonType() == BARGRAPH && bt->getLevelChannel() == c)
            {
                if (bt->isBargraphInverted())
                    level = (bt->getRangeHigh() - bt->getRangeLow()) - level;

                bt->drawBargraph(bt->getActiveInstance(), level);
                bt->sendBargraphLevel();
#if TESTMODE == 1
                if (_gTestMode)
                    _gTestMode->setResult(intToString(bt->getLevelValue()));
#endif
            }
            else if (bt->getButtonType() == JOYSTICK)
            {
                int x = (bt->getLevelChannel() == c ? level : bt->getLevelAxisX());
                int y = (bt->getLevelChannel() == c ? bt->getLevelAxisY() : level);

                if (bt->isBargraphInverted())
                    x = (bt->getRangeHigh() - bt->getRangeLow()) - x;

                if (bt->isJoystickAuxInverted())
                    y = (bt->getRangeHigh() - bt->getRangeLow()) - y;

                bt->drawJoystick(x, y);
                bt->sendJoystickLevels();
#if TESTMODE == 1
                if (_gTestMode)
                {
                    std::stringstream s;
                    s << x << "|" << y;
                    _gTestMode->setResult(s.str());
                }
#endif
            }
            else if (bt->getButtonType() == MULTISTATE_BARGRAPH && bt->getLevelChannel() == c)
            {
                int state = (int)((double)bt->getStateCount() / (double)(bt->getRangeHigh() - bt->getRangeLow()) * (double)level);
                bt->setActive(state);
                bt->sendBargraphLevel();
#if TESTMODE == 1
                if (_gTestMode)
                    _gTestMode->setResult(intToString(bt->getActiveInstance()));
#endif
            }
        }
    }
    else
        MSG_DEBUG("No buttons found!");

#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::doBLINK(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBLINK(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 4)
    {
        MSG_WARNING("Command BLINK expects 4 parameters! Command ignored.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    vector<int> sysButtons = { 141, 142, 143, 151, 152, 153, 154, 155, 156, 157, 158 };
    vector<TMap::MAP_T> map = findButtons(0, sysButtons);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
        MSG_WARNING("No system buttons found.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);
    vector<Button::TButton *>::iterator mapIter;

    for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
    {
        Button::TButton *bt = *mapIter;
        bt->setActive(0);
#if TESTMODE == 1
                if (_gTestMode)
                    _gTestMode->setResult(intToString(bt->getActiveInstance() + 1));
#endif
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Send the version of the panel to the NetLinx. This is the real application
 * version.
 */
void TPageManager::doVER(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doVER(int, vector<int>&, vector<string>&)");

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg.assign(string("^VER-")+VERSION_STRING());
    scmd.MC = 0x008c;

    if (gAmxNet)
    {
        gAmxNet->sendCommand(scmd);
#if TESTMODE == 1
        __success = true;

        if (_gTestMode)
            _gTestMode->setResult(VERSION_STRING());
#endif
    }
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
#if TESTMODE == 1
    setAllDone();
#endif
}

/**
 * Returns the user name used to connect to a SIP server. An empty string is
 * returned if there is no user defined.
 */
#ifndef _NOSIP_
void TPageManager::doWCN(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doWCN(int, vector<int>&, vector<string>&)");

    if (!TConfig::getSIPstatus())
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg.assign("^WCN-" + TConfig::getSIPuser());
    scmd.MC = 0x008c;

    if (gAmxNet)
    {
        gAmxNet->sendCommand(scmd);
#if TESTMODE == 1
        __success = true;

        if (_gTestMode)
            _gTestMode->setResult(TConfig::getSIPuser());
#endif
    }
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
#if TESTMODE == 1
        setAllDone();
#endif
}
#endif
/**
 * Flip to specified page using the named animation.
 * FIXME: Implement animation for pages.
 */
void TPageManager::doAFP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doAFP(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 4)
    {
        MSG_ERROR("Command AFP: Less than 4 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    string pname = pars[0];
//    string ani = pars[1];
//    int origin = atoi(pars[2].c_str());
//    int duration = atoi(pars[3].c_str());

    // FIXME: Animation of pages is currently not implemented.

    if (!pname.empty())
        setPage(pname);
    else if (mPreviousPage)
        setPage(mPreviousPage);
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(getActualPage()->getName());

    setDone();
#endif
}

/**
 * Add a specific popup page to a specified popup group if it does not already
 * exist. If the new popup is added to a group which has a popup displayed on
 * the current page along with the new pop-up, the displayed popup will be
 * hidden and the new popup will be displayed.
 */
void TPageManager::doAPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doAPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command APG: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    closeGroup(pars[1]);

    TPage *page = nullptr;
    TSubPage *subPage = deliverSubPage(pars[0], &page);

    if (!subPage)
    {
        MSG_ERROR("Subpage " << pars[0] << " couldn't either found or created!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    if (!page)
    {
        MSG_ERROR("There seems to be no page for subpage " << pars[0]);
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    page->addSubPage(subPage);
    subPage->setGroup(pars[1]);
    subPage->setZOrder(page->getNextZOrder());
    MSG_DEBUG("Setting new Z-order " << page->getActZOrder() << " on page " << page->getName());
    subPage->show();
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(subPage->getGroupName() + ":" + subPage->getName());

    setDone();
#endif
}

/**
 * Clear all popup pages from specified popup group.
 */
void TPageManager::doCPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doCPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command CPG: Expecting 1 parameter but got only 1!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    vector<SUBPAGELIST_T> pageList = mPageList->getSubPageList();

    if (pageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); pgIter++)
        {
            if (pgIter->group.compare(pars[0]) == 0)
            {
                pgIter->group.clear();
                TSubPage *pg = getSubPage(pgIter->pageID);

                if (pg)
                    pg->setGroup(pgIter->group);
#if TESTMODE == 1
                __success = true;
#endif
            }
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Delete a specific popup page from specified popup group if it exists.
 */
void TPageManager::doDPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doDPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command DPG: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    SUBPAGELIST_T listPg = findSubPage(pars[0]);

    if (!listPg.isValid)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    if (listPg.group.compare(pars[1]) == 0)
    {
        listPg.group.clear();
        TSubPage *pg = getSubPage(listPg.pageID);

        if (pg)
            pg->setGroup(listPg.group);
#if TESTMODE == 1
        __success = true;
#endif
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Set the hide effect for the specified popup page to the named hide effect.
 */
void TPageManager::doPHE(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHE(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PHE: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    if (strCaseCompare(pars[1], "fade") == 0)
        pg->setHideEffect(SE_FADE);
    else if (strCaseCompare(pars[1], "slide to left") == 0)
        pg->setHideEffect(SE_SLIDE_LEFT);
    else if (strCaseCompare(pars[1], "slide to right") == 0)
        pg->setHideEffect(SE_SLIDE_RIGHT);
    else if (strCaseCompare(pars[1], "slide to top") == 0)
        pg->setHideEffect(SE_SLIDE_TOP);
    else if (strCaseCompare(pars[1], "slide to bottom") == 0)
        pg->setHideEffect(SE_SLIDE_BOTTOM);
    else if (strCaseCompare(pars[1], "slide to left fade") == 0)
        pg->setHideEffect(SE_SLIDE_LEFT_FADE);
    else if (strCaseCompare(pars[1], "slide to right fade") == 0)
        pg->setHideEffect(SE_SLIDE_RIGHT_FADE);
    else if (strCaseCompare(pars[1], "slide to top fade") == 0)
        pg->setHideEffect(SE_SLIDE_TOP_FADE);
    else if (strCaseCompare(pars[1], "slide to bottom fade") == 0)
        pg->setHideEffect(SE_SLIDE_BOTTOM_FADE);
    else
        pg->setHideEffect(SE_NONE);
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(intToString(pg->getHideEffect()));

    __success = true;
    setAllDone();
#endif
}

/**
 * Set the hide effect position. Only 1 coordinate is ever needed for an effect;
 * however, the command will specify both. This command sets the location at
 * which the effect will end at.
 */
void TPageManager::doPHP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command: PHP: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    size_t pos = pars[1].find(",");
    int x, y;

    if (pos == string::npos)
    {
        x = atoi(pars[1].c_str());
        y = 0;
    }
    else
    {
        x = atoi(pars[1].substr(0, pos).c_str());
        y = atoi(pars[1].substr(pos+1).c_str());
    }

    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    pg->setHideEndPosition(x, y);
#if TESTMODE == 1
    if (_gTestMode)
    {
        int x, y;
        pg->getHideEndPosition(&x, &y);
        _gTestMode->setResult(intToString(x) + "," + intToString(y));
    }

    __success = true;
    setAllDone();
#endif
}

/**
 * Set the hide effect time for the specified popup page.
 */
void TPageManager::doPHT(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PHT: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    pg->setHideTime(atoi(pars[1].c_str()));
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(intToString(pg->getHideTime()));

    __success = true;
    setAllDone();
#endif
}

/**
 * @brief G5: Open Collapsible Popup Command
 * Moves the named collapsible popup to the open position.
 */
void TPageManager::doPOP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPOP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command POP: Expect at least 1 parameter but got none!");
        return;
    }

    string popup = pars[0];
    string page;

    if (pars.size() > 1)
        page = pars[1];

    TSubPage *sp = loadSubPage(popup);

    MSG_DEBUG("Subpage " << popup << (sp ? " found" : " NOT found"));

    if (!sp || !sp->isCollapsible() || sp->getCollapseState() == COL_FULL)
        return;

    MSG_DEBUG("Setting collaped state to FULL");
    sp->setCollapsible(COL_FULL);
}

/**
 * Close all popups on a specified page. If the page name is empty, the current
 * page is used. Same as the ’Clear Page’ command in TPDesign4.
 */
void TPageManager::doPPA(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPA(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    TError::clear();
    TPage *pg;

    if (pars.size() == 0)
        pg = getPage(mActualPage);
    else
        pg = getPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    pg->drop();
    pg->resetZOrder();
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Deactivate a specific popup page on either a specified page or the current
 * page. If the page name is empty, the current page is used. If the popup page
 * is part of a group, the whole group is deactivated. This command works in
 * the same way as the ’Hide Popup’ command in TPDesign4.
 */
void TPageManager::doPPF(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPF(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command PPF: At least 1 parameter is expected!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    hideSubPage(pars[0]);
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Toggle a specific popup page on either a specified page or the current page.
 * If the page name is empty, the current page is used. Toggling refers to the
 * activating/deactivating (On/Off) of a popup page. This command works in the
 * same way as the ’Toggle Popup’ command in TPDesign4.
 */
void TPageManager::doPPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command PPG: At least 1 parameter is expected!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TSubPage *pg = getSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    if (pg->isVisible())
    {
        pg->drop();
        page->decZOrder();
#if TESTMODE == 1
        setDone();
#endif
        return;
    }

    TSubPage *sub = getFirstSubPageGroup(pg->getGroupName());

    while(sub)
    {
        if (sub->getGroupName().compare(pg->getGroupName()) == 0 && sub->isVisible())
            sub->drop();

        sub = getNextSubPageGroup(pg->getGroupName(), sub);
    }

    pg->setZOrder(page->getNextZOrder());
    MSG_DEBUG("Setting new Z-order " << page->getActZOrder() << " on page " << page->getName());
    pg->show();
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Kill refers to the deactivating (Off) of a popup window from all pages. If
 * the pop-up page is part of a group, the whole group is deactivated. This
 * command works in the same way as the 'Clear Group' command in TPDesign 4.
 */
void TPageManager::doPPK(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPK(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command PPK: At least 1 parameter is expected!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TSubPage *pg = getSubPage(pars[0]);

    if (pg)
    {
        pg->drop();
        page->decZOrder();
    }
#if TESTMODE == 1
        setDone();
#endif
}

/**
 * Set the modality of a specific popup page to Modal or NonModal.
 * A Modal popup page, when active, only allows you to use the buttons and
 * features on that popup page. All other buttons on the panel page are
 * inactivated.
 */
void TPageManager::doPPM(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPM(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PPM: Expecting 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TSubPage *pg = getSubPage(pars[0]);

    if (pg)
    {
        if (pars[1] == "1" || strCaseCompare(pars[1], "modal") == 0)
            pg->setModal(1);
        else
            pg->setModal(0);
#if TESTMODE == 1
        if (_gTestMode)
            _gTestMode->setResult(pg->isModal() ? "TRUE" : "FALSE");
#endif
    }
#if TESTMODE == 1
        setAllDone();
#endif
}

/**
 * Activate a specific popup page to launch on either a specified page or the
 * current page. If the page name is empty, the current page is used. If the
 * popup page is already on, do not re-draw it. This command works in the same
 * way as the ’Show Popup’ command in TPDesign4.
 */
void TPageManager::doPPN(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPN(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command PPN: At least 1 parameter is expected!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    showSubPage(pars[0]);
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Set a specific popup page to timeout within a specified time. If timeout is
 * empty, popup page will clear the timeout.
 */
void TPageManager::doPPT(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPPT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PPT: Expecting 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    pg->setTimeout(atoi(pars[1].c_str()));
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(intToString(pg->getTimeout()));

    __success = true;
    setAllDone();
#endif
}

/**
 * Close all popups on all pages. This command works in the same way as the
 * 'Clear All' command in TPDesign 4.
 */
void TPageManager::doPPX(int, std::vector<int>&, std::vector<std::string>&)
{
    DECL_TRACER("TPageManager::doPPX(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    TError::clear();
    PCHAIN_T *chain = mPchain;

    while(chain)
    {
        TSubPage *sub = chain->page->getFirstSubPage();

        while (sub)
        {
            MSG_DEBUG("Dropping subpage " << sub->getNumber() << ", \"" << sub->getName() << "\".");
            sub->drop();
            sub = chain->page->getNextSubPage();
        }

        chain = chain->next;
    }

    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    page->resetZOrder();
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Set the show effect for the specified popup page to the named show effect.
 */
void TPageManager::doPSE(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPSE(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PSE: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    if (strCaseCompare(pars[1], "fade") == 0)
        pg->setShowEffect(SE_FADE);
    else if (strCaseCompare(pars[1], "slide to left") == 0)
        pg->setShowEffect(SE_SLIDE_LEFT);
    else if (strCaseCompare(pars[1], "slide to right") == 0)
        pg->setShowEffect(SE_SLIDE_RIGHT);
    else if (strCaseCompare(pars[1], "slide to top") == 0)
        pg->setShowEffect(SE_SLIDE_TOP);
    else if (strCaseCompare(pars[1], "slide to bottom") == 0)
        pg->setShowEffect(SE_SLIDE_BOTTOM);
    else if (strCaseCompare(pars[1], "slide to left fade") == 0)
        pg->setShowEffect(SE_SLIDE_LEFT_FADE);
    else if (strCaseCompare(pars[1], "slide to right fade") == 0)
        pg->setShowEffect(SE_SLIDE_RIGHT_FADE);
    else if (strCaseCompare(pars[1], "slide to top fade") == 0)
        pg->setShowEffect(SE_SLIDE_TOP_FADE);
    else if (strCaseCompare(pars[1], "slide to bottom fade") == 0)
        pg->setShowEffect(SE_SLIDE_BOTTOM_FADE);
    else
        pg->setShowEffect(SE_NONE);
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(intToString(pg->getShowEffect()));

    __success = true;
    setAllDone();
#endif
}

/**
 * Set the show effect position. Only 1 coordinate is ever needed for an effect;
 * however, the command will specify both. This command sets the location at
 * which the effect will begin.
 */
void TPageManager::doPSP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPSP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PSP: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    size_t pos = pars[1].find(",");
    int x, y;

    if (pos == string::npos)
    {
        x = atoi(pars[1].c_str());
        y = 0;
    }
    else
    {
        x = atoi(pars[1].substr(0, pos).c_str());
        y = atoi(pars[1].substr(pos+1).c_str());
    }

    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    pg->setShowEndPosition(x, y);
#if TESTMODE == 1
    pg->getShowEndPosition(&x, &y);

    if (_gTestMode)
        _gTestMode->setResult(intToString(x) + "," + intToString(y));

    __success = true;
    setAllDone();
#endif
}

/**
 * Set the show effect time for the specified popup page.
 */
void TPageManager::doPST(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPST(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PST: Less than 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    pg->setShowTime(atoi(pars[1].c_str()));
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(intToString(pg->getShowTime()));

    __success = 1;
    setAllDone();
#endif
}

/**
 * Flips to a page with a specified page name. If the page is currently active,
 * it will not redraw the page.
 */
void TPageManager::doPAGE(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPAGE(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command PAGE: Got no page parameter!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    setPage(pars[0]);
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(intToString(getActualPageNumber()));

    setDone();
#endif
}

/**
 * @brief TPageManager::doPCL - Collapse Collapsible Popup Command
 * Moves the named closeable popup to the collapsed position.
 * Syntax:
 *      "'^PCL-<popup name>;[optional target page]'"
 * Variables:
 *      Popup name = the name of the popup to collapse
 *      Target page = name of the page hosting the popup to affect the change upon. If target page is not specified, the command
 *                    is applied to the current page.
 * Examples :
 *      SEND_COMMAND Panel,"'^PCL-Contacts'"
 *          Collapse the Contacts popup on the current page.
 *      SEND_COMMAND Panel,"'^PCL-Contacts;Teleconference Control'"
 *          Collapse the Contacts popup on the Teleconference Control page.
 */
void TPageManager::doPCL(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPCL(int port, vector<int>& channels, vector<std::string>& pars)");

    Q_UNUSED(port);
    Q_UNUSED(channels);

    if (pars.empty())
    {
        MSG_WARNING("Command PCL: Got no page parameter!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    string popup = pars[0];
    string page;

    if (pars.size() >= 2)
        page = pars[1];

    if (!page.empty())
    {
        TPage *pg = getPage(page);

        if (pg)
        {
            TSubPage *sp = pg->getSubPage(popup);

            if (sp)
                sp->setCollapsible(COL_SMALL);
        }
    }
    else
    {
        TSubPage *sp = loadSubPage(popup);

        if (!sp || !sp->isCollapsible())
            return;

        sp->setCollapsible(COL_SMALL);
    }
}

/**
 * @brief TPageManager::doPCT - Collapsible Popup Custom Toggle Command
 * This is an advanced "toggle" command for collapsible popups, working with a
 * comma-separated list of commands. This list is parsed and a command table is
 * created. Based on the current state of the collapsible popup, the correct
 * command is executed.
 * Syntax:
 *      "'^PCT-<popup>,<custom toggle commands>;[optional target page]'"
 * Variables:
 *      Popup = popup name
 *      Custom toggle commands = a comma separated list of commands. This list
 *          is parsed and a command table is created. The state letters are as follows:
 *              o - open
 *              c - collapsed
 *              d - dynamic, followed by an integer indicating the offset.
 *              * - wildcard, always last in the list
 *          Before and after states are separated by -> characters.
 *      Target page = name of the page hosting the popup to affect the change
 *              upon. If target page is not specified, the command is applied
 *              to the current page.
 * Example:
 *      SEND_COMMAND Panel,"'^PCT-RightSlider,c->o,o->d100,*->c'"
 * The RightSlider open if collapsed, move to d100 if open, and collapse otherwise.
 */
void TPageManager::doPCT(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPCT(int port, vector<int>& channels, vector<std::string>& pars)");

    Q_UNUSED(port);
    Q_UNUSED(channels);

    if (pars.empty())
    {
        MSG_WARNING("Command PCT: Got no page parameter!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();

    if (pars.size() < 2)
    {
        MSG_WARNING("Command PCT: Expected at least 2 parameters but got " << pars.size() << "!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    string popup = pars[0];
    string page;

    MSG_DEBUG("Switching collapsible page " << popup << " ...");
    // Parse the command table
    mCmdTable.clear();
    vector<string>::iterator iter;
    int idx = 0;

    for (iter = pars.begin(); iter != pars.end(); ++iter)
    {
        if (idx == 0)
        {
            idx++;
            continue;
        }

        size_t pos;

        if ((pos = iter->find_first_of(';')) != string::npos)
        {
            page = iter->substr(pos+1);
            *iter = iter->substr(0, pos);
        }

        pos = iter->find("->");

        if (pos == string::npos)
        {
            idx++;
            continue;
        }

        string from = iter->substr(0, pos);
        string to = iter->substr(pos+2);
        trim(from);
        trim(to);
        MSG_DEBUG("Command from: " << from << ", to: " << to);
        SUBCOMMAND_t sc;

        switch(from[0])
        {
            case 'o':
            case 'O': sc.from = POPSTATE_OPEN; break;
            case 'c':
            case 'C': sc.from = POPSTATE_CLOSED; break;

            case 'd':
            case 'D':
                sc.from = POPSTATE_DYNAMIC;
                sc.offset = atoi(from.substr(1).c_str());
            break;

            case '*': sc.from = POPSTATE_ANY; break;
            default:
                sc.from = POPSTATE_UNKNOWN;
        }

        switch(to[0])
        {
            case 'o':
            case 'O': sc.to = POPSTATE_OPEN; break;
            case 'c':
            case 'C': sc.to = POPSTATE_CLOSED; break;

            case 'd':
            case 'D':
                sc.to = POPSTATE_DYNAMIC;
                sc.offset = atoi(to.substr(1).c_str());
            break;

            case '*': sc.to = POPSTATE_ANY; break;
            default:
                sc.from = POPSTATE_UNKNOWN;
        }

        mCmdTable.push_back(sc);
        idx++;
    }

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        vector<SUBCOMMAND_t>::iterator dbgIter;

        for (dbgIter = mCmdTable.begin(); dbgIter != mCmdTable.end(); ++dbgIter)
        {
            MSG_DEBUG("States from: " << dbgIter->from << ", to: " << dbgIter->to << ", offset: " << dbgIter->offset);
        }
    }

    TSubPage *sp = nullptr;

    if (!page.empty())
    {
        TPage *pg = getPage(page);

        if (pg)
            sp = pg->getSubPage(popup);
    }
    else
        sp = loadSubPage(popup);

    if (!sp || !sp->isCollapsible())
        return;


    bool visible = sp->isVisible();
    // Go through command table and execute it
    vector<SUBCOMMAND_t>::iterator cmdIter;
    COLLAPS_STATE_t cs = sp->getCollapseState();

    for (cmdIter = mCmdTable.begin(); cmdIter != mCmdTable.end(); ++cmdIter)
    {
        if (!visible && cmdIter->to != POPSTATE_CLOSED)
        {
            sp->setCollapsible(COL_FULL, cmdIter->offset);
            break;
        }
        else if (visible)
        {
            if (cmdIter->to == POPSTATE_CLOSED)
                sp->setCollapsible(COL_CLOSED);
            else if (cs == COL_FULL && cmdIter->to == POPSTATE_ANY)
                sp->setCollapsible(COL_SMALL);
            else if (cs == COL_FULL && cmdIter->to == POPSTATE_DYNAMIC)
                sp->setCollapsible(COL_SMALL, cmdIter->offset);
            else if (cs == COL_SMALL && cmdIter->to == POPSTATE_ANY)
                sp->setCollapsible(COL_FULL);
            else if (cs == COL_SMALL && cmdIter->to == POPSTATE_DYNAMIC)
                sp->setCollapsible(COL_FULL, cmdIter->offset);

            break;
        }
    }
}

/**
 * @brief TPageManager::doPTC - Toggle Collapsible Popup Collapsed Command
 * Toggles the named collapsible popup between the open and collapsed positions.
 * More specifically, if the popup is not fully collapsed, it is collapsed.
 * Syntax:
 *      "'^PTC-<popup>;[optional target page]'"
 * Variables:
 *      Popup = the name of the popup to toggle
 *      Target page = name of the page hosting the popup to affect the change
 *                    upon. If target page is not specified, the command is
 *                    applied to the current page.
 * Examples:
 *      SEND_COMMAND Panel,"'^PTC-Contacts'"
 *  Toggle the Contacts popup collapsed on the current page.
 *      SEND_COMMAND Panel,"'^PTC-Contacts;Teleconference Control'"
 *  Toggle the Contacts popup collapsed on the Teleconference Control page.
 *  Note: Collapsible popup send commands do not automatically show the popup
 *        on the target page. The popup must be first shown with a standard
 *        show command. This applies even when the collapsible popup is a
 *        member of a popup group. For all of these commands, if the target
 *        page is blank, the current page is used. If the named popup is not
 *        collapsible, the commands are ignored.
 */
void TPageManager::doPTC(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPTC(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command PTC: Expect at least 1 parameter but got none!");
        return;
    }

    string popup = pars[0];
    string page;

    if (pars.size() > 1)
        page = pars[1];

    TSubPage *sp = loadSubPage(popup);

    if (!sp || !sp->isCollapsible() || sp->getCollapseState() == COL_CLOSED)
        return;

    if (!sp->isVisible())
    {
        sp->setCollapsible(COL_CLOSED);
        return;
    }

    if (sp->getCollapseState() == COL_SMALL)
        sp->setCollapsible(COL_FULL);
    else if (sp->getCollapseState() == COL_FULL)
        sp->setCollapsible(COL_SMALL);

    // TODO: Add code to honor the "page", if there is one.
}

/**
 * @brief TPageManager::doPTO - Toggle Collapsed Popup Open Command
 * Toggles the named collapsible popup between the open and collapsed positions.
 * More specifically, if the popup is not fully open, it is opened.
 * Syntax:
 *      "'^PTO-<popup>;[optional target page]'"
 * Variables:
 *      Popup = the name of the popup to toggle
 *      Target page = name of the page hosting the popup to affect the change
 *                    upon. If target page is not specified, the command is
 *                    applied to the current page.
 * Examples:
 *      SEND_COMMAND Panel,"'^PTO-Contacts'"
 *  Toggle the Contacts popup open on the current page.
 *      SEND_COMMAND Panel,"'^PTO-Contacts;Teleconference Control'"
 *  Toggle the Contacts popup open on the Teleconference Control page.
 *  Note: Collapsible popup send commands do not automatically show the popup
 *        on the target page. The popup must be first shown with a standard
 *        show command. This applies even when the collapsible popup is a
 *        member of a popup group. For all of these commands, if the target
 *        page is blank, the current page is used. If the named popup is not
 *        collapsible, the commands are ignored.
 */
void TPageManager::doPTO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPTO(int port, vector<int>& channels, vector<string>& pars)");

    Q_UNUSED(port);
    Q_UNUSED(channels);

    if (pars.empty())
    {
        MSG_WARNING("Command PTO: Expect at least 1 parameter but got none!");
        return;
    }

    string popup = pars[0];
    string page;

    if (pars.size() > 1)
        page = pars[1];

    TSubPage *sp = loadSubPage(popup);

    if (!sp || !sp->isCollapsible() || sp->getCollapseState() == COL_CLOSED)
        return;

    if (!sp->isVisible())
    {
        sp->setCollapsible(COL_CLOSED);
        return;
    }

    if (sp->getCollapseState() == COL_SMALL)
        sp->setCollapsible(COL_FULL);
    else if (sp->getCollapseState() == COL_FULL)
        sp->setCollapsible(COL_SMALL);

    // TODO: Add code to honor the "page", if there is one.
}

/**
 * @brief TPageManager::doANI Run a button animation (in 1/10 second).
 * Syntax:
 *      ^ANI-<vt addr range>,<start state>,<end state>,<time>
 * Variable:
 *      variable text address range = 1 - 4000.
 *      start state = Beginning of button state (0= current state).
 *      end state = End of button state.
 *      time = In 1/10 second intervals.
 * Example:
 *      SEND_COMMAND Panel,"'^ANI-500,1,25,100'"
 * Runs a button animation at text range 500 from state 1 to state 25 for 10 seconds.
 *
 * @param port      The port number
 * @param channels  The channels of the buttons
 * @param pars      The parameters
 */
void TPageManager::doANI(int port, std::vector<int> &channels, std::vector<std::string> &pars)
{
    DECL_TRACER("TPageManager::doANI(int port, std::vector<int> &channels, std::vector<std::string> &pars)");

    if (pars.size() < 3)
    {
        MSG_ERROR("Command ANI: Expecting 3 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int stateStart = atoi(pars[0].c_str());
    int endState = atoi(pars[1].c_str());
    int runTime = atoi(pars[2].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->startAnimation(stateStart, endState, runTime);
        }
    }
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(intToString(stateStart) + "," + intToString(endState) + "," + intToString(runTime));

    setDone();
#endif
}

/**
 * Add page flip action to a button if it does not already exist.
 */
void TPageManager::doAPF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doAPF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command APF: Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    string action = pars[0];
    string pname = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->addPushFunction(action, pname);
        }
    }
#if TESTMODE == 1
    if (_gTestMode)
        _gTestMode->setResult(toUpper(action) + "," + toUpper(pname));

    __success = true;
    setAllDone();
#endif
}

/**
 * Append non-unicode text.
 */
void TPageManager::doBAT(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doBAT(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command BAT: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;

    if (pars.size() > 1)
        text = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.empty())
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *>::iterator mapIter;

    for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
    {
        Button::TButton *bt = *mapIter;

        bt->appendText(text, btState - 1);
#if TESTMODE == 1
        if (_gTestMode)
        {
            int st = (btState > 0 ? (btState - 1) : 0);
            _gTestMode->setResult(bt->getText(st));
        }

        __success = true;
#endif
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * @brief Append unicode text. Same format as ^UNI.
 * This command allows to set up to 50 characters of ASCII code. The unicode
 * characters must be set as hex numbers.
 */
void TPageManager::doBAU(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBAU(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command BAU: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;
    char ch[3];

    if (pars.size() > 1)
        text = pars[1];

    if ((text.size() % 4) == 0)
    {
        try
        {
            text = pars[1];
            MSG_DEBUG("Processing UTF16 string: " << text);
            // Because the unicode characters are hex numbers, we scan the text
            // and convert the hex numbers into real numbers.
            size_t len = text.length();
            bool inHex = false;
            int lastChar = 0;
            uint16_t *numstr = new uint16_t[len / 4];
            int uniPos = 0;
            int cntCount = 0;

            for (size_t i = 0; i < len; i++)
            {
                int c = text.at(i);

                if (!inHex && isHex(c))
                {
                    inHex = true;
                    lastChar = c;
                    continue;
                }

                if (inHex && !isHex(c))
                    break;

                if (inHex && isHex(c))
                {
                    ch[0] = lastChar;
                    ch[1] = c;
                    ch[2] = 0;
                    uint16_t num = (uint16_t)strtol(ch, NULL, 16);

                    if ((cntCount % 2) != 0)
                    {
                        numstr[uniPos] |= num;
                        uniPos++;
                    }
                    else
                        numstr[uniPos] = (num << 8) & 0xff00;

                    cntCount++;
                    inHex = false;

                    if (uniPos >= 50)
                        break;
                }
            }

            text.clear();
            // Here we make from the real numbers a UTF8 string
            for (size_t i = 0; i < len / 4; ++i)
            {
                if (numstr[i] <= 0x00ff)
                {
                    ch[0] = numstr[i];
                    ch[1] = 0;
                    text.append(ch);
                }
                else
                {
                    ch[0] = (numstr[i] >> 8) & 0x00ff;
                    ch[1] = numstr[i] & 0x00ff;
                    ch[2] = 0;
                    text.append(ch);
                }
            }

            delete[] numstr;
        }
        catch (std::exception const & e)
        {
            MSG_ERROR("Character conversion error: " << e.what());
#if TESTMODE == 1
            setAllDone();
#endif
            return;
        }
    }
    else
    {
        MSG_WARNING("No or invalid UTF16 string: " << text);
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            bt->appendText(text, btState - 1);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getText(btState - 1));

            __success = true;
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * @brief TPageManager::doBCB Set the border color.
 * Set the border color to the specified color. Only if the specified border
 * color is not the same as the current color.
 * Note: Color can be assigned by color name (without spaces), number or
 * R,G,B value (RRGGBB or RRGGBBAA).
 */
void TPageManager::doBCB(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doBCB(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BCB: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    if (pars.size() > 1)
        color = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBorderColor(color, btState - 1);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getBorderColor(btState == 0 ? 0 : btState - 1));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/*
 * Get the border color and send it as a custom event.
 */
void TPageManager::getBCB(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBCB(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get BCB: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.empty())
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *>::iterator iter;

    for (iter = buttons.begin(); iter != buttons.end(); ++iter)
    {
        Button::TButton *bt = *iter;

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                color = bt->getBorderColor(i);

                if (color.empty())
                    continue;

                sendCustomEvent(i + 1, (int)color.length(), 0, color, 1011, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
                __success = true;

                if (_gTestMode)
                    _gTestMode->setResult(color);
#endif
            }
        }
        else
        {
            color = bt->getBorderColor(btState - 1);

            if (color.empty())
                continue;

            sendCustomEvent(btState, (int)color.length(), 0, color, 1011, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
            __success = true;

            if (_gTestMode)
                _gTestMode->setResult(color);
#endif
        }
    }
#if TESTMODE == 1
    setAllDone();
#endif
}

/**
 * @brief Set the fill color to the specified color.
 * Only if the specified fill color is not the same as the current color.
 * Note: Color can be assigned by color name (without spaces), number or R,G,B value (RRGGBB or RRGGBBAA).
 */
void TPageManager::doBCF(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBCF(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BCF: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    if (pars.size() > 1)
        color = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setFillColor(color, btState - 1);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getFillColor(btState == 0 ? 0 : btState - 1));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/*
 * Get the fill color and send it via a custom event to the NetLinx.
 */
void TPageManager::getBCF(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBCF(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get BCF: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.empty())
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *>::iterator iter;

    for (iter = buttons.begin(); iter != buttons.end(); ++iter)
    {
        Button::TButton *bt = *iter;

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                color = bt->getFillColor(i);

                if (color.empty())
                    continue;

                sendCustomEvent(i + 1, (int)color.length(), 0, color, 1012, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
                __success = true;

                if (_gTestMode)
                    _gTestMode->setResult(color);
#endif
            }
        }
        else
        {
            color = bt->getFillColor(btState-1);

            if (color.empty())
                continue;

            sendCustomEvent(btState, (int)color.length(), 0, color, 1012, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
            __success = true;

            if (_gTestMode)
                _gTestMode->setResult(color);
#endif
        }
    }
#if TESTMODE == 1
    setAllDone();
#endif
}

/**
 * @brief Set the text color to the specified color.
 * Only if the specified text color is not the same as the current color.
 * Note: Color can be assigned by color name (without spaces), number or R,G,B value (RRGGBB or RRGGBBAA).
 */
void TPageManager::doBCT(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBCT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BCT: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    if (pars.size() > 1)
        color = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setTextColor(color, btState - 1);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getTextColor(btState == 0 ? 0 : btState - 1));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/*
 * Get the text color of a button and send it via a custom event to the NetLinx.
 */
void TPageManager::getBCT(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBCT(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get BCT: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.empty())
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *>::iterator iter;

    for (iter = buttons.begin(); iter != buttons.end(); ++iter)
    {
        Button::TButton *bt = *iter;

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                color = bt->getTextColor(i);

                if (color.empty())
                    continue;

                sendCustomEvent(i + 1, (int)color.length(), 0, color, 1013, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
                __success = true;

                if (_gTestMode)
                    _gTestMode->setResult(color);
#endif
            }
        }
        else
        {
            color = bt->getTextColor(btState - 1);

            if (color.empty())
                continue;

            sendCustomEvent(btState, (int)color.length(), 0, color, 1013, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
            __success = true;

            if (_gTestMode)
                _gTestMode->setResult(color);
#endif
        }
    }
#if TESTMODE == 1
    setAllDone();
#endif
}

/**
 * Set the button draw order
 * Determines what order each layer of the button is drawn.
 */
void TPageManager::doBDO(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBDO(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BDO: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string order;

    if (pars.size() > 1)
    {
        string ord = pars[1];
        // Convert the numbers into the expected draw order
        for (size_t i = 0; i < ord.length(); i++)
        {
            if (ord.at(i) >= '1' && ord.at(i) <= '5')
            {
                char hv0[32];
                snprintf(hv0, sizeof(hv0), "%02d", (int)(ord.at(i) - '0'));
                order.append(hv0);
            }
            else
            {
                MSG_ERROR("Illegal order number " << ord.substr(i, 1) << "!");
                return;
            }
        }

        if (order.length() != 10)
        {
            MSG_ERROR("Expected 5 order numbers but got " << (order.length() / 2)<< "!");
            return;
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setDrawOrder(order, btState - 1);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getDrawOrder(btState == 0 ? 0 : btState - 1));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Set the feedback type of the button.
 * ONLY works on General-type buttons.
 */
void TPageManager::doBFB(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBFB(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BFB: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    Button::FEEDBACK type = Button::FB_NONE;
    string stype = pars[0];
    vector<string> stypes = { "None", "Channel", "Invert", "On", "Momentary", "Blink" };
    vector<string>::iterator iter;
    int i = 0;

    for (iter = stypes.begin(); iter != stypes.end(); ++iter)
    {
        if (strCaseCompare(stype, *iter) == 0)
        {
            type = (Button::FEEDBACK)i;
            break;
        }

        i++;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setFeedback(type);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(intToString(bt->getFeedback()));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/*
 * Set the input mask for the specified address.
 */
void TPageManager::doBIM(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBIM(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BIM: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    string mask = pars[0];
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setInputMask(mask);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getInputMask());
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * @brief Button copy command.
 * Copy attributes of the source button to all the
 * destination buttons. Note that the source is a single button state. Each
 * state must be copied as a separate command. The <codes> section represents
 * what attributes will be copied. All codes are 2 char pairs that can be
 * separated by comma, space, percent or just ran together.
 */
void TPageManager::doBMC(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBMC(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 5)
    {
        MSG_ERROR("Command do BMC: Expecting 5 parameters but got " << pars.size() << ". Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int src_port = atoi(pars[1].c_str());
    int src_addr = atoi(pars[2].c_str());
    int src_state = atoi(pars[3].c_str());
    string src_codes = pars[4];
    vector<int> src_channel;
    src_channel.push_back(src_addr);

    vector<TMap::MAP_T> src_map = findButtons(src_port, src_channel);

    if (src_map.size() == 0)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " does not exist!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *>src_buttons = collectButtons(src_map);

    if (src_buttons.size() == 0)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " does not exist!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    if (src_buttons[0]->getNumberInstances() < src_state)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " has less then " << src_state << " elements.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    if (src_state < 1)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " has invalid source state " << src_state << ".");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    src_state--;

    if (btState > 0)
        btState--;

    vector<TMap::MAP_T> map = findButtons(port, channels);
    vector<Button::TButton *> buttons = collectButtons(map);
    //                        0     1     2     3     4     5     6     7
    vector<string>codes = { "BM", "BR", "CB", "CF", "CT", "EC", "EF", "FT",
                            "IC", "JB", "JI", "JT", "LN", "OP", "SO", "TX", // 8 - 15
                            "VI", "WW" };   // 16, 17

    for (size_t ibuttons = 0; ibuttons < buttons.size(); ibuttons++)
    {
        vector<string>::iterator iter;
        int idx = 0;

        for (iter = codes.begin(); iter != codes.end(); ++iter)
        {
            if (src_codes.find(*iter) != string::npos)
            {
                int j, x, y;

                switch(idx)
                {
                    case 0: buttons[ibuttons]->setBitmap(src_buttons[0]->getBitmapName(src_state), btState, 0); break;
                    case 1: buttons[ibuttons]->setBorderStyle(src_buttons[0]->getBorderStyle(src_state), btState); break;
                    case 2: buttons[ibuttons]->setBorderColor(src_buttons[0]->getBorderColor(src_state), btState); break;
                    case 3: buttons[ibuttons]->setFillColor(src_buttons[0]->getFillColor(src_state), btState); break;
                    case 4: buttons[ibuttons]->setTextColor(src_buttons[0]->getTextColor(src_state), btState); break;
                    case 5: buttons[ibuttons]->setTextEffectColor(src_buttons[0]->getTextEffectColor(src_state), btState); break;
                    case 6: buttons[ibuttons]->setTextEffect(src_buttons[0]->getTextEffect(src_state), btState); break;
                    case 7: buttons[ibuttons]->setFontIndex(src_buttons[0]->getFontIndex(src_state), btState); break;
                    case 8: buttons[ibuttons]->setIcon(src_buttons[0]->getIconIndex(src_state), btState); break;

                    case 9:
                        j = src_buttons[0]->getBitmapJustification(&x, &y, src_state);
                        buttons[ibuttons]->setBitmapJustification(j, x, y, btState);
                    break;

                    case 10:
                        j = src_buttons[0]->getIconJustification(&x, &y, src_state);
                        buttons[ibuttons]->setIconJustification(j, x, y, btState);
                    break;

                    case 11:
                        j = src_buttons[0]->getTextJustification(&x, &y, src_state);
                        buttons[ibuttons]->setTextJustification(j, x, y, btState);
                    break;

                    case 12: MSG_INFO("\"Lines of video removed\" not supported!"); break;
                    case 13: buttons[ibuttons]->setOpacity(src_buttons[0]->getOpacity(src_state), btState); break;
                    case 14: buttons[ibuttons]->setSound(src_buttons[0]->getSound(src_state), btState); break;
                    case 15: buttons[ibuttons]->setText(src_buttons [0]->getText(src_state), btState); break;
                    case 16: MSG_INFO("\"Video slot ID\" not supported!"); break;
                    case 17: buttons[ibuttons]->setTextWordWrap(src_buttons[0]->getTextWordWrap(src_state), btState); break;
                }
            }

            idx++;
        }
    }
}

void TPageManager::doBMF (int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBMF (int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command BMF: Less then 2 parameters!");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str()) - 1;
    string commands;

    for (size_t i = 1; i < pars.size(); ++i)
    {
        if (i > 1)
            commands += ",";

        commands += pars[i];
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    // Start of parsing the command line
    // We splitt the command line into parts by searching for a percent (%) sign.
    vector<string> parts = StrSplit(commands, "%");

    if (parts.empty())
        parts.push_back(commands);

    // Search for all buttons who need to be updated
    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (!bt)
            {
                MSG_WARNING("Command ^BMF found an invalid pointer to a button!")
                continue;
            }

            // Iterate through commands and apply them to button
            vector<string>::iterator iter;

            for (iter = parts.begin(); iter != parts.end(); ++iter)
            {
                char cmd = iter->at(0);
                char cmd2;
                string content;

                switch(cmd)
                {
                    case 'B':   // Border style
                        if (iter->at(1) == ',')
                            content = iter->substr(2);
                        else
                            content = iter->substr(1);

                        if (!content.empty() && isdigit(content[0]))
                            bt->setBorderStyle(atoi(content.c_str()), btState);
                        else
                            bt->setBorderStyle(content, btState);
#if TESTMODE == 1
                        if (_gTestMode)
                            _gTestMode->setResult(bt->getBorderStyle(btState < 0 ? 0 : btState));
#endif
                    break;

                    case 'C':   // Colors
                        cmd2 = iter->at(1);
                        content = iter->substr(2);

                        switch(cmd2)
                        {
                            case 'B':   // Border color
                                bt->setBorderColor(content, btState);
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(bt->getBorderColor(btState < 0 ? 0 : btState));
#endif
                            break;

                            case 'F':   // Fill color
                                bt->setFillColor(content, btState);
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(bt->getFillColor(btState < 0 ? 0 : btState));
#endif
                            break;

                            case 'T':   // Text color
                                bt->setTextColor(content, btState);
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(bt->getTextColor(btState < 0 ? 0 : btState));
#endif
                            break;
                        }
                    break;

                    case 'D':   // Draw order
                        cmd2 = iter->at(1);
                        content = iter->substr(2);

                        if (cmd2 == 'O')
                        {
                            bt->setDrawOrder(content, btState);
#if TESTMODE == 1
                            if (_gTestMode)
                                _gTestMode->setResult(bt->getDrawOrder(btState < 0 ? 0 : btState));
#endif
                        }
                    break;

                    case 'E':   // Text effect
                        cmd2 = iter->at(1);
                        content = iter->substr(2);

                        switch(cmd2)
                        {
                            case 'C':   // Text effect color
                                bt->setTextEffectColor(content, btState);
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(bt->getTextEffectColor(btState < 0 ? 0 : btState));
#endif
                            break;

                            case 'F':   // Text effect name
                                bt->setTextEffectName(content, btState);
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(bt->getTextEffectName(btState < 0 ? 0 : btState));
#endif
                            break;

                            case 'N':   // Enable/disable button
                                bt->setEnable((content[0] == '1' ? true : false));
#if TESTMODE == 1
                                if (_gTestMode)
                                {
                                    _gTestMode->setResult(bt->isEnabled() ? "TRUE" : "FALSE");
                                    __success = true;
                                    setScreenDone();
                                }
#endif
                            break;
                        }
                    break;

                    case 'F':   // Set font file name
                        if (iter->at(1) == ',')
                            content = iter->substr(2);
                        else
                            content = iter->substr(1);

                        if (!isdigit(content[0]))
                            bt->setFontName(content, btState);
                        else
                            bt->setFontIndex(atoi(content.c_str()), btState);
#if TESTMODE == 1
                        if (_gTestMode)
                            _gTestMode->setResult(intToString(bt->getFontIndex(btState < 0 ? 0 : btState)));
#endif
                    break;

                    case 'G':   // Bargraphs
                        cmd2 = iter->at(1);
                        content = iter->substr(2);

                        switch(cmd2)
                        {
                            case 'C':   // Bargraph slider color
                                bt->setBargraphSliderColor(content);
                            break;

                            case 'D':   // Ramp down time
                                bt->setBargraphRampDownTime(atoi(content.c_str()));
                            break;

                            case 'G':   // Drag increment
                                bt->setBargraphDragIncrement(atoi(content.c_str()));
                            break;

                            case 'H':   // Upper limit
                                bt->setBargraphUpperLimit(atoi(content.c_str()));
                            break;

                            case 'I':   // Invert/noninvert
                                if (bt->getButtonType() == BARGRAPH || bt->getButtonType() == MULTISTATE_BARGRAPH)
                                    bt->setBargraphInvert(atoi(content.c_str()) > 0 ? true : false);
                            break;

                            case 'L':   // Lower limit
                                bt->setBargraphLowerLimit(atoi(content.c_str()));
                            break;

                            case 'N':   // Slider name
                                bt->setBargraphSliderName(content);
                            break;

                            case 'R':   // Repeat interval
                                // FIXME: Add function to set repeat interval
                            break;

                            case 'U':   // Ramp up time
                                bt->setBargraphRampUpTime(atoi(content.c_str()));
                            break;

                            case 'V':   // Bargraph value
                                bt->setBargraphLevel(atoi(content.c_str()));
                            break;
                        }
                    break;

                    case 'I':   // Set the icon
                        content = iter->substr(1);
                        bt->setIcon(atoi(content.c_str()), btState);
#if TESTMODE == 1
                        if (_gTestMode)
                            _gTestMode->setResult(intToString(bt->getIconIndex()));
#endif
                    break;

                    case 'J':   // Set text justification
                        cmd2 = iter->at(1);

                        if (cmd2 == ',')
                        {
                            content = iter->substr(1);
                            int just = atoi(content.c_str());
                            int x = 0, y = 0;

                            if (just == 0)
                            {
                                vector<string> coords = StrSplit(content, ",");

                                if (coords.size() >= 3)
                                {
                                    x = atoi(coords[1].c_str());
                                    y = atoi(coords[2].c_str());
                                }
                            }

                            bt->setTextJustification(atoi(content.c_str()), x, y, btState);
#if TESTMODE == 1
                            if (_gTestMode)
                            {
                                just = bt->getTextJustification(&x, &y, btState < 0 ? 0 : btState);
                                string s = intToString(just) + "," + intToString(x) + "," + intToString(y);
                                _gTestMode->setResult(s);
                            }
#endif
                        }
                        else if (cmd2 == 'T' || cmd2 == 'B' || cmd2 == 'I')
                        {
                            content = iter->substr(2);
                            int x = 0, y = 0;
                            int just = atoi(content.c_str());

                            if (just == 0)
                            {
                                vector<string> coords = StrSplit(content, ",");

                                if (coords.size() >= 3)
                                {
                                    x = atoi(coords[1].c_str());
                                    y = atoi(coords[2].c_str());
                                }
                            }

                            switch(cmd2)
                            {
                                case 'B':   // Alignment of bitmap
                                    bt->setBitmapJustification(atoi(content.c_str()), x, y, btState);
#if TESTMODE == 1
                                    just = bt->getBitmapJustification(&x, &y, btState < 0 ? 0 : btState);
#endif
                                break;

                                case 'I':   // Alignment of icon
                                    bt->setIconJustification(atoi(content.c_str()), x, y, btState);
#if TESTMODE == 1
                                    just = bt->getIconJustification(&x, &y, btState < 0 ? 0 : btState);
#endif
                                break;

                                case 'T':   // Alignment of text
                                    bt->setTextJustification(atoi(content.c_str()), x, y, btState);
#if TESTMODE == 1
                                    just = bt->getTextJustification(&x, &y, btState < 0 ? 0 : btState);
#endif
                                break;
                            }
#if TESTMODE == 1
                            if (_gTestMode)
                            {
                                string s = intToString(just) + "," + intToString(x) + "," + intToString(y);
                                _gTestMode->setResult(s);
                            }
#endif
                        }
                    break;

                    case 'M':   // Text area
                        cmd2 = iter->at(1);
                        content = iter->substr(2);

                        switch(cmd2)
                        {
                            case 'I':   // Set mask image
                                // FIXME: Add code for image mask
                            break;

                            case 'K':   // Input mask of text area
                                // FIXME: Add input mask
                            break;

                            case 'L':   // Maximum length of text area
                                // FIXME: Add code to set maximum length
                            break;
                        }
                    break;

                    case 'O':   // Set feedback typ, opacity
                        cmd2 = iter->at(1);

                        switch(cmd2)
                        {
                            case 'P':   // Set opacity
                                bt->setOpacity(atoi(iter->substr(2).c_str()), btState);
                            break;

                            case 'T':   // Set feedback type
                                content = iter->substr(2);
                                content = toUpper(content);

                                if (content == "NONE")
                                    bt->setFeedback(Button::FB_NONE);
                                else if (content == "CHANNEL")
                                    bt->setFeedback(Button::FB_CHANNEL);
                                else if (content == "INVERT")
                                    bt->setFeedback(Button::FB_INV_CHANNEL);
                                else if (content == "ON")
                                    bt->setFeedback(Button::FB_ALWAYS_ON);
                                else if (content == "MOMENTARY")
                                    bt->setFeedback(Button::FB_MOMENTARY);
                                else if (content == "BLINK")
                                    bt->setFeedback(Button::FB_BLINK);
                                else
                                {
                                    MSG_WARNING("Unknown feedback type " << content);
                                }
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(intToString(bt->getFeedback()));
#endif
                            break;

                            default:
                                content = iter->substr(1);
                                // FIXME: Add code to set the feedback type
                        }
                    break;

                    case 'P':   // Set picture/bitmap file name
                        content = iter->substr(1);

                        if (content.find(".") == string::npos)  // If the image has no extension ...
                        {                                       // we must find the image in the map
                            string iname = findImage(content);

                            if (!iname.empty())
                                content = iname;
                        }

                        bt->setBitmap(content, btState, 0);
                    break;

                    case 'R':   // Set rectangle
                    {
                        content = iter->substr(1);
                        vector<string> corners = StrSplit(content, ",");

                        if (corners.size() > 0)
                        {
                            vector<string>::iterator itcorn;
                            int pos = 0;
                            int left, top, right, bottom;
                            left = top = right = bottom = 0;

                            for (itcorn = corners.begin(); itcorn != corners.end(); ++itcorn)
                            {
                                switch(pos)
                                {
                                    case 0: left   = atoi(itcorn->c_str()); break;
                                    case 1: top    = atoi(itcorn->c_str()); break;
                                    case 2: right  = atoi(itcorn->c_str()); break;
                                    case 3: bottom = atoi(itcorn->c_str()); break;
                                }

                                pos++;
                            }

                            if (pos >= 4)
                            {
                                bt->setRectangle(left, top, right, bottom);
                                bt->refresh();
                            }
                        }
#if TESTMODE == 1
                        if (_gTestMode)
                        {
                            int left, top, width, height;
                            bt->getRectangle(&left, &top, &height, &width);
                            string res(intToString(left) + "," + intToString(top) + "," + intToString(width) + "," + intToString(height));
                            _gTestMode->setResult(res);
                        }
#endif
                    }
                    break;

                    case 'S':   // show/hide, style, sound
                        cmd2 = iter->at(1);
                        content = iter->substr(2);

                        switch(cmd2)
                        {
                            case 'F':   // Set focus of text area button
                                // FIXME: Add code to set the focus of text area button
                            break;

                            case 'M':   // Submit text
                                if (content.find("|"))  // To be replaced by LF (0x0a)?
                                {
                                    size_t pos = 0;

                                    while ((pos = content.find("|")) != string::npos)
                                        content = content.replace(pos, 1, "\n");
                                }

                                bt->setText(content, btState);
                            break;

                            case 'O':   // Sound
                                bt->setSound(content, btState);
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(bt->getSound(btState < 0 ? 0 : btState));
#endif
                            break;

                            case 'T':   // Button style
                                // FIXME: Add code to set the button style
                            break;

                            case 'W':   // Show / hide button
                                if (content[0] == '0')
                                    bt->hide(true);
                                else
                                    bt->show();
#if TESTMODE == 1
                                if (_gTestMode)
                                    _gTestMode->setResult(bt->isVisible() ? "TRUE" : "FALSE");
#endif
                            break;
                        }
                    break;

                    case 'T':   // Set text
                        content = iter->substr(1);

                        if (content.find("|"))  // To be replaced by LF (0x0a)?
                        {
                            size_t pos = 0;

                            while ((pos = content.find("|")) != string::npos)
                                content = content.replace(pos, 1, "\n");
                        }

                        bt->setText(content, btState);
#if TESTMODE == 1
                        if (_gTestMode)
                            _gTestMode->setResult(bt->getText(btState < 0 ? 0 : btState));
#endif
                    break;

                    case 'U':   // Set the unicode text
                        if (iter->at(1) == 'N')
                        {
                            content = iter->substr(2);
                            string byte, text;
                            size_t pos = 0;

                            while (pos < content.length())
                            {
                                byte = content.substr(pos, 2);
                                char ch = (char)strtol(byte.c_str(), NULL, 16);
                                text += ch;
                                pos += 2;
                            }

                            if (text.find("|"))  // To be replaced by LF (0x0a)?
                            {
                                size_t pos = 0;

                                while ((pos = text.find("|")) != string::npos)
                                    text = text.replace(pos, 1, "\n");
                            }

                            bt->setText(text, btState);
                        }
                    break;

                    case 'V':   // Video on / off
                        cmd2 = iter->at(1);
                        // Controlling a computer remotely is not supported.
                        if (cmd2 != 'L' && cmd2 != 'N' && cmd2 != 'P')
                        {
                            content = iter->substr(2);
                            // FIXME: Add code to switch video on or off
                        }
                    break;

                    case 'W':   // Word wrap
                        if (iter->at(1) == 'W')
                        {
                            content = iter->substr(2);
                            bt->setTextWordWrap(content[0] == '1' ? true : false, btState);
                        }
                    break;
                }
            }
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Set the maximum length of the text area button. If this value is set to
 * zero (0), the text area has no max length. The maximum length available is
 * 2000. This is only for a Text area input button and not for a Text area input
 * masking button.
 */
void TPageManager::doBML(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBML(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command BML: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int maxLen = atoi(pars[0].c_str());

    if (maxLen < 0 || maxLen > 2000)
    {
        MSG_WARNING("Got illegal length of text area! [" << maxLen << "]");
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setTextMaxChars(maxLen);
        }
    }
}

/**
 * Assign a picture to those buttons with a defined address range.
 *
 * TP4 Syntax:
 *    ^BMP-<vt addr range>,<button states range>,<name of bitmap/picture>
 *
 * G5 Syntax:
 *    ^BMP-<addr range>,<button states range>,<name of bitmap/picture>,[bitmap index],[optional justification]
 */
void TPageManager::doBMP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBMP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do BMP: Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str()) - 1;
    string bitmap = pars[1];
    // If this is a G5 command, we may have up to 2 additional parameters.
    int slot = -1, justify = -1, jx = 0, jy = 0;

    if (pars.size() > 2)
    {
        slot = atoi(pars[2].c_str());       // G5: The bitmap index

        if (pars.size() >= 4)
        {
            justify = atoi(pars[3].c_str());

            if (justify == 0)
            {
                if (pars.size() >= 5)
                    jx = atoi(pars[4].c_str());

                if (pars.size() >= 6)
                    jy = atoi(pars[5].c_str());
            }
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            int bst = bt->getNumberInstances();
            MSG_DEBUG("Setting bitmap " << bitmap << " on all " << bst << " instances...");

            if (justify >= 0)
            {
                if (slot == 2 && !TTPInit::isG5())
                    bt->setIconJustification(justify, jx, jy, btState);
                else
                    bt->setBitmapJustification(justify, jx, jy, btState);
            }

            if (slot >= 0)
            {
                if (!TTPInit::isG5())
                {
                    switch(slot)
                    {
                        case 0: bt->setCameleon(bitmap, btState); break;
                        case 2: bt->setIcon(bitmap, btState); break;  // On G4 we have no bitmap layer. Therefor we use layer 2 as icon layer.
                        default:
                            bt->setBitmap(bitmap, btState, 1);
                    }
                }
                else
                    bt->setBitmap(bitmap, btState, slot, justify, jx, jy);
            }
            else
                bt->setBitmap(bitmap, btState, 1, justify, jx, jy);
        }
    }
}

void TPageManager::getBMP(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBMP(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get BMP: Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string bmp;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                bmp = bt->getBitmapName(i);

                if (bmp.empty())
                    continue;

                sendCustomEvent(i + 1, (int)bmp.length(), 0, bmp, 1002, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            bmp = bt->getTextColor(btState-1);
            sendCustomEvent(btState, (int)bmp.length(), 0, bmp, 1002, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Set the button opacity. The button opacity can be specified as a decimal
 * between 0 - 255, where zero (0) is invisible and 255 is opaque, or as a
 * HEX code, as used in the color commands by preceding the HEX code with
 * the # sign. In this case, #00 becomes invisible and #FF becomes opaque.
 * If the opacity is set to zero (0), this does not make the button inactive,
 * only invisible.
 */
void TPageManager::doBOP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBOP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do BOP: Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str()) - 1;
    int btOpacity = 0;

    if (pars[1].at(0) == '#')
        btOpacity = (int)strtol(pars[1].substr(1).c_str(), NULL, 16);
    else
        btOpacity = atoi(pars[1].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setOpacity(btOpacity, btState);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(intToString(bt->getOpacity(btState < 0 ? 0 : btState)));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::getBOP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getBOP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get BOP: Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                int oo = bt->getOpacity(i);
                sendCustomEvent(i + 1, oo, 0, "", 1015, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            int oo = bt->getOpacity(btState-1);
            sendCustomEvent(btState, oo, 0, "", 1015, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

void TPageManager::doBOR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBOR(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BOR: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    // Numbers of styles from 0 to 41
    string bor = pars[0];
    string border = "None";
    int ibor = -1;
    Border::TIntBorder borders;

    if (bor.at(0) >= '0' && bor.at(0) <= '9')
    {
        ibor = atoi(bor.c_str());

        if (ibor >= 0 && ibor <= 41)
            border = borders.getTP4BorderName(ibor);
        else
        {
            MSG_WARNING("Invalid border style ID " << ibor);
#if TESTMODE == 1
            setAllDone();
#endif
            return;
        }

        MSG_DEBUG("Id " << ibor << " is border " << border);
    }
    else
    {
        if (!borders.isTP4BorderValid(bor))
        {
            MSG_WARNING("Unknown border style " << bor);
#if TESTMODE == 1
            setAllDone();
#endif
            return;
        }

        border = bor;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBorderStyle(border);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getBorderStyle(0));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::doBOS(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBOS(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command BOS: Expecting at least 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int videoState = atoi(pars[1].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)       // All instances?
                bt->setDynamic(videoState);
            else
                bt->setDynamic(videoState, btState-1);
        }
    }
}

/**
 * Set the border of a button state/states.
 * The border names are available through the TPDesign4 border-name drop-down
 * list.
 */
void TPageManager::doBRD(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBRD(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BRD: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string border = "None";

    if (pars.size() > 1)
        border = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
//            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setBorderStyle(border, i+1);
            }
            else
                bt->setBorderStyle(border, btState);
        }
    }
}

void TPageManager::getBRD(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getBRD(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get BRD: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string bname = bt->getBorderStyle(i);
                sendCustomEvent(i + 1, (int)bname.length(), 0, bname, 1014, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            string bname = bt->getBorderStyle(btState-1);
            sendCustomEvent(btState, (int)bname.length(), 0, bname, 1014, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Set the button size and its position on the page.
 */
void TPageManager::doBSP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBSP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command BSP: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    bool bLeft = false, bTop = false, bRight = false, bBottom = false;
    int x, y;

    if (pars.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = pars.begin(); iter != pars.end(); iter++)
        {
            if (iter->compare("left") == 0)
                bLeft = true;
            else if (iter->compare("top") == 0)
                bTop = true;
            else if (iter->compare("right") == 0)
                bRight = true;
            else if (iter->compare("bottom") == 0)
                bBottom = true;
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);
    x = y = 0;

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (bLeft)
                x = 0;

            if (bTop)
                y = 0;

            if (bRight || bBottom)
            {
                ulong handle = bt->getHandle();
                int parentID = (handle >> 16) & 0x0000ffff;
                int pwidth = 0;
                int pheight = 0;

                if (parentID < 500)
                {
                    TPage *pg = getPage(parentID);

                    if (!pg)
                    {
                        MSG_ERROR("Internal error: Page " << parentID << " not found!");
                        return;
                    }

                    pwidth = pg->getWidth();
                    pheight = pg->getHeight();
                }
                else
                {
                    TSubPage *spg = getSubPage(parentID);

                    if (!spg)
                    {
                        MSG_ERROR("Internal error: Subpage " << parentID << " not found!");
                        return;
                    }

                    pwidth = spg->getWidth();
                    pheight = spg->getHeight();
                }

                if (bRight)
                    x = pwidth - bt->getWidth();

                if (bBottom)
                    y = pheight - bt->getHeight();
            }

            bt->setLeftTop(x, y);
#if TESTMODE == 1
            if (_gTestMode)
            {
                int left = bt->getLeftPosition();
                int top = bt->getTopPosition();
                string res = intToString(left) + "," + intToString(top);
                _gTestMode->setResult(res);
            }
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

/**
 * Submit text for text area buttons. This command causes the text areas to
 * send their text as strings to the NetLinx Master.
 */
void TPageManager::doBSM(int port, vector<int>& channels, vector<string>&)
{
    DECL_TRACER("TPageManager::doBSM(int port, vector<int>& channels, vector<string>& pars)");

    TError::clear();
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (bt->getButtonType() != TEXT_INPUT && bt->getButtonType() != GENERAL)
            {
#if TESTMODE == 1
                setAllDone();
#endif
                return;
            }

            amx::ANET_SEND scmd;
            scmd.port = bt->getChannelPort();
            scmd.channel = bt->getChannelNumber();
            scmd.ID = scmd.channel;
            scmd.msg = bt->getText(0);
            scmd.MC = 0x008b;       // string value

            if (gAmxNet)
                gAmxNet->sendCommand(scmd);
            else
                MSG_WARNING("Missing global class TAmxNet. Can't send message!");

        }
    }
}

/**
 * Set the sound played when a button is pressed. If the sound name is blank
 * the sound is then cleared. If the sound name is not matched, the button
 * sound is not changed.
 */
void TPageManager::doBSO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBSO(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command BSO: Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    if (!gPrjResources)
        return;

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string sound = pars[1];

    if (!soundExist(sound))
        return;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setSound(sound, i);
            }
            else
                bt->setSound(sound, btState-1);
        }
    }
}

/**
 * Set the button word wrap feature to those buttons with a defined address
 * range. By default, word-wrap is Off.
 */
void TPageManager::doBWW(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBWW(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do BWW: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
//            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting word wrap on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setTextWordWrap(true, i);
            }
            else
                bt->setTextWordWrap(true, btState - 1);
        }
    }
}

void TPageManager::getBWW(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getBWW(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get BWW: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
                sendCustomEvent(i + 1, bt->getTextWordWrap(i), 0, "", 1010, bt->getChannelPort(), bt->getChannelNumber());
        }
        else
            sendCustomEvent(btState, bt->getTextWordWrap(btState-1), 0, "", 1010, bt->getChannelPort(), bt->getChannelNumber());
    }
}

/**
 * Clear all page flips from a button.
 */
void TPageManager::doCPF(int port, vector<int>& channels, vector<string>&)
{
    DECL_TRACER("TPageManager::doCPF(int port, vector<int>& channels, vector<string>& pars)");

    TError::clear();
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
//            setButtonCallbacks(bt);
            bt->clearPushFunctions();
        }
    }
}

/**
 * Delete page flips from button if it already exists.
 */
void TPageManager::doDPF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doDPF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command DPF: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    string action = pars[0];
    string pname;

    if (pars.size() >= 2)
    {
        pname = pars[1];
        vector<Button::TButton *> list;
        // First we search for a subpage because this is more likely
        TSubPage *spg = getSubPage(pname);

        if (spg)
            list = spg->getButtons(port, channels[0]);
        else    // Then for a page
        {
            TPage *pg = getPage(pname);

            if (pg)
                list = pg->getButtons(port, channels[0]);
            else
            {
                MSG_WARNING("The name " << pname << " doesn't name either a page or a subpage!");
                return;
            }
        }

        if (list.empty())
            return;

        vector<Button::TButton *>::iterator it;

        for (it = list.begin(); it != list.end(); it++)
        {
            Button::TButton *bt = *it;
//            setButtonCallbacks(bt);
            bt->clearPushFunction(action);
        }

        return;
    }

    // Here we don't have a page name
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
//            setButtonCallbacks(bt);
            bt->clearPushFunction(action);
        }
    }
}

/**
 * Enable or disable buttons with a set variable text range.
 */
void TPageManager::doENA(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doENA(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_ERROR("Command ENA: Expecting 1 parameter but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int cvalue = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setEnable(((cvalue)?true:false));
#if TESTMODE == 1
            __success = true;

            if (_gTestMode)
                _gTestMode->setResult(intToString(cvalue));
#endif
        }
    }
#if TESTMODE == 1
    setAllDone();
#endif
}

/**
 * Set a font to a specific Font ID value for those buttons with a defined
 * address range. Font ID numbers are generated by the TPDesign4 programmers
 * report.
 */
void TPageManager::doFON(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doFON(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do FON: Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str()) - 1;
    int fvalue = atoi(pars[1].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setFont(fvalue, btState);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(intToString(bt->getFontIndex(btState < 0 ? 0 : btState)));
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::getFON(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getFON(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get FON: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
                sendCustomEvent(i + 1, bt->getFontIndex(i), 0, "", 1007, bt->getChannelPort(), bt->getChannelNumber());
        }
        else
            sendCustomEvent(btState, bt->getFontIndex(btState - 1), 0, "", 1007, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
        if (_gTestMode)
            _gTestMode->setResult(intToString(bt->getFontIndex(btState < 0 ? 0 : btState)));
#endif
    }
#if TESTMODE == 1
    __success = true;
    setAllDone();
#endif
}

void TPageManager::doGDI(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doGDI(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command GDI: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int inc = atoi(pars[0].c_str());

    if (inc < 0)
    {
        MSG_ERROR("Invalid drag increment of " << inc << "!");
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphDragIncrement(inc);
        }
    }
}
/*
 * Invert the joystick axis to move the origin to another corner.
 */
void TPageManager::doGIV(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doGIV(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.empty())
        return;

    TError::clear();
    int inv = atoi(pars[0].c_str());

    if (inv < 0 || inv > 3)
    {
        MSG_ERROR("Invalid invert type " << inv);
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphInvert(inv);
        }
    }
}

/**
 * Change the bargraph upper limit.
 */
void TPageManager::doGLH(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doGLH(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command GLH: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int limit = atoi(pars[0].c_str());

    if (limit < 1)
    {
        MSG_ERROR("Invalid upper limit " << limit << "!");
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphUpperLimit(limit);
        }
    }
}

/**
 * Change the bargraph lower limit.
 */
void TPageManager::doGLL(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doGLL(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command GLL: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int limit = atoi(pars[0].c_str());

    if (limit < 1)
    {
        MSG_ERROR("Invalid lower limit " << limit << "!");
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphLowerLimit(limit);
        }
    }
}

/*
 * Change the bargraph slider color or joystick cursor color.
 */
void TPageManager::doGSC(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doGSC(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command GSC: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    string color = pars[0];
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphSliderColor(color);
        }
    }
}

/*
 * Set bargraph ramp down time in 1/10 seconds.
 */
void TPageManager::doGRD(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doGRD(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command GRD: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int t = atoi(pars[0].c_str());

    if (t < 0)
    {
        MSG_ERROR("Invalid ramp down time limit " << t << "!");
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphRampDownTime(t);
        }
    }
}

/*
 * Set bargraph ramp up time in 1/10 seconds.
 */
void TPageManager::doGRU(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doGRU(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command GRU: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int t = atoi(pars[0].c_str());

    if (t < 0)
    {
        MSG_ERROR("Invalid ramp up time limit " << t << "!");
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphRampUpTime(t);
        }
    }
}

/*
 * Change the bargraph slider name or joystick cursor name.
 */
void TPageManager::doGSN(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doGSN(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command GSN: Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    string name = pars[0];
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setBargraphSliderName(name);
        }
    }
}

/**
 * Set the icon to a button.
 */
void TPageManager::doICO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doICO(int port, vector<int>& channels, vector<string>& pars)");

    if (TTPInit::isG5())
    {
        MSG_INFO("Command ^ICO is not supported by G5 standard!");
        return;
    }

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do ICO: Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int iconIdx = atoi(pars[1].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)       // All instances?
            {
                if (iconIdx > 0)
                    bt->setIcon(iconIdx, -1);
                else
                    bt->revokeIcon(-1);
            }
            else if (iconIdx > 0)
                bt->setIcon(iconIdx, btState - 1);
            else
                bt->revokeIcon(btState - 1);
        }
    }
}

void TPageManager::getICO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getICO(int port, vector<int>& channels, vector<string>& pars)");

    if (TTPInit::isG5())
    {
        MSG_INFO("Command ?ICO is not supported by G5 standard!");
        return;
    }

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get ICO: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
                sendCustomEvent(i + 1, bt->getIconIndex(i), 0, "", 1003, bt->getChannelPort(), bt->getChannelNumber());
        }
        else
            sendCustomEvent(btState, bt->getIconIndex(btState - 1), 0, "", 1003, bt->getChannelPort(), bt->getChannelNumber());
    }
}

/**
 * Set bitmap/picture alignment using a numeric keypad layout for those buttons
 * with a defined address range. The alignment of 0 is followed by
 * ',<left>,<top>'. The left and top coordinates are relative to the upper left
 * corner of the button.
 */
void TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command: do JSB: Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int align = atoi(pars[1].c_str());
    int x = 0, y = 0;

    if (!align && pars.size() >= 3)
    {
        x = atoi(pars[2].c_str());

        if (pars.size() >= 4)
            y = atoi(pars[3].c_str());
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setBitmapJustification(align, x, y, -1);
            else
                bt->setBitmapJustification(align, x, y, btState-1);
        }
    }
}

void TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get JSB: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int j, x, y;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                j = bt->getBitmapJustification(&x, &y, i);
                sendCustomEvent(i + 1, j, 0, "", 1005, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            j = bt->getBitmapJustification(&x, &y, btState-1);
            sendCustomEvent(btState, j, 0, "", 1005, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Set icon alignment using a numeric keypad layout for those buttons with a
 * defined address range. The alignment of 0 is followed by ',<left>,<top>'.
 * The left and top coordinates are relative to the upper left corner of the
 * button.
 */
void TPageManager::doJSI(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do JSI: Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int align = atoi(pars[1].c_str());
    int x = 0, y = 0;

    if (!align && pars.size() >= 3)
    {
        x = atoi(pars[2].c_str());

        if (pars.size() >= 4)
            y = atoi(pars[3].c_str());
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setIconJustification(align, x, y, -1);
            else
                bt->setIconJustification(align, x, y, btState-1);
        }
    }
}

void TPageManager::getJSI(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get JSI: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int j, x, y;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                j = bt->getIconJustification(&x, &y, i);
                sendCustomEvent(i + 1, j, 0, "", 1006, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            j = bt->getIconJustification(&x, &y, btState-1);
            sendCustomEvent(btState, j, 0, "", 1006, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

void TPageManager::doJST(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do JST: Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int align = atoi(pars[1].c_str());
    int x = 0, y = 0;

    if (!align && pars.size() >= 3)
    {
        x = atoi(pars[2].c_str());

        if (pars.size() >= 4)
            y = atoi(pars[3].c_str());
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setTextJustification(align, x, y, -1);
            else
                bt->setTextJustification(align, x, y, btState-1);
        }
    }
}

void TPageManager::getJST(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get JST: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int j, x, y;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                j = bt->getTextJustification(&x, &y, i);
                sendCustomEvent(i + 1, j, 0, "", 1004, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            j = bt->getTextJustification(&x, &y, btState-1);
            sendCustomEvent(btState, j, 0, "", 1004, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * @brief TPageManager::getMSP
 * Sets the speed of a marquee line. Allowed range is from 1 to 10, where 10 is
 * the fastest speed.
 *
 * @param port      The port number
 * @param channels  The channels
 * @param pars      Parameters
 */
void TPageManager::doMSP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getMSP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do MSP: Expecting at least 2 parameter but got less! Command ignored.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str()) - 1;
    int speed = atoi(pars[1].c_str());

    if (speed < 1 || speed > 10)
    {
        MSG_ERROR("Speed for marquee line is out of range!");
        return;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator iter;

        for (iter = buttons.begin(); iter != buttons.end(); ++iter)
        {
            Button::TButton *bt = buttons[0];
            bt->setMarqueeSpeed(speed, btState);
        }
    }
}

void TPageManager::doTEC(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTEC(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do TEC: Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setTextEffectColor(color);
            else
                bt->setTextEffectColor(color, btState-1);
        }
    }
}

void TPageManager::getTEC(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getTEC(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get TEC: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string c = bt->getTextEffectColor(i);
                sendCustomEvent(i + 1, (int)c.length(), 0, c, 1009, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            string c = bt->getTextEffectColor(btState-1);
            sendCustomEvent(btState, (int)c.length(), 0, c, 1009, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

void TPageManager::doTEF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTEF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command do TEF: Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string tef = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setTextEffectName(tef);
            else
                bt->setTextEffectName(tef, btState-1);
        }
    }
}

void TPageManager::getTEF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getTEF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get TEF: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string c = bt->getTextEffectName(i);
                sendCustomEvent(i + 1, (int)c.length(), 0, c, 1008, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            string c = bt->getTextEffectName(btState-1);
            sendCustomEvent(btState, (int)c.length(), 0, c, 1008, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Assign a text string to those buttons with a defined address range.
 * Sets Non-Unicode text.
 */
void TPageManager::doTXT(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTXT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do TXT: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str()) - 1;
    string text;

    // Every comma (,) in the text produces a new parameter. Therefor we must
    // concatenate this parameters together and insert the comma.
    if (pars.size() > 1)
    {
        for (size_t i = 1; i < pars.size(); ++i)
        {
            if (i > 1)
                text += ",";

            text += pars[i];
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            if (!bt)
                continue;

            bt->setText(text, btState);
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(bt->getText(btState < 0 ? 0 : btState));

            __success = true;
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::getTXT(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getTXT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get TXT: Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string c = bt->getText(i);
                sendCustomEvent(i + 1, (int)c.length(), 0, c, 1001, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
                if (_gTestMode)
                    _gTestMode->setResult(c);
#endif
            }
        }
        else
        {
            string c = bt->getText(btState-1);
            sendCustomEvent(btState, (int)c.length(), 0, c, 1001, bt->getChannelPort(), bt->getChannelNumber());
#if TESTMODE == 1
            if (_gTestMode)
                _gTestMode->setResult(c);
#endif
        }
    }
#if TESTMODE == 1
    setAllDone();
#endif
}

/*
 * Set button state legacy unicode text command.
 *
 * Set Unicode text in the legacy G4 format. For the ^UNI command, the Unicode
 * text is sent as ASCII-HEX nibbles.
 */
void TPageManager::doUNI(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doUNI(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command UNI: Expecting 1 parameters but got none! Ignoring command.");
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str()) - 1;
    string text;

    // Unicode is the stadard character set used by Windows internally. It
    // consists of 16 bit unsiged numbers. This can't be transported into a
    // standard character string because a NULL byte means end of string.
    // Therefor we must convert it to UFT-8.
    if (pars.size() > 1)
    {
        string byte;
        std::wstring uni;
        size_t pos = 0;

        while (pos < pars[1].length())
        {
            byte = pars[1].substr(pos, 4);
            wchar_t ch = (char)strtol(byte.c_str(), NULL, 16);
            uni += ch;
            pos += 4;
        }

        text = UnicodeToUTF8(uni);
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
    {
        PRINT_LAST_ERROR();
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

#if TESTMODE == 1
            bool res = bt->setText(text, btState);

            if (_gTestMode)
                _gTestMode->setResult(bt->getText(btState < 0 ? 0 : btState));

            __success = res;
#else
            bt->setText(text, btState);
#endif
        }
    }
#if TESTMODE == 1
    setDone();
#endif
}

void TPageManager::doUTF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTXT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command UTF: Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;

    if (pars.size() > 1)
    {
        for (size_t i = 1; i < pars.size(); ++i)
        {
            if (i > 1)
                text += ",";

            text += pars[i];
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setText(text, i);
            }
            else
                bt->setText(text, btState - 1);
        }
    }
}

/**
 * Simulates a touch/release/pulse at the given coordinate. If the push event
 * is less then 0 or grater than 2 the command is ignored. It is also ignored
 * if the x and y coordinate is out of range. The range must be between 0 and
 * the maximum with and height.
 */
void TPageManager::doVTP (int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doVTP (int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 3)
    {
        MSG_ERROR("Command VTP: Expected 3 parameters but got only " << pars.size() << " parameters!");
        return;
    }

    int pushType = atoi(pars[0].c_str());
    int x = atoi(pars[1].c_str());
    int y = atoi(pars[2].c_str());

    if (pushType < 0 || pushType > 2)
    {
        MSG_ERROR("Invalid push type " << pushType << ". Ignoring command!");
        return;
    }

    if (x < 0 || x > mTSettings->getWidth() || y < 0 || y > mTSettings->getHeight())
    {
        MSG_ERROR("Illegal coordinates " << x << " x " << y << ". Ignoring command!");
        return;
    }

    if (pushType == 0 || pushType == 2)
        mouseEvent(x, y, true);

    if (pushType == 1 || pushType == 2)
        mouseEvent(x, y, false);
}


/**
 * Set the keyboard passthru.
 */
void TPageManager::doKPS(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doKPS(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command KPS: Got no parameter. Ignoring command!");
        return;
    }

    int state = atoi(pars[0].c_str());

    if (state == 0)
        mPassThrough = false;
    else if (state == 5)
        mPassThrough = true;
}

void TPageManager::doVKS(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doVKS(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command VKS: Got no parameter. Ignoring command!");
        return;
    }

    if (_sendVirtualKeys)
        _sendVirtualKeys(pars[0]);
}

void TPageManager::doLPB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doLPB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
        return;

    TError::clear();
    string passwd = pars[0];
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setUserName(passwd);
        }
    }
}

void TPageManager::doLPC(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doLPC(int, vector<int>&, vector<string>&)");

    TConfig::clearUserPasswords();
}

void TPageManager::doLPR(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doLPR(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
        return;

    string user = pars[0];
    TConfig::clearUserPassword(user);
}

void TPageManager::doLPS(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doLPS(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 2)
        return;

    string user = pars[0];
    string password;

    // In case the password contains one or more comma (,), the password is
    // splitted. The following loop concatenates the password into one. Because
    // the comma is lost, we must add it again.
    for (size_t i = 0; i < pars.size(); ++i)
    {
        if (i > 0)
            password += ",";

        password += pars[i];
    }

    TConfig::setUserPassword(user, password);
}

/*
 * Set the page flip password. @PWD sets the level 1 password only.
 */
void TPageManager::doAPWD(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPWD(int port, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command APWD: Got less then 1 parameter!");
        return;
    }

    string password;
    // In case the password contains one or more comma (,), the password is
    // splitted. The following loop concatenates the password into one. Because
    // the comma is lost, we must add it again.
    for (size_t i = 0; i < pars.size(); ++i)
    {
        if (i > 0)
            password += ",";

        password += pars[i];
    }

    TConfig::savePassword1(password);
}

/*
 * Set the page flip password. Password level is required and must be 1 - 4
 */
void TPageManager::doPWD(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPWD(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command PWD: Got less then 2 parameters!");
        return;
    }

    int pwIdx = atoi(pars[0].c_str());
    string password;
    // In case the password contains one or more comma (,), the password is
    // splitted. The following loop concatenates the password into one. Because
    // the comma is lost, we must add it again.
    for (size_t i = 1; i < pars.size(); ++i)
    {
        if (i > 1)
            password += ",";

        password += pars[i];
    }

    switch(pwIdx)
    {
        case 1: TConfig::savePassword1(password); break;
        case 2: TConfig::savePassword2(password); break;
        case 3: TConfig::savePassword3(password); break;
        case 4: TConfig::savePassword4(password); break;
    }
}

/*
 * Set the bitmap of a button to use a particular resource.
 * Syntax:
 *    "'^BBR-<vt addr range>,<button states range>,<resource name>'"
 * Variable:
 *    variable text address range = 1 - 4000.
 *    button states range = 1 - 256 for multi-state buttons (0 = All states, for General buttons 1 = Off state and 2 = On state).
 *    resource name = 1 - 50 ASCII characters.
 * Example:
 *    SEND_COMMAND Panel,"'^BBR-700,1,Sports_Image'"
 *    Sets the resource name of the button to ’Sports_Image’.
 */
void TPageManager::doBBR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBBR(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command BBR: Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string resName = pars[1];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
//            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting BBR on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setResourceName(resName, i);
            }
            else
                bt->setResourceName(resName, btState - 1);

            if (bt->isVisible())
                bt->refresh();
            else if (_setVisible)
                _setVisible(bt->getHandle(), false);
        }
    }
}

/*
 * Add new resources
 * Adds any and all resource parameters by sending embedded codes and data.
 * Since the embedded codes are preceded by a '%' character, any '%' character
 * contained in* the URL must be escaped with a second '%' character (see
 * example).
 * The file name field (indicated by a %F embedded code) may contain special
 * escape sequences as shown in the ^RAF, ^RMF.
 * Syntax:
 *    "'^RAF-<resource name>,<data>'"
 * Variables:
 *    resource name = 1 - 50 ASCII characters.
 *    data = Refers to the embedded codes, see the ^RAF, ^RMF.
 * Example:
 *    SEND_COMMAND Panel,"'^RAF-New Image,%P0%HAMX.COM%ALab/Test%%5Ffile%Ftest.jpg'"
 *    Adds a new resource.
 *    The resource name is ’New Image’
 *    %P (protocol) is an HTTP
 *    %H (host name) is AMX.COM
 *    %A (file path) is Lab/Test_f ile
 *    %F (file name) is test.jpg.
 *    Note that the %%5F in the file path is actually encoded as %5F.
 */
void TPageManager::doRAF(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRAF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command RAF: Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    string name = pars[0];
    string data = pars[1];

    vector<string> parts = StrSplit(data, "%");
    RESOURCE_T res;

    if (parts.size() > 0)
    {
        vector<string>::iterator sIter;

        for (sIter = parts.begin(); sIter != parts.end(); sIter++)
        {
            const char *s = sIter->c_str();
            string ss = *sIter;
            MSG_DEBUG("Parsing \"" << ss << "\" with token << " << ss[0]);

            switch(*s)
            {
                case 'P':
                    if (*(s+1) == '0')
                        res.protocol = "HTTP";
                    else
                        res.protocol = "FTP";
                    break;

                case 'U': res.user = sIter->substr(1); break;
                case 'S': res.password = sIter->substr(1); break;
                case 'H': res.host = sIter->substr(1); break;
                case 'F': res.file = sIter->substr(1); break;
                case 'A': res.path = sIter->substr(1); break;
                case 'R': res.refresh = atoi(sIter->substr(1).c_str()); break;

                default:
                    MSG_WARNING("Option " << sIter->at(0) << " is currently not implemented!");
            }
        }

        if (gPrjResources)
            gPrjResources->addResource(name, res.protocol, res.host, res.path, res.file, res.user, res.password, res.refresh);
    }
}

void TPageManager::doRFR(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRFR(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command RFR: Expecting 1 parameter but got none! Ignoring command.");
        return;
    }

    string name = pars[0];
    vector<TMap::MAP_T> map = findButtonByName(name);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (bt->isVisible())
            {
//                setButtonCallbacks(bt);
                bt->refresh();
            }
        }
    }
}

/*
 * Modify an existing resource
 *
 * Modifies any and all resource parameters by sending embedded codes and data.
 * Since the embedded codes are preceded by a '%' character, any '%' character
 * contained in the URL must be escaped with a second '%' character (see
 * example).
 * The file name field (indicated by a %F embedded code) may contain special
 * escape sequences as shown in the ^RAF.
 *
 * Syntax:
 * "'^RMF-<resource name>,<data>'"
 * Variables:
 *   • resource name = 1 - 50 ASCII characters
 *   • data = Refers to the embedded codes, see the ^RAF, ^RMF.
 * Example:
 *   SEND_COMMAND Panel,"'^RMF-Sports_Image,%ALab%%5FTest/Images%Ftest.jpg'"
 * Changes the resource ’Sports_Image’ file name to ’test.jpg’ and the path to
 * ’Lab_Test/Images’.
 * Note that the %%5F in the file path is actually encoded as %5F.
 */
void TPageManager::doRMF(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRMF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command RMF: Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    string name = pars[0];
    string data = pars[1];

    vector<string> parts = StrSplit(data, "%");
    RESOURCE_T res;

    if (parts.size() > 0)
    {
        vector<string>::iterator sIter;

        for (sIter = parts.begin(); sIter != parts.end(); sIter++)
        {
            const char *s = sIter->c_str();
            string ss = *sIter;
            MSG_DEBUG("Parsing \"" << ss << "\" with token << " << ss[0]);

            switch(*s)
            {
                case 'P':
                    if (*(s+1) == '0')
                        res.protocol = "HTTP";
                    else
                        res.protocol = "FTP";
                break;

                case 'U': res.user = sIter->substr(1); break;
                case 'S': res.password = sIter->substr(1); break;
                case 'H': res.host = sIter->substr(1); break;
                case 'F': res.file = sIter->substr(1); break;
                case 'A': res.path = sIter->substr(1); break;
                case 'R': res.refresh = atoi(sIter->substr(1).c_str()); break;

                default:
                    MSG_WARNING("Option " << sIter->at(0) << " is currently not implemented!");
            }
        }

        if (gPrjResources)
            gPrjResources->setResource(name, res.protocol, res.host, res.path, res.file, res.user, res.password, res.refresh);
    }
}

/**
 * Change the refresh rate for a given resource.
 */
void TPageManager::doRSR(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRSR(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Command RSR: Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    string resName = pars[0];
    int resRefresh = atoi(pars[1].c_str());

    if (!gPrjResources)
    {
        MSG_ERROR("Missing the resource module. Ignoring command!");
        return;
    }

    RESOURCE_T res = gPrjResources->findResource(resName);

    if (res.name.empty() || res.refresh == resRefresh)
        return;

    gPrjResources->setResource(resName, res.protocol, res.host, res.path, res.file, res.user, res.password, resRefresh);
}

/**
 * @brief TPageManager::doAKB - Pop up the keyboard icon
 * Pop up the keyboard icon and initialize the text string to that specified.
 * Keyboard string is set to null on power up and is stored until power is lost.
 * The Prompt Text is optional.
 */
void TPageManager::doAKB(int, vector<int>&, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doAKB(int, vector<int>&, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command AKB: Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (initText.empty())
        initText = mAkbText;
    else
        mAkbText = initText;

    if (_callKeyboard)
        _callKeyboard(initText, promptText, false);
}

/**
 * Pop up the keyboard icon and initialize the text string to that
 * specified.
 */
void TPageManager::doAKEYB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doAKEYB(int port, vector<int>& channels, vector<string>& pars)");

    doAKB(port, channels, pars);
}

void TPageManager::doAKEYP(int port, std::vector<int>& channels, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doAKEYP(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    doAKP(port, channels, pars);
}

/**
 * Remove keyboard or keypad that was displayed using 'AKEYB', 'AKEYP', 'PKEYP',
 * @AKB, @AKP, @PKP, @EKP, or @TKP commands.
 */
void TPageManager::doAKEYR(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doAKEYR(int, vector<int>&, vector<string>&)");

    if (_callResetKeyboard)
        _callResetKeyboard();
}

/**
 * @brief TPageManager::doAKP - Pop up the keypad icon
 * Pop up the keypad icon and initialize the text string to that specified.
 * Keypad string is set to null on power up and is stored until power is lost.
 * The Prompt Text is optional.
 */
void TPageManager::doAKP(int, std::vector<int>&, std::vector<std::string> &pars)
{
    DECL_TRACER("TPageManager::doAKP(int, vector<int>&, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command AKP: Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (initText.empty())
        initText = mAkpText;
    else
        mAkpText = initText;

    if (_callKeypad)
        _callKeypad(initText, promptText, false);
}

/**
 * Remove keyboard or keypad that was displayed using 'AKEYB', 'AKEYP', 'PKEYP',
 * @AKB, @AKP, @PKP, @EKP, or @TKP commands.
 */
void TPageManager::doAKR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doAKR(int, vector<int>&, vector<string>&)");

    doAKEYR(port, channels, pars);
}

void TPageManager::doABEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
    {
#if TESTMODE == 1
        setAllDone();
#endif
        return;
    }

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getSingleBeepSound();
    TValidateFile vf;

    if (vf.isValidFile(snd))
        _playSound(snd);
#if TESTMODE == 1
    else
    {
        MSG_PROTOCOL("Sound file invalid!");
        setAllDone();
    }
#endif
}

void TPageManager::doADBEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doDBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
        return;

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getDoubleBeepSound();
    TValidateFile vf;

    if (vf.isValidFile(snd))
        _playSound(snd);
#if TESTMODE == 1
    else
    {
        MSG_PROTOCOL("Sound file invalid!");
        setAllDone();
    }
#endif
}

void TPageManager::doBEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
    {
#if TESTMODE == 1
        MSG_PROTOCOL("Method \"playSound()\" not initialized!");
        setAllDone();
#endif
        return;
    }

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getSingleBeepSound();
    TValidateFile vf;
    TSystemSound sysSound(TConfig::getSystemPath(TConfig::SOUNDS));

    if (sysSound.getSystemSoundState() && vf.isValidFile(snd))
        _playSound(snd);
#if TESTMODE == 1
    else
    {
        if (!sysSound.getSystemSoundState())
        {
            MSG_PROTOCOL("Sound state disabled!")
        }
        else
        {
            MSG_PROTOCOL("Sound file invalid!");
        }

        setAllDone();
    }
#endif
}

void TPageManager::doDBEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doDBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
        return;

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getDoubleBeepSound();
    TValidateFile vf;
    TSystemSound sysSound(TConfig::getSystemPath(TConfig::SOUNDS));

    if (sysSound.getSystemSoundState() && vf.isValidFile(snd))
        _playSound(snd);
#if TESTMODE == 1
    else
    {
        if (!sysSound.getSystemSoundState())
        {
            MSG_PROTOCOL("Sound state disabled!")
        }
        else
        {
            MSG_PROTOCOL("Sound file invalid!");
        }

        setAllDone();
    }
#endif
}

/**
 * @brief Pop up the keypad icon and initialize the text string to that specified.
 * Keypad string is set to null on power up and is stored until power is lost.
 * The Prompt Text is optional.
 */
void TPageManager::doEKP(int port, std::vector<int>& channels, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doEKP(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    doAKP(port, channels, pars);
}

/**
 * @brief Present a private keyboard.
 * Pops up the keyboard icon and initializes the text string to that specified.
 * Keyboard displays a '*' instead of the letters typed. The Prompt Text is optional.
 */
void TPageManager::doPKB(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPKB(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command PKB: Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (_callKeyboard)
        _callKeyboard(initText, promptText, true);
}

/**
 * @brief Present a private keypad.
 * Pops up the keypad icon and initializes the text string to that specified.
 * Keypad displays a '*' instead of the numbers typed. The Prompt Text is optional.
 */
void TPageManager::doPKP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPKP(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command PKP: Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (_callKeypad)
        _callKeypad(initText, promptText, true);
}

/**
 * @brief Reset protected password command
 * This command is used to reset the protected setup password to the factory
 * default value.
 */
void TPageManager::doRPP(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doRPP(int, vector<int>&, vector<string>&)");

    TConfig::savePassword1("1988");
}

/**
 * Send panel to SETUP page.
 */
void TPageManager::doSetup(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doSetup(int, vector<int>&, vector<string>&)");

    if (_callShowSetup)
        _callShowSetup();
}

/**
 * Shut down the App
 */
void TPageManager::doShutdown(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doShutdown(int, vector<int>&, vector<string>&)");

    MSG_PROTOCOL("Received shutdown ...");
#ifdef __ANDROID__
    stopNetworkState();
#endif
    prg_stopped = true;
    killed = true;

    if (_shutdown)
        _shutdown();
}

void TPageManager::doSOU(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSOU(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("@SOU: Expecting a sound file as parameter! Ignoring command.");
        return;
    }

    if (!_playSound)
    {
        MSG_ERROR("@SOU: Missing sound module!");
        return;
    }

    if (pars[0].empty() || strCaseCompare(pars[0], "None") == 0)
        return;

    _playSound(pars[0]);
}

void TPageManager::doMUT(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doMUT(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("^MUT: Expecting a state parameter! Ignoring command.");
        return;
    }

    bool mute = 0;

    if (pars[0] == "0")
        mute = false;
    else
        mute = true;

    TConfig::setMuteState(mute);
#if TESTMODE == 1
    if (_gTestMode)
    {
        bool st = TConfig::getMuteState();
        _gTestMode->setResult(st ? "1" : "0");
    }

    __success = true;
    setAllDone();
#endif
}

/**
 * @brief Present a telephone keypad.
 * Pops up the keypad icon and initializes the text string to that specified.
 * The Prompt Text is optional.
 */
void TPageManager::doTKP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTKP(int port, vector<int>& channels, vector<string>& pars)");

    // TODO: Implement a real telefone keypad.
    doAKP(port, channels, pars);
}

/**
 * Popup the virtual keyboard
 */
void TPageManager::doVKB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doVKB(int port, vector<int>& channels, vector<string>& pars)");

    doAKP(port, channels, pars);
}

/**
 * Panel model name. If the panel supports intercom hardware it will respond
 * with its model name. Older hardware or newer hardware that has intercom
 * support disabled with not respond to this command.
 */
void TPageManager::getMODEL(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::getMODEL(int, vector<int>&, vector<string>&)");

    amx::ANET_SEND scmd;
    scmd.port = mTSettings->getSettings().voipCommandPort;
    scmd.channel = TConfig::getChannel();
#ifdef Q_OS_IOS
    scmd.msg = "^MODEL-iPhonei";
#elif defined(Q_OS_ANDROID)
    scmd.msg = "^MODEL-Androidi";
#else
    scmd.msg = TConfig::getPanelType();
#endif
    scmd.MC = 0x008c;
    MSG_DEBUG("Sending model: " << scmd.msg);

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send model type!");
}

/**
 * @brief Intercom start.
 * Starts a call to the specified IP address and ports, where initial mode is
 * either 1 (talk) or 0 (listen) or 2 (both). If no mode is specified
 * 0 (listen) is assumed. Please note, however, that no data packets will
 * actually flow until the intercom modify command is sent to the panel.
 */
void TPageManager::doICS(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doICS(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 3)
    {
        MSG_ERROR("Command ICS expects 3 parameters but got only " << pars.size());
        return;
    }

    INTERCOM_t ic;
    ic.ip = pars[0];
    ic.txPort = atoi(pars[1].c_str());
    ic.rxPort = atoi(pars[2].c_str());
    ic.mode = 0;

    if (pars.size() >= 4)
        ic.mode = atoi(pars[3].c_str());

    if (getInitializeIntercom())
        getInitializeIntercom()(ic);
}

/**
 * @brief Intercom end.
 * This terminates an intercom call/connection.
 */
void TPageManager::doICE(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doICE(int, vector<int>&, vector<string>&)");

    if (_intercomStop)
        _intercomStop();
}

/**
 * Intercom modify command.
 */
void TPageManager::doICM(int, vector<int>&, vector<string>& pars)
{
    if (pars.empty() || pars[0] == "TALK" || pars[0] == "LISTEN")
    {
        if (_intercomStart)
            _intercomStart();
    }
    else if (pars[0] == "MICLEVEL" && pars.size() >= 2)
    {
        int micLevel = atoi(pars[1].c_str());

        if (micLevel < 0 || micLevel > 100)
        {
            MSG_WARNING("Microphon level is out of range [0 ... 100]: " << micLevel);
            return;
        }

        TConfig::saveSystemGain(micLevel);

        if (_intercomMicLevel)
            _intercomMicLevel(micLevel);
    }
    else if (pars[0] == "MUTEMIC" && pars.size() >= 2)
    {
        int mute = atoi(pars[1].c_str());
        bool bmute = mute == 0 ? false : true;

        if (_intercomMute)
            _intercomMute(bmute);
    }
    else if (pars[0] == "SPEAKERLEVEL" && pars.size() >= 2)
    {
        int speakerLevel = atoi(pars[1].c_str());

        if (speakerLevel < 0 || speakerLevel > 100)
        {
            MSG_WARNING("Speaker level is out of range [0 ... 100]: " << speakerLevel);
            return;
        }

        TConfig::saveSystemVolume(speakerLevel);

        if (_intercomSpkLevel)
            _intercomSpkLevel(speakerLevel);
    }
}

#ifndef _NOSIP_
void TPageManager::sendPHN(vector<string>& cmds)
{
    DECL_TRACER("TPageManager::sendPHN(const vector<string>& cmds)");

    vector<int> channels;
    doPHN(-1, channels, cmds);
}

void TPageManager::actPHN(vector<string>& cmds)
{
    DECL_TRACER("TPageManager::actPHN(const vector<string>& cmds)");

    vector<int> channels;
    doPHN(1, channels, cmds);
}

void TPageManager::phonePickup(int id)
{
    DECL_TRACER("TPageManager::phonePickup(int id)");

    if (id < 0 || id >= 4)
        return;

    if (mSIPClient)
        mSIPClient->pickup(id);
}

void TPageManager::phoneHangup(int id)
{
    DECL_TRACER("TPageManager::phoneHangup(int id)");

    if (id < 0 || id >= 4)
        return;

    if (mSIPClient)
        mSIPClient->terminate(id);
}

/**
 * @brief Phone commands.
 * The phone commands could come from the master or are send to the master.
 * If the parameter \p port is less then 0 (zero) a command is send to the
 * master. In any other case the command came from the mater.
 *
 * @param port  This is used to signal if the command was sent by the master
 *              or generated from the panel. If ths is less then 0, then the
 *              method was called because of an event happen in the panel.
 *              If this is grater or equal 0, then the event is comming from
 *              the master.
 * @param pars  This are parameters. The first parameter defines the action
 *              to be done. According to the command this parameter may have a
 *              different number of arguments.
 */
void TPageManager::doPHN(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHN(int port, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command do PHN: Expecting at least 1 parameter but got none! Ignoring command.");
        return;
    }

    string sCommand;
    string cmd = toUpper(pars[0]);

    // Master to panel
    if (port >= 0)
    {
        if (!mSIPClient)
        {
            MSG_ERROR("SIP client class was not initialized!")
            return;
        }

        if (cmd == "ANSWER")
        {
            if (pars.size() >= 2)
            {
                int id = atoi(pars[1].c_str());

                if (mSIPClient->getSIPState(id) == TSIPClient::SIP_HOLD)
                    mSIPClient->resume(id);
                else
                    mSIPClient->pickup(id);
            }
        }
        else if (cmd == "AUTOANSWER")
        {
            if (pars.size() >= 2)
            {
                if (pars[1].at(0) == '0')
                    mPHNautoanswer = false;
                else
                    mPHNautoanswer = true;

                vector<string> cmds;
                cmds = { "AUTOANSWER", to_string(mPHNautoanswer ? 1 : 0) };
                sendPHN(cmds);
            }
        }
        else if (cmd == "CALL")     // Initiate a call
        {
            if (pars.size() >= 2)
                mSIPClient->call(pars[1]);
        }
        else if (cmd == "DTMF")     // Send tone modified codes
        {
            if (pars.size() >= 2)
                mSIPClient->sendDTMF(pars[1]);
        }
        else if (cmd == "HANGUP")   // terminate a call
        {
            if (pars.size() >= 2)
            {
                int id = atoi(pars[1].c_str());
                mSIPClient->terminate(id);
            }
        }
        else if (cmd == "HOLD")     // Hold the line
        {
            if (pars.size() >= 2)
            {
                int id = atoi(pars[1].c_str());
                mSIPClient->hold(id);
            }
        }
        else if (cmd == "LINESTATE") // State of all line
        {
            mSIPClient->sendLinestate();
        }
        else if (cmd == "PRIVACY")  // Set/unset "do not disturb"
        {
            if (pars.size() >= 2)
            {
                bool state = (pars[1].at(0) == '1' ? true : false);
                mSIPClient->sendPrivate(state);
            }
        }
        else if (cmd == "REDIAL")   // Redials the last number
        {
            mSIPClient->redial();
        }
        else if (cmd == "TRANSFER") // Transfer call to provided number
        {
            if (pars.size() >= 3)
            {
                int id = atoi(pars[1].c_str());
                string num = pars[2];

                if (mSIPClient->transfer(id, num))
                {
                    vector<string> cmds;
                    cmds.push_back("TRANSFERRED");
                    sendPHN(cmds);
                }
            }
        }
        else if (cmd == "IM")
        {
            if (pars.size() < 3)
                return;

            string to = pars[1];
            string msg = pars[2];
            string toUri;

            if (to.find("sip:") == string::npos)
                toUri = "sip:";

            toUri += to;

            if (to.find("@") == string::npos)
                toUri += "@" + TConfig::getSIPproxy();

            mSIPClient->sendIM(toUri, msg);
        }
        else if (cmd == "SETUP")    // Some temporary settings
        {
            if (pars.size() < 2)
                return;

            if (pars[1] == "DOMAIN" && pars.size() >= 3)
                TConfig::setSIPdomain(pars[2]);
            else if (pars[1] == "DTMFDURATION")
            {
                unsigned int ms = atoi(pars[2].c_str());
                mSIPClient->setDTMFduration(ms);
            }
            else if (pars[1] == "ENABLE")   // (re)register user
            {
                TConfig::setSIPstatus(true);
                mSIPClient->cleanUp();
                mSIPClient->init();
            }
            else if (pars[1] == "DOMAIN" && pars.size() >= 3)
                TConfig::setSIPdomain(pars[2]);
            else if (pars[1] == "PASSWORD" && pars.size() >= 3)
                TConfig::setSIPpassword(pars[2]);
            else if (pars[1] == "PORT" && pars.size() != 3)
                TConfig::setSIPport(atoi(pars[2].c_str()));
            else if (pars[1] == "PROXYADDR" && pars.size() >= 3)
                TConfig::setSIPproxy(pars[2]);
            else if (pars[1] == "STUNADDR" && pars.size() >= 3)
                TConfig::setSIPstun(pars[2]);
            else if (pars[1] == "USERNAME" && pars.size() >= 3)
                TConfig::setSIPuser(pars[2]);
        }
        else
        {
            MSG_ERROR("Unknown command ^PHN-" << cmd << " ignored!");
        }
    }
    else   // Panel to master
    {
        vector<string>::iterator iter;

        for (iter = pars.begin(); iter != pars.end(); ++iter)
        {
            if (!sCommand.empty())
                sCommand += ",";

            sCommand += *iter;
        }

        sendPHNcommand(sCommand);
    }
}

void TPageManager::getPHN(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getPHN(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command get PHN: Invalid number of arguments!");
        return;
    }

    string cmd = pars[0];

    if (cmd == "AUTOANSWER")
        sendPHNcommand(cmd + "," + (mPHNautoanswer ? "1" : "0"));
    else if (cmd == "LINESTATE")
    {
        if (!mSIPClient)
            return;

        mSIPClient->sendLinestate();
    }
    else if (cmd == "MSGWAITING")
    {
        size_t num = mSIPClient->getNumberMessages();
        sendPHNcommand(cmd + "," + (num > 0 ? "1" : "0") + "," + std::to_string(num) + "0,0,0");
    }
    else if (cmd == "PRIVACY")
    {
        if (mSIPClient->getPrivate())
            sendPHNcommand(cmd + ",1");
        else
            sendPHNcommand(cmd + ",0");
    }
    else if (cmd == "REDIAL")
    {
        if (pars.size() < 2)
            return;

        sendPHNcommand(cmd + "," + pars[1]);
    }
    else
    {
        MSG_WARNING("Unknown command " << cmd << " found!");
    }
}
#endif  // _NOSIP_

/**
 * @brief Subpage custom event command
 * Configure subpage custom events. This command can be used to enable or
 * disable the transmission of custom events to the master whenever certain
 * operations occur. For example, the system programmer may want to be notified
 * whenever a subpage enters the anchor position. The notification mechanism is
 * a custom event. The ^SCE command takes the form of a addr range specifying
 * one or more subpage viewer buttons followed by a comma separated list of
 * custom event numbers. If the number is 0 or blank for a given event type
 * then no custom event will be transmitted when that event occurs. If a number
 * is specified, then it is used as the EVENTID value for the custom event.
 * The range of 32001 to 65535 has been reserved in the panel for user custom
 * event numbers. A different value could be used but might collide with other
 * AMX event numbers. Event configuration is not permanent and all event
 * numbers revert to the default of 0 when the panel restarts.
 *
 * SYNTAX:
 *    "'^SCE-<addr range>,<optional anchor event num>,<optional onscreen event num>,
 *    <optional offscreen event num>,<optional reorder event num>'"
 * VARIABLES:
 *    address range: Address codes of buttons to affect. A '.' between addresses
 *       includes the range, and & between addresses includes each address.
 *    anchor event number: 0 for no event or a value from 32001 to 65535.
 *    onscreen event number: 0 for no event or a value from 32001 to 65535.
 *    offscreen event number: 0 for no event or a value from 32001 to 65535.
 *    reorder event number: 0 for no event or a value from 32001 to 65535.
 *
 * The events are:
 * • anchor - a new subpage has docked in the anchor position.
 * • onscreen - a docking operation has been completed and the subpages in the
 *   list are now onscreen. This list will include the anchor along with any
 *   subpages that may be partially onscreen.
 * • offscreen - a docking operation has been completed and the subpages in the
 *   list are now offscreen.
 * • reorder - the user has reordered the subpages in the set and the list
 *   contains all subpages in the new order without regard to onscreen or
 *   offscreen state.
 *   In response to any or all of the above events, the panel will create a
 *   string which is a list of subpage names separated by a pipe (|) character.
 *   The string for the anchor event is a single subpage name. If this string is
 *   too long to be transmitted in a single custom event, then multiple custom
 *   events will be created and transmitted. If defined, the events are sent in
 *   this order when a docking operation completes on a given viewer button:
 *   anchor, onscreen, offscreen. If reorder is defined and occurs, it is sent
 *   first: reorder, anchor, onscreen, offscreen. The format of the custom event
 *   transmitted to the master is as follows:
 *   Custom Event   Property Value
 *     Port         port command was received on
 *     ID           address of the button generating the event
 *     Type         the non-zero event number in the ^SCE command
 *     Flag         0
 *     Value 1      which one of possible multiple events this is (1 based)
 *     Value 2      total number of events needed to send the entire string
 *     Value 3      the total size of the original string in bytes
 *     Text         pipe character separated list of subpage names
 */
void TPageManager::doSCE(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSCE(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
        return;

    int anchorNum = 0;
    int onscreenNum = 0;
    int offscreenNum = 0;
    int reorderNum = 0;

    for (size_t i = 0; i < pars.size(); ++i)
    {
        switch (i)
        {
            case 0: anchorNum = atoi(pars[i].c_str()); break;
            case 1: onscreenNum = atoi(pars[i].c_str()); break;
            case 2: offscreenNum = atoi(pars[i].c_str()); break;
            case 3: reorderNum = atoi(pars[i].c_str()); break;
            default:
                MSG_WARNING("Unknown parameter " << pars[i] << " is ignored!");
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError())
    {
        PRINT_LAST_ERROR();
        return;
    }

    if (map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (!buttons.empty())
    {
        ::map<ulong, string> spages;
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            // Find the button in the list before adding it
            vector<SCE_EVENT_t>::iterator iter;
            bool found = false;
            TSubPage *sp = getSubPage(bt->getParent());

            if (!mSceEvents.empty())
            {
                for (iter = mSceEvents.begin(); iter != mSceEvents.end(); ++iter)
                {
                    if (iter->handle == bt->getHandle())
                    {
                        iter->anchor = anchorNum;
                        iter->onscreen = onscreenNum;
                        iter->offscreen = offscreenNum;
                        iter->reorder = reorderNum;

                        if (sp)
                            spages.insert(pair<ulong, string>(bt->getParent(), sp->getName()));

                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                SCE_EVENT_t sce;
                sce.anchor = anchorNum;
                sce.onscreen = onscreenNum;
                sce.offscreen = offscreenNum;
                sce.reorder = reorderNum;
                sce.port = bt->getChannelPort();
                sce.channel = bt->getChannelNumber();
                sce.handle = bt->getHandle();
                mSceEvents.push_back(sce);

                if (sp)
                    spages.insert(pair<ulong, string>(bt->getParent(), sp->getName()));
            }
        }

        // Create a list of pages separated by a |
        ::map<ulong, string>::iterator pgIter;
        string pages;

        for (pgIter = spages.begin(); pgIter != spages.end(); ++pgIter)
        {
            if (!pages.empty())
                pages.append("|");

            pages += pgIter->second;
        }

        // Apply the the page list to each event entry
        vector<SCE_EVENT_t>::iterator evIter;

        for (evIter = mSceEvents.begin(); evIter != mSceEvents.end(); ++evIter)
            evIter->pages = pages;
    }
}

/*
 *  Hide all subpages in a subpage viewer button.
 */
void TPageManager::doSHA(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doSHA(int port, vector<int> &channels, vector<string> &pars)");

    Q_UNUSED(pars);
    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (!buttons.empty())
    {
        vector<Button::TButton *>::iterator mapIter;
        int evCount = 0;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (_hideAllSubViewItems)
                _hideAllSubViewItems(bt->getHandle());

            // Send a custom event in case there is one defined.
            vector<SCE_EVENT_t>::iterator evIter;

            for (evIter = mSceEvents.begin(); evIter != mSceEvents.end(); ++evIter)
            {
                if (evIter->offscreen && evIter->handle == bt->getHandle())
                {
                    evCount++;
                    sendCustomEvent(evCount, 1, 1, evIter->pages, evIter->offscreen, bt->getChannelPort(), bt->getChannelNumber());
                }
            }
        }
    }
}

/**
 * @brief TPageManager::doSHD - Subpage Hide Command
 * This command will hide the named subpage and relocate the surrounding
 * subpages as necessary to close the gap. If the subpage to be hidden is
 * currently offscreen then it is removed without any other motion on the
 * subpage viewer button.
 * Syntax:
 *      "'^SHD-<addr range>,<name>,<optional time>'"
 * Variables:
 *      address range: Address codes of buttons to affect. A '.' between
 *          addresses includes the range, and & between addresses includes each
 *          address.
 *      name: name of subpage to hide. If name is __all, then all subpages are
 *          hidden.
 *      time: Can range from 0 to 30 and represents tenths of a second. This is
 *          the amount of time used to move the subpages around when subpages
 *          are hidden from a button.
 * Example:
 *      SEND_COMMAND Panel,"'^SHD-200,menu1,10'"
 *  Remove the menu1 subpage from subpage viewer button with address 200 over
 *  one second.
 */
void TPageManager::doSHD(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSHD(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
        return;

    string name = pars[0];

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (!buttons.empty())
    {
        int evCount = 0;
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            vector<TSubPage *> subviews = createSubViewList(bt->getSubViewID());

            if (subviews.empty())
                continue;

            vector<TSubPage *>::iterator itSub;

            for (itSub = subviews.begin(); itSub != subviews.end(); ++itSub)
            {
                TSubPage *sub = *itSub;

                if (sub && sub->getName() == name)
                {
                    if (_hideSubViewItem)
                        _hideSubViewItem(sub->getHandle(), bt->getHandle());

                    // Send a custom event in case there is one defined.
                    vector<SCE_EVENT_t>::iterator evIter;

                    for (evIter = mSceEvents.begin(); evIter != mSceEvents.end(); ++evIter)
                    {
                        if (evIter->offscreen && evIter->handle == bt->getHandle())
                        {
                            evCount++;
                            sendCustomEvent(evCount, 1, 1, evIter->pages, evIter->offscreen, bt->getChannelPort(), bt->getChannelNumber());
                        }
                    }

                    break;
                }
            }
        }
    }
}

/**
 * Show or hide a button with a set variable text range.
 */
void TPageManager::doSHO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSHO(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_ERROR("Command SHO: Expecting 1 parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int cvalue = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            int pgID = (bt->getParent() >> 16) & 0x0000ffff;
            bool pVisible = false;

            if (pgID < 500)
            {
                TPage *pg = getPage(pgID);

                if (pg && pg->isVisilble())
                    pVisible = true;
            }
            else
            {
                TSubPage *pg = getSubPage(pgID);

                if (pg && pg->isVisible())
                    pVisible = true;
            }

            bool oldV = bt->isVisible();
            bool visible = cvalue ? true : false;
            MSG_DEBUG("Button " << bt->getButtonIndex() << ", \"" << bt->getButtonName() << "\" set " << (visible ? "VISIBLE" : "HIDDEN") << " (Previous: " << (oldV ? "VISIBLE" : "HIDDEN") << ")");

            if (visible != oldV)
            {
                bt->setVisible(visible);

                if (pVisible)
                {
                    setButtonCallbacks(bt);

                    if (_setVisible)
                        _setVisible(bt->getHandle(), visible);
                    else
                        bt->refresh();
                }
            }
        }
    }
}

void TPageManager::doSPD(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSPD(int port, vector<int>& channel, vector<string>& pars)");

    if (pars.size() < 1)
        return;

    TError::clear();
    int padding = atoi(pars[0].c_str());

    if (padding < 0 || padding > 100)
        return;

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (!buttons.empty())
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (_setSubViewPadding)
                _setSubViewPadding(bt->getHandle(), padding);
        }
    }
}

/*
 * This command will perform one of three different operations based on the following conditions:
 * 1. If the named subpage is hidden in the set associated with the viewer button it will be shown in the anchor position.
 * 2. If the named subpage is not present in the set it will be added to the set and shown in the anchor position.
 * 3. If the named subpage is already present in the set and is not hidden then the viewer button will move it to the anchor
 * position. The anchor position is the location on the subpage viewer button specified by its weighting. This will either be
 * left, center or right for horizontal subpage viewer buttons or top, center or bottom for vertical subpage viewer buttons.
 * Surrounding subpages are relocated on the viewer button as needed to accommodate the described operations
 */
void TPageManager::doSSH(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doSSH(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command SSH: Expecting 1 parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    string name = pars[0];
    int position = 0;   // optional
    int time = 0;       // optional

    if (pars.size() > 1)
        position = atoi(pars[1].c_str());

    if (pars.size() > 2)
        time = atoi(pars[2].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (!buttons.empty())
    {
        int evCount = 0;
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            vector<TSubPage *> subviews = createSubViewList(bt->getSubViewID());

            if (subviews.empty() || !bt)
                continue;

            vector<TSubPage *>::iterator itSub;

            for (itSub = subviews.begin(); itSub != subviews.end(); ++itSub)
            {
                TSubPage *sub = *itSub;

                if (sub && sub->getName() == name)
                {
                    if (_showSubViewItem)
                        _showSubViewItem(sub->getHandle(), bt->getHandle(), position, time);

                    // Send a custom event in case there is one defined.
                    vector<SCE_EVENT_t>::iterator evIter;

                    for (evIter = mSceEvents.begin(); evIter != mSceEvents.end(); ++evIter)
                    {
                        if (evIter->anchor && evIter->handle == bt->getHandle())
                        {
                            evCount++;
                            sendCustomEvent(evCount, 1, 1, evIter->pages, evIter->anchor, bt->getChannelPort(), bt->getChannelNumber());
                        }
                    }
                    break;
                }
            }
        }
    }
}

void TPageManager::doSTG(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSTG(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_ERROR("Command STG: Expecting 1 parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    string name = pars[0];
    int position = 0;   // optional
    int time = 0;       // optional

    if (pars.size() > 1)
        position = atoi(pars[1].c_str());

    if (pars.size() > 2)
        time = atoi(pars[2].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (!buttons.empty())
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            vector<TSubPage *> subviews = createSubViewList(bt->getSubViewID());

            if (subviews.empty() || !bt)
                continue;

            vector<TSubPage *>::iterator itSub;

            for (itSub = subviews.begin(); itSub != subviews.end(); ++itSub)
            {
                TSubPage *sub = *itSub;

                if (sub && sub->getName() == name)
                {
                    if (_toggleSubViewItem)
                        _toggleSubViewItem(sub->getHandle(), bt->getHandle(), position, time);

                    break;
                }
            }
        }
    }
}

void TPageManager::doLVD(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVD(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command LVD: Expecting one parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    string source = pars[0];
    vector<string> configs;

    if (pars.size() > 1)
    {
        for (size_t i = 1; i < pars.size(); ++i)
        {
            string low = toLower(pars[i]);

            if (low.find_first_of("user=") != string::npos ||
                low.find_first_of("pass=") != string::npos ||
                low.find_first_of("csv=")  != string::npos ||
                low.find_first_of("has_headers=") != string::npos)
            {
                configs.push_back(pars[i]);
            }
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setListSource(source, configs);
        }
    }

}

void TPageManager::doLVE(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVE(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command LVE: Expecting one parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int num = atoi(pars[0].c_str());

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setListViewEventNumber(num);
        }
    }

}

void TPageManager::doLVF(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVF(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command LVF: Expecting one parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    string filter;

    vector<string>::iterator iter;

    for (iter = pars.begin(); iter != pars.end(); ++iter)
    {
        if (filter.length() > 0)
            filter += ",";

        filter += *iter;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setListSourceFilter(filter);
        }
    }
}

void TPageManager::doLVL(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVL(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command LVL: Expecting one parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    bool hasColumns = false;
    int columns = 0;
    bool hasLayout = false;
    int layout = 0;
    bool hasComponent = false;
    int component = 0;
    bool hasCellHeight = false;
    bool cellHeightPercent = false;
    int cellheight = 0;
    bool hasP1 = false;
    int p1 = 0;
    bool hasP2 = false;
    int p2 = 0;
    bool hasFilter = false;
    bool filter = false;
    bool hasFilterHeight = false;
    bool filterHeightPercent = false;
    int filterheight = 0;
    bool hasAlphaScroll = false;
    bool alphascroll = false;

    vector<string>::iterator iter;

    for (iter = pars.begin(); iter != pars.end(); ++iter)
    {
        string low = toLower(*iter);

        if (low.find("columns=") != string::npos ||
            low.find("nc=") != string::npos ||
            low.find("numcol=") != string::npos)
        {
            size_t pos = low.find("=");
            string sCols = low.substr(pos + 1);
            columns = atoi(sCols.c_str());
            hasColumns = true;
        }
        else if (low.find("c=") != string::npos || low.find("comp=") != string::npos)
        {
            size_t pos = low.find("=");
            string sComp = low.substr(pos + 1);
            component |= atoi(sComp.c_str());
            hasComponent = true;
        }
        else if (low.find("l=") != string::npos || low.find("layout=") != string::npos)
        {
            size_t pos = low.find("=");
            string sLay = low.substr(pos + 1);
            layout = atoi(sLay.c_str());
            hasLayout = true;
        }
        else if (low.find("ch=") != string::npos || low.find("cellheight=") != string::npos)
        {
            size_t pos = low.find("=");
            string sCh = low.substr(pos + 1);
            cellheight = atoi(sCh.c_str());

            if (low.find("%") != string::npos)
                cellHeightPercent = true;

            hasCellHeight = true;
        }
        else if (low.find("p1=") != string::npos)
        {
            size_t pos = low.find("=");
            string sP1 = low.substr(pos + 1);
            p1 = atoi(sP1.c_str());
            hasP1 = true;
        }
        else if (low.find("p2=") != string::npos)
        {
            size_t pos = low.find("=");
            string sP2 = low.substr(pos + 1);
            p2 = atoi(sP2.c_str());
            hasP2 = true;
        }
        else if (low.find("f=") != string::npos || low.find("filter=") != string::npos)
        {
            size_t pos = low.find("=");
            string sFilter = low.substr(pos + 1);
            filter = isTrue(sFilter);
            hasFilter = true;
        }
        else if (low.find("fh=") != string::npos || low.find("filterheight=") != string::npos)
        {
            size_t pos = low.find("=");
            string sFilter = low.substr(pos + 1);
            filterheight = atoi(sFilter.c_str());

            if (low.find("%") != string::npos)
                filterHeightPercent = true;

            hasFilterHeight = true;
        }
        else if (low.find("as=") != string::npos || low.find("alphascroll=") != string::npos)
        {
            size_t pos = low.find("=");
            string sAlpha = low.substr(pos + 1);
            alphascroll = isTrue(sAlpha);
            hasAlphaScroll = true;
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (hasColumns)         bt->setListViewColumns(columns);
            if (hasComponent)       bt->setListViewComponent(component);
            if (hasLayout)          bt->setListViewLayout(layout);
            if (hasCellHeight)      bt->setListViewCellheight(cellheight, cellHeightPercent);
            if (hasP1)              bt->setListViewP1(p1);
            if (hasP2)              bt->setListViewP2(p2);
            if (hasFilter)          bt->setListViewColumnFilter(filter);
            if (hasFilterHeight)    bt->setListViewFilterHeight(filterheight, filterHeightPercent);
            if (hasAlphaScroll)     bt->setListViewAlphaScroll(alphascroll);
        }
    }
}

void TPageManager::doLVM(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVM(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command LVM: Expecting one parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    map<string,string> mapField;

    vector<string>::iterator iter;

    for (iter = pars.begin(); iter != pars.end(); ++iter)
    {
        string left, right;
        size_t pos = 0;

        if ((pos = iter->find("=")) != string::npos)
        {
            string left = iter->substr(0, pos);
            left = toLower(left);
            string right = iter->substr(pos + 1);

            if (left == "t1" || left == "t2" || left == "i1")
                mapField.insert(pair<string,string>(left, right));
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setListViewFieldMap(mapField);
        }
    }
}

void TPageManager::doLVN(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVN(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Command LVN: Expecting one parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    string command = pars[0];
    bool select = false;

    if (pars.size() > 1)
    {
        if (isTrue(pars[1]))
            select = true;
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->listViewNavigate(command, select);
        }
    }
}

void TPageManager::doLVR(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVR(int port, vector<int> &channels, vector<string> &pars)");

    TError::clear();
    int interval = -1;
    bool force = false;

    if (pars.size() > 0)
        interval = atoi(pars[0].c_str());

    if (pars.size() > 1)
        force = isTrue(pars[1]);

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->listViewRefresh(interval, force);
        }
    }
}

void TPageManager::doLVS(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doLVS(int port, vector<int> &channels, vector<string> &pars)");

    TError::clear();
    vector<string> sortColumns;
    Button::LIST_SORT sort = Button::LIST_SORT_NONE;
    string override;

    if (pars.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = pars.begin(); iter != pars.end(); ++iter)
        {
            if (iter->find(";") == string::npos)
                sortColumns.push_back(*iter);
            else
            {
                vector<string> parts = StrSplit(*iter, ";");
                sortColumns.push_back(parts[0]);

                if (parts[1].find("a") != string::npos || parts[1].find("A") != string::npos)
                    sort = Button::LIST_SORT_ASC;
                else if (parts[1].find("d") != string::npos || parts[1].find("D") != string::npos)
                    sort = Button::LIST_SORT_DESC;
                else if (parts[1].find("*") != string::npos)
                {
                    if (parts.size() > 2 && !parts[2].empty())
                    {
                        override = parts[2];
                        sort = Button::LIST_SORT_OVERRIDE;
                    }
                }
                else if (parts[1].find("n") != string::npos || parts[1].find("N") != string::npos)
                    sort = Button::LIST_SORT_NONE;
            }
        }
    }

    vector<TMap::MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->listViewSortData(sortColumns, sort, override);
        }
    }
}

void TPageManager::doTPCCMD(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTPCCMD(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Too few arguments for TPCCMD!");
        return;
    }

    string cmd = pars[0];

    if (strCaseCompare(cmd, "LocalHost") == 0)
    {
        if (pars.size() < 2 || pars[1].empty())
        {
            MSG_ERROR("The command \"LocalHost\" requires an additional parameter!");
            return;
        }

        TConfig::saveController(pars[1]);
    }
    else if (strCaseCompare(cmd, "LocalPort") == 0)
    {
        if (pars.size() < 2 || pars[1].empty())
        {
            MSG_ERROR("The command \"LocalPort\" requires an additional parameter!");
            return;
        }

        int port = atoi(pars[1].c_str());

        if (port > 0 && port < 65536)
            TConfig::savePort(port);
        else
        {
            MSG_ERROR("Invalid network port " << port);
        }
    }
    else if (strCaseCompare(cmd, "DeviceID") == 0)
    {
        if (pars.size() < 2 || pars[1].empty())
        {
            MSG_ERROR("The command \"DeviceID\" requires an additional parameter!");
            return;
        }

        int id = atoi(pars[1].c_str());

        if (id >= 10000 && id < 30000)
            TConfig::setSystemChannel(id);
    }
    else if (strCaseCompare(cmd, "ApplyProfile") == 0)
    {
        // We restart the network connection only
        if (gAmxNet)
            gAmxNet->reconnect();
    }
    else if (strCaseCompare(cmd, "QueryDeviceInfo") == 0)
    {
        string info = "DEVICEINFO-TPANELID," + TConfig::getPanelType();
        info += ";HOSTNAME,";
        char hostname[HOST_NAME_MAX];

        if (gethostname(hostname, HOST_NAME_MAX) != 0)
        {
            MSG_ERROR("Can't get host name: " << strerror(errno));
            return;
        }

        info.append(hostname);
        info += ";UUID," + TConfig::getUUID();
        sendGlobalString(info);
    }
    else if (strCaseCompare(cmd, "LockRotation") == 0)
    {
        if (pars.size() < 2 || pars[1].empty())
        {
            MSG_ERROR("The command \"LockRotation\" requires an additional parameter!");
            return;
        }

        if (strCaseCompare(pars[1], "true") == 0)
            TConfig::setRotationFixed(true);
        else
            TConfig::setRotationFixed(false);
    }
    else if (strCaseCompare(cmd, "ButtonHit") == 0)
    {
        if (pars.size() < 2 || pars[1].empty())
        {
            MSG_ERROR("The command \"ButtonHit\" requires an additional parameter!");
            return;
        }

        if (strCaseCompare(pars[1], "true") == 0)
            TConfig::saveSystemSoundState(true);
        else
            TConfig::saveSystemSoundState(false);
    }
    else if (strCaseCompare(cmd, "ReprocessTP4") == 0)
    {
        if (_resetSurface)
            _resetSurface();
    }
}

void TPageManager::doTPCACC(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTPCACC(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Too few arguments for TPCACC!");
        return;
    }

    string cmd = pars[0];

    if (strCaseCompare(cmd, "ENABLE") == 0)
    {
        mInformOrientation = true;
        sendOrientation();
    }
    else if (strCaseCompare(cmd, "DISABLE") == 0)
    {
        mInformOrientation = false;
    }
    else if (strCaseCompare(cmd, "QUERY") == 0)
    {
        sendOrientation();
    }
}

#ifndef _NOSIP_
void TPageManager::doTPCSIP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTPCSIP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
        return;

    string cmd = toUpper(pars[0]);

    if (cmd == "SHOW" && _showPhoneDialog)
        _showPhoneDialog(true);
    else if (!_showPhoneDialog)
    {
        MSG_ERROR("There is no phone dialog registered!");
    }
}
#endif

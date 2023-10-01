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

#include <fstream>
#include <vector>
#include <iterator>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __ANDROID__
#include <QUuid>
#include <android/log.h>
#include "tvalidatefile.h"
#include "tvalidatefile.h"
#else
#include <uuid/uuid.h>
#endif
#include "ttpinit.h"
#include "tconfig.h"
#include "terror.h"
#include "tresources.h"
#include "tlock.h"
#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_SIMULATOR || TARGET_OS_IOS
#include <QString>
#include "ios/QASettings.h"
#endif
#endif
using std::string;
using std::ifstream;
using std::ofstream;
using std::fstream;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;

bool TConfig::mInitialized{false};
int TConfig::mChannel{0};
bool TConfig::mMute{false};
bool TConfig::mTemporary{false};
bool TConfig::mLogFileEnabled{false};
std::mutex config_mutex;

/**
 * @struct SETTINGS
 * @brief The SETTINGS struct bundles the configuration options.
 *
 * This structure contains variables for all possible configuration options.
 * It is used by the class TConfig. Through this class it's possible to
 * access all configuration options.
 */
struct SETTINGS
{
    string pname{"tpanel"};     //!< Name of the program (default "tpanel")
    string path;                //!< The path where the configuration file is located
    string name;                //!< The name of the configuration file
    string project;             //!< The path where the original project files are located
    string server;              //!< The name or IP address of the server to connect
    int system{0};              //!< The number of the AMX system
    int port{0};                //!< The port number
    int ID{0};                  //!< the panel ID (a number starting by 10000)
    string ptype;               //!< The type of the panel (android, ipad, iphone, ...)
    string version;             //!< The "firmware" version
    string logFile;             //!< Optional path and name of a logfile
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#ifdef QT_DEBUG
    string logLevel{"INFO|WARNING|ERROR|DEBUG"};   //!< The log level(s).
    uint logLevelBits{HLOG_INFO|HLOG_WARNING|HLOG_ERROR|HLOG_DEBUG};//!< The numeric bit field of the loglevel
#else
    string logLevel{"NONE"};   //!< The log level(s).
    uint logLevelBits{HLOG_NONE};//!< The numeric bit field of the loglevel
#endif
#else
    string logLevel{"PROTOCOL"};//!< The log level(s).
    uint logLevelBits{HLOG_PROTOCOL};//!< The numeric bit field of the loglevel
#endif
    bool longformat{false};     //!< TRUE = long format
    bool noBanner{false};       //!< Startup without showing a banner on the command line.
    bool certCheck{false};      //!< TRUE = Check certificate for SSL connection
    bool scale{false};          //!< TRUE = Images are scaled to fit the whole screen
    bool tbsuppress{false};     //!< TRUE = Don't show toolbar even if enough space
    bool tbforce{true};         //!< Only if "tbsuppress" = FALSE: TRUE = The toolbar is forced to display, FALSE = The toolbar is only visible if there is enough space left
    bool profiling{false};      //!< TRUE = The declaration traces meassure the time and write it to the log
    size_t max_cache{100};      //!< Size of internal button cache in Mb
    string password1;           //!< First panel password
    string password2;           //!< Second panel password
    string password3;           //!< Third panel password
    string password4;           //!< Fourth panel password
    bool systemRotationFix{false};  //!< TRUE = Rotation is blocked and orientation sensor is ignored.
    string uuid;                //!< An UUID set automatically after first start.
    // FTP credentials
    string ftpUser;             //!< The username for FTP of the controller (default: administrator)
    string ftpPassword;         //!< The password for FTP of the controller (default: password)
    string ftpSurface;          //!< The name of the file containing the TPDesign4 file to load
    bool ftpPassive{true};      //!< If false the data port 20 is used for file transfer
    time_t ftpLastDownload{0};  //!< The timestamp of the last download
    // SIP settings
    string sip_proxy;           //!< The address of the SIP proxy
    int sip_port{5060};         //!< Initializes the port of the SIP proxy to 5060
    int sip_portTLS{0};         //!< Initializes the TLS port of the SIP proxy to 0 (not used by default)
    string sip_stun;            //!< STUN address
    string sip_domain;          //!< Local domain
    string sip_user;            //!< The SIP user to connect.
    string sip_password;        //!< The SIP password to connect. Note: This password is saved in plain text!
    bool sip_ipv4{true};        //!< Default: TRUE, Enables or disables IPv4.
    bool sip_ipv6{true};        //!< Default: TRUE, Enables or disables IPv6. Has precedence over IPv4.
    bool sip_iphone{false};     //!< Default: FALSE, if enabled and SIP is enabled then the internal phone dialog is used.
    TConfig::SIP_FIREWALL_t sip_firewall{TConfig::SIP_NO_FIREWALL}; //!< Defines how to deal with a firewall.
    bool sip_enabled{false};    //!< By default SIP is disabled
    // Sound settings
    string systemSound;         //!< name of the set system sound played on every touch.
    bool systemSoundState{false};   //!< TRUE = play systemsound on every touch
    string systemSingleBeep;    //!< name of the system sound file to play a single beep.
    string systemDoubleBeep;    //!< name of the system sound file to play a double beep.
    int systemVolume{100};      //!< The set volume to use [0 ... 100]
    int systemGain{100};        //!< The set microphone level to use [0 ... 100]
};

typedef struct SETTINGS settings_t;
static settings_t localSettings;        //!< Global defines settings used in class TConfig.
static settings_t localSettings_temp;   //!< Global defines settings temporary settings

/**
 * @brief TConfig::TConfig constructor
 *
 * @param path  A path and name of a configuration file.
 */
TConfig::TConfig(const std::string& path)
    : mPath(path)
{
#if TARGET_OS_IOS == 0 && TARGET_OS_SIMULATOR == 0
    // Initialize the possible configuration paths
    mCfgPaths.push_back("/etc");
    mCfgPaths.push_back("/etc/tpanel");
    mCfgPaths.push_back("/usr/etc");
    mCfgPaths.push_back("/usr/etc/tpanel");
#ifdef __APPLE__
    mCfgPaths.push_back("/opt/local/etc");
    mCfgPaths.push_back("/opt/local/etc/tpanel");
    mCfgPaths.push_back("/opt/local/usr/etc");
    mCfgPaths.push_back("/opt/local/usr/etc/tpanel");
#endif
    if (findConfig())
        readConfig();
#else
    readConfig();
#endif
}

/**
 * Simple method to read the configuration again. This is usefull if, for
 * example the configuration options changed but should not be saved. Instead
 * they were canceled and therefor the options are read again from file.
 *
 * @return On success it returns TRUE.
 */
bool TConfig::reReadConfig()
{
    return readConfig();
}

/**
 * @brief TConfig::setTemporary - Activate/deactivate temporary config
 * Activates or deactivates the state of the temporary configuration. The
 * temporary configuration is a shadow configuration which holds all changes
 * to the configuration without saving it.
 *
 * @param tmp   State of the active configuration.
 * @return
 * Returns the previous state of the configuration setting.
 */
bool TConfig::setTemporary(bool tmp)
{
    DECL_TRACER("TConfig::setTemporary(bool tmp)");

    bool old = mTemporary;
    mTemporary = tmp;
    return old;
}

void TConfig::reset()
{
    DECL_TRACER("TConfig::reset()");

    localSettings_temp = localSettings;
    mTemporary = false;
}

/**
 * @brief TConfig::setProgName Sets the name of the application.
 * @param pname The name of the application.
 */
void TConfig::setProgName(const std::string& pname)
{
    if (mTemporary)
        localSettings_temp.pname = pname;
    else
        localSettings.pname = pname;

    mTemporary = false;
}

/**
 * @brief TConfig::getProgName Retrieves the prevously stored application name.
 * @return The name of this application.
 */
std::string & TConfig::getProgName()
{
    return mTemporary ? localSettings_temp.pname : localSettings.pname;
}

/**
 * @brief TConfig::getChannel returns the AMX channel to use.
 *
 * The AMX channels an AMX panel can use start at 10000. This method returns
 * the channel number found in the configuration file. If there was no
 * channel defination found, it returns the default channel 10001.
 *
 * @return The AMX channel number to use.
 */
int TConfig::getChannel()
{
    DECL_TRACER("TConfig::getChannel()");

    int ID = mTemporary ? localSettings_temp.ID : localSettings.ID;

    if (mChannel > 0 && mChannel != ID)
        return mChannel;

    return ID;
}

/**
 * @brief TConfig::getConfigFileName returns the name of the configuration file.
 *
 * @return The name of the configuration file.
 */
std::string& TConfig::getConfigFileName()
{
    return mTemporary ? localSettings_temp.name : localSettings.name;
}

/**
 * @brief TConfig::getConfigPath returns the path configuration file.
 *
 * The path was defined on the command line or found by searching the standard
 * directories.
 *
 * @return The path of the configuration file.
 */
std::string& TConfig::getConfigPath()
{
    return mTemporary ? localSettings_temp.path : localSettings.path;
}

/**
 * @brief TConfig::getController returns the network name or IP address of the AMX controller.
 *
 * The network name or the IP address was read from the configuration file.
 *
 * @return The network name of the AMX controller.
 */
std::string& TConfig::getController()
{
    DECL_TRACER("TConfig::getController()");

    return mTemporary ? localSettings_temp.server : localSettings.server;
}

/**
 * @brief TConfig::getSystem return the AMX system number.
 *
 * This number was read from the configuration file. If there was no system
 * number defined in the configuration file, then the default number 0 is
 * returned.
 *
 * @return The AMX system number.
 */
int TConfig::getSystem()
{
    DECL_TRACER("TConfig::getSystem()");

    return mTemporary ? localSettings_temp.system :  localSettings.system;
}

/**
 * @brief TConfig::getFirmVersion returns the version of the firmware.
 *
 * This option was read from the configuration file. There can be any version
 * number defined. But you must keep in mind, that the AMX controller may not
 * accept any number. If there was no version number defined, the standard
 * version number 1.0 is returned.
 *
 * @return The firmware version of this panel.
 */
std::string& TConfig::getFirmVersion()
{
    DECL_TRACER("TConfig::getFirmVersion()");

    return mTemporary ? localSettings_temp.version : localSettings.version;
}

/**
 * @brief TConfig::getLogFile the path and name of a logfile.
 *
 * If there is a logfile name defined in the configuration file, it is used
 * to write messages there. It depends on the _log level_ what is logged.
 *
 * @return The path and name of a logfile.
 */
std::string& TConfig::getLogFile()
{
    return mTemporary ? localSettings_temp.logFile : localSettings.logFile;
}

/**
 * @brief TConfig::getLogLevel returns the defined log level.
 *
 * The loglevel can consist of the following values:
 *
 *     NONE         Logs nothing (default for Android)
 *     INFO         Logs only informations
 *     WARNING      Logs only warnings
 *     ERROR        Logs only errors
 *     TRACE        Logs only trace messages
 *     DEBUG        Logs only debug messages
 *     PROTOCOL     Logs only INFO and ERROR (default if NOT Android)
 *     ALL          Logs everything
 *
 * All log levels can be combined by concatenating them with the | symbol.
 *
 * @return The log level(s) as a string.
 */
string& TConfig::getLogLevel()
{
    return mTemporary ? localSettings_temp.logLevel : localSettings.logLevel;
}

/**
 * @brief TConfig::getLogLevelBits
 *
 * Returns the raw bit field defining the loglevels selected.
 *
 * @return The bit field of representing the selected log levels.
 */
uint TConfig::getLogLevelBits()
{
    DECL_TRACER("TConfig::getLogLevelBits()");

    return mTemporary ? localSettings_temp.logLevelBits : localSettings.logLevelBits;
}
/**
 * @brief TConfig::getPanelType the AMX type name of the panel.
 *
 * The type name of the panel is defined in the configuration file. If this
 * option was not defined, the default panel _android_ is returned.
 *
 * @return The type name of the panel.
 */
std::string& TConfig::getPanelType()
{
    DECL_TRACER("TConfig::getPanelType()");

    return mTemporary ? localSettings_temp.ptype : localSettings.ptype;
}

/**
 * @brief TConfig::getPort returnes the AMX port number to connect to.
 *
 * The port number can be defined in the configuration file. If there is no
 * configuration the default number 1319 is returned.
 *
 * @return The AMX network port number.
 */
int TConfig::getPort()
{
    DECL_TRACER("TConfig::getPort()");

    return mTemporary ? localSettings_temp.port : localSettings.port;
}

/**
 * @brief TConfig::getProjectPath returns the path to the AMX configuration files.
 *
 * The path was read from the configuration file. This path contains all the
 * files needed to display the elements of the surface.
 *
 * @return The path to the AMX configuration files.
 */
string& TConfig::getProjectPath()
{
    return localSettings.project;
}

/**
 * @brief TConfig::getSystemProjectPath returns the path to the AMX setup
 * configuration files.
 *
 * The path was read from the configuration file. This path contains all the
 * files needed to display the setup elements of the setup dialog.
 *
 * @return The path to the AMX setup configuration files.
 */
string TConfig::getSystemProjectPath()
{
    return localSettings.project + "/__system";
}

string TConfig::getSystemPath(SYSTEMRESOURCE_t sres)
{
    DECL_TRACER("TConfig::getSystemPath(SYSTEMRESOURCE_t sres)");

    string p;

    switch(sres)
    {
        case BORDERS:   p = "/borders"; break;
        case FONTS:     p = "/fonts"; break;
        case IMAGES:    p = "/images"; break;
        case SLIDERS:   p = "/sliders"; break;
        case SOUNDS:    p = "/sounds"; break;
        default:
            p.clear();
    }

    return localSettings.project + "/__system/graphics" + p;
}

bool TConfig::saveLogFile(const string &file)
{
    DECL_TRACER("TConfig::saveLogFile(const string &file)");

    string logFile = mTemporary ? localSettings_temp.logFile : localSettings.logFile;

    if (file.empty() || logFile.compare(file) == 0)
        return false;

    if (mTemporary)
        localSettings_temp.logFile = file;
    else
        localSettings.logFile = file;

    mTemporary = false;
    return true;
}

bool TConfig::saveLogLevel(const string &level)
{
    DECL_TRACER("TConfig::saveLogLevel(const string &level)");

    if (level.find(SLOG_NONE) == string::npos && level.find(SLOG_INFO) == string::npos && level.find(SLOG_WARNING) == string::npos &&
            level.find(SLOG_ERROR) == string::npos && level.find(SLOG_TRACE) == string::npos && level.find(SLOG_DEBUG) == string::npos &&
            level.find(SLOG_PROTOCOL) == string::npos && level.find(SLOG_ALL) == string::npos)
        return false;

    if (mTemporary)
    {
        localSettings_temp.logLevel = level;
        localSettings_temp.logLevelBits = logLevelStrToBits(level);
    }
    else
    {
        localSettings.logLevel = level;
        localSettings.logLevelBits = logLevelStrToBits(level);
    }

    MSG_INFO("New log level: " << level);
    mTemporary = false;
    return true;
}

bool TConfig::saveLogLevel(uint level)
{
    DECL_TRACER("TConfig::saveLogLevel(uint level)");

    if (level != 0 && !(level&HLOG_INFO) && !(level&HLOG_WARNING) &&
            !(level&HLOG_ERROR) && !(level&HLOG_TRACE) && !(level&HLOG_DEBUG))
        return false;

    if (mTemporary)
    {
        localSettings_temp.logLevelBits = level;
        localSettings_temp.logLevel = logLevelBitsToString(level);
    }
    else
    {
        localSettings.logLevelBits = level;
        localSettings.logLevel = logLevelBitsToString(level);
        TStreamError::setLogLevel(level);
        MSG_INFO("New log level from bits: " << localSettings.logLevel);
    }

    mTemporary = false;
    return true;
}

bool TConfig::saveChannel(int channel)
{
    DECL_TRACER("TConfig::saveChannel(int channel)");

    if (channel < 10000 || channel > 20000)
        return false;

    if (mTemporary)
        localSettings_temp.ID = channel;
    else
        localSettings.ID = channel;

    mTemporary = false;
    return true;
}

bool TConfig::saveController(const string &cnt)
{
    DECL_TRACER("TConfig::saveController(const string &cnt)");

    if (mTemporary)
        localSettings_temp.server = cnt;
    else
        localSettings.server = cnt;

    mTemporary = false;
    return true;
}

bool TConfig::savePanelType(const string &pt)
{
    DECL_TRACER("TConfig::savePanelType(const string &pt)");

    if (mTemporary)
        localSettings_temp.ptype = pt;
    else
        localSettings.ptype = pt;

    mTemporary = false;
    return true;
}

bool TConfig::savePort(int port)
{
    DECL_TRACER("TConfig::savePort(int port)");

    if (port < 1024 || port > 32767)
        return false;

    if (mTemporary)
        localSettings_temp.port = port;
    else
        localSettings.port = port;

    mTemporary = false;
    return true;
}

bool TConfig::saveProjectPath(const string &path)
{
    DECL_TRACER("TConfig::saveProjectPath(const string &path)");

    if (path.empty())
        return false;

    if (mTemporary)
        localSettings_temp.project = path;
    else
        localSettings.project = path;

    mTemporary = false;
    return true;
}

void TConfig::saveFormat(bool format)
{
    DECL_TRACER(string("TConfig::saveFormat(bool format) ") + (format ? "[TRUE]" : "[FALSE]"));

    if (mTemporary)
        localSettings_temp.longformat = format;
    else
        localSettings.longformat = format;

    mTemporary = false;
}

void TConfig::saveScale(bool scale)
{
    DECL_TRACER("TConfig::saveScale(bool scale)");

    if (mTemporary)
        localSettings_temp.scale = scale;
    else
        localSettings.scale = scale;

    mTemporary = false;
}

void TConfig::saveBanner(bool banner)
{
    DECL_TRACER("TConfig::saveBanner(bool banner)");

    if (mTemporary)
        localSettings_temp.noBanner = banner;
    else
        localSettings.noBanner = banner;

    mTemporary = false;
}

void TConfig::saveToolbarForce(bool tb)
{
    DECL_TRACER("TConfig::saveToolbarForce(bool tb)");

    if (mTemporary)
        localSettings_temp.tbforce = tb;
    else
        localSettings.tbforce = tb;

    mTemporary = false;
}

void TConfig::saveToolbarSuppress(bool tb)
{
    DECL_TRACER("TConfig::saveToolbarSuppress(bool tb)");

    if (mTemporary)
        localSettings_temp.tbsuppress = tb;
    else
        localSettings.tbsuppress = tb;

    mTemporary = false;
}

void TConfig::saveProfiling(bool prof)
{
    DECL_TRACER("TConfig::saveProfiling(bool prof)");

    if (mTemporary)
        localSettings_temp.profiling = prof;
    else
        localSettings.profiling = prof;

    mTemporary = false;
}

void TConfig::saveButtonCache(size_t size)
{
    DECL_TRACER("TConfig::saveButtonCache(size_t size)");

    if (mTemporary)
        localSettings_temp.max_cache = size;
    else
        localSettings.max_cache = size;

    mTemporary = false;
}

void TConfig::savePassword1(const std::string& pw)
{
    DECL_TRACER("TConfig::savePassword1(const std::string& pw)");

    if (mTemporary)
        localSettings_temp.password1 = pw;
    else
        localSettings.password1 = pw;

    mTemporary = false;
}

void TConfig::savePassword2(const std::string& pw)
{
    DECL_TRACER("TConfig::savePassword2(const std::string& pw)");

    if (mTemporary)
        localSettings_temp.password2 = pw;
    else
        localSettings.password2 = pw;

    mTemporary = false;
}

void TConfig::savePassword3(const std::string& pw)
{
    DECL_TRACER("TConfig::savePassword3(const std::string& pw)");

    if (mTemporary)
        localSettings_temp.password3 = pw;
    else
        localSettings.password3 = pw;

    mTemporary = false;
}

void TConfig::savePassword4(const std::string& pw)
{
    DECL_TRACER("TConfig::savePassword4(const std::string& pw)");

    if (mTemporary)
        localSettings_temp.password4 = pw;
    else
        localSettings.password4 = pw;

    mTemporary = false;
}

void TConfig::saveSystemSoundFile(const std::string& snd)
{
    DECL_TRACER("TConfig::saveSystemSoundFile(const std::string& snd)");

    if (mTemporary)
        localSettings_temp.systemSound = snd;
    else
        localSettings.systemSound = snd;

    mTemporary = false;
}

void TConfig::saveSystemSoundState(bool state)
{
    DECL_TRACER("TConfig::saveSystemSoundState(bool state)");

    if (mTemporary)
        localSettings_temp.systemSoundState = state;
    else
        localSettings.systemSoundState = localSettings_temp.systemSoundState = state;

    mTemporary = false;
}

void TConfig::saveSingleBeepFile(const std::string& snd)
{
    DECL_TRACER("TConfig::saveSingleBeepFile(const std::string& snd)");

    if (mTemporary)
        localSettings_temp.systemSingleBeep = snd;
    else
        localSettings.systemSingleBeep = snd;

    mTemporary = false;
}

void TConfig::saveDoubleBeepFile(const std::string& snd)
{
    DECL_TRACER("TConfig::saveDoubleBeepFile(const std::string& snd)");

    if (mTemporary)
        localSettings_temp.systemDoubleBeep = snd;
    else
        localSettings.systemDoubleBeep = snd;

    mTemporary = false;
}

void TConfig::saveSystemVolume(int volume)
{
    DECL_TRACER("TConfig::saveSystemVolume(int volume)");

    if (volume < 0 || volume > 100)
        return;

    if (mTemporary)
        localSettings_temp.systemVolume = volume;
    else
        localSettings.systemVolume = volume;

    mTemporary = false;
}

void TConfig::saveSystemGain(int gain)
{
    DECL_TRACER("TConfig::saveSystemGain(int gain)");

    if (gain < 0 || gain > 100)
        return;

    if (mTemporary)
        localSettings_temp.systemGain = gain;
    else
        localSettings.systemGain = gain;

    mTemporary = false;
}

void TConfig::saveFtpUser(const string& user)
{
    DECL_TRACER("TConfig::saveFtpUser(const string& user)");

    if (mTemporary)
        localSettings_temp.ftpUser = user;
    else
        localSettings.ftpUser = user;

    mTemporary = false;
}

void TConfig::saveFtpPassword(const string& pw)
{
    DECL_TRACER("TConfig::saveFtpPassword(const string& pw)");

    if (mTemporary)
        localSettings_temp.ftpPassword = pw;
    else
        localSettings.ftpPassword = pw;

    mTemporary = false;
}

void TConfig::saveFtpSurface(const string& fname)
{
    DECL_TRACER("TConfig::saveFtpSurface(const string& fname)");

    if (mTemporary)
        localSettings_temp.ftpSurface = fname;
    else
        localSettings.ftpSurface = fname;

    mTemporary = false;
}

void TConfig::saveFtpPassive(bool mode)
{
    DECL_TRACER("TConfig::saveFtpPassive(bool mode)");

    if (mTemporary)
        localSettings_temp.ftpPassive = mode;
    else
        localSettings.ftpPassive = mode;

    mTemporary = false;
}

void TConfig::saveFtpDownloadTime(time_t t)
{
    DECL_TRACER("TConfig::saveFtpDownloadTime(time_t t)");

    if (mTemporary)
        localSettings_temp.ftpLastDownload = t;
    else
        localSettings.ftpLastDownload = t;

    mTemporary = false;
}

std::string& TConfig::getSIPproxy()
{
    DECL_TRACER("TConfig::getSIPproxy()");

    return mTemporary ? localSettings_temp.sip_proxy : localSettings.sip_proxy;
}

void TConfig::setSIPproxy(const std::string& address)
{
    DECL_TRACER("TConfig::setSIPproxy(const std::string& address)");

    if (mTemporary)
        localSettings_temp.sip_proxy = address;
    else
        localSettings.sip_proxy = address;

    mTemporary = false;
}

int TConfig::getSIPport()
{
    DECL_TRACER("TConfig::getSIPport()");

    return mTemporary ? localSettings_temp.sip_port : localSettings.sip_port;
}

void TConfig::setSIPport(int port)
{
    DECL_TRACER("TConfig::setSIPport(int port)");

    if (mTemporary)
        localSettings_temp.sip_port = port;
    else
        localSettings.sip_port = port;

    mTemporary = false;
}

int TConfig::getSIPportTLS()
{
    DECL_TRACER("TConfig::getSIPportTLS()");

    return mTemporary ? localSettings_temp.sip_portTLS : localSettings.sip_portTLS;
}

void TConfig::setSIPportTLS(int port)
{
    DECL_TRACER("TConfig::setSIPportTLS(int port)");

    if (mTemporary)
        localSettings_temp.sip_portTLS = port;
    else
        localSettings.sip_portTLS = port;

    mTemporary = false;
}

std::string& TConfig::getSIPstun()
{
    DECL_TRACER("TConfig::getSIPstun()");

    return mTemporary ? localSettings_temp.sip_stun : localSettings.sip_stun;
}

void TConfig::setSIPstun(const std::string& address)
{
    DECL_TRACER("TConfig::setSIPstun(const std::string& address)");

    if (mTemporary)
        localSettings_temp.sip_stun = address;
    else
        localSettings.sip_stun = address;

    mTemporary = false;
}

std::string& TConfig::getSIPdomain()
{
    DECL_TRACER("TConfig::getSIPdomain()");

    return mTemporary ? localSettings_temp.sip_domain : localSettings.sip_domain;
}

void TConfig::setSIPdomain(const std::string& domain)
{
    DECL_TRACER("TConfig::setSIPdomain(const std::string& domain)");

    if (mTemporary)
        localSettings_temp.sip_domain = domain;
    else
        localSettings.sip_domain = domain;

    mTemporary = false;
}

std::string& TConfig::getSIPuser()
{
    DECL_TRACER("TConfig::getSIPuser()");

    return mTemporary ? localSettings_temp.sip_user : localSettings.sip_user;
}

void TConfig::setSIPuser(const std::string& user)
{
    DECL_TRACER("TConfig::setSIPuser(const std::string& user)");

    if (mTemporary)
        localSettings_temp.sip_user = user;
    else
        localSettings.sip_user = user;

    mTemporary = false;
}

std::string& TConfig::getSIPpassword()
{
    DECL_TRACER("TConfig::getSIPpassword()");

    return mTemporary ? localSettings_temp.sip_password : localSettings.sip_password;
}

void TConfig::setSIPpassword(const std::string& pw)
{
    DECL_TRACER("TConfig::setSIPpassword(const std::string& pw)");

    if (mTemporary)
        localSettings_temp.sip_password = pw;
    else
        localSettings.sip_password = pw;

    mTemporary = false;
}

bool TConfig::getSIPstatus()
{
    DECL_TRACER("TConfig::getSIPstatus()");

    return mTemporary ? localSettings_temp.sip_enabled : localSettings.sip_enabled;
}

bool TConfig::getSIPnetworkIPv4()
{
    DECL_TRACER("TConfig::getSIPnetworkIPv4()");

    return mTemporary ? localSettings_temp.sip_ipv4 : localSettings.sip_ipv4;
}

void TConfig::setSIPnetworkIPv4(bool state)
{
    DECL_TRACER("TConfig::setSIPnetworkIPv4(bool state)");

    if (mTemporary)
        localSettings_temp.sip_ipv4 = state;
    else
        localSettings.sip_ipv4 = state;

    mTemporary = false;
}

bool TConfig::getSIPnetworkIPv6()
{
    DECL_TRACER("TConfig::getSIPnetworkIPv6()");

    return mTemporary ? localSettings_temp.sip_ipv6 : localSettings.sip_ipv6;
}

void TConfig::setSIPnetworkIPv6(bool state)
{
    DECL_TRACER("TConfig::setSIPnetworkIPv6(bool state)");

    if (mTemporary)
        localSettings_temp.sip_ipv6 = state;
    else
        localSettings.sip_ipv6 = state;

    mTemporary = false;
}

void TConfig::setSIPiphone(bool state)
{
    DECL_TRACER("TConfig::setSIPiphone(bool state)");

    if (mTemporary)
        localSettings_temp.sip_iphone = state;
    else
        localSettings.sip_iphone = state;

    mTemporary = false;
}

bool TConfig::getSIPiphone()
{
    DECL_TRACER("TConfig::getSIPiphone()");

    return mTemporary ? localSettings_temp.sip_iphone : localSettings.sip_iphone;
}

TConfig::SIP_FIREWALL_t TConfig::getSIPfirewall()
{
    DECL_TRACER("TConfig::getSIPfirewall()");

    return mTemporary ? localSettings_temp.sip_firewall : localSettings.sip_firewall;
}

string TConfig::getSIPfirewallStr()
{
    DECL_TRACER("TConfig::getSIPfirewallStr()");

    return sipFirewallToString(mTemporary ? localSettings_temp.sip_firewall : localSettings.sip_firewall);
}

void TConfig::setSIPfirewall(TConfig::SIP_FIREWALL_t fw)
{
    DECL_TRACER("TConfig::setSIPfirewall(TConfig::SIP_FIREWALL_t fw)")

    if (mTemporary)
        localSettings_temp.sip_firewall = fw;
    else
        localSettings.sip_firewall = fw;

    mTemporary = false;
}

void TConfig::setSIPstatus(bool state)
{
    DECL_TRACER("TConfig::setSIPstatus(bool state)");

    if (mTemporary)
        localSettings_temp.sip_enabled = state;
    else
        localSettings.sip_enabled = state;

    mTemporary = false;
}

bool TConfig::saveSettings()
{
    DECL_TRACER("TConfig::saveSettings()");

    TLock<std::mutex> guard(config_mutex);

    try
    {
        string fname = localSettings.path + "/" + localSettings.name;

        if (mTemporary)
        {
            localSettings = localSettings_temp;
            MSG_INFO("Temporary settings were copied over.");
        }

        MSG_DEBUG("Saving to file " << fname);
        ofstream file(fname);
        string lines = "LogFile=" + localSettings.logFile + "\n";
        lines += "LogLevel=" + localSettings.logLevel + "\n";
        lines += "ProjectPath=" + localSettings.project + "\n";
        lines += string("NoBanner=") + (localSettings.noBanner ? "true" : "false") + "\n";
        lines += string("ToolbarSuppress=") + (localSettings.tbsuppress ? "true" : "false") + "\n";
        lines += string("ToolbarForce=") + (localSettings.tbforce ? "true" : "false") + "\n";
        lines += string("LongFormat=") + (localSettings.longformat ? "true" : "false") + "\n";
        lines += "Address=" + localSettings.server + "\n";
        lines += "Port=" + std::to_string(localSettings.port) + "\n";
        lines += "Channel=" + std::to_string(localSettings.ID) + "\n";
        lines += "System=" + std::to_string(localSettings.system) + "\n";
        lines += "PanelType=" + localSettings.ptype + "\n";
        lines += "Firmware=" + localSettings.version + "\n";
        lines += string("CertCheck=") + (localSettings.certCheck ? "true" : "false") + "\n";
        lines += string("Scale=") + (localSettings.scale ? "true" : "false") + "\n";
        lines += string("Profiling=") + (localSettings.profiling ? "true" : "false") + "\n";
        lines += "MaxButtonCache=" + std::to_string(localSettings.max_cache) + "\n";
        lines += string("Password1=") + localSettings.password1 + "\n";
        lines += string("Password2=") + localSettings.password2 + "\n";
        lines += string("Password3=") + localSettings.password3 + "\n";
        lines += string("Password4=") + localSettings.password4 + "\n";
        lines += string("SystemSoundFile=") + localSettings.systemSound + "\n";
        lines += string("SystemSoundState=") + (localSettings.systemSoundState ? "ON" : "OFF") + "\n";
        lines += string("SystemSingleBeep=") + localSettings.systemSingleBeep + "\n";
        lines += string("SystemDoubleBeep=") + localSettings.systemDoubleBeep + "\n";
        lines += "SystemVolume=" + std::to_string(localSettings.systemVolume) + "\n";
        lines += "SystemGain=" + std::to_string(localSettings.systemGain) + "\n";
        lines += string("SystemRotationFix=") + (localSettings.systemRotationFix ? "ON" : "OFF") + "\n";
        lines += string("UUID=") + localSettings.uuid + "\n";
        // FTP credentials
        lines += string("FTPuser=") + localSettings.ftpUser + "\n";
        lines += string("FTPpassword=") + localSettings.ftpPassword + "\n";
        lines += string("FTPsurface=") + localSettings.ftpSurface + "\n";
        lines += string("FTPpassive=") + (localSettings.ftpPassive ? "true" : "false") + "\n";
        lines += string("FTPdownloadTime=") + std::to_string(localSettings.ftpLastDownload) + "\n";
        // SIP settings
        lines += string("SIP_DOMAIN=") + localSettings.sip_domain + "\n";
        lines += string("SIP_PROXY=") + localSettings.sip_proxy + "\n";
        lines += string("SIP_PORT=") + std::to_string(localSettings.sip_port) + "\n";
        lines += string("SIP_PORTTLS=") + std::to_string(localSettings.sip_portTLS) + "\n";
        lines += string("SIP_STUN=") + localSettings.sip_stun + "\n";
        lines += string("SIP_USER=") + localSettings.sip_user + "\n";
        lines += string("SIP_PASSWORD=") + localSettings.sip_password + "\n";
        lines += string("SIP_IPV4=") + (localSettings.sip_ipv4 ? "true" : "false") + "\n";
        lines += string("SIP_IPV6=") + (localSettings.sip_ipv6 ? "true" : "false") + "\n";
        lines += string("SIP_IPHONE=") + (localSettings.sip_iphone ? "true" : "false") + "\n";
        lines += "SIP_FIREWALL=" + sipFirewallToString(localSettings.sip_firewall) + "\n";
        lines += string("SIP_ENABLED=") + (localSettings.sip_enabled ? "true" : "false") + "\n";
        file.write(lines.c_str(), lines.size());
        file.close();
        MSG_INFO("Actual log level: " << localSettings.logLevel);

        if (mTemporary)
        {
            TError::Current()->setLogLevel(localSettings.logLevel);
            TError::Current()->setLogFile(localSettings.logFile);
        }

        mTemporary = false;
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Couldn't write configs: " << e.what());
        return false;
    }

    return true;
}

/**
 * @brief TConfig::isLongFormat defines the format in the logfile.
 *
 * If this returns `true` the format in the logfile is a long format. This
 * means, that in front of each message is an additional timestamp.
 *
 * @return `true` = long format, `false` = short format (default).
 */
bool TConfig::isLongFormat()
{
    return mTemporary ? localSettings_temp.longformat : localSettings.longformat;
}

/**
 * @brief TConfig::showBanner defines whether the banner should be showed or not.
 *
 * If this method returns `false` the banner on startup is not displayed.
 *
 * @return `true` = display the banner (default), `false` = show no banner.
 */
bool TConfig::showBanner()
{
    DECL_TRACER("TConfig::showBanner()");

    return mTemporary ? (!localSettings_temp.noBanner) : (!localSettings.noBanner);
}

/**
 * @brief TConfig::getScale returns the scale setting
 *
 * If this is set to TRUE, all images are scaled to fit the screen.
 *
 * @return scale state
 */
bool TConfig::getScale()
{
    DECL_TRACER("TConfig::getScale()");

    return mTemporary ? localSettings_temp.scale : localSettings.scale;
}

bool TConfig::getToolbarForce()
{
    DECL_TRACER("TConfig::getToolbarForce()");

    return mTemporary ? localSettings_temp.tbforce : localSettings.tbforce;
}

bool TConfig::getToolbarSuppress()
{
    DECL_TRACER("TConfig::getToolbarSuppress()");

    return mTemporary ? localSettings_temp.tbsuppress : localSettings.tbsuppress;
}

bool TConfig::getProfiling()
{
    return mTemporary ? localSettings_temp.profiling : localSettings.profiling;
}

size_t TConfig::getButttonCache()
{
    if (mTemporary && localSettings_temp.max_cache > 0)
        return localSettings_temp.max_cache;

    if (!mTemporary && localSettings.max_cache > 0)
        return localSettings.max_cache * 1000 * 1000;

    return 0;
}

string & TConfig::getPassword1()
{
    DECL_TRACER("TConfig::getPassword1()");

    return mTemporary ? localSettings_temp.password1 : localSettings.password1;
}

string & TConfig::getPassword2()
{
    DECL_TRACER("TConfig::getPassword2()");

    return mTemporary ? localSettings_temp.password2 : localSettings.password2;
}

string & TConfig::getPassword3()
{
    DECL_TRACER("TConfig::getPassword3()");

    return mTemporary ? localSettings_temp.password3 : localSettings.password3;
}

string & TConfig::getPassword4()
{
    DECL_TRACER("TConfig::getPassword4()");

    return mTemporary ? localSettings_temp.password4 : localSettings.password4;
}

string & TConfig::getSystemSound()
{
    DECL_TRACER("TConfig::getSystemSound()");

    return mTemporary ? localSettings_temp.systemSound : localSettings.systemSound;
}

bool TConfig::getSystemSoundState()
{
    DECL_TRACER("TConfig::getSystemSoundState()");

    return mTemporary ? localSettings_temp.systemSoundState : localSettings.systemSoundState;
}

int TConfig::getSystemVolume()
{
    DECL_TRACER("TConfig::getSystemVolume()");

    return mTemporary ? localSettings_temp.systemVolume : localSettings.systemVolume;
}

int TConfig::getSystemGain()
{
    DECL_TRACER("TConfig::getSystemGain()");

    return mTemporary ? localSettings_temp.systemGain : localSettings.systemGain;
}

bool TConfig::getRotationFixed()
{
    DECL_TRACER("TConfig::getRotationFixed()");

    return mTemporary ? localSettings_temp.systemRotationFix : localSettings.systemRotationFix;
}

void TConfig::setRotationFixed(bool fix)
{
    DECL_TRACER("TConfig::setRotationFixed(bool fix)");

    if (mTemporary)
        localSettings_temp.systemRotationFix = fix;
    else
        localSettings.systemRotationFix = fix;

    mTemporary = false;
}

void TConfig::setSystemChannel(int ch)
{
    DECL_TRACER("TConfig::setSystemChannel(int ch)");

    if (ch >= 10000 && ch < 30000)
        mChannel = ch;
}

string& TConfig::getSingleBeepSound()
{
    DECL_TRACER("TConfig::getSingleBeepSound()");

    return mTemporary ? localSettings_temp.systemSingleBeep : localSettings.systemSingleBeep;
}

string& TConfig::getDoubleBeepSound()
{
    DECL_TRACER("TConfig::getDoubleBeepSound()");

    return mTemporary ? localSettings_temp.systemDoubleBeep : localSettings.systemDoubleBeep;
}

string& TConfig::getUUID()
{
    DECL_TRACER("TConfig::getUUID()");

    return mTemporary ? localSettings_temp.uuid : localSettings.uuid;
}

string& TConfig::getFtpUser()
{
    DECL_TRACER("TConfig::getFtpUser()");

    return mTemporary ? localSettings_temp.ftpUser : localSettings.ftpUser;
}

string& TConfig::getFtpPassword()
{
    DECL_TRACER("TConfig::getFtpPassword()");

    return mTemporary ? localSettings_temp.ftpPassword : localSettings.ftpPassword;
}

string& TConfig::getFtpSurface()
{
    DECL_TRACER("TConfig::getFtpSurface()");

    return mTemporary ? localSettings_temp.ftpSurface : localSettings.ftpSurface;
}

bool TConfig::getFtpPassive()
{
    DECL_TRACER("TConfig::getFtpPassive()");

    return mTemporary ? localSettings_temp.ftpPassive : localSettings.ftpPassive;
}

time_t TConfig::getFtpDownloadTime()
{
    DECL_TRACER("TConfig::getFtpDownloadTime()");

    return mTemporary ? localSettings_temp.ftpLastDownload : localSettings.ftpLastDownload;
}

/**
 * @brief TConfig::certCheck check the certificate if the connection is encrypted.
 *
 * Currently not implemented!
 *
 * @return `true` = check the certificate, `false` = accept any certificate (default).
 */
bool TConfig::certCheck()
{
    return mTemporary ? localSettings_temp.certCheck : localSettings.certCheck;
}

/**
 * @brief TConfig::logLevelStrToBits
 * Converts a string containing one or more loglevels into a bit field.
 * @param level A string containing the log levels
 * @return A bit field representing the log levels.
 */
uint TConfig::logLevelStrToBits(const string& level)
{
    uint l = 0;

    if (level.find("INFO") != string::npos)
        l |= HLOG_INFO;

    if (level.find("WARNING") != string::npos)
        l |= HLOG_WARNING;

    if (level.find("ERROR") != string::npos)
        l |= HLOG_ERROR;

    if (level.find("TRACE") != string::npos)
        l |= HLOG_TRACE;

    if (level.find("DEBUG") != string::npos)
        l |= HLOG_DEBUG;

    if (level.find("PROTOCOL") != string::npos)
        l |= HLOG_PROTOCOL;

    if (level.find("ALL") != string::npos)
        l = HLOG_ALL;

    return l;
}

string TConfig::logLevelBitsToString(uint level)
{
    string l;

    if (level == 0)
    {
        l = SLOG_NONE;
        return l;
    }

    if (level & HLOG_INFO)
    {
        if (l.length() > 0)
            l.append("|");

        l.append(SLOG_INFO);
    }

    if (level & HLOG_WARNING)
    {
        if (l.length() > 0)
            l.append("|");

        l.append(SLOG_WARNING);
    }

    if (level & HLOG_ERROR)
    {
        if (l.length() > 0)
            l.append("|");

        l.append(SLOG_ERROR);
    }

    if (level & HLOG_TRACE)
    {
        if (l.length() > 0)
            l.append("|");

        l.append(SLOG_TRACE);
    }

    if (level & HLOG_DEBUG)
    {
        if (l.length() > 0)
            l.append("|");

        l.append(SLOG_DEBUG);
    }

    return l;
}

string TConfig::sipFirewallToString(TConfig::SIP_FIREWALL_t fw)
{
    switch(fw)
    {
        case SIP_NO_FIREWALL:   return "SIP_NO_FIREWALL";
        case SIP_NAT_ADDRESS:   return "SIP_NAT_ADDRESS";
        case SIP_STUN:          return "SIP_STUN";
        case SIP_ICE:           return "SIP_ICE";
        case SIP_UPNP:          return "SIP_UPNP";
    }

    return string();
}

TConfig::SIP_FIREWALL_t TConfig::sipFirewallStrToEnum(const std::string& str)
{
    if (strCaseCompare(str, "SIP_NO_FIREWALL") == 0)
        return SIP_NO_FIREWALL;
    else if (strCaseCompare(str, "SIP_NAT_ADDRESS") == 0)
        return SIP_NAT_ADDRESS;
    else if (strCaseCompare(str, "SIP_STUN") == 0)
        return SIP_STUN;
    else if (strCaseCompare(str, "SIP_ICE") == 0)
        return SIP_ICE;
    else if (strCaseCompare(str, "SIP_UPNP") == 0)
        return SIP_UPNP;

    return SIP_NO_FIREWALL;
}

string TConfig::makeConfigDefault(const std::string& log, const std::string& project)
{
    string content = "LogFile=" + log + "\n";
#ifndef NDEBUG
    content += "LogLevel=ALL\n";
#else
    content += "LogLevel=NONE\n";
#endif
    content += "ProjectPath=" + project + "\n";
    content += "LongFormat=false\n";
    content += "Address=0.0.0.0\n";
    content += "Port=1319\n";
    content += "Channel=10001\n";
    content += "PanelType=Android\n";
    content += string("Firmware=") + VERSION_STRING() + "\n";
    content += "Scale=true\n";
    content += "ToolbarForce=true\n";
    content += "Profiling=false\n";
    content += "Password1=1988\n";
    content += "Password2=1988\n";
    content += "Password3=1988\n";
    content += "Password4=1988\n";
    content += "SystemSoundFile=singleBeep.wav\n";
    content += "SystemSoundState=ON\n";
    content += "SystemSingleBeep=singleBeep01.wav\n";
    content += "SystemDoubleBeep=doubleBeep01.wav\n";
    content += "SystemRotationFix=" + string(localSettings.systemRotationFix ? "TRUE" : "FALSE") + "\n";
    content += "UUID=" + localSettings.uuid + "\n";
    content += "FTPuser=administrator\n";
    content += "FTPpassword=password\n";
    content += "FTPsurface=tpanel.tp4\n";
    content += "FTPpassive=true\n";
    content += "FTPdownloadTime=0\n";
    content += "SIP_PROXY=" + localSettings.sip_proxy + "\n";
    content += "SIP_PORT=" + std::to_string(localSettings.sip_port) + "\n";
    content += "SIP_PORTTLS=" + std::to_string(localSettings.sip_portTLS) + "\n";
    content += "SIP_STUN=" + localSettings.sip_stun + "\n";
    content += "SIP_DOMAIN=" + localSettings.sip_domain + "\n";
    content += "SIP_USER=" + localSettings.sip_user + "\n";
    content += "SIP_PASSWORD=" + localSettings.sip_password + "\n";
    content += "SIP_IPV4=" + string(localSettings.sip_ipv4 ? "TRUE" : "FALSE") + "\n";
    content += "SIP_IPV6=" + string(localSettings.sip_ipv6 ? "TRUE" : "FALSE") + "\n";
    content += "SIP_IPHONE=" + string(localSettings.sip_iphone ? "TRUE" : "FALSE") + "\n";
    content += "SIP_FIREWALL=" + sipFirewallToString(localSettings.sip_firewall) + "\n";
    content += "SIP_ENABLED=" + string(localSettings.sip_ipv6 ? "TRUE" : "FALSE") + "\n";

    return content;
}

/**
 * @brief TConfig::findConfig search for the location of the configuration file.
 *
 * If there was no configuration file given on the command line, this method
 * searches for a configuration file on a few standard directories. This are:
 *
 *     /etc/tpanel.conf
 *     /etc/tpanel/tpanel.conf
 *     /usr/etc/tpanel.conf
 *     $HOME/.tpanel.conf
 *
 * On macOS the following additional directories are searched:
 *
 *     /opt/local/etc/tpanel.conf
 *     /opt/local/etc/tpanel/tpanel.conf
 *     /opt/local/usr/etc/tpanel.conf
 *
 * @return On success `true`, otherwise `false`.
 */
bool TConfig::findConfig()
{
    TLock<std::mutex> guard(config_mutex);

    char *HOME = nullptr;
    string sFileName;

    if (!mPath.empty())
    {
        size_t pos = mPath.find_last_of("/");

        if (pos != string::npos)
        {
            localSettings.path = mPath.substr(0, pos);
            localSettings.name = mPath.substr(pos+1);
            localSettings_temp.path = localSettings.path;
            localSettings_temp.name = localSettings.name;
            mCFile = mPath;
            return !mCFile.empty();
        }

        localSettings.path = ".";
        localSettings.name = mPath;
        localSettings_temp.path = localSettings.path;
        localSettings_temp.name = localSettings.name;
        mCFile = mPath;
        return !mCFile.empty();
    }

    localSettings.name = "tpanel.conf";
    localSettings.password1 = "1988";
    localSettings.password2 = "1988";
    localSettings.password3 = "1988";
    localSettings.password4 = "1988";
    localSettings.systemSound = "singleBeep.wav";
    localSettings.systemSoundState = true;
    localSettings.systemSingleBeep = "singleBeep01.wav";
    localSettings.systemDoubleBeep = "doubleBeep01.wav";
    localSettings.ftpUser = "administrator";
    localSettings.ftpPassword = "password";
    localSettings.ftpSurface = "tpanel.tp4";
#ifdef __ANDROID__
    std::stringstream s;
    TValidateFile vf;
    HOME = getenv("HOME");

    if (!HOME || !*HOME)
    {
        s << "Error: Environment variable HOME does not exist!";
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "%s", s.str().c_str());
        TError::setErrorMsg(s.str());
        return false;
    }

    localSettings.path = HOME;

    if (!vf.isValidDir(localSettings.path))
    {
        s << "Error: Path " << localSettings.path << " does not exist!";
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "%s", s.str().c_str());
        TError::setErrorMsg(s.str());
        return false;
    }

    sFileName = localSettings.path + "/" + localSettings.name;

    if (access(sFileName.c_str(), F_OK) == -1)    // Does the configuration file exist?
    {                                       // No, than create it and also the directory structure
        try
        {
            ofstream cfg(sFileName);

            string content = makeConfigDefault(localSettings.path + "/tpanel.log", localSettings.path + "/tpanel");
            cfg.write(content.c_str(), content.size());
            cfg.close();

            string path = localSettings.path + "/tpanel";
            TTPInit init(path);
        }
        catch (std::exception& e)
        {
            s << "Error: " << e.what();
            __android_log_print(ANDROID_LOG_ERROR, "tpanel" , "%s", s.str().c_str());
            TError::setErrorMsg(TERRERROR, string("Error: ") + e.what());
            return false;
        }
    }
#else
    if (!(HOME = getenv("HOME")))
        MSG_ERROR("TConfig::findConfig: No environment variable HOME!");

    vector<string>::iterator iter;
    bool found = false;

    for (iter = mCfgPaths.begin(); iter != mCfgPaths.end(); ++iter)
    {
        string f = *iter + "/tpanel.conf";

        if (!access(f.c_str(), R_OK))
        {
            sFileName = f;
            localSettings.path = *iter;
            found = true;
            break;
        }
    }

    // The local configuration file has precedence over the global one.
    if (HOME)
    {
        string f = HOME;
#ifndef __ANDROID__
        f += "/.tpanel.conf";
#else
        f += "/tpanel.conf";
#endif
        if (!access(f.data(), R_OK))
        {
            sFileName = f;
            localSettings.path = HOME;
            found = true;
        }
    }

    localSettings_temp = localSettings;

    if (!found)
    {
        MSG_WARNING("This seems to be the first start because of missing configuration file. Will try to create a default one ...");

        if (HOME)
        {
            localSettings.path = HOME;
            sFileName = string(HOME) + "/.tpanel.conf";

            try
            {
                ofstream cfg(sFileName);

                string content = makeConfigDefault(localSettings.path + "/tpanel.log", localSettings.path + "/tpanel");
                cfg.write(content.c_str(), content.size());
                cfg.close();

                localSettings_temp = localSettings;
                string path = localSettings.path + "/tpanel";
                TTPInit init(path);
            }
            catch (std::exception& e)
            {
                cerr << "Error: " << e.what();
                TError::setErrorMsg(TERRERROR, string("Error: ") + e.what());
                return false;
            }
        }
        else
        {
            MSG_ERROR("TConfig::findConfig: Can't find any configuration file!");
            TError::setError();
            sFileName.clear();
            localSettings.name.clear();
            localSettings.path.clear();
            localSettings_temp = localSettings;
        }
    }
#endif
    mCFile = sFileName;
    return !sFileName.empty();
}

/**
 * @brief TConfig::readConfig reads a config file.
 *
 * This method reads a config file and stores the known options into the
 * struct localSettings.
 *
 * @return `true` on success.\n
 * Returns `false` on error and sets the internal error.
 */
bool TConfig::readConfig()
{
#if defined(__APPLE__) && (TARGET_OS_IOS || TARGET_OS_SIMULATOR)
    QASettings settings;
    localSettings.path = QASettings::getLibraryPath().toStdString();
    localSettings.project = localSettings.path + "/tpanel";
    localSettings.version = "1.0";
    localSettings.ptype = "iPhone";
    localSettings.max_cache = 100;
    localSettings.logLevel = SLOG_NONE;
    localSettings.logLevelBits = HLOG_NONE;
    mkdir(localSettings.project.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    localSettings.name = "tpanel.cfg";
    localSettings.scale = true;
    localSettings.tbforce = false;
    localSettings.tbsuppress = false;

    // Settings for NetLinx
    settings.registerDefaultPrefs();
    localSettings.server = settings.getNetlinxIP().toStdString();
    localSettings.port = settings.getNetlinxPort();
    localSettings.ID = settings.getNetlinxChannel();
    localSettings.ptype = settings.getNetlinxPanelType().toStdString();
    localSettings.ftpUser = settings.getFTPUser().toStdString();
    localSettings.ftpPassword = settings.getFTPPassword().toStdString();
    localSettings.ftpSurface = settings.getNetlinxSurface().toStdString();

    // Settings for SIP
    localSettings.sip_proxy = settings.getSipProxy().toStdString();
    localSettings.sip_port = settings.getSipNetworkPort();
    localSettings.sip_portTLS = settings.getSipNetworkTlsPort();
    localSettings.sip_stun = settings.getSipStun().toStdString();
    localSettings.sip_domain = settings.getSipDomain().toStdString();
    localSettings.sip_ipv4 = settings.getSipNetworkIpv4();
    localSettings.sip_ipv6 = settings.getSipNetworkIpv6();
    localSettings.sip_user = settings.getSipUser().toStdString();
    localSettings.sip_password = settings.getSipPassword().toStdString();
    localSettings.sip_enabled = settings.getSipEnabled();
    localSettings.sip_iphone = settings.getSipIntegratedPhone();

    // Settings for View
    localSettings.scale = settings.getViewScale();
    localSettings.tbforce = settings.getViewToolbarForce();
    localSettings.tbsuppress = !settings.getViewToolbarVisible();

    if (localSettings.tbforce)
        localSettings.tbsuppress = false;

    localSettings.systemRotationFix = settings.getViewRotation();

    // Settings for sound
    localSettings.systemSound = settings.getSoundSystem().toStdString();
    localSettings.systemSingleBeep = settings.getSoundSingleBeep().toStdString();
    localSettings.systemDoubleBeep = settings.getSoundDoubleBeep().toStdString();
    localSettings.systemSoundState = settings.getSoundEnabled();
    localSettings.systemVolume = settings.getSoundVolume();
    localSettings.systemGain = settings.getSoundGain();

    // Settings for logging
    unsigned int logLevel = 0;

    if (settings.getLoggingInfo())
        logLevel |= HLOG_INFO;

    if (settings.getLoggingWarning())
        logLevel |= HLOG_WARNING;

    if (settings.getLoggingError())
        logLevel |= HLOG_ERROR;

    if (settings.getLoggingDebug())
        logLevel |= HLOG_DEBUG;

    if (settings.getLoggingTrace())
        logLevel |= HLOG_TRACE;

    if (settings.getLoggingProfile())
        localSettings.profiling = true;
    else
        localSettings.profiling = false;

    if (settings.getLoggingLogFormat())
        localSettings.longformat = true;
    else
        localSettings.longformat = false;

    localSettings.logLevelBits = logLevel;
    localSettings.logLevel = logLevelBitsToString(logLevel);
    TStreamError::setLogLevel(localSettings.logLevel);

    if (settings.getLoggingLogfileEnabled())
    {
        string fname = settings.getLoggingLogfile().toStdString();

        if (!fname.empty())
        {
            localSettings.logFile = QASettings::getDocumentPath().toStdString() + "/" + fname;
            TStreamError::setLogFile(localSettings.logFile);
        }
    }
    else
        localSettings.logFile.clear();

    if (localSettings.uuid.empty())
    {
        uuid_t uuid;
        char sUUID[256];

        uuid_generate_random(uuid);
        uuid_unparse_lower(uuid, sUUID);
        localSettings.uuid.assign(sUUID);
        localSettings_temp = localSettings;
    }

    mInitialized = true;

    if (IS_LOG_DEBUG())
    {
        cout << "Selected Parameters:" << endl;
        cout << "    Path to cfg.: " << localSettings.path << endl;
        cout << "    Name of cfg.: " << localSettings.name << endl;
        cout << "    Logfile use:  " << (settings.getLoggingLogfileEnabled() ? "YES": "NO") << endl;
        cout << "    Logfile:      " << localSettings.logFile  << endl;
        cout << "    LogLevel:     " << localSettings.logLevel  << endl;
        cout << "    Long format:  " << (localSettings.longformat ? "YES" : "NO")  << endl;
        cout << "    Project path: " << localSettings.project  << endl;
        cout << "    Controller:   " << localSettings.server  << endl;
        cout << "    Port:         " << localSettings.port  << endl;
        cout << "    Channel:      " << localSettings.ID  << endl;
        cout << "    Panel type:   " << localSettings.ptype  << endl;
        cout << "    Firmware ver. " << localSettings.version  << endl;
        cout << "    Scaling:      " << (localSettings.scale ? "YES" : "NO")  << endl;
        cout << "    Profiling:    " << (localSettings.profiling ? "YES" : "NO")  << endl;
        cout << "    Button cache: " << localSettings.max_cache  << endl;
        cout << "    System Sound: " << localSettings.systemSound  << endl;
        cout << "    Sound state:  " << (localSettings.systemSoundState ? "ACTIVATED" : "DEACTIVATED")  << endl;
        cout << "    Single beep:  " << localSettings.systemSingleBeep  << endl;
        cout << "    Double beep:  " << localSettings.systemDoubleBeep  << endl;
        cout << "    Volume:       " << localSettings.systemVolume  << endl;
        cout << "    Gain:         " << localSettings.systemGain  << endl;
        cout << "    Rotation:     " << (localSettings.systemRotationFix ? "LOCKED" : "UNLOCKED")  << endl;
        cout << "    UUID:         " << localSettings.uuid  << endl;
        cout << "    FTP user:     " << localSettings.ftpUser  << endl;
        cout << "    FTP password: " << localSettings.ftpPassword  << endl;
        cout << "    FTP surface:  " << localSettings.ftpSurface  << endl;
        cout << "    FTP passive:  " << (localSettings.ftpPassive ? "YES" : "NO")  << endl;
        cout << "    FTP dl. time: " << localSettings.ftpLastDownload  << endl;
        cout << "    SIP proxy:    " << localSettings.sip_proxy  << endl;
        cout << "    SIP port:     " << localSettings.sip_port  << endl;
        cout << "    SIP TLS port: " << localSettings.sip_portTLS  << endl;
        cout << "    SIP STUN:     " << localSettings.sip_stun  << endl;
        cout << "    SIP doamain:  " << localSettings.sip_domain  << endl;
        cout << "    SIP user:     " << localSettings.sip_user  << endl;
        cout << "    SIP IPv4:     " << (localSettings.sip_ipv4 ? "YES" : "NO")  << endl;
        cout << "    SIP IPv6:     " << (localSettings.sip_ipv6 ? "YES" : "NO")  << endl;
        cout << "    SIP Int.Phone:" << (localSettings.sip_iphone ? "YES" : "NO")  << endl;
        cout << "    SIP firewall: " << sipFirewallToString(localSettings.sip_firewall)  << endl;
        cout << "    SIP enabled:  " << (localSettings.sip_enabled ? "YES" : "NO")  << endl;
    }
#else
    ifstream fs;

    mTemporary = false;
    // First initialize the defaults
    localSettings.ID = 0;
    localSettings.port = 1397;
    localSettings.ptype = "android";
    localSettings.version = "1.0";
    localSettings.longformat = false;
    localSettings.profiling = false;
#ifdef __ANDROID__
    localSettings.max_cache = 100;
#else
    localSettings.max_cache = 400;
#endif
    localSettings.systemSoundState = true;
    localSettings.systemSound = "singleBeep.wav";
    localSettings.systemSingleBeep = "singleBeep01.wav";
    localSettings.systemDoubleBeep = "doubleBeep01.wav";
    localSettings.ftpUser = "administrator";
    localSettings.ftpPassword = "password";
    localSettings.ftpSurface = "tpanel.tp4";
    localSettings.sip_port = 5060;
    localSettings.sip_portTLS = 0;

    // Now get the settings from file
    try
    {
        fs.open(mCFile.c_str(), fstream::in);
    }
    catch (const fstream::failure e)
    {
        localSettings_temp = localSettings;
        cerr << "TConfig::readConfig: Error on file " << mCFile << ": " << e.what() << endl;
        TError::setError();
        return false;
    }

    for (string line; getline(fs, line);)
    {
        size_t pos;

        if ((pos = line.find("#")) != string::npos)
        {
            if (pos == 0)
                line.clear();
            else
                line = line.substr(0, pos);
        }

        if (line.empty() || line.find("=") == string::npos)
            continue;

        vector<string> parts = split(line, "=", true);

        if (parts.size() == 2)
        {
            string left = parts[0];
            string right = ltrim(parts[1]);

            if (caseCompare(left, "PORT") == 0 && !right.empty())
                localSettings.port = atoi(right.c_str());
            else if (caseCompare(left, "LOGFILE") == 0 && !right.empty())
            {
                localSettings.logFile = right;
                TStreamError::setLogFileOnly(right);
            }
            else if (caseCompare(left, "LOGLEVEL") == 0 && !right.empty())
            {
                TStreamError::setLogLevel(right);
                localSettings.logLevel = right;
                localSettings.logLevelBits = logLevelStrToBits(right);
            }
            else if (caseCompare(left, "ProjectPath") == 0 && !right.empty())
            {
                localSettings.project = right;
#ifdef __ANDROID__
                TValidateFile vf;

                if (!vf.isValidFile(right))
                {
                    char *HOME = getenv("HOME");

                    if (HOME)
                    {
                        localSettings.project = HOME;
                        localSettings.project += "/tpanel";
                    }
                }
#endif
            }
            else if (caseCompare(left, "System") == 0 && !right.empty())
                localSettings.system = atoi(right.c_str());
            else if (caseCompare(left, "PanelType") == 0 && !right.empty())
                localSettings.ptype = right;
            else if (caseCompare(left, "Address") == 0 && !right.empty())
                localSettings.server = right;
            else if (caseCompare(left, "Firmware") == 0 && !right.empty())
                localSettings.version = right;
            else if (caseCompare(left, "LongFormat") == 0 && !right.empty())
                localSettings.longformat = isTrue(right);
            else if (caseCompare(left, "NoBanner") == 0 && !right.empty())
                localSettings.noBanner = isTrue(right);
            else if (caseCompare(left, "CertCheck") == 0 && !right.empty())
                localSettings.certCheck = isTrue(right);
            else if (caseCompare(left, "Channel") == 0 && !right.empty())
            {
                localSettings.ID = atoi(right.c_str());

                if (localSettings.ID < 10000 || localSettings.ID >= 29000)
                {
#ifdef __ANDROID__
                    __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TConfig::readConfig: Invalid port number %s", right.c_str());
#else
                    cerr << "TConfig::readConfig: Invalid port number " << right << endl;
#endif
                    localSettings.ID = 0;
                }

                mChannel = localSettings.ID;
            }
            else if (caseCompare(left, "Scale") == 0 && !right.empty())
                localSettings.scale = isTrue(right);
            else if (caseCompare(left, "ToolbarForce") == 0 && !right.empty())
                localSettings.tbforce = isTrue(right);
            else if (caseCompare(left, "ToolbarSuppress") == 0 && !right.empty())
                localSettings.tbsuppress = isTrue(right);
            else if (caseCompare(left, "Profiling") == 0 && !right.empty())
                localSettings.profiling = isTrue(right);
            else if (caseCompare(left, "MaxButtonCache") == 0 && !right.empty())
                localSettings.max_cache = atoi(right.c_str());
            else if (caseCompare(left, "Password1") == 0 && !right.empty())
                localSettings.password1 = right;
            else if (caseCompare(left, "Password2") == 0 && !right.empty())
                localSettings.password2 = right;
            else if (caseCompare(left, "Password3") == 0 && !right.empty())
                localSettings.password3 = right;
            else if (caseCompare(left, "Password4") == 0 && !right.empty())
                localSettings.password4 = right;
            else if (caseCompare(left, "SystemSoundFile") == 0 && !right.empty())
                localSettings.systemSound = right;
            else if (caseCompare(left, "SystemSoundState") == 0 && !right.empty())
                localSettings.systemSoundState = isTrue(right);
            else if (caseCompare(left, "SystemSingleBeep") == 0 && !right.empty())
                localSettings.systemSingleBeep = right;
            else if (caseCompare(left, "SystemDoubleBeep") == 0 && !right.empty())
                localSettings.systemDoubleBeep = right;
            else if (caseCompare(left, "SystemVolume") == 0 && !right.empty())
            {
                int volume = atoi(right.c_str());

                if (volume < 0)
                    volume = 0;
                else if (volume > 100)
                    volume = 100;

                localSettings.systemVolume = volume;
            }
            else if (caseCompare(left, "SystemGain") == 0 && !right.empty())
            {
                int gain = atoi(right.c_str());

                if (gain < 0)
                    gain = 0;
                else if (gain > 100)
                    gain = 100;

                localSettings.systemGain = gain;
            }
            else if (caseCompare(left, "SystemRotationFix") == 0 && !right.empty())
                localSettings.systemRotationFix = isTrue(right);
            else if (caseCompare(left, "UUID") == 0 && !right.empty())
                localSettings.uuid = right;
            else if (caseCompare(left, "FTPuser") == 0 && !right.empty())       // FTP credentials
                localSettings.ftpUser = right;
            else if (caseCompare(left, "FTPpassword") == 0 && !right.empty())
                localSettings.ftpPassword = right;
            else if (caseCompare(left, "FTPsurface") == 0 && !right.empty())
                localSettings.ftpSurface = right;
            else if (caseCompare(left, "FTPpassive") == 0 && !right.empty())
                localSettings.ftpPassive = isTrue(right);
            else if (caseCompare(left, "FTPdownloadTime") == 0 && !right.empty())
                localSettings.ftpLastDownload = atol(right.c_str());
            else if (caseCompare(left, "SIP_PROXY") == 0 && !right.empty())     // SIP settings starting here
                localSettings.sip_proxy = right;
            else if (caseCompare(left, "SIP_PORT") == 0 && !right.empty())
                localSettings.sip_port = atoi(right.c_str());
            else if (caseCompare(left, "SIP_PORTTLS") == 0 && !right.empty())
                localSettings.sip_portTLS = atoi(right.c_str());
            else if (caseCompare(left, "SIP_STUN") == 0 && !right.empty())
                localSettings.sip_stun = right;
            else if (caseCompare(left, "SIP_DOMAIN") == 0 && !right.empty())
                localSettings.sip_domain = right;
            else if (caseCompare(left, "SIP_USER") == 0 && !right.empty())
                localSettings.sip_user = right;
            else if (caseCompare(left, "SIP_PASSWORD") == 0 && !right.empty())
                localSettings.sip_password = right;
            else if (caseCompare(left, "SIP_IPV4") == 0 && !right.empty())
                localSettings.sip_ipv4 = isTrue(right);
            else if (caseCompare(left, "SIP_IPV6") == 0 && !right.empty())
                localSettings.sip_ipv6 = isTrue(right);
            else if (caseCompare(left, "SIP_IPHONE") == 0 && !right.empty())
                localSettings.sip_iphone = isTrue(right);
            else if (caseCompare(left, "SIP_FIREWALL") == 0 && !right.empty())
                localSettings.sip_firewall = sipFirewallStrToEnum(right);
            else if (caseCompare(left, "SIP_ENABLED") == 0 && !right.empty())
                localSettings.sip_enabled = isTrue(right);
        }
    }

    fs.close();
    mInitialized = true;
    TStreamError::setLogLevel(localSettings.logLevel);
    TStreamError::setLogFile(localSettings.logFile);

    if (localSettings.uuid.empty())
    {
#ifdef __ANDROID__
        QUuid qUid = QUuid::createUuid();
        localSettings.uuid = qUid.toString().toStdString();
#else
        uuid_t uuid;
        char sUUID[256];

        uuid_generate_random(uuid);
        uuid_unparse_lower(uuid, sUUID);
        localSettings.uuid.assign(sUUID);
#endif
        localSettings_temp = localSettings;
        saveSettings();
    }

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        MSG_INFO("Selected Parameters:");
        MSG_INFO("    Path to cfg.: " << localSettings.path);
        MSG_INFO("    Name of cfg.: " << localSettings.name);
#ifndef __ANDROID__
        MSG_INFO("    Logfile:      " << localSettings.logFile);
#endif
        MSG_INFO("    LogLevel:     " << localSettings.logLevel);
        MSG_INFO("    Long format:  " << (localSettings.longformat ? "YES" : "NO"));
        MSG_INFO("    Project path: " << localSettings.project);
#ifndef __ANDROID__
        MSG_INFO("    Show banner:  " << (localSettings.noBanner ? "NO" : "YES"));
#endif
        MSG_INFO("    Controller:   " << localSettings.server);
        MSG_INFO("    Port:         " << localSettings.port);
        MSG_INFO("    Channel:      " << localSettings.ID);
        MSG_INFO("    Panel type:   " << localSettings.ptype);
        MSG_INFO("    Firmware ver. " << localSettings.version);
        MSG_INFO("    Scaling:      " << (localSettings.scale ? "YES" : "NO"));
        MSG_INFO("    Profiling:    " << (localSettings.profiling ? "YES" : "NO"));
        MSG_INFO("    Button cache: " << localSettings.max_cache);
        MSG_INFO("    System Sound: " << localSettings.systemSound);
        MSG_INFO("    Sound state:  " << (localSettings.systemSoundState ? "ACTIVATED" : "DEACTIVATED"));
        MSG_INFO("    Single beep:  " << localSettings.systemSingleBeep);
        MSG_INFO("    Double beep:  " << localSettings.systemDoubleBeep);
        MSG_INFO("    Volume:       " << localSettings.systemVolume);
        MSG_INFO("    Gain:         " << localSettings.systemGain);
        MSG_INFO("    Rotation:     " << (localSettings.systemRotationFix ? "LOCKED" : "UNLOCKED"));
        MSG_INFO("    UUID:         " << localSettings.uuid);
        MSG_INFO("    FTP user:     " << localSettings.ftpUser);
        MSG_INFO("    FTP password: " << localSettings.ftpPassword);
        MSG_INFO("    FTP surface:  " << localSettings.ftpSurface);
        MSG_INFO("    FTP passive:  " << (localSettings.ftpPassive ? "YES" : "NO"));
        MSG_INFO("    FTP dl. time: " << localSettings.ftpLastDownload);
        MSG_INFO("    SIP proxy:    " << localSettings.sip_proxy);
        MSG_INFO("    SIP port:     " << localSettings.sip_port);
        MSG_INFO("    SIP TLS port: " << localSettings.sip_portTLS);
        MSG_INFO("    SIP STUN:     " << localSettings.sip_stun);
        MSG_INFO("    SIP doamain:  " << localSettings.sip_domain);
        MSG_INFO("    SIP user:     " << localSettings.sip_user);
        MSG_INFO("    SIP IPv4:     " << (localSettings.sip_ipv4 ? "YES" : "NO"));
        MSG_INFO("    SIP IPv6:     " << (localSettings.sip_ipv6 ? "YES" : "NO"));
        MSG_INFO("    SIP Int.Phone:" << (localSettings.sip_iphone ? "YES" : "NO"));
        MSG_INFO("    SIP firewall: " << sipFirewallToString(localSettings.sip_firewall));
        MSG_INFO("    SIP enabled:  " << (localSettings.sip_enabled ? "YES" : "NO"));
    }

    localSettings_temp = localSettings;
#endif
    return true;
}

/**
 * @brief TConfig::split splitts a string into parts.
 *
 * The method splitts a string into parts separated by \p seps. It puts the
 * parts into a vector array.
 *
 * @param str           The string to split
 * @param seps          The separator(s)
 * @param trimEmpty     `true` = trum the parts.
 *
 * @return A vector array containing the parts of the string \p str.
 */
vector<string> TConfig::split(const string& str, const string& seps, const bool trimEmpty)
{
    DECL_TRACER("TConfig::split(const string& str, const string& seps, const bool trimEmpty)");

	size_t pos = 0, mark = 0;
	vector<string> parts;

	for (auto it = str.begin(); it != str.end(); ++it)
	{
		for (auto sepIt = seps.begin(); sepIt != seps.end(); ++sepIt)
		{
			if (*it == *sepIt)
			{
				size_t len = pos - mark;
				parts.push_back(str.substr(mark, len));
				mark = pos + 1;
				break;
			}
		}

		pos++;
	}

	parts.push_back(str.substr(mark));

	if (trimEmpty)
	{
		vector<string> nparts;

		for (auto it = parts.begin(); it != parts.end(); ++it)
		{
			if (it->empty())
				continue;

			nparts.push_back(*it);
		}

		return nparts;
	}

	return parts;
}

/**
 * @brief TConfig::caseCompare compares 2 strings
 *
 * This method compares 2 strings case insensitive. This means that it ignores
 * the case of the letters. For example:
 *
 *     BLAME
 *     blame
 *     Blame
 *
 * are all the same and would return 0, which means _equal_.
 *
 * @param str1  1st string to compare
 * @param str2  2nd string to compare
 *
 * @return 0 if the strings are equal\n
 * less than 0 if the byte of \p str1 is bigger than the byte of \p str2\n
 * grater than 0 if the byte of \p str1 is smaller than the byte of \p str2.
 */
int TConfig::caseCompare(const string& str1, const string& str2)
{
    DECL_TRACER("TConfig::caseCompare(const string& str1, const string& str2)");

	size_t i = 0;

	if (str1.length() != str2.length())
		return ((str1.length() < str2.length()) ? -1 : 1);

	for (auto it = str1.begin(); it != str1.end(); ++it)
	{
		if (tolower(*it) != tolower(str2.at(i)))
			return (int)(*it - str2.at(i));

		i++;
	}

	return 0;
}

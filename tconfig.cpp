/*
 * Copyright (C) 2020, 2021 by Andreas Theofilu <andreas@theosys.at>
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
#include <android/log.h>
#include "tvalidatefile.h"
#include "ttpinit.h"
#include "tvalidatefile.h"
#endif
#include "tconfig.h"
#include "terror.h"
#include "tresources.h"

using std::string;
using std::ifstream;
using std::ofstream;
using std::fstream;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;

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
    string logLevel;            //!< The log level(s).
    uint logLevelBits;          //!< The numeric bit field of the loglevel
    bool longformat{false};     //!< TRUE = long format
    bool noBanner{false};       //!< Startup without showing a banner on the command line.
    bool certCheck{false};      //!< TRUE = Check certificate for SSL connection
    bool scale{false};          //!< TRUE = Images are scaled to fit the whole screen
    bool profiling{false};      //!< TRUE = The declaration traces meassure the time and write it to the log
    string password1;           //!< First panel password
    string password2;           //!< Second panel password
    string password3;           //!< Third panel password
    string password4;           //!< Fourth panel password
    string systemSound;         //!< name of the set system sound played on every touch.
    bool systemSoundState{false};   //!< TRUE = play systemsound on every touch
    string systemSingleBeep;    //!< name of the system sound file to play a single beep.
    string systemDoubleBeep;    //!< name of the system sound file to play a double beep.
    // SIP settings
    string sip_proxy;           //!< The address of the SIP proxy
    int sip_port{5060};         //!< Initializes the port of the SIP proxy to 5060
    string sip_stun;            //!< STUN address
    string sip_domain;          //!< Local domain
    string sip_user;            //!< The SIP user to connect.
    string sip_password;        //!< The SIP password to connect. Note: This password is saved in plain text!
    bool sip_enabled{false};    //!< By default SIP is disabled
};

typedef struct SETTINGS settings_t;
static settings_t localSettings;    //!< Global defines settings used in class TConfig.

/**
 * @brief TConfig::TConfig constructor
 *
 * @param path  A path and name of a configuration file.
 */
TConfig::TConfig(const std::string& path)
	: mPath(path)
{
    // Initialize the possible configuration paths
    mCfgPaths.push_back("/etc");
    mCfgPaths.push_back("/etc/tpanel");
    mCfgPaths.push_back("/usr/etc");
    mCfgPaths.push_back("/usr/etc/tpanel");
#ifdef __APPLE__
    mCfgPath.push_back("/opt/local/etc");
    mCfgPath.push_back("/opt/local/etc/tpanel");
    mCfgPath.push_back("/opt/local/usr/etc");
    mCfgPath.push_back("/opt/local/usr/etc/tpanel");
#endif
    if (findConfig())
        readConfig();
}

bool TConfig::reReadConfig()
{
    return readConfig();
}

/**
 * @brief TConfig::setProgName Sets the name of the application.
 * @param pname The name of the application.
 */
void TConfig::setProgName(const std::string& pname)
{
	localSettings.pname = pname;
}

/**
 * @brief TConfig::getProgName Retrieves the prevously stored application name.
 * @return The name of this application.
 */
std::string & TConfig::getProgName()
{
	return localSettings.pname;
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
	return localSettings.ID;
}

/**
 * @brief TConfig::getConfigFileName returns the name of the configuration file.
 *
 * @return The name of the configuration file.
 */
std::string& TConfig::getConfigFileName()
{
	return localSettings.name;
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
	return localSettings.path;
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
	return localSettings.server;
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
    return localSettings.system;
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
	return localSettings.version;
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
	return localSettings.logFile;
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
	return localSettings.logLevel;
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
    return localSettings.logLevelBits;
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
	return localSettings.ptype;
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
	return localSettings.port;
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

string TConfig::getSystemPath(SYSTEMRESOURCE_t sres)
{
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

    if (file.empty() || localSettings.logFile.compare(file) == 0)
        return false;

    localSettings.logFile = file;
    return true;
}

bool TConfig::saveLogLevel(const string &level)
{
    DECL_TRACER("TConfig::saveLogLevel(const string &level)");

    if (level.find(SLOG_NONE) == string::npos && level.find(SLOG_INFO) == string::npos && level.find(SLOG_WARNING) == string::npos &&
            level.find(SLOG_ERROR) == string::npos && level.find(SLOG_TRACE) == string::npos && level.find(SLOG_DEBUG) == string::npos &&
            level.find(SLOG_PROTOCOL) == string::npos && level.find(SLOG_ALL) == string::npos)
        return false;

    localSettings.logLevel = level;
    localSettings.logLevelBits = logLevelStrToBits(level);
    MSG_INFO("New log level: " << level);
    return true;
}

bool TConfig::saveLogLevel(uint level)
{
    DECL_TRACER("TConfig::saveLogLevel(uint level)");

    if (level != 0 && !(level&HLOG_INFO) && !(level&HLOG_WARNING) &&
            !(level&HLOG_ERROR) && !(level&HLOG_TRACE) && !(level&HLOG_DEBUG))
        return false;

    localSettings.logLevelBits = level;
    localSettings.logLevel = logLevelBitsToString(level);
    MSG_INFO("New log level from bits: " << localSettings.logLevel);
    return true;
}

bool TConfig::saveChannel(int channel)
{
    DECL_TRACER("TConfig::saveChannel(int channel)");

    if (channel < 10000 || channel > 12000)
        return false;

    localSettings.ID = channel;
    return true;
}

bool TConfig::saveController(const string &cnt)
{
    DECL_TRACER("TConfig::saveController(const string &cnt)");

    localSettings.server = cnt;
    return true;
}

bool TConfig::savePanelType(const string &pt)
{
    DECL_TRACER("TConfig::savePanelType(const string &pt)");

    localSettings.ptype = pt;
    return true;
}

bool TConfig::savePort(int port)
{
    DECL_TRACER("TConfig::savePort(int port)");

    if (port < 1024 || port > 32767)
        return false;

    localSettings.port = port;
    return true;
}

bool TConfig::saveProjectPath(const string &path)
{
    DECL_TRACER("TConfig::saveProjectPath(const string &path)");

    if (path.empty())
        return false;

    localSettings.project = path;
    return true;
}

void TConfig::saveFormat(bool format)
{
    DECL_TRACER("TConfig::saveFormat(bool format)");

    localSettings.longformat = format;
}

void TConfig::saveScale(bool scale)
{
    DECL_TRACER("TConfig::saveScale(bool scale)");

    localSettings.scale = scale;
}

void TConfig::saveProfiling(bool prof)
{
    localSettings.profiling = prof;
}

void TConfig::savePassword1(const std::string& pw)
{
    localSettings.password1 = pw;
}

void TConfig::savePassword2(const std::string& pw)
{
    localSettings.password2 = pw;
}

void TConfig::savePassword3(const std::string& pw)
{
    localSettings.password3 = pw;
}

void TConfig::savePassword4(const std::string& pw)
{
    localSettings.password4 = pw;
}

void TConfig::saveSystemSoundFile(const std::string& snd)
{
    localSettings.systemSound = snd;
}

void TConfig::saveSystemSoundState(bool state)
{
    localSettings.systemSoundState = state;
}

std::string& TConfig::getSIPproxy()
{
    return localSettings.sip_proxy;
}

void TConfig::setSIPproxy(const std::string& address)
{
    localSettings.sip_proxy = address;
}

int TConfig::getSIPport()
{
    return localSettings.sip_port;
}

void TConfig::setSIPport(int port)
{
    localSettings.sip_port = port;
}

std::string& TConfig::getSIPstun()
{
    return localSettings.sip_stun;
}

void TConfig::setSIPstun(const std::string& address)
{
    localSettings.sip_stun = address;
}

std::string& TConfig::getSIPdomain()
{
    return localSettings.sip_domain;
}

void TConfig::setSIPdomain(const std::string& domain)
{
    localSettings.sip_domain = domain;
}

std::string& TConfig::getSIPuser()
{
    return localSettings.sip_user;
}

void TConfig::setSIPuser(const std::string& user)
{
    localSettings.sip_user = user;
}

std::string& TConfig::getSIPpassword()
{
    return localSettings.sip_password;
}

void TConfig::setSIPpassword(const std::string& pw)
{
    localSettings.sip_password = pw;
}

bool TConfig::getSIPstatus()
{
    return localSettings.sip_enabled;
}

void TConfig::setSIPstatus(bool state)
{
    localSettings.sip_enabled = state;
}

bool TConfig::saveSettings()
{
    DECL_TRACER("TConfig::saveSettings()");

    try
    {
        string fname = localSettings.path + "/" + localSettings.name;
        ofstream file(fname);
        string lines = "LogFile=" + localSettings.logFile + "\n";
        lines += "LogLevel=" + localSettings.logLevel + "\n";
        lines += "ProjectPath=" + localSettings.project + "\n";
        lines += string("NoBanner=") + (localSettings.noBanner ? "true" : "false") + "\n";
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
        lines += string("Password1=") + localSettings.password1 + "\n";
        lines += string("Password2=") + localSettings.password2 + "\n";
        lines += string("Password3=") + localSettings.password3 + "\n";
        lines += string("Password4=") + localSettings.password4 + "\n";
        lines += string("SystemSoundFile=") + localSettings.systemSound + "\n";
        lines += string("SystemSoundState=") + (localSettings.systemSoundState ? "ON" : "OFF") + "\n";
        lines += string("SystemSingleBeep=") + localSettings.systemSingleBeep + "\n";
        lines += string("SystemDoubleBeep=") + localSettings.systemDoubleBeep + "\n";
        // SIP settings
        lines += string("SIP_DOMAIN") + localSettings.sip_domain + "\n";
        lines += string("SIP_PROXY") + localSettings.sip_password + "\n";
        lines += string("SIP_PORT") + std::to_string(localSettings.sip_port) + "\n";
        lines += string("SIP_STUN") + localSettings.sip_stun + "\n";
        lines += string("SIP_USER") + localSettings.sip_user + "\n";
        lines += string("SIP_PASSWORD") + localSettings.sip_password + "\n";
        lines += string("SIP_ENABLED") + (localSettings.sip_enabled ? "true" : "false") + "\n";
        file.write(lines.c_str(), lines.size());
        file.close();
        MSG_INFO("Actual log level: " << localSettings.logLevel);
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Couldn't write configs: " << e.what());
        return false;
    }

    TError::Current()->setLogFile(localSettings.logFile);
    TError::Current()->setLogLevel(localSettings.logLevel);
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
	return localSettings.longformat;
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
	return !localSettings.noBanner;
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
    return localSettings.scale;
}

bool TConfig::getProfiling()
{
    return localSettings.profiling;
}

string & TConfig::getPassword1()
{
    return localSettings.password1;
}

string & TConfig::getPassword2()
{
    return localSettings.password2;
}

string & TConfig::getPassword3()
{
    return localSettings.password3;
}

string & TConfig::getPassword4()
{
    return localSettings.password4;
}

string & TConfig::getSystemSound()
{
    return localSettings.systemSound;
}

bool TConfig::getSystemSoundState()
{
    return localSettings.systemSoundState;
}

string& TConfig::getSingleBeepSound()
{
    return localSettings.systemSingleBeep;
}

string& TConfig::getDoubleBeepSound()
{
    return localSettings.systemDoubleBeep;
}

/**
 * Checks a string if it contains a word which may be interpreted as a
 * boolean TRUE.
 *
 * @param boolean   A string containg a word.
 *
 * @return If the string \p boolean contains a word that can be interpreted as
 * a boolean TRUE, it returns TRUE. In any other case it returns FALSE.
 */
bool TConfig::isTrue(const string& boolean)
{
    if (caseCompare(boolean, "1") == 0 || caseCompare(boolean, "yes") == 0 ||
        caseCompare(boolean, "true") == 0 || caseCompare(boolean, "on") == 0)
        return true;

    return false;
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
    return localSettings.certCheck;
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
    char *HOME = nullptr;
    string sFileName;

    if (!mPath.empty())
    {
        size_t pos = mPath.find_last_of("/");

        if (pos != string::npos)
        {
            localSettings.path = mPath.substr(0, pos);
            localSettings.name = mPath.substr(pos+1);
            mCFile = mPath;
            return !mCFile.empty();
        }

        localSettings.name = mPath;
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

            string content = "LogFile=" + localSettings.path + "/tpanel.log\n";
            content += "LogLevel=NONE\n";
            content += "ProjectPath=" + localSettings.path + "/tpanel\n";
            content += "LongFormat=false\n";
            content += "Address=0.0.0.0\n";
            content += "Port=1319\n";
            content += "Channel=10001\n";
            content += "PanelType=Android\n";
            content += string("Firmware=") + VERSION_STRING() + "\n";
            content += "Scale=true\n";
            content += "Profiling=false\n";
            content += "Password1=1988\n";
            content += "Password2=1988\n";
            content += "Password3=1988\n";
            content += "Password4=1988\n";
            content += "SystemSoundFile=singleBeep.wav\n";
            content += "SystemSoundState=ON\n";
            content += "SystemSingleBeep=singleBeep01.wav\n";
            content += "SystemDoubleBeep=doubleBeep01.wav\n";
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
        std::cerr << "TConfig::findConfig: No environment variable HOME!" << std::endl;

    vector<string>::iterator iter;
    bool found = false;

    for (iter = mCfgPaths.begin(); iter != mCfgPaths.end(); ++iter)
    {
        string f = *iter + "/tpanel.conf";

        if (!access(f.c_str(), R_OK))
        {
            sFileName = f;
            localSettings.path = *iter;
            break;
        }
    }

    // The local configuration file has precedence over the global one.
    if (HOME)
    {
        string f = HOME;
        f += "/.tpanel.conf";

        if (!access(f.data(), R_OK))
        {
            sFileName = f;
            localSettings.path = HOME;
            found = true;
        }
    }

    if (!found)
    {
        std::cerr << "TConfig::findConfig: Can't find any configuration file!" << std::endl;
        TError::setError();
        sFileName.clear();
        localSettings.name.clear();
        localSettings.path.clear();
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
    ifstream fs;

    // First initialize the defaults
    localSettings.ID = 0;
    localSettings.port = 1397;
    localSettings.ptype = "android";
    localSettings.version = "1.0";
    localSettings.longformat = false;
    localSettings.profiling = false;
    localSettings.systemSoundState = true;
    localSettings.systemSound = "singleBeep.wav";
    localSettings.systemSingleBeep = "singleBeep01.wav";
    localSettings.systemDoubleBeep = "doubleBeep01.wav";
#ifdef __ANDROID__
    localSettings.logLevel = SLOG_NONE;
    localSettings.logLevelBits = HLOG_NONE;
#else
    localSettings.logLevel = SLOG_PROTOCOL;
#endif

    // Now get the settings from file
    try
    {
        fs.open(mCFile.c_str(), fstream::in);
    }
    catch (const fstream::failure e)
    {
        std::cerr << "TConfig::readConfig: Error on file " << mCFile << ": " << e.what() << std::endl;
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

                if (localSettings.ID < 10000 || localSettings.ID >= 11000)
                {
                    std::cerr << "TConfig::readConfig: Invalid port number " << right << std::endl;
                    localSettings.ID = 0;
                }
            }
            else if (caseCompare(left, "Scale") == 0 && !right.empty())
                localSettings.scale = isTrue(right);
            else if (caseCompare(left, "Profiling") == 0 && !right.empty())
                localSettings.profiling = isTrue(right);
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
            else if (caseCompare(left, "SIP_PROXY") == 0 && !right.empty())     // SIP settings starting here
                localSettings.sip_proxy = right;
            else if (caseCompare(left, "SIP_PORT") == 0 && !right.empty())
                localSettings.sip_port = atoi(right.c_str());
            else if (caseCompare(left, "SIP_STUN") == 0 && !right.empty())
                localSettings.sip_stun = right;
            else if (caseCompare(left, "SIP_DOMAIN") == 0 && !right.empty())
                localSettings.sip_domain = right;
            else if (caseCompare(left, "SIP_USER") == 0 && !right.empty())
                localSettings.sip_user = right;
            else if (caseCompare(left, "SIP_PASSWORD") == 0 && !right.empty())
                localSettings.sip_password = right;
            else if (caseCompare(left, "SIP_ENABLED") == 0 && !right.empty())
                localSettings.sip_enabled = isTrue(right);
        }
    }

    fs.close();

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
        MSG_INFO("    System Sound: " << localSettings.systemSound);
        MSG_INFO("    Sound state:  " << (localSettings.systemSoundState ? "ACTIVATED" : "DEACTIVATED"));
        MSG_INFO("    Single beep:  " << localSettings.systemSingleBeep);
        MSG_INFO("    Double beep:  " << localSettings.systemDoubleBeep);
    }

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

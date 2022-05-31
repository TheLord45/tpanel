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
#ifndef __TCONFIG_H__
#define __TCONFIG_H__

#include <string>
#include <vector>
#include <algorithm>
#include "terror.h"

#define V_MAJOR     1
#define V_MINOR     3
#define V_PATCH     2
//#define V_ADD       "b8"

#ifndef V_SERIAL
#define V_SERIAL    "20220531TP1320"
#endif

/**
 * @def TPANEL_VERSION
 * Defines the version of this application.
 */
#define TPANEL_VERSION      ((V_MAJOR * 0x10000) + (V_MINOR * 0x100) + V_PATCH)

#define VERSION_STRING() _GET_X_VERSION(V_MAJOR, V_MINOR, V_PATCH)
#define _GET_X_VERSION(a, b, c) _GET_VERSION(a, b, c)
#define _GET_VERSION(a, b, c) ( #a "." #b "." #c )          // Release version
//#define _GET_VERSION(a, b, c) ( #a "." #b "." #c V_ADD)     // Beta version

/**
 * @brief The TConfig class manages the configurations.
 *
 * The class contains methods to read a config file, parse it's content and
 * hold the configuration options in the class. All public methods are static.
 */
class TConfig
{
    public:
        TConfig(const std::string& path);
        bool reReadConfig();

        typedef enum SYSTEMRESOURCE_t
        {
            BASE,
            BORDERS,
            FONTS,
            IMAGES,
            SLIDERS,
            SOUNDS
        }SYSTEMRESOURCE_t;

        typedef enum SIP_FIREWALL_t
        {
            SIP_NO_FIREWALL,
            SIP_NAT_ADDRESS,    // Currently not configurable
            SIP_STUN,
            SIP_ICE,
            SIP_UPNP
        }SIP_FIREWALL_t;

        static void setProgName(const std::string& pname);
        static std::string& getProgName();
        static std::string& getConfigPath();
        static std::string& getConfigFileName();
        static std::string& getProjectPath();
        static std::string getSystemPath(SYSTEMRESOURCE_t sres);
        static std::string& getLogFile();
        static std::string& getLogLevel();
        static uint getLogLevelBits();
        static bool isLongFormat();
        static bool showBanner();
        static bool getScale();
        static bool getToolbarForce();
        static bool getToolbarSuppress();
        static bool getProfiling();
        static std::string& getPassword1();
        static std::string& getPassword2();
        static std::string& getPassword3();
        static std::string& getPassword4();
        static std::string& getSystemSound();
        static bool getSystemSoundState();
        static int getSystemVolume();
        static int getSystemGain();
        static bool getRotationFixed();
        static void setRotationFixed(bool fix);
        static void setSystemChannel(int ch);
        static std::string& getUUID();
        static std::string& getSingleBeepSound();
        static std::string& getDoubleBeepSound();
        static std::string& getFtpUser();
        static std::string& getFtpPassword();
        static std::string& getFtpSurface();
        static bool getFtpPassive();
        static time_t getFtpDownloadTime();

        static std::string& getController();
        static int getSystem();
        static int getPort();
        static int getChannel();
        static std::string& getPanelType();
        static std::string& getFirmVersion();
        static bool certCheck();
        static bool isInitialized() { return mInitialized; }
        static bool getMuteState() { return mMute; }
        static void setMuteState(bool state) { mMute = state; }

        static bool saveProjectPath(const std::string& path);
        static bool saveLogFile(const std::string& file);
        static bool saveLogLevel(const std::string& level);
        static bool saveLogLevel(uint level);
        static bool saveController(const std::string& cnt);
        static bool savePort(int port);
        static bool saveChannel(int channel);
        static bool savePanelType(const std::string& pt);
        static bool saveSettings();
        static void saveFormat(bool format);
        static void saveScale(bool scale);
        static void saveBanner(bool banner);
        static void saveToolbarForce(bool tb);
        static void saveToolbarSuppress(bool tb);
        static void saveProfiling(bool prof);
        static void savePassword1(const std::string& pw);
        static void savePassword2(const std::string& pw);
        static void savePassword3(const std::string& pw);
        static void savePassword4(const std::string& pw);
        static void saveSystemSoundFile(const std::string& snd);
        static void saveSystemSoundState(bool state);
        static void saveSingleBeepFile(const std::string& snd);
        static void saveDoubleBeepFile(const std::string& snd);
        static void saveSystemVolume(int volume);
        static void saveSystemGain(int gain);
        static void saveFtpUser(const std::string& user);
        static void saveFtpPassword(const std::string& pw);
        static void saveFtpSurface(const std::string& fname);
        static void saveFtpPassive(bool mode);
        static void saveFtpDownloadTime(time_t t);

        // SIP management
        static std::string& getSIPproxy();
        static void setSIPproxy(const std::string& address);
        static int getSIPport();
        static void setSIPport(int port);
        static int getSIPportTLS();
        static void setSIPportTLS(int port);
        static std::string& getSIPstun();
        static void setSIPstun(const std::string& address);
        static std::string& getSIPdomain();
        static void setSIPdomain(const std::string& domain);
        static std::string& getSIPuser();
        static void setSIPuser(const std::string& user);
        static std::string& getSIPpassword();
        static void setSIPpassword(const std::string& pw);
        static bool getSIPstatus();
        static bool getSIPnetworkIPv4();
        static void setSIPnetworkIPv4(bool state);
        static bool getSIPnetworkIPv6();
        static void setSIPnetworkIPv6(bool state);
        static SIP_FIREWALL_t getSIPfirewall();
        static std::string getSIPfirewallStr();
        static void setSIPfirewall(SIP_FIREWALL_t fw);
        static void setSIPstatus(bool state);
        static void setSIPiphone(bool state);
        static bool getSIPiphone();

    protected:
        static bool isTrue(const std::string& boolean);

    private:
        static uint logLevelStrToBits(const std::string& level);
        static std::string logLevelBitsToString(uint level);
        static std::string sipFirewallToString(SIP_FIREWALL_t fw);
        static SIP_FIREWALL_t sipFirewallStrToEnum(const std::string& str);
        bool findConfig();
        bool readConfig();
        std::vector<std::string> split(const std::string& str, const std::string& seps, const bool trimEmpty);
        static int caseCompare(const std::string& str1, const std::string& str2);
        static std::string makeConfigDefault(const std::string& log, const std::string& project);

        std::string mPath;
        std::string mCFile;
        std::vector<std::string> mCfgPaths;
        static bool mInitialized;
        static int mChannel;        // If the channel was changed by a command, this variable holds the new value.
        static bool mMute;          // Holds the mute status. This is temporary!
#ifdef __ANDROID__
        std::string mRoot;
#endif
};

#endif

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

#ifndef QASETTINGS_H
#define QASETTINGS_H

#include <string>

class QString;
class QMargins;

class QASettings
{
    public:
        QASettings();
        ~QASettings();

        // register default preferences from plist file
        void registerDefaultPrefs(void);
        void unregisterPrefs(void);

        // Settings for NetLinx
        QString getNetlinxIP(void);
        int getNetlinxPort(void);
        int getNetlinxChannel(void);
        QString getNetlinxPanelType(void);
        QString getFTPUser(void);
        QString getFTPPassword(void);
        bool getFTPPassive(void);
        QString getNetlinxSurface(void);

        // Settings for SIP
        QString getSipProxy(void);
        int getSipNetworkPort(void);
        int getSipNetworkTlsPort(void);
        QString getSipStun(void);
        QString getSipDomain(void);
        QString getSipUser(void);
        QString getSipPassword(void);
        bool getSipNetworkIpv4(void);
        bool getSipNetworkIpv6(void);
        bool getSipEnabled(void);
        bool getSipIntegratedPhone(void);

        // Settings for View
        bool getViewScale(void);
        bool getViewToolbarVisible(void);
        bool getViewToolbarForce(void);
        bool getViewRotation(void);

        // Settings for sound
        QString getSoundSystem(void);
        QString getSoundSingleBeep(void);
        QString getSoundDoubleBeep(void);
        bool getSoundEnabled(void);
        int getSoundVolume(void);
        int getSoundGain(void);

        // Settings for logging
        bool getLoggingInfo(void);
        bool getLoggingWarning(void);
        bool getLoggingError(void);
        bool getLoggingTrace(void);
        bool getLoggingDebug(void);
        bool getLoggingProfile(void);
        bool getLoggingLogFormat(void);
        bool getLoggingLogfileEnabled(void);
        QString getLoggingLogfile(void);

        // Some static methods
        static QString getLibraryPath(void);
        static QString getDocumentPath(void);
        static QMargins getNotchSize();
        static void openSettings();

        static std::string& getOldNetlinx() { return oldNetlinx; }
        static int getOldPort() { return oldPort; }
        static int getOoldChannelID() { return oldChannelID; }
        static std::string& getOldSurface() { return oldSurface; }
        static bool getOldToolbarSuppress() { return oldToolbarSuppress; }
        static bool getOldToolbarForce() { return oldToolbarForce; }

        static void writeLog(int type, const std::string& msg);

    private:        /// methods
        bool getDefaultBool(char *key, bool def);
        int getDefaultNumber(char *key, int def);

        // private variables
        static std::string oldNetlinx;
        static int oldPort;
        static int oldChannelID;
        static std::string oldSurface;
        static bool oldToolbarSuppress;
        static bool oldToolbarForce;

};

#endif // QASETTINGS_H

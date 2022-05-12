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

#ifndef AMXPANEL_SETTINGS_H
#define AMXPANEL_SETTINGS_H

#include <iostream>
#include <string>
#include <vector>
#include "tvalidatefile.h"
#include "terror.h"
#include "tprjresources.h"

typedef struct PALETTE_SETUP
{
    std::string name;
    std::string file;
    int paletteID{0};
}PALETTE_SETUP;

typedef struct PANEL_SETUP
{
    int portCount{0};                   //!< Number of total ports available
    int setupPort{0};                   //!< The number of the setup port used for setup pages. Usualy 0
    int addressCount{0};
    int channelCount{0};
    int levelCount{0};
    std::string powerUpPage;            //!< The name of the page to display on startup
    std::vector<std::string> powerUpPopup;  //!< The popup(s) to display on startup
    int feedbackBlinkRate{0};
    std::string startupString;
    std::string wakeupString;           //!< A string which wake up the panel if received
    std::string sleepString;            //!< A string which put panel to sleep when received
    std::string standbyString;          //!< A string which put panel in standby mode
    std::string shutdownString;         //!< A string which shut off the panel
    std::string idlePage;               //!< A page called when the panel is idle
    int idleTimeout{0};                 //!< Time until enter idle mode when no touch occured
    int extButtonsKey{0};
    int screenWidth{0};                 //!< Width of the screen in pixels
    int screenHeight{0};                //!< Height of screen in pixels
    int screenRefresh{0};
    int screenRotate{0};                //!< 0 = landscape; 1 = portrait
    std::string screenDescription;
    int pageTracking{0};
    int cursor{0};
    int brightness{0};
    int lightSensorLevelPort{0};
    int lightSensorLevelCode{0};
    int lightSensorChannelPort{0};
    int lightSensorChannelCode{0};
    int motionSensorChannelPort{0};
    int motionSensorChannelCode{0};
    int batteryLevelPort{0};
    int batteryLevelCode{0};
    int irPortAMX38Emit{0};
    int irPortAMX455Emit{0};
    int irPortAMX38Recv{0};
    int irPortAMX455Recv{0};
    int irPortUser1{0};
    int irPortUser2{0};
    int cradleChannelPort{0};
    int cradleChannelCode{0};
    std::string uniqueID;
    std::string appCreated;
    int buildNumber{0};
    std::string appModified;
    int buildNumberMod{0};
    std::string buildStatusMod;
    int activePalette{0};
    int marqueeSpeed{0};
    int setupPagesProject{0};
    int voipCommandPort{0};
    std::vector<PALETTE_SETUP> palettes;
}PANEL_SETUP_T;

class TSettings : public TValidateFile
{
    public:
        TSettings(const std::string& path);

        const std::string& getPath() { return mPath; }
        PANEL_SETUP_T& getSettings() { return mSetup; }
        int getWith() { return mSetup.screenWidth; }
        int getHeight() { return mSetup.screenHeight; }
        int getRotate() { return mSetup.screenRotate; }
        bool isPortrait();
        bool isLandscape();
        std::vector<RESOURCE_LIST_T>& getResourcesList() { return mResourceLists; }
        bool loadSettings(bool initial=false);
        std::string& getPowerUpPage() { return mSetup.powerUpPage; }
        int getVoipCmdPort() { return mSetup.voipCommandPort; }

    private:
        RESOURCE_LIST_T findResourceType(const std::string& type);

        std::string mPath;                              // The path to the resources
        PANEL_SETUP_T mSetup;                           // Settings read from file prj.xma
        std::vector<RESOURCE_LIST_T> mResourceLists;    // Resources (cameras) read from prj.xma
};



#endif //AMXPANEL_SETTINGS_H

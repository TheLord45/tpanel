/*
 * Copyright (C) 2024 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef TAPPS_H
#define TAPPS_H

#include "tconfig.h"
#include "tvalidatefile.h"

typedef struct APP_ASPECT_RATIO_t
{
    double percent{0.0};
    int ratioWidth{0};
    int ratioHeight{0};
}APP_ASPECT_RATIO_t;

typedef struct APP_ASPECT_RATIO_LIMIT_t
{
    int minWidth{0};
    int minHeight{0};
}APP_ASPECT_RATIO_LIMIT_t;

typedef struct APP_WINDOW_t
{
    bool aspectFixed{false};
    std::vector<APP_ASPECT_RATIO_t> aspectRatios;
    APP_ASPECT_RATIO_LIMIT_t aspectRatioLimits;
}APP_WINDOW_t;

typedef struct APP_IMAGES_t
{
    std::string thumbImage;
    std::string windowImage;
}APP_IMAGES_t;

typedef struct APP_PAR_STRINGS_t
{
    std::string key;
    std::string value;
}APP_PAR_STRINGS_t;

typedef struct APP_PARAMETER_t
{
    std::string name;
    std::string eDataType;
    std::string value;
    std::string info;
    std::vector<APP_PAR_STRINGS_t> stringValues;
}APP_PARAMETER_t;

typedef struct APP_SETTINGS_t
{
    std::string appName;
    std::string appInfo;
    std::string packageName;        // Android package name
    std::string appID;
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    std::string path;               // Desktop OS: Path and name of the application
#endif
    APP_WINDOW_t appWindow;
    APP_IMAGES_t appImages;
    std::vector<APP_PARAMETER_t> parameters;
}APP_SETTINGS_t;

typedef struct APP_BUTTON_t
{
    std::string eLocation;
    int order{0};
    int spacing{0};
    std::vector<std::string> buttonImage;
}APP_BUTTON_t;

typedef struct APP_WINDOW_FRAME_t
{
    std::string eType;
    int edgeSize{0};
    int barSize{0};
    std::vector<APP_BUTTON_t> buttons;
}APP_WINDOW_FRAME_t;

class TApps : public TValidateFile
{
    public:
        TApps();
        ~TApps();

        bool parseApps();
        static APP_SETTINGS_t getApp(const std::string& name);
        static APP_WINDOW_FRAME_t getWindowFrame(const std::string& type);

    private:
        static std::vector<APP_SETTINGS_t> mAppSettings;
        static std::vector<APP_WINDOW_FRAME_t> mWindowFrames;
};

#endif // TAPPS_H

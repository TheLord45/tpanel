/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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
#include <fstream>

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <QFile>
#ifdef __ANDROID__
#include <QtAndroid>
#endif
#include <QMap>
#include <QHash>

#include "ttpinit.h"
#include "terror.h"
#include "tvalidatefile.h"
#include "tconfig.h"
#include "tfsfreader.h"

using std::string;
using std::vector;
using std::ostream;

TTPInit::TTPInit()
{
    DECL_TRACER("TTPInit::TTPInit()");
}

TTPInit::TTPInit(const string& path)
    : mPath(path)
{
    DECL_TRACER("TTPInit::TTPInit(const string& path)")

    createDirectoryStructure();
    createPanelConfigs();
    loadSurfaceFromController();
}

bool TTPInit::createPanelConfigs()
{
    DECL_TRACER("TTPInit::createPanelConfigs()");

    vector<string> resFiles = {
        ":ressources/external.xma",
        ":ressources/fnt.xma",
        ":ressources/icon.xma",
        ":ressources/_main.xml",
        ":ressources/manifest.xma",
        ":ressources/map.xma",
        ":ressources/pal_001.xma",
        ":ressources/prj.xma",
        ":ressources/_setup.xml",
        ":ressources/table.xma",
        ":ressources/fonts/arial.ttf",
        ":ressources/images/theosys_logo.png",
        ":ressources/__system/graphics/fonts/amxbold_.ttf",
        ":ressources/__system/graphics/fonts/arialbd.ttf",
        ":ressources/__system/graphics/fonts/arial.ttf",
        ":ressources/__system/graphics/fonts/cour.ttf",
        ":ressources/__system/graphics/sounds/audioTest.wav",
        ":ressources/__system/graphics/sounds/docked.mp3",
        ":ressources/__system/graphics/sounds/doubleBeep01.wav",
        ":ressources/__system/graphics/sounds/doubleBeep02.wav",
        ":ressources/__system/graphics/sounds/doubleBeep03.wav",
        ":ressources/__system/graphics/sounds/doubleBeep04.wav",
        ":ressources/__system/graphics/sounds/doubleBeep05.wav",
        ":ressources/__system/graphics/sounds/doubleBeep06.wav",
        ":ressources/__system/graphics/sounds/doubleBeep07.wav",
        ":ressources/__system/graphics/sounds/doubleBeep08.wav",
        ":ressources/__system/graphics/sounds/doubleBeep09.wav",
        ":ressources/__system/graphics/sounds/doubleBeep10.wav",
        ":ressources/__system/graphics/sounds/doubleBeep.wav",
        ":ressources/__system/graphics/sounds/ringback.wav",
        ":ressources/__system/graphics/sounds/ringtone.wav",
        ":ressources/__system/graphics/sounds/singleBeep01.wav",
        ":ressources/__system/graphics/sounds/singleBeep02.wav",
        ":ressources/__system/graphics/sounds/singleBeep03.wav",
        ":ressources/__system/graphics/sounds/singleBeep04.wav",
        ":ressources/__system/graphics/sounds/singleBeep05.wav",
        ":ressources/__system/graphics/sounds/singleBeep06.wav",
        ":ressources/__system/graphics/sounds/singleBeep07.wav",
        ":ressources/__system/graphics/sounds/singleBeep08.wav",
        ":ressources/__system/graphics/sounds/singleBeep09.wav",
        ":ressources/__system/graphics/sounds/singleBeep10.wav",
        ":ressources/__system/graphics/sounds/singleBeep.wav",
        ":ressources/__system/graphics/draw.xma",
        ":ressources/__system/graphics/fnt.xma",
        ":ressources/__system/graphics/version.xma"
    };

    bool err = false;
    vector<string>::iterator iter;

    for (iter = resFiles.begin(); iter != resFiles.end(); ++iter)
    {
        if (!copyFile(*iter))
            err = true;
    }


    return err;
}

bool TTPInit::createDirectoryStructure()
{
    DECL_TRACER("TTPInit::createDirectoryStructure()");

    if (mPath.empty())
    {
        MSG_ERROR("Got no path to create the directory structure!");
        return false;
    }

    string pfad = mPath;

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/fonts";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/images";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/sounds";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/graphics";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/graphics/fonts";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/graphics/images";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/graphics/sounds";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/graphics/cursors";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/graphics/sliders";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/graphics/borders";

    if (!_makeDir(pfad))
        return false;

    return true;
}

bool TTPInit::_makeDir(const std::string& dir)
{
    DECL_TRACER("TTPInit::_makeDir(const std::string& dir)");

    TValidateFile vf;

    if (!vf.isValidDir(dir))
    {
        if (mkdir (dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXG) != 0)
        {
            MSG_ERROR("Directory " << dir << ": " << strerror(errno));
            return false;
        }
    }

    return true;
}

bool TTPInit::copyFile(const std::string& fname)
{
    DECL_TRACER("TTPInit::copyFile(const std::string& fname)");

    bool err = false;
    QFile external(fname.c_str());
    size_t pos = fname.find_first_of("/");
    string bname;

    if (pos != string::npos)
        bname = fname.substr(pos);
    else
        bname = fname;

    if (external.exists())
    {
        QString path = mPath.c_str();
        path += bname.c_str();

        if (!external.copy(path))
        {
#ifdef __ANDROID__
            if (!askPermissions())
            {
                MSG_ERROR("Could not copy \"" << bname << "\" to " << path.toStdString() << " because permission was denied!");
                err = true;
            }
            else if (!external.copy(path))
            {
                MSG_ERROR("Could not copy \"" << bname << "\" to " << path.toStdString());
                err = true;
            }
#else
            MSG_ERROR("Could not copy \"" << bname << "\" to " << path.toStdString());
            err = true;
#endif
        }
    }
    else
    {
        MSG_ERROR("File " << external.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    return err;
}

/**
 * This methods checks if there exists a previous downloaded TP4 file. If this
 * is the case, nothing happens.
 * If there is no previous downloaded file it checks if there is one on the
 * controller and downloads it if it exists. After successfull download the
 * file is unpacked.
 *
 * @return TRUE is returned when there was a file successfully downloaded and
 * unpacked. In any other case FALSE is returned.
 */
bool TTPInit::loadSurfaceFromController(bool force)
{
    DECL_TRACER("TTPInit::loadSurfaceFromController(bool force)");

    TFsfReader reader;
    string target = mPath + "/" + TConfig::getFtpSurface();

    if (!force)
    {
        QFile pan(target.c_str());

        if (pan.exists() || TConfig::getFtpDownloadTime() > 0)
            return false;
    }

    if (!reader.copyOverFTP(TConfig::getFtpSurface(), target))
        return false;

    if (!reader.unpack(TConfig::getFtpSurface(), mPath))
        return false;

    TConfig::saveFtpDownloadTime(time(NULL));
    TConfig::saveSettings();
    return true;
}

#ifdef __ANDROID__
bool TTPInit::askPermissions()
{
    DECL_TRACER("TTPInit::askPermissions()");

    QStringList permissions = { "android.permission.READ_EXTERNAL_STORAGE", "android.permission.WRITE_EXTERNAL_STORAGE" };
    QtAndroid::PermissionResultMap perms = QtAndroid::requestPermissionsSync(permissions);

    for (auto iter = perms.begin(); iter != perms.end(); ++iter)
    {
        if (iter.value() == QtAndroid::PermissionResult::Denied)
            return false;
    }

    return true;
}
#endif

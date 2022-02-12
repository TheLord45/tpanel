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
}

bool TTPInit::createPanelConfigs()
{
    DECL_TRACER("TTPInit::createPanelConfigs()");

    bool err = false;
    QFile external(":ressources/external.xma");

    if (external.exists())
    {
        QString path = mPath.c_str();
        path += "/external.xma";

        if (!external.copy(path))
        {
#ifdef __ANDROID__
            if (!askPermissions())
            {
                MSG_ERROR("Could not copy \"external.xma\" to " << path.toStdString());
                err = true;
            }
            else if (!external.copy(path))
            {
                MSG_ERROR("Could not copy \"external.xma\" to " << path.toStdString());
                err = true;
            }
#else
            MSG_ERROR("Could not copy \"external.xma\" to " << path.toStdString());
            err = true;
#endif
        }
    }
    else
    {
        MSG_ERROR("File " << external.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile fnt(":ressources/fnt.xma");

    if (fnt.exists())
    {
        QString path = mPath.c_str();
        path += "/fnt.xma";

        if (!fnt.copy(path))
        {
            MSG_ERROR("Could not copy \"fnt.xma\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << fnt.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile icon(":ressources/icon.xma");

    if (icon.exists())
    {
        QString path = mPath.c_str();
        path += "/icon.xma";

        if (!icon.copy(path))
        {
            MSG_ERROR("Could not copy \"icon.xma\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << icon.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile _main(":ressources/_main.xml");

    if (_main.exists())
    {
        QString path = mPath.c_str();
        path += "/_main.xml";

        if (!_main.copy(path))
        {
            MSG_ERROR("Could not copy \"_main.xml\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << _main.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile manifest(":ressources/manifest.xma");

    if (manifest.exists())
    {
        QString path = mPath.c_str();
        path += "/manifest.xma";

        if (!manifest.copy(path))
        {
            MSG_ERROR("Could not copy \"manifest.xma\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << manifest.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile map(":ressources/map.xma");

    if (map.exists())
    {
        QString path = mPath.c_str();
        path += "/map.xma";

        if (!map.copy(path))
        {
            MSG_ERROR("Could not copy \"map.xma\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << map.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile pal(":ressources/pal_001.xma");

    if (pal.exists())
    {
        QString path = mPath.c_str();
        path += "/pal_001.xma";

        if (!pal.copy(path))
        {
            MSG_ERROR("Could not copy \"pal_001.xma\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << pal.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile prj(":ressources/prj.xma");

    if (prj.exists())
    {
        QString path = mPath.c_str();
        path += "/prj.xma";

        if (!prj.copy(path))
        {
            MSG_ERROR("Could not copy \"prj.xma\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << prj.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile _setup(":ressources/_setup.xml");

    if (_setup.exists())
    {
        QString path = mPath.c_str();
        path += "/_setup.xml";

        if (!_setup.copy(path))
        {
            MSG_ERROR("Could not copy \"_setup.xml\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << _setup.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile table(":ressources/table.xma");

    if (table.exists())
    {
        QString path = mPath.c_str();
        path += "/table.xma";

        if (!table.copy(path))
        {
            MSG_ERROR("Could not copy \"table.xma\" to " << path.toStdString());
            err = true;
        }
    }
    else
    {
        MSG_ERROR("File " << table.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile fonts(":ressources/fonts/arial.ttf");

    if (fonts.exists())
    {
        QString path = mPath.c_str();
        path += "/fonts/arial.ttf";

        if (!fonts.copy(path))
        {
#ifdef __ANDROID__
            if (!askPermissions())
            {
                MSG_ERROR("Could not copy \"arial.ttf\" to " << path.toStdString());
                err = true;
            }
            else if (!fonts.copy(path))
            {
                MSG_ERROR("Could not copy \"arial.ttf\" to " << path.toStdString());
                err = true;
            }
#else
        MSG_ERROR("Could not copy \"arial.ttf\" to " << path.toStdString());
        err = true;
#endif
        }
    }
    else
    {
        MSG_ERROR("File " << fonts.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    QFile images(":ressources/images/theosys_logo.png");

    if (images.exists())
    {
        QString path = mPath.c_str();
        path += "/images/theosys_logo.png";

        if (!images.copy(path))
        {
#ifdef __ANDROID__
            if (!askPermissions())
            {
                MSG_ERROR("Could not copy \"theosys_logo.png\" to " << path.toStdString());
                err = true;
            }
            else if (!images.copy(path))
            {
                MSG_ERROR("Could not copy \"theosys_logo.png\" to " << path.toStdString());
                err = true;
            }
#else
        MSG_ERROR("Could not copy \"theosys_logo.png\" to " << path.toStdString());
        err = true;
#endif
        }
    }
    else
    {
        MSG_ERROR("File " << images.fileName().toStdString() << " doesn't exist!");
        err = true;
    }

    vector<QString> sysFonts = {
        "/__system/graphics/fonts/amxbold_.ttf",
        "/__system/graphics/fonts/arialbd.ttf",
        "/__system/graphics/fonts/arial.ttf",
        "/__system/graphics/fonts/cour.ttf"
    };

    vector<QString>::iterator iter;

    for (iter = sysFonts.begin(); iter != sysFonts.end(); ++iter)
    {
        QFile sys;
        sys.setFileName(":ressources" + *iter);

        if (sys.exists())
        {
            QString path = mPath.c_str();
            path += *iter;

            if (!sys.copy(path))
            {
                MSG_ERROR("Could not copy \":ressources" << iter->toStdString() << "\" to " << path.toStdString());
                err = true;
            }
        }

        if (sys.isOpen())
            sys.close();
    }

    vector<QString> sounds = {
        "/__system/graphics/sounds/audioTest.wav",
        "/__system/graphics/sounds/docked.mp3",
        "/__system/graphics/sounds/doubleBeep01.wav",
        "/__system/graphics/sounds/doubleBeep02.wav",
        "/__system/graphics/sounds/doubleBeep03.wav",
        "/__system/graphics/sounds/doubleBeep04.wav",
        "/__system/graphics/sounds/doubleBeep05.wav",
        "/__system/graphics/sounds/doubleBeep06.wav",
        "/__system/graphics/sounds/doubleBeep07.wav",
        "/__system/graphics/sounds/doubleBeep08.wav",
        "/__system/graphics/sounds/doubleBeep09.wav",
        "/__system/graphics/sounds/doubleBeep10.wav",
        "/__system/graphics/sounds/doubleBeep.wav",
        "/__system/graphics/sounds/ringback.wav",
        "/__system/graphics/sounds/ringtone.wav",
        "/__system/graphics/sounds/singleBeep01.wav",
        "/__system/graphics/sounds/singleBeep02.wav",
        "/__system/graphics/sounds/singleBeep03.wav",
        "/__system/graphics/sounds/singleBeep04.wav",
        "/__system/graphics/sounds/singleBeep05.wav",
        "/__system/graphics/sounds/singleBeep06.wav",
        "/__system/graphics/sounds/singleBeep07.wav",
        "/__system/graphics/sounds/singleBeep08.wav",
        "/__system/graphics/sounds/singleBeep09.wav",
        "/__system/graphics/sounds/singleBeep10.wav",
        "/__system/graphics/sounds/singleBeep.wav"
    };

    for (iter = sounds.begin(); iter != sounds.end(); ++iter)
    {
        QFile sysSounds;
        sysSounds.setFileName(":ressources" + *iter);

        if (sysSounds.exists())
        {
            QString path = mPath.c_str();
            path += *iter;

            if (!sysSounds.copy(path))
            {
                MSG_ERROR("Could not copy \"" << iter->toStdString() << "\" to " << path.toStdString());
                err = true;
            }
        }

        if (sysSounds.isOpen())
            sysSounds.close();
    }

    vector<QString> sysSettings = {
        "/__system/graphics/draw.xma",
        "/__system/graphics/fnt.xma",
        "/__system/graphics/version.xma"
    };

    for (iter = sysSettings.begin(); iter != sysSettings.end(); ++iter)
    {
        QFile sys;
        sys.setFileName(":ressources" + *iter);

        if (sys.exists())
        {
            QString path = mPath.c_str();
            path += *iter;

            if (!sys.copy(path))
            {
                MSG_ERROR("Could not copy \"" << iter->toStdString() << "\" to " << path.toStdString());
                err = true;
            }
        }

        if (sys.isOpen())
            sys.close();
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

#ifdef __ANDROID__
bool TTPInit::askPermissions()
{
    DECL_TRACER("TTPInit::askPermissions(const std::string& path)");

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

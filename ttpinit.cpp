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
#include <functional>

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

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
#include "tdirectory.h"
#include "tresources.h"

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

using std::string;
using std::vector;
using std::ostream;
using std::bind;
using std::ifstream;

#define SYSTEM_DEFAULT      "/.system"

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

    // Mark files as system default files
    try
    {
        string marker = mPath + SYSTEM_DEFAULT;
        std::ofstream mark(marker);
        time_t t = time(NULL);
        mark.write((char *)&t, sizeof(time_t));
        mark.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error creating a marker file: " << e.what());
        err = true;
    }

    return err;
}

bool TTPInit::createSystemConfigs()
{
    DECL_TRACER("TTPInit::createSystemConfigs()");

    vector<string> resFiles = {
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
        bname = path.toStdString();

        // If the target already exists we must delete it first.
        if (access(bname.data(), F_OK) == 0)
            remove(bname.data());

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

string TTPInit::getTmpFileName()
{
    DECL_TRACER("TTPInit::getTmpFileName()");

    const string alphanum =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    size_t stringLength = alphanum.length() - 1;
    std::string Str;
    char *tmp = getenv("TMP");

    if (!tmp)
        tmp = getenv("TEMP");

    if (!tmp)
        tmp = getenv("HOME");
    else
        tmp = (char *)"/tmp";

    Str.assign(tmp);
    Str.append("/");

    for(size_t i = 0; i < MAX_TMP_LEN; ++i)
        Str += alphanum[rand() % stringLength];

    // We create the file. YES, this is a security hole but in our case we have
    // no real alternative for now.
    try
    {
        std::ofstream tmpfile;
        tmpfile.open(Str, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

        if (!tmpfile.is_open())
        {
            MSG_ERROR("Error opening a temporary file!");
        }
        else
            tmpfile.flush();

        tmpfile.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Couldn't create a temporary file: " << e.what());
        return string();
    }

    return Str;
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
    reader.regCallbackProgress(bind(&TTPInit::progressCallback, this, std::placeholders::_1));
    string target = mPath + "/" + TConfig::getFtpSurface();
    size_t pos = 0;

    if ((pos = mPath.find_last_of("/")) != string::npos)
        target = mPath.substr(0, pos) + "/" + TConfig::getFtpSurface();

    if (!force)
    {
        if (!isVirgin() || TConfig::getFtpDownloadTime() > 0)
            return false;
    }

    if (_processEvents)
        _processEvents();

    // To be sure the target directory tree is empty, we delete all files but
    // keep the system directories and their content, if they exist.
    dir::TDirectory dir;

    if (dir.exists(mPath))
    {
        dir.dropDir(mPath);
        dir.dropDir(mPath + "/fonts");
        dir.dropDir(mPath + "/images");
        dir.dropDir(mPath + "/sounds");
    }

    if (!reader.copyOverFTP(TConfig::getFtpSurface(), target))
    {
        if (TConfig::getFtpDownloadTime() == 0)
            createPanelConfigs();

        return false;
    }

    if (_processEvents)
        _processEvents();

    if (!reader.unpack(target, mPath))
    {
        MSG_ERROR("Unpacking was not successfull.");
        return false;
    }

    if (!force || !dir.exists(mPath + "/__system"))
    {
        createDirectoryStructure();
        createSystemConfigs();
    }

    if (_processEvents)
        _processEvents();

    dir.dropFile(target);       // We remove our traces
    dir.dropFile(mPath + SYSTEM_DEFAULT);   // No more system default files
    TConfig::saveFtpDownloadTime(time(NULL));
    TConfig::saveSettings();
    return true;
}

vector<string>& TTPInit::getFileList(const string& filter)
{
    DECL_TRACER("TTPInit::getFileList(const string& filter)");

    ftplib *ftp = new ftplib();
    ftp->regLogging(bind(&TTPInit::logging, this, std::placeholders::_1, std::placeholders::_2));

    if (TConfig::getFtpPassive())
        ftp->SetConnmode(ftplib::pasv);
    else
        ftp->SetConnmode(ftplib::port);

    string scon = TConfig::getController() + ":21";
    MSG_DEBUG("Trying to connect to " << scon);

    if (!ftp->Connect(scon.c_str()))
    {
        delete ftp;
        return mDirList;
    }

    string sUser = TConfig::getFtpUser();
    string sPass = TConfig::getFtpPassword();
    MSG_DEBUG("Trying to login <" << sUser << ", " << sPass << ">");

    if (!ftp->Login(sUser.c_str(), sPass.c_str()))
    {
        delete ftp;
        return mDirList;
    }

//    string tmpFile = std::tmpnam(nullptr);
    string tmpFile = getTmpFileName();
    MSG_DEBUG("Reading remote directory / into file " << tmpFile);
    ftp->Nlst(tmpFile.c_str(), "/");
    ftp->Quit();
    delete ftp;
    mDirList.clear();

    try
    {
        char buffer[1024];
        string uFilter = toUpper((std::string&)filter);

        std::ifstream ifile(tmpFile);

        while (ifile.getline(buffer, sizeof(buffer)))
        {
            string buf = buffer;
            string fname = trim(buf);
            MSG_DEBUG("FTP line: " << buf << " (" << fname << ")");

            if (!filter.empty())
            {
                if (endsWith(toUpper(buf), uFilter))
                    mDirList.push_back(trim(fname));
            }
            else
                mDirList.push_back(trim(fname));
        }

        ifile.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error opening file " << tmpFile << ": " << e.what());
    }

    fs::remove(tmpFile);

    if (mDirList.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = mDirList.begin(); iter != mDirList.end(); ++iter)
        {
            MSG_DEBUG("File: " << *iter);
        }
    }

    return mDirList;
}

int TTPInit::progressCallback(off64_t xfer)
{
    DECL_TRACER("TTPInit::progressCallback(off64_t xfer)");

    if (_processEvents && xfer > 0)
        _processEvents();

    return 1;
}

bool TTPInit::isSystemDefault()
{
    DECL_TRACER("TTPInit::isSystemDefault()");

    try
    {
        string marker = mPath + SYSTEM_DEFAULT;
        return fs::exists(marker);
    }
    catch (std::exception& e)
    {
        MSG_ERROR("File system error: " << e.what())
        return false;
    }

    return true;
}

bool TTPInit::isVirgin()
{
    DECL_TRACER("TTPInit::isVirgin()");

    try
    {
        if (!fs::exists(mPath) || isSystemDefault())
            return true;
    }
    catch (std::exception& e)
    {
        MSG_ERROR("File system error: " << e.what());
        return true;
    }

    return false;
}

void TTPInit::logging(int level, const std::string &msg)
{
    switch(level)
    {
        case LOG_INFO:      MSG_INFO(msg); break;
        case LOG_WARNING:   MSG_WARNING(msg); break;
        case LOG_ERROR:     MSG_ERROR(msg); break;
        case LOG_TRACE:     MSG_TRACE(msg); break;
        case LOG_DEBUG:     MSG_DEBUG(msg); break;
    }
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

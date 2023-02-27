/*
 * Copyright (C) 2021, 2022 by Andreas Theofilu <andreas@theosys.at>
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
#include <QDir>
#ifdef Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtAndroidExtras/QtAndroid>
#else
#include <QtCore>
#include <QFuture>
#include <QtCore/private/qandroidextras_p.h>
#endif
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
#ifdef Q_OS_IOS
#include "ios/QASettings.h"
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

    if (!loadSurfaceFromController())
        createDemoPage();
}

void TTPInit::setPath(const string& p)
{
    DECL_TRACER("TTPInit::setPath(const string& p)");

    mPath = p;
    string dirs = "/__system";
    string config = p + "/__system/prj.xma";
    string sysFiles = p + "/__system/graphics/version.xma";
    string regular = p + "/prj.xma";

    if (!fs::exists(dirs))
        createDirectoryStructure();

    if (!fs::exists(sysFiles))
        createSystemConfigs();

    if (!fs::exists(config))
        createPanelConfigs();

    if (!fs::exists(regular))
        createDemoPage();
}

bool TTPInit::createPanelConfigs()
{
    DECL_TRACER("TTPInit::createPanelConfigs()");

    if (!fs::exists(mPath + "/__system/prj.xma"))
        mPanelConfigsCreated = false;

    if (mPanelConfigsCreated)
        return false;

    vector<string> resFiles = {
        ":ressources/__system/Controller.xml",
        ":ressources/__system/DoubleBeep.xml",
        ":ressources/__system/external.xma",
        ":ressources/__system/fnt.xma",
        ":ressources/__system/fonts/arial.ttf",
        ":ressources/__system/fonts/amxbold_.ttf",
        ":ressources/__system/fonts/ariblk.ttf",
        ":ressources/__system/fonts/webdings.ttf",
        ":ressources/__system/icon.xma",
        ":ressources/__system/Logging.xml",
        ":ressources/__system/images/setup_download_green.png",
        ":ressources/__system/images/setup_download_red.png",
        ":ressources/__system/images/setup_fileopen.png",
        ":ressources/__system/images/setup_note.png",
        ":ressources/__system/images/setup_reset.png",
        ":ressources/__system/images/theosys_logo.png",
        ":ressources/__system/manifest.xma",
        ":ressources/__system/map.xma",
        ":ressources/__system/pal_001.xma",
        ":ressources/__system/prj.xma",
        ":ressources/__system/SingleBeep.xml",
        ":ressources/__system/SIP.xml",
        ":ressources/__system/Sound.xml",
        ":ressources/__system/SystemSound.xml",
        ":ressources/__system/table.xma",
        ":ressources/__system/TP4FileName.xml",
        ":ressources/__system/View.xml"
    };

    bool err = false;
    vector<string>::iterator iter;

    for (iter = resFiles.begin(); iter != resFiles.end(); ++iter)
    {
        if (!copyFile(*iter))
            err = true;
    }

    mPanelConfigsCreated = !err;
    return err;
}

bool TTPInit::createDemoPage()
{
    DECL_TRACER("TTPInit::createDemoPage()");

    if (fs::exists(mPath + "/prj.xma"))
    {
        MSG_DEBUG("There exists already a page.");
        return false;
    }

    if (mDemoPageCreated)
        return false;

    vector<string> resFiles = {
        ":ressources/_Copyright.xml",
        ":ressources/_WhatItIs.xml",
        ":ressources/_main.xml",
        ":ressources/external.xma",
        ":ressources/fnt.xma",
        ":ressources/icon.xma",
        ":ressources/images/theosys_logo.png",
        ":ressources/manifest.xma",
        ":ressources/map.xma",
        ":ressources/pal_001.xma",
        ":ressources/prj.xma",
        ":ressources/table.xma"
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

    mDemoPageCreated = !err;
    return err;
}

bool TTPInit::createSystemConfigs()
{
    DECL_TRACER("TTPInit::createSystemConfigs()");

    if (!fs::exists(mPath + "/__system/graphics/sounds/docked.mp3"))
        mSystemConfigsCreated = false;

    if (mSystemConfigsCreated)
        return false;

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
        ":ressources/__system/graphics/borders/AMXeliteL-off_b.png",
        ":ressources/__system/graphics/borders/AMXeliteL-off_bl.png",
        ":ressources/__system/graphics/borders/AMXeliteL-off_br.png",
        ":ressources/__system/graphics/borders/AMXeliteL-off_l.png",
        ":ressources/__system/graphics/borders/AMXeliteL-off_r.png",
        ":ressources/__system/graphics/borders/AMXeliteL-off_t.png",
        ":ressources/__system/graphics/borders/AMXeliteL-off_tl.png",
        ":ressources/__system/graphics/borders/AMXeliteL-off_tr.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_b.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_bl.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_br.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_l.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_r.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_t.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_tl.png",
        ":ressources/__system/graphics/borders/AMXeliteL-on_tr.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_b.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_bl.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_br.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_l.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_r.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_t.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_tl.png",
        ":ressources/__system/graphics/borders/AMXeliteM-off_tr.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_b.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_bl.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_br.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_l.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_r.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_t.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_tl.png",
        ":ressources/__system/graphics/borders/AMXeliteM-on_tr.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_b.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_bl.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_br.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_l.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_r.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_t.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_tl.png",
        ":ressources/__system/graphics/borders/AMXeliteS-off_tr.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_b.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_bl.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_br.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_l.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_r.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_t.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_tl.png",
        ":ressources/__system/graphics/borders/AMXeliteS-on_tr.png",
        ":ressources/__system/graphics/borders/Ball_b.png",
        ":ressources/__system/graphics/borders/Ball_b_alpha.png",
        ":ressources/__system/graphics/borders/Ball_bl.png",
        ":ressources/__system/graphics/borders/Ball_bl_alpha.png",
        ":ressources/__system/graphics/borders/Ball_br.png",
        ":ressources/__system/graphics/borders/Ball_br_alpha.png",
        ":ressources/__system/graphics/borders/Ball_l.png",
        ":ressources/__system/graphics/borders/Ball_l_alpha.png",
        ":ressources/__system/graphics/borders/Ball_r.png",
        ":ressources/__system/graphics/borders/Ball_r_alpha.png",
        ":ressources/__system/graphics/borders/Ball_t.png",
        ":ressources/__system/graphics/borders/Ball_t_alpha.png",
        ":ressources/__system/graphics/borders/Ball_tl.png",
        ":ressources/__system/graphics/borders/Ball_tl_alpha.png",
        ":ressources/__system/graphics/borders/Ball_tr.png",
        ":ressources/__system/graphics/borders/Ball_tr_alpha.png",
        ":ressources/__system/graphics/borders/BvlDblIM_b.png",
        ":ressources/__system/graphics/borders/BvlDblIM_bl.png",
        ":ressources/__system/graphics/borders/BvlDblIM_br.png",
        ":ressources/__system/graphics/borders/BvlDblIM_l.png",
        ":ressources/__system/graphics/borders/BvlDblIM_r.png",
        ":ressources/__system/graphics/borders/BvlDblIM_t.png",
        ":ressources/__system/graphics/borders/BvlDblIM_tl.png",
        ":ressources/__system/graphics/borders/BvlDblIM_tr.png",
        ":ressources/__system/graphics/borders/BvlDblIS_b.png",
        ":ressources/__system/graphics/borders/BvlDblIS_bl.png",
        ":ressources/__system/graphics/borders/BvlDblIS_br.png",
        ":ressources/__system/graphics/borders/BvlDblIS_l.png",
        ":ressources/__system/graphics/borders/BvlDblIS_r.png",
        ":ressources/__system/graphics/borders/BvlDblIS_t.png",
        ":ressources/__system/graphics/borders/BvlDblIS_tl.png",
        ":ressources/__system/graphics/borders/BvlDblIS_tr.png",
        ":ressources/__system/graphics/borders/BvlDblRM_b.png",
        ":ressources/__system/graphics/borders/BvlDblRM_bl.png",
        ":ressources/__system/graphics/borders/BvlDblRM_br.png",
        ":ressources/__system/graphics/borders/BvlDblRM_l.png",
        ":ressources/__system/graphics/borders/BvlDblRM_r.png",
        ":ressources/__system/graphics/borders/BvlDblRM_t.png",
        ":ressources/__system/graphics/borders/BvlDblRM_tl.png",
        ":ressources/__system/graphics/borders/BvlDblRM_tr.png",
        ":ressources/__system/graphics/borders/BvlIM_b.png",
        ":ressources/__system/graphics/borders/BvlIM_bl.png",
        ":ressources/__system/graphics/borders/BvlIM_br.png",
        ":ressources/__system/graphics/borders/BvlIM_l.png",
        ":ressources/__system/graphics/borders/BvlIM_r.png",
        ":ressources/__system/graphics/borders/BvlIM_t.png",
        ":ressources/__system/graphics/borders/BvlIM_tl.png",
        ":ressources/__system/graphics/borders/BvlIM_tr.png",
        ":ressources/__system/graphics/borders/BvlRM_b.png",
        ":ressources/__system/graphics/borders/BvlRM_bl.png",
        ":ressources/__system/graphics/borders/BvlRM_br.png",
        ":ressources/__system/graphics/borders/BvlRM_l.png",
        ":ressources/__system/graphics/borders/BvlRM_r.png",
        ":ressources/__system/graphics/borders/BvlRM_t.png",
        ":ressources/__system/graphics/borders/BvlRM_tl.png",
        ":ressources/__system/graphics/borders/BvlRM_tr.png",
        ":ressources/__system/graphics/borders/CrclBevL_b.png",
        ":ressources/__system/graphics/borders/CrclBevL_b_alpha.png",
        ":ressources/__system/graphics/borders/CrclBevL_bl.png",
        ":ressources/__system/graphics/borders/CrclBevL_bl_alpha.png",
        ":ressources/__system/graphics/borders/CrclBevL_br.png",
        ":ressources/__system/graphics/borders/CrclBevL_br_alpha.png",
        ":ressources/__system/graphics/borders/CrclBevL_l.png",
        ":ressources/__system/graphics/borders/CrclBevL_l_alpha.png",
        ":ressources/__system/graphics/borders/CrclBevL_r.png",
        ":ressources/__system/graphics/borders/CrclBevL_r_alpha.png",
        ":ressources/__system/graphics/borders/CrclBevL_t.png",
        ":ressources/__system/graphics/borders/CrclBevL_t_alpha.png",
        ":ressources/__system/graphics/borders/CrclBevL_tl.png",
        ":ressources/__system/graphics/borders/CrclBevL_tl_alpha.png",
        ":ressources/__system/graphics/borders/CrclBevL_tr.png",
        ":ressources/__system/graphics/borders/CrclBevL_tr_alpha.png",
        ":ressources/__system/graphics/borders/CustomFrame_b.png",
        ":ressources/__system/graphics/borders/CustomFrame_bl.png",
        ":ressources/__system/graphics/borders/CustomFrame_br.png",
        ":ressources/__system/graphics/borders/CustomFrame_l.png",
        ":ressources/__system/graphics/borders/CustomFrame_r.png",
        ":ressources/__system/graphics/borders/CustomFrame_t.png",
        ":ressources/__system/graphics/borders/CustomFrame_tl.png",
        ":ressources/__system/graphics/borders/CustomFrame_tr.png",
        ":ressources/__system/graphics/borders/Glow25_b_alpha.png",
        ":ressources/__system/graphics/borders/Glow25_bl_alpha.png",
        ":ressources/__system/graphics/borders/Glow25_br_alpha.png",
        ":ressources/__system/graphics/borders/Glow25_l_alpha.png",
        ":ressources/__system/graphics/borders/Glow25_r_alpha.png",
        ":ressources/__system/graphics/borders/Glow25_t_alpha.png",
        ":ressources/__system/graphics/borders/Glow25_tl_alpha.png",
        ":ressources/__system/graphics/borders/Glow25_tr_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_b_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_bl_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_br_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_l_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_r_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_t_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_tl_alpha.png",
        ":ressources/__system/graphics/borders/Glow50_tr_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_b_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_bl_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_br_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_l_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_r_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_t_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_tl_alpha.png",
        ":ressources/__system/graphics/borders/HOval100x50_tr_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_b_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_bl_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_br_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_l_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_r_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_t_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_tl_alpha.png",
        ":ressources/__system/graphics/borders/HOval150x75_tr_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_b_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_bl_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_br_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_l_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_r_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_t_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_tl_alpha.png",
        ":ressources/__system/graphics/borders/HOval200x100_tr_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_b_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_bl_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_br_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_l_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_r_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_t_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_tl_alpha.png",
        ":ressources/__system/graphics/borders/HOval60x30_tr_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_b_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_bl_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_br_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_l_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_r_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_t_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_tl_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown2_tr_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_b_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_bl_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_br_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_l_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_r_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_t_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_tl_alpha.png",
        ":ressources/__system/graphics/borders/HelpDown_tr_alpha.png",
        ":ressources/__system/graphics/borders/Line1_b.png",
        ":ressources/__system/graphics/borders/Line1_bl.png",
        ":ressources/__system/graphics/borders/Line1_br.png",
        ":ressources/__system/graphics/borders/Line1_l.png",
        ":ressources/__system/graphics/borders/Line1_r.png",
        ":ressources/__system/graphics/borders/Line1_t.png",
        ":ressources/__system/graphics/borders/Line1_tl.png",
        ":ressources/__system/graphics/borders/Line1_tr.png",
        ":ressources/__system/graphics/borders/Line2_b.png",
        ":ressources/__system/graphics/borders/Line2_bl.png",
        ":ressources/__system/graphics/borders/Line2_br.png",
        ":ressources/__system/graphics/borders/Line2_l.png",
        ":ressources/__system/graphics/borders/Line2_r.png",
        ":ressources/__system/graphics/borders/Line2_t.png",
        ":ressources/__system/graphics/borders/Line2_tl.png",
        ":ressources/__system/graphics/borders/Line2_tr.png",
        ":ressources/__system/graphics/borders/Line4_b.png",
        ":ressources/__system/graphics/borders/Line4_bl.png",
        ":ressources/__system/graphics/borders/Line4_br.png",
        ":ressources/__system/graphics/borders/Line4_l.png",
        ":ressources/__system/graphics/borders/Line4_r.png",
        ":ressources/__system/graphics/borders/Line4_t.png",
        ":ressources/__system/graphics/borders/Line4_tl.png",
        ":ressources/__system/graphics/borders/Line4_tr.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_b_alpha.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_bl_alpha.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_br_alpha.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_l_alpha.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_r_alpha.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_t_alpha.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_tl_alpha.png",
        ":ressources/__system/graphics/borders/MenuHorizontalRounded_tr_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_b_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_bl_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_br_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_l_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_r_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_t_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_tl_alpha.png",
        ":ressources/__system/graphics/borders/MenuVerticalRounded_tr_alpha.png",
        ":ressources/__system/graphics/borders/PF_b_alpha.png",
        ":ressources/__system/graphics/borders/PF_bl_alpha.png",
        ":ressources/__system/graphics/borders/PF_br_alpha.png",
        ":ressources/__system/graphics/borders/PF_l_alpha.png",
        ":ressources/__system/graphics/borders/PF_r_alpha.png",
        ":ressources/__system/graphics/borders/PF_t_alpha.png",
        ":ressources/__system/graphics/borders/PF_tl_alpha.png",
        ":ressources/__system/graphics/borders/PF_tr_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_b_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_bl_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_br_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_l_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_r_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_t_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_tl_alpha.png",
        ":ressources/__system/graphics/borders/VOval100x200_tr_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_b_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_bl_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_br_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_l_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_r_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_t_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_tl_alpha.png",
        ":ressources/__system/graphics/borders/VOval30x60_tr_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_b_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_bl_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_br_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_l_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_r_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_t_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_tl_alpha.png",
        ":ressources/__system/graphics/borders/VOval50x100_tr_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_b_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_bl_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_br_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_l_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_r_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_t_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_tl_alpha.png",
        ":ressources/__system/graphics/borders/VOval75x150_tr_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_b.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_b_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_bl.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_bl_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_br.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_br_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_l.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_l_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_r.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_r_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_t.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_t_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_tl.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_tl_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_tr.png",
        ":ressources/__system/graphics/borders/WindowsPopupStatus_tr_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_b.png",
        ":ressources/__system/graphics/borders/WindowsPopup_b_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_bl.png",
        ":ressources/__system/graphics/borders/WindowsPopup_bl_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_br.png",
        ":ressources/__system/graphics/borders/WindowsPopup_br_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_l.png",
        ":ressources/__system/graphics/borders/WindowsPopup_l_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_r.png",
        ":ressources/__system/graphics/borders/WindowsPopup_r_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_t.png",
        ":ressources/__system/graphics/borders/WindowsPopup_t_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_tl.png",
        ":ressources/__system/graphics/borders/WindowsPopup_tl_alpha.png",
        ":ressources/__system/graphics/borders/WindowsPopup_tr.png",
        ":ressources/__system/graphics/borders/WindowsPopup_tr_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_b.png",
        ":ressources/__system/graphics/borders/aquaLarge_b_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_bl.png",
        ":ressources/__system/graphics/borders/aquaLarge_bl_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_br.png",
        ":ressources/__system/graphics/borders/aquaLarge_br_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_l.png",
        ":ressources/__system/graphics/borders/aquaLarge_l_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_r.png",
        ":ressources/__system/graphics/borders/aquaLarge_r_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_t.png",
        ":ressources/__system/graphics/borders/aquaLarge_t_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_tl.png",
        ":ressources/__system/graphics/borders/aquaLarge_tl_alpha.png",
        ":ressources/__system/graphics/borders/aquaLarge_tr.png",
        ":ressources/__system/graphics/borders/aquaLarge_tr_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_b.png",
        ":ressources/__system/graphics/borders/aquaMed_b_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_bl.png",
        ":ressources/__system/graphics/borders/aquaMed_bl_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_br.png",
        ":ressources/__system/graphics/borders/aquaMed_br_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_l.png",
        ":ressources/__system/graphics/borders/aquaMed_l_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_r.png",
        ":ressources/__system/graphics/borders/aquaMed_r_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_t.png",
        ":ressources/__system/graphics/borders/aquaMed_t_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_tl.png",
        ":ressources/__system/graphics/borders/aquaMed_tl_alpha.png",
        ":ressources/__system/graphics/borders/aquaMed_tr.png",
        ":ressources/__system/graphics/borders/aquaMed_tr_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_b.png",
        ":ressources/__system/graphics/borders/aquaSmall_b_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_bl.png",
        ":ressources/__system/graphics/borders/aquaSmall_bl_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_br.png",
        ":ressources/__system/graphics/borders/aquaSmall_br_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_l.png",
        ":ressources/__system/graphics/borders/aquaSmall_l_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_r.png",
        ":ressources/__system/graphics/borders/aquaSmall_r_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_t.png",
        ":ressources/__system/graphics/borders/aquaSmall_t_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_tl.png",
        ":ressources/__system/graphics/borders/aquaSmall_tl_alpha.png",
        ":ressources/__system/graphics/borders/aquaSmall_tr.png",
        ":ressources/__system/graphics/borders/aquaSmall_tr_alpha.png",
        ":ressources/__system/graphics/borders/aqua_b.png",
        ":ressources/__system/graphics/borders/aqua_b_alpha.png",
        ":ressources/__system/graphics/borders/aqua_bl.png",
        ":ressources/__system/graphics/borders/aqua_bl_alpha.png",
        ":ressources/__system/graphics/borders/aqua_br.png",
        ":ressources/__system/graphics/borders/aqua_br_alpha.png",
        ":ressources/__system/graphics/borders/aqua_l.png",
        ":ressources/__system/graphics/borders/aqua_l_alpha.png",
        ":ressources/__system/graphics/borders/aqua_r.png",
        ":ressources/__system/graphics/borders/aqua_r_alpha.png",
        ":ressources/__system/graphics/borders/aqua_t.png",
        ":ressources/__system/graphics/borders/aqua_t_alpha.png",
        ":ressources/__system/graphics/borders/aqua_tl.png",
        ":ressources/__system/graphics/borders/aqua_tl_alpha.png",
        ":ressources/__system/graphics/borders/aqua_tr.png",
        ":ressources/__system/graphics/borders/aqua_tr_alpha.png",
        ":ressources/__system/graphics/borders/bevel-off_b.png",
        ":ressources/__system/graphics/borders/bevel-off_bl.png",
        ":ressources/__system/graphics/borders/bevel-off_br.png",
        ":ressources/__system/graphics/borders/bevel-off_l.png",
        ":ressources/__system/graphics/borders/bevel-off_r.png",
        ":ressources/__system/graphics/borders/bevel-off_t.png",
        ":ressources/__system/graphics/borders/bevel-off_tl.png",
        ":ressources/__system/graphics/borders/bevel-off_tr.png",
        ":ressources/__system/graphics/borders/bevel-on_b.png",
        ":ressources/__system/graphics/borders/bevel-on_bl.png",
        ":ressources/__system/graphics/borders/bevel-on_br.png",
        ":ressources/__system/graphics/borders/bevel-on_l.png",
        ":ressources/__system/graphics/borders/bevel-on_r.png",
        ":ressources/__system/graphics/borders/bevel-on_t.png",
        ":ressources/__system/graphics/borders/bevel-on_tl.png",
        ":ressources/__system/graphics/borders/bevel-on_tr.png",
        ":ressources/__system/graphics/borders/bevelL-off_b.png",
        ":ressources/__system/graphics/borders/bevelL-off_bl.png",
        ":ressources/__system/graphics/borders/bevelL-off_br.png",
        ":ressources/__system/graphics/borders/bevelL-off_l.png",
        ":ressources/__system/graphics/borders/bevelL-off_r.png",
        ":ressources/__system/graphics/borders/bevelL-off_t.png",
        ":ressources/__system/graphics/borders/bevelL-off_tl.png",
        ":ressources/__system/graphics/borders/bevelL-off_tr.png",
        ":ressources/__system/graphics/borders/bevelL-on_b.png",
        ":ressources/__system/graphics/borders/bevelL-on_bl.png",
        ":ressources/__system/graphics/borders/bevelL-on_br.png",
        ":ressources/__system/graphics/borders/bevelL-on_l.png",
        ":ressources/__system/graphics/borders/bevelL-on_r.png",
        ":ressources/__system/graphics/borders/bevelL-on_t.png",
        ":ressources/__system/graphics/borders/bevelL-on_tl.png",
        ":ressources/__system/graphics/borders/bevelL-on_tr.png",
        ":ressources/__system/graphics/borders/bevelM-off_b.png",
        ":ressources/__system/graphics/borders/bevelM-off_bl.png",
        ":ressources/__system/graphics/borders/bevelM-off_br.png",
        ":ressources/__system/graphics/borders/bevelM-off_l.png",
        ":ressources/__system/graphics/borders/bevelM-off_r.png",
        ":ressources/__system/graphics/borders/bevelM-off_t.png",
        ":ressources/__system/graphics/borders/bevelM-off_tl.png",
        ":ressources/__system/graphics/borders/bevelM-off_tr.png",
        ":ressources/__system/graphics/borders/bevelM-on_b.png",
        ":ressources/__system/graphics/borders/bevelM-on_bl.png",
        ":ressources/__system/graphics/borders/bevelM-on_br.png",
        ":ressources/__system/graphics/borders/bevelM-on_l.png",
        ":ressources/__system/graphics/borders/bevelM-on_r.png",
        ":ressources/__system/graphics/borders/bevelM-on_t.png",
        ":ressources/__system/graphics/borders/bevelM-on_tl.png",
        ":ressources/__system/graphics/borders/bevelM-on_tr.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded105_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded115_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded125_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded135_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded145_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded155_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded15_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded165_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded175_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded185_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded195_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded25_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded35_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded45_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded55_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded65_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded75_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded85_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_b_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_br_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_l_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_r_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_t_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottomMenuRounded95_tr_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_b_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_bl_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_br_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_l_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_r_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_t_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_tl_alpha.png",
        ":ressources/__system/graphics/borders/bottom_cursor_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_b_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_br_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_l_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_r_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_t_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle-down_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_b_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_br_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_l_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_r_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_t_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle-left_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_b_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_br_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_l_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_r_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_t_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle-right_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_b_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_br_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_l_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_r_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_t_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle-up_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle105_b_alpha.png",
        ":ressources/__system/graphics/borders/circle105_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle105_br_alpha.png",
        ":ressources/__system/graphics/borders/circle105_l_alpha.png",
        ":ressources/__system/graphics/borders/circle105_r_alpha.png",
        ":ressources/__system/graphics/borders/circle105_t_alpha.png",
        ":ressources/__system/graphics/borders/circle105_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle105_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle115_b_alpha.png",
        ":ressources/__system/graphics/borders/circle115_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle115_br_alpha.png",
        ":ressources/__system/graphics/borders/circle115_l_alpha.png",
        ":ressources/__system/graphics/borders/circle115_r_alpha.png",
        ":ressources/__system/graphics/borders/circle115_t_alpha.png",
        ":ressources/__system/graphics/borders/circle115_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle115_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle125_b_alpha.png",
        ":ressources/__system/graphics/borders/circle125_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle125_br_alpha.png",
        ":ressources/__system/graphics/borders/circle125_l_alpha.png",
        ":ressources/__system/graphics/borders/circle125_r_alpha.png",
        ":ressources/__system/graphics/borders/circle125_t_alpha.png",
        ":ressources/__system/graphics/borders/circle125_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle125_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle135_b_alpha.png",
        ":ressources/__system/graphics/borders/circle135_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle135_br_alpha.png",
        ":ressources/__system/graphics/borders/circle135_l_alpha.png",
        ":ressources/__system/graphics/borders/circle135_r_alpha.png",
        ":ressources/__system/graphics/borders/circle135_t_alpha.png",
        ":ressources/__system/graphics/borders/circle135_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle135_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle145_b_alpha.png",
        ":ressources/__system/graphics/borders/circle145_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle145_br_alpha.png",
        ":ressources/__system/graphics/borders/circle145_l_alpha.png",
        ":ressources/__system/graphics/borders/circle145_r_alpha.png",
        ":ressources/__system/graphics/borders/circle145_t_alpha.png",
        ":ressources/__system/graphics/borders/circle145_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle145_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle155_b_alpha.png",
        ":ressources/__system/graphics/borders/circle155_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle155_br_alpha.png",
        ":ressources/__system/graphics/borders/circle155_l_alpha.png",
        ":ressources/__system/graphics/borders/circle155_r_alpha.png",
        ":ressources/__system/graphics/borders/circle155_t_alpha.png",
        ":ressources/__system/graphics/borders/circle155_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle155_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle15_b_alpha.png",
        ":ressources/__system/graphics/borders/circle15_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle15_br_alpha.png",
        ":ressources/__system/graphics/borders/circle15_l_alpha.png",
        ":ressources/__system/graphics/borders/circle15_r_alpha.png",
        ":ressources/__system/graphics/borders/circle15_t_alpha.png",
        ":ressources/__system/graphics/borders/circle15_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle15_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle165_b_alpha.png",
        ":ressources/__system/graphics/borders/circle165_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle165_br_alpha.png",
        ":ressources/__system/graphics/borders/circle165_l_alpha.png",
        ":ressources/__system/graphics/borders/circle165_r_alpha.png",
        ":ressources/__system/graphics/borders/circle165_t_alpha.png",
        ":ressources/__system/graphics/borders/circle165_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle165_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle175_b_alpha.png",
        ":ressources/__system/graphics/borders/circle175_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle175_br_alpha.png",
        ":ressources/__system/graphics/borders/circle175_l_alpha.png",
        ":ressources/__system/graphics/borders/circle175_r_alpha.png",
        ":ressources/__system/graphics/borders/circle175_t_alpha.png",
        ":ressources/__system/graphics/borders/circle175_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle175_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle185_b_alpha.png",
        ":ressources/__system/graphics/borders/circle185_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle185_br_alpha.png",
        ":ressources/__system/graphics/borders/circle185_l_alpha.png",
        ":ressources/__system/graphics/borders/circle185_r_alpha.png",
        ":ressources/__system/graphics/borders/circle185_t_alpha.png",
        ":ressources/__system/graphics/borders/circle185_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle185_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle195_b_alpha.png",
        ":ressources/__system/graphics/borders/circle195_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle195_br_alpha.png",
        ":ressources/__system/graphics/borders/circle195_l_alpha.png",
        ":ressources/__system/graphics/borders/circle195_r_alpha.png",
        ":ressources/__system/graphics/borders/circle195_t_alpha.png",
        ":ressources/__system/graphics/borders/circle195_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle195_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle25_b_alpha.png",
        ":ressources/__system/graphics/borders/circle25_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle25_br_alpha.png",
        ":ressources/__system/graphics/borders/circle25_l_alpha.png",
        ":ressources/__system/graphics/borders/circle25_r_alpha.png",
        ":ressources/__system/graphics/borders/circle25_t_alpha.png",
        ":ressources/__system/graphics/borders/circle25_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle25_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle35_b_alpha.png",
        ":ressources/__system/graphics/borders/circle35_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle35_br_alpha.png",
        ":ressources/__system/graphics/borders/circle35_l_alpha.png",
        ":ressources/__system/graphics/borders/circle35_r_alpha.png",
        ":ressources/__system/graphics/borders/circle35_t_alpha.png",
        ":ressources/__system/graphics/borders/circle35_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle35_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle45_b_alpha.png",
        ":ressources/__system/graphics/borders/circle45_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle45_br_alpha.png",
        ":ressources/__system/graphics/borders/circle45_l_alpha.png",
        ":ressources/__system/graphics/borders/circle45_r_alpha.png",
        ":ressources/__system/graphics/borders/circle45_t_alpha.png",
        ":ressources/__system/graphics/borders/circle45_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle45_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle55_b_alpha.png",
        ":ressources/__system/graphics/borders/circle55_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle55_br_alpha.png",
        ":ressources/__system/graphics/borders/circle55_l_alpha.png",
        ":ressources/__system/graphics/borders/circle55_r_alpha.png",
        ":ressources/__system/graphics/borders/circle55_t_alpha.png",
        ":ressources/__system/graphics/borders/circle55_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle55_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle65_b_alpha.png",
        ":ressources/__system/graphics/borders/circle65_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle65_br_alpha.png",
        ":ressources/__system/graphics/borders/circle65_l_alpha.png",
        ":ressources/__system/graphics/borders/circle65_r_alpha.png",
        ":ressources/__system/graphics/borders/circle65_t_alpha.png",
        ":ressources/__system/graphics/borders/circle65_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle65_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle75_b_alpha.png",
        ":ressources/__system/graphics/borders/circle75_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle75_br_alpha.png",
        ":ressources/__system/graphics/borders/circle75_l_alpha.png",
        ":ressources/__system/graphics/borders/circle75_r_alpha.png",
        ":ressources/__system/graphics/borders/circle75_t_alpha.png",
        ":ressources/__system/graphics/borders/circle75_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle75_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle85_b_alpha.png",
        ":ressources/__system/graphics/borders/circle85_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle85_br_alpha.png",
        ":ressources/__system/graphics/borders/circle85_l_alpha.png",
        ":ressources/__system/graphics/borders/circle85_r_alpha.png",
        ":ressources/__system/graphics/borders/circle85_t_alpha.png",
        ":ressources/__system/graphics/borders/circle85_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle85_tr_alpha.png",
        ":ressources/__system/graphics/borders/circle95_b_alpha.png",
        ":ressources/__system/graphics/borders/circle95_bl_alpha.png",
        ":ressources/__system/graphics/borders/circle95_br_alpha.png",
        ":ressources/__system/graphics/borders/circle95_l_alpha.png",
        ":ressources/__system/graphics/borders/circle95_r_alpha.png",
        ":ressources/__system/graphics/borders/circle95_t_alpha.png",
        ":ressources/__system/graphics/borders/circle95_tl_alpha.png",
        ":ressources/__system/graphics/borders/circle95_tr_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_b_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_bl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_br_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_l_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_r_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_t_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_tl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleDown_tr_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_b_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_bl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_br_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_l_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_r_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_t_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_tl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleLeft_tr_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_b_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_bl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_br_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_l_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_r_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_t_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_tl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleRight_tr_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_b_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_bl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_br_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_l_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_r_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_t_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_tl_alpha.png",
        ":ressources/__system/graphics/borders/cursorHoleUp_tr_alpha.png",
        ":ressources/__system/graphics/borders/dbevelL-off_b.png",
        ":ressources/__system/graphics/borders/dbevelL-off_bl.png",
        ":ressources/__system/graphics/borders/dbevelL-off_br.png",
        ":ressources/__system/graphics/borders/dbevelL-off_l.png",
        ":ressources/__system/graphics/borders/dbevelL-off_r.png",
        ":ressources/__system/graphics/borders/dbevelL-off_t.png",
        ":ressources/__system/graphics/borders/dbevelL-off_tl.png",
        ":ressources/__system/graphics/borders/dbevelL-off_tr.png",
        ":ressources/__system/graphics/borders/dbevelL-on_b.png",
        ":ressources/__system/graphics/borders/dbevelL-on_bl.png",
        ":ressources/__system/graphics/borders/dbevelL-on_br.png",
        ":ressources/__system/graphics/borders/dbevelL-on_l.png",
        ":ressources/__system/graphics/borders/dbevelL-on_r.png",
        ":ressources/__system/graphics/borders/dbevelL-on_t.png",
        ":ressources/__system/graphics/borders/dbevelL-on_tl.png",
        ":ressources/__system/graphics/borders/dbevelL-on_tr.png",
        ":ressources/__system/graphics/borders/dbevelM-off_b.png",
        ":ressources/__system/graphics/borders/dbevelM-off_bl.png",
        ":ressources/__system/graphics/borders/dbevelM-off_br.png",
        ":ressources/__system/graphics/borders/dbevelM-off_l.png",
        ":ressources/__system/graphics/borders/dbevelM-off_r.png",
        ":ressources/__system/graphics/borders/dbevelM-off_t.png",
        ":ressources/__system/graphics/borders/dbevelM-off_tl.png",
        ":ressources/__system/graphics/borders/dbevelM-off_tr.png",
        ":ressources/__system/graphics/borders/dbevelM-on_b.png",
        ":ressources/__system/graphics/borders/dbevelM-on_bl.png",
        ":ressources/__system/graphics/borders/dbevelM-on_br.png",
        ":ressources/__system/graphics/borders/dbevelM-on_l.png",
        ":ressources/__system/graphics/borders/dbevelM-on_r.png",
        ":ressources/__system/graphics/borders/dbevelM-on_t.png",
        ":ressources/__system/graphics/borders/dbevelM-on_tl.png",
        ":ressources/__system/graphics/borders/dbevelM-on_tr.png",
        ":ressources/__system/graphics/borders/dbevelS-off_b.png",
        ":ressources/__system/graphics/borders/dbevelS-off_bl.png",
        ":ressources/__system/graphics/borders/dbevelS-off_br.png",
        ":ressources/__system/graphics/borders/dbevelS-off_l.png",
        ":ressources/__system/graphics/borders/dbevelS-off_r.png",
        ":ressources/__system/graphics/borders/dbevelS-off_t.png",
        ":ressources/__system/graphics/borders/dbevelS-off_tl.png",
        ":ressources/__system/graphics/borders/dbevelS-off_tr.png",
        ":ressources/__system/graphics/borders/dbevelS-on_b.png",
        ":ressources/__system/graphics/borders/dbevelS-on_bl.png",
        ":ressources/__system/graphics/borders/dbevelS-on_br.png",
        ":ressources/__system/graphics/borders/dbevelS-on_l.png",
        ":ressources/__system/graphics/borders/dbevelS-on_r.png",
        ":ressources/__system/graphics/borders/dbevelS-on_t.png",
        ":ressources/__system/graphics/borders/dbevelS-on_tl.png",
        ":ressources/__system/graphics/borders/dbevelS-on_tr.png",
        ":ressources/__system/graphics/borders/diamond105_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond105_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond105_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond105_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond105_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond105_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond105_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond105_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond115_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond125_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond135_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond145_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond155_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond15_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond165_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond175_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond185_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond195_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond25_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond35_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond45_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond55_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond65_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond75_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond85_tr_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_b_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_bl_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_br_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_l_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_r_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_t_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_tl_alpha.png",
        ":ressources/__system/graphics/borders/diamond95_tr_alpha.png",
        ":ressources/__system/graphics/borders/down_b_alpha.png",
        ":ressources/__system/graphics/borders/down_bl_alpha.png",
        ":ressources/__system/graphics/borders/down_br_alpha.png",
        ":ressources/__system/graphics/borders/down_l_alpha.png",
        ":ressources/__system/graphics/borders/down_r_alpha.png",
        ":ressources/__system/graphics/borders/down_t_alpha.png",
        ":ressources/__system/graphics/borders/down_tl_alpha.png",
        ":ressources/__system/graphics/borders/down_tr_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_b_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_bl_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_br_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_l_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_r_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_t_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_tl_alpha.png",
        ":ressources/__system/graphics/borders/fuzzyBorder_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded105_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded115_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded125_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded135_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded145_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded155_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded15_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded165_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded175_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded185_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded195_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded25_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded35_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded45_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded55_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded65_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded75_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded85_tr_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_b_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_bl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_br_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_l_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_r_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_t_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_tl_alpha.png",
        ":ressources/__system/graphics/borders/leftMenuRounded95_tr_alpha.png",
        ":ressources/__system/graphics/borders/left_b_alpha.png",
        ":ressources/__system/graphics/borders/left_bl_alpha.png",
        ":ressources/__system/graphics/borders/left_br_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_b_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_bl_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_br_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_l_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_r_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_t_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_tl_alpha.png",
        ":ressources/__system/graphics/borders/left_cursor_tr_alpha.png",
        ":ressources/__system/graphics/borders/left_l_alpha.png",
        ":ressources/__system/graphics/borders/left_r_alpha.png",
        ":ressources/__system/graphics/borders/left_t_alpha.png",
        ":ressources/__system/graphics/borders/left_tl_alpha.png",
        ":ressources/__system/graphics/borders/left_tr_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_b.png",
        ":ressources/__system/graphics/borders/neon150-f_b_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_bl.png",
        ":ressources/__system/graphics/borders/neon150-f_bl_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_br.png",
        ":ressources/__system/graphics/borders/neon150-f_br_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_l.png",
        ":ressources/__system/graphics/borders/neon150-f_l_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_r.png",
        ":ressources/__system/graphics/borders/neon150-f_r_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_t.png",
        ":ressources/__system/graphics/borders/neon150-f_t_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_tl.png",
        ":ressources/__system/graphics/borders/neon150-f_tl_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_tr.png",
        ":ressources/__system/graphics/borders/neon150-f_tr_alpha.png",
        ":ressources/__system/graphics/borders/neon150-f_tr_alpha_r1_c4.png",
        ":ressources/__system/graphics/borders/neon150-f_tr_alpha_r2_c2.png",
        ":ressources/__system/graphics/borders/neon150-f_tr_alpha_r4_c1.png",
        ":ressources/__system/graphics/borders/neon150-n_b.png",
        ":ressources/__system/graphics/borders/neon150-n_b_alpha.png",
        ":ressources/__system/graphics/borders/neon150-n_bl.png",
        ":ressources/__system/graphics/borders/neon150-n_bl_alpha.png",
        ":ressources/__system/graphics/borders/neon150-n_br.png",
        ":ressources/__system/graphics/borders/neon150-n_br_alpha.png",
        ":ressources/__system/graphics/borders/neon150-n_l.png",
        ":ressources/__system/graphics/borders/neon150-n_l_alpha.png",
        ":ressources/__system/graphics/borders/neon150-n_r.png",
        ":ressources/__system/graphics/borders/neon150-n_r_alpha.png",
        ":ressources/__system/graphics/borders/neon150-n_t.png",
        ":ressources/__system/graphics/borders/neon150-n_t_alpha.png",
        ":ressources/__system/graphics/borders/neon150-n_tl.png",
        ":ressources/__system/graphics/borders/neon150-n_tl_alpha.png",
        ":ressources/__system/graphics/borders/neon150-n_tr.png",
        ":ressources/__system/graphics/borders/neon150-n_tr_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_b.png",
        ":ressources/__system/graphics/borders/neon75-f_b_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_bl.png",
        ":ressources/__system/graphics/borders/neon75-f_bl_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_br.png",
        ":ressources/__system/graphics/borders/neon75-f_br_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_l.png",
        ":ressources/__system/graphics/borders/neon75-f_l_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_r.png",
        ":ressources/__system/graphics/borders/neon75-f_r_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_t.png",
        ":ressources/__system/graphics/borders/neon75-f_t_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_tl.png",
        ":ressources/__system/graphics/borders/neon75-f_tl_alpha.png",
        ":ressources/__system/graphics/borders/neon75-f_tr.png",
        ":ressources/__system/graphics/borders/neon75-f_tr_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_b.png",
        ":ressources/__system/graphics/borders/neon75-n_b_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_bl.png",
        ":ressources/__system/graphics/borders/neon75-n_bl_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_br.png",
        ":ressources/__system/graphics/borders/neon75-n_br_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_l.png",
        ":ressources/__system/graphics/borders/neon75-n_l_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_r.png",
        ":ressources/__system/graphics/borders/neon75-n_r_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_t.png",
        ":ressources/__system/graphics/borders/neon75-n_t_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_tl.png",
        ":ressources/__system/graphics/borders/neon75-n_tl_alpha.png",
        ":ressources/__system/graphics/borders/neon75-n_tr.png",
        ":ressources/__system/graphics/borders/neon75-n_tr_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_b_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_bl_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_br_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_l_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_r_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_t_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_tl_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderLeft_tr_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_b_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_bl_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_br_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_l_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_r_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_t_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_tl_alpha.png",
        ":ressources/__system/graphics/borders/newsHeaderRight_tr_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_b_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_bl_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_br_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_l_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_r_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_t_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_tl_alpha.png",
        ":ressources/__system/graphics/borders/newsHeader_tr_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_b.png",
        ":ressources/__system/graphics/borders/pipe100_b_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_bl.png",
        ":ressources/__system/graphics/borders/pipe100_bl_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_br.png",
        ":ressources/__system/graphics/borders/pipe100_br_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_l.png",
        ":ressources/__system/graphics/borders/pipe100_l_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_r.png",
        ":ressources/__system/graphics/borders/pipe100_r_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_t.png",
        ":ressources/__system/graphics/borders/pipe100_t_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_tl.png",
        ":ressources/__system/graphics/borders/pipe100_tl_alpha.png",
        ":ressources/__system/graphics/borders/pipe100_tr.png",
        ":ressources/__system/graphics/borders/pipe100_tr_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_b.png",
        ":ressources/__system/graphics/borders/pipe50_b_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_bl.png",
        ":ressources/__system/graphics/borders/pipe50_bl_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_br.png",
        ":ressources/__system/graphics/borders/pipe50_br_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_l.png",
        ":ressources/__system/graphics/borders/pipe50_l_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_r.png",
        ":ressources/__system/graphics/borders/pipe50_r_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_t.png",
        ":ressources/__system/graphics/borders/pipe50_t_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_tl.png",
        ":ressources/__system/graphics/borders/pipe50_tl_alpha.png",
        ":ressources/__system/graphics/borders/pipe50_tr.png",
        ":ressources/__system/graphics/borders/pipe50_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded105_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded115_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded125_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded135_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded145_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded155_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded15_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded165_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded175_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded185_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded195_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded25_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded35_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded45_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded55_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded65_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded75_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded85_tr_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_b_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_bl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_br_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_l_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_r_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_t_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_tl_alpha.png",
        ":ressources/__system/graphics/borders/rightMenuRounded95_tr_alpha.png",
        ":ressources/__system/graphics/borders/right_b_alpha.png",
        ":ressources/__system/graphics/borders/right_bl_alpha.png",
        ":ressources/__system/graphics/borders/right_br_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_b_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_bl_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_br_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_l_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_r_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_t_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_tl_alpha.png",
        ":ressources/__system/graphics/borders/right_cursor_tr_alpha.png",
        ":ressources/__system/graphics/borders/right_l_alpha.png",
        ":ressources/__system/graphics/borders/right_r_alpha.png",
        ":ressources/__system/graphics/borders/right_t_alpha.png",
        ":ressources/__system/graphics/borders/right_tl_alpha.png",
        ":ressources/__system/graphics/borders/right_tr_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_b.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_b_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_bl.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_bl_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_br.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_br_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_l.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_l_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_r.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_r_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_t.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_t_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_tl.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_tl_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_tr.png",
        ":ressources/__system/graphics/borders/sbSquaredLarge_tr_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_b.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_b_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_bl.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_bl_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_br.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_br_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_l.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_l_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_r.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_r_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_t.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_t_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_tl.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_tl_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_tr.png",
        ":ressources/__system/graphics/borders/sbSquaredMed_tr_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_b.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_b_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_bl.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_bl_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_br.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_br_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_l.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_l_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_r.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_r_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_t.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_t_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_tl.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_tl_alpha.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_tr.png",
        ":ressources/__system/graphics/borders/sbSquaredSmall_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded105_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded115_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded125_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded135_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded145_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded155_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded15_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded165_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded175_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded185_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded195_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded25_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded35_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded45_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded55_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded65_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded75_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded85_tr_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_b_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_bl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_br_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_l_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_r_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_t_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_tl_alpha.png",
        ":ressources/__system/graphics/borders/topMenuRounded95_tr_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_b_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_bl_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_br_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_l_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_r_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_t_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_tl_alpha.png",
        ":ressources/__system/graphics/borders/top_cursor_tr_alpha.png",
        ":ressources/__system/graphics/borders/up_b_alpha.png",
        ":ressources/__system/graphics/borders/up_bl_alpha.png",
        ":ressources/__system/graphics/borders/up_br_alpha.png",
        ":ressources/__system/graphics/borders/up_l_alpha.png",
        ":ressources/__system/graphics/borders/up_r_alpha.png",
        ":ressources/__system/graphics/borders/up_t_alpha.png",
        ":ressources/__system/graphics/borders/up_tl_alpha.png",
        ":ressources/__system/graphics/borders/up_tr_alpha.png",
        ":ressources/__system/graphics/sliders/aqua_b_alpha.png",
        ":ressources/__system/graphics/sliders/aqua_b.png",
        ":ressources/__system/graphics/sliders/aqua_h_alpha.png",
        ":ressources/__system/graphics/sliders/aqua_h.png",
        ":ressources/__system/graphics/sliders/aqua_l_alpha.png",
        ":ressources/__system/graphics/sliders/aqua_l.png",
        ":ressources/__system/graphics/sliders/aqua_r_alpha.png",
        ":ressources/__system/graphics/sliders/aqua_r.png",
        ":ressources/__system/graphics/sliders/aquaS_b_alpha.png",
        ":ressources/__system/graphics/sliders/aquaS_b.png",
        ":ressources/__system/graphics/sliders/aquaS_h_alpha.png",
        ":ressources/__system/graphics/sliders/aquaS_h.png",
        ":ressources/__system/graphics/sliders/aquaS_l_alpha.png",
        ":ressources/__system/graphics/sliders/aquaS_l.png",
        ":ressources/__system/graphics/sliders/aquaS_r_alpha.png",
        ":ressources/__system/graphics/sliders/aquaS_r.png",
        ":ressources/__system/graphics/sliders/aquaS_t_alpha.png",
        ":ressources/__system/graphics/sliders/aquaS_t.png",
        ":ressources/__system/graphics/sliders/aquaS_v_alpha.png",
        ":ressources/__system/graphics/sliders/aquaS_v.png",
        ":ressources/__system/graphics/sliders/aqua_t_alpha.png",
        ":ressources/__system/graphics/sliders/aqua_t.png",
        ":ressources/__system/graphics/sliders/aqua_v_alpha.png",
        ":ressources/__system/graphics/sliders/aqua_v.png",
        ":ressources/__system/graphics/sliders/Ball_b_alpha.png",
        ":ressources/__system/graphics/sliders/Ball_b.png",
        ":ressources/__system/graphics/sliders/Ball_h_alpha.png",
        ":ressources/__system/graphics/sliders/Ball_h.png",
        ":ressources/__system/graphics/sliders/Ball_l_alpha.png",
        ":ressources/__system/graphics/sliders/Ball_l.png",
        ":ressources/__system/graphics/sliders/Ball_r_alpha.png",
        ":ressources/__system/graphics/sliders/Ball_r.png",
        ":ressources/__system/graphics/sliders/Ball_t_alpha.png",
        ":ressources/__system/graphics/sliders/Ball_t.png",
        ":ressources/__system/graphics/sliders/Ball_v_alpha.png",
        ":ressources/__system/graphics/sliders/Ball_v.png",
        ":ressources/__system/graphics/sliders/CircleL_b_alpha.png",
        ":ressources/__system/graphics/sliders/CircleL_h_alpha.png",
        ":ressources/__system/graphics/sliders/CircleL_l_alpha.png",
        ":ressources/__system/graphics/sliders/CircleL_r_alpha.png",
        ":ressources/__system/graphics/sliders/CircleL_t_alpha.png",
        ":ressources/__system/graphics/sliders/CircleL_v_alpha.png",
        ":ressources/__system/graphics/sliders/CircleM_b_alpha.png",
        ":ressources/__system/graphics/sliders/CircleM_h_alpha.png",
        ":ressources/__system/graphics/sliders/CircleM_l_alpha.png",
        ":ressources/__system/graphics/sliders/CircleM_r_alpha.png",
        ":ressources/__system/graphics/sliders/CircleM_t_alpha.png",
        ":ressources/__system/graphics/sliders/CircleM_v_alpha.png",
        ":ressources/__system/graphics/sliders/CircleS_b_alpha.png",
        ":ressources/__system/graphics/sliders/CircleS_h_alpha.png",
        ":ressources/__system/graphics/sliders/CircleS_l_alpha.png",
        ":ressources/__system/graphics/sliders/CircleS_r_alpha.png",
        ":ressources/__system/graphics/sliders/CircleS_t_alpha.png",
        ":ressources/__system/graphics/sliders/CircleS_v_alpha.png",
        ":ressources/__system/graphics/sliders/LineL_b.png",
        ":ressources/__system/graphics/sliders/LineL_h.png",
        ":ressources/__system/graphics/sliders/LineL_l.png",
        ":ressources/__system/graphics/sliders/LineL_r.png",
        ":ressources/__system/graphics/sliders/LineL_t.png",
        ":ressources/__system/graphics/sliders/LineL_v.png",
        ":ressources/__system/graphics/sliders/LineM_b.png",
        ":ressources/__system/graphics/sliders/LineM_h.png",
        ":ressources/__system/graphics/sliders/LineM_l.png",
        ":ressources/__system/graphics/sliders/LineM_r.png",
        ":ressources/__system/graphics/sliders/LineM_t.png",
        ":ressources/__system/graphics/sliders/LineM_v.png",
        ":ressources/__system/graphics/sliders/LineS_b.png",
        ":ressources/__system/graphics/sliders/LineS_h.png",
        ":ressources/__system/graphics/sliders/LineS_l.png",
        ":ressources/__system/graphics/sliders/LineS_r.png",
        ":ressources/__system/graphics/sliders/LineS_t.png",
        ":ressources/__system/graphics/sliders/LineS_v.png",
        ":ressources/__system/graphics/sliders/prec_b_alpha.png",
        ":ressources/__system/graphics/sliders/prec_h_alpha.png",
        ":ressources/__system/graphics/sliders/prec_l_alpha.png",
        ":ressources/__system/graphics/sliders/prec_r_alpha.png",
        ":ressources/__system/graphics/sliders/prec_t_alpha.png",
        ":ressources/__system/graphics/sliders/prec_v_alpha.png",
        ":ressources/__system/graphics/sliders/windowActive_b_alpha.png",
        ":ressources/__system/graphics/sliders/windowActive_b.png",
        ":ressources/__system/graphics/sliders/windowActive_h_alpha.png",
        ":ressources/__system/graphics/sliders/windowActive_h.png",
        ":ressources/__system/graphics/sliders/windowActive_l_alpha.png",
        ":ressources/__system/graphics/sliders/windowActive_l.png",
        ":ressources/__system/graphics/sliders/windowActive_r_alpha.png",
        ":ressources/__system/graphics/sliders/windowActive_r.png",
        ":ressources/__system/graphics/sliders/windowActive_t_alpha.png",
        ":ressources/__system/graphics/sliders/windowActive_t.png",
        ":ressources/__system/graphics/sliders/windowActive_v_alpha.png",
        ":ressources/__system/graphics/sliders/windowActive_v.png",
        ":ressources/__system/graphics/sliders/windows_b.png",
        ":ressources/__system/graphics/sliders/windows_h.png",
        ":ressources/__system/graphics/sliders/windows_l.png",
        ":ressources/__system/graphics/sliders/windows_r.png",
        ":ressources/__system/graphics/sliders/windows_t.png",
        ":ressources/__system/graphics/sliders/windows_v.png",
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

    mSystemConfigsCreated = !err;
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

    if (!fs::exists(mPath + "/__system/graphics/fonts/arial.ttf"))
        mDirStructureCreated = false;

    if (mDirStructureCreated)
        return true;

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

    pfad = mPath + "/__system/fonts";

    if (!_makeDir(pfad))
        return false;

    pfad = mPath + "/__system/images";

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

    mDirStructureCreated = true;
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

        // Check if target path exists and create it if not.
        if ((pos = bname.find_last_of("/")) != string::npos)
        {
            string targetPath = bname.substr(0, pos);
            QDir dir = QDir(targetPath.c_str());

            if (!dir.exists())
            {
                if (!dir.mkpath(targetPath.c_str()))
                {
                    MSG_ERROR("Error creating path <" << targetPath << ">");
                    return false;
                }
            }
        }


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
#ifdef Q_OS_IOS
    Str = QASettings::getLibraryPath().toStdString();
#else
    char *tmp = getenv("TMP");

    if (!tmp)
        tmp = getenv("TEMP");

    if (!tmp)
        tmp = getenv("HOME");
    else
        tmp = (char *)"/tmp";

    Str.assign(tmp);
#endif
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
    string surface = TConfig::getFtpSurface();
    string target = mPath + "/" + surface;
    size_t pos = 0;

    if ((pos = mPath.find_last_of("/")) != string::npos)
        target = mPath.substr(0, pos) + "/" + surface;

    if (!force)
    {
        if (!isVirgin() && TConfig::getFtpDownloadTime() > 0)
            return false;
    }

    MSG_INFO("Starting download of surface " << surface << " from " << TConfig::getController());

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

    if (!reader.copyOverFTP(surface, target))
    {
        if (TConfig::getFtpDownloadTime() == 0)
        {
            createDirectoryStructure();
            createSystemConfigs();
            createPanelConfigs();
        }

        mDemoPageCreated = false;
        createDemoPage();
        return false;
    }

    if (_processEvents)
        _processEvents();

    if (!reader.unpack(target, mPath))
    {
        MSG_ERROR("Unpacking was not successfull.");
        mDemoPageCreated = false;
        createDemoPage();
        return false;
    }

    if (!force || !dir.exists(mPath + "/__system"))
    {
        createDirectoryStructure();
        createSystemConfigs();
        createPanelConfigs();
    }

    if (_processEvents)
        _processEvents();

    dir.dropFile(target);       // We remove our traces
    dir.dropFile(mPath + SYSTEM_DEFAULT);   // No more system default files
    TConfig::saveFtpDownloadTime(time(NULL));
    TConfig::saveSettings();
    return true;
}

vector<TTPInit::FILELIST_t>& TTPInit::getFileList(const string& filter)
{
    DECL_TRACER("TTPInit::getFileList(const string& filter)");

    string netlinx = TConfig::getController();

    if (netlinx.empty() || netlinx == "0.0.0.0")
    {
        MSG_WARNING("Refusing to connect to " << netlinx << ":21!");
        mDirList.clear();
        return mDirList;
    }

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
    MSG_DEBUG("Trying to login <" << sUser << ", ********>");

    if (!ftp->Login(sUser.c_str(), sPass.c_str()))
    {
        delete ftp;
        return mDirList;
    }

    string tmpFile = getTmpFileName();
    MSG_DEBUG("Reading remote directory / into file " << tmpFile);
    ftp->Dir(tmpFile.c_str(), "/");
    ftp->Quit();
    delete ftp;
    mDirList.clear();

    try
    {
        bool oldNetLinx = false;
        char buffer[1024];
        string uFilter = toUpper((std::string&)filter);

        std::ifstream ifile(tmpFile);

        while (ifile.getline(buffer, sizeof(buffer)))
        {
            string buf = buffer;
            string fname, sSize;
            size_t size = 0;
            // We must detect whether we have a new NetLinx or an old one.
            if (buf.at(42) != ' ')
                oldNetLinx = true;

            // Filter out the filename and it's size
            if (oldNetLinx)
            {
                size = atoll(buf.substr(27, 12).c_str());
                fname = buf.substr(53);
            }
            else
            {
                size = atoll(buf.substr(30, 12).c_str());
                fname = buf.substr(56);
            }

            if (!filter.empty())
            {
                if (endsWith(toUpper(buf), uFilter))
                {
                    FILELIST_t fl;
                    fl.size = size;
                    fl.fname = fname;
                    mDirList.push_back(fl);
                }
            }
            else
            {
                FILELIST_t fl;
                fl.size = size;
                fl.fname = fname;
                mDirList.push_back(fl);
            }
        }

        ifile.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error opening file " << tmpFile << ": " << e.what());
    }

    fs::remove(tmpFile);

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        if (mDirList.size() > 0)
        {
            vector<FILELIST_t>::iterator iter;

            for (iter = mDirList.begin(); iter != mDirList.end(); ++iter)
            {
                MSG_DEBUG("File: " << iter->size << " " << iter->fname);
            }
        }
    }

    return mDirList;
}

int TTPInit::progressCallback(off64_t xfer)
{
    DECL_TRACER("TTPInit::progressCallback(off64_t xfer)");

    if (_progressBar && xfer > 0)
    {
        int percent = 0;

        if (mFileSize > 0)
            percent = (int)(100.0 / (long double)mFileSize * (long double)xfer);
        else
            percent = 50;

        _progressBar(percent);
    }

    if (_processEvents && xfer > 0)
        _processEvents();

    return 1;
}

off64_t TTPInit::getFileSize(const string& file)
{
    DECL_TRACER("TTPInit::getFileSize(const string& file)");

    vector<FILELIST_t>::iterator iter;

    if (!mDirList.empty())
    {
        for (iter = mDirList.begin(); iter != mDirList.end(); ++iter)
        {
            if (iter->fname == file)
                return iter->size;
        }
    }

    // Here we know that we've no files in our cache. Therefor we'll read from
    // the NetLinx, if possible.
    getFileList(".tp4");

    if (mDirList.empty())
        return 0;

    // Now search again for the file.
    for (iter = mDirList.begin(); iter != mDirList.end(); ++iter)
    {
        if (iter->fname == file)
            return iter->size;
    }

    // The file doesn't exist, or we couldn't read from a NetLinx.
    return 0;
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

        if (!fs::exists(mPath + "/prj.xma") || !fs::exists(mPath + "/manifest.xma"))
            return true;
    }
    catch (std::exception& e)
    {
        MSG_ERROR("File system error: " << e.what());
        return true;
    }

    return false;
}

bool TTPInit::makeSystemFiles()
{
    DECL_TRACER("TTPInit::makeSystemFiles()");

    if (!fs::exists(mPath + "/__system/graphics/borders/AMXeliteL-off_b.png"))
        return createSystemConfigs();

    return true;
}

bool TTPInit::reinitialize()
{
    DECL_TRACER("TTPInit::reinitialize()");

    bool err = false;

    if (!createDirectoryStructure())
    {
        MSG_WARNING("Error creating the directory structure!");
    }

    if (!createSystemConfigs())
    {
        MSG_WARNING("Error creating system graphics!");
    }

    if (!createPanelConfigs())
    {
        MSG_ERROR("Error creating the panel configuration!");
        err = true;
    }

    if (!loadSurfaceFromController())
    {
        string surface = TConfig::getFtpSurface();
        string ctrl = TConfig::getController();
        MSG_WARNING("Couldn't load the surface " << surface << " from " << ctrl);
    }

    return !err;
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

bool TTPInit::haveSystemMarker()
{
    DECL_TRACER("TTPInit::haveSystemMarker()");

    try
    {
        string marker = TConfig::getConfigPath() + SYSTEM_DEFAULT;
        return fs::exists(marker);
    }
    catch (std::exception& e)
    {
        MSG_ERROR("File system error: " << e.what())
        return false;
    }

    return true;
}

#ifdef Q_OS_ANDROID
bool TTPInit::askPermissions()
{
    DECL_TRACER("TTPInit::askPermissions()");

    QStringList permissions = { "android.permission.READ_EXTERNAL_STORAGE", "android.permission.WRITE_EXTERNAL_STORAGE" };
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QtAndroid::PermissionResultMap perms = QtAndroid::requestPermissionsSync(permissions);

    for (auto iter = perms.begin(); iter != perms.end(); ++iter)
    {
        if (iter.value() == QtAndroid::PermissionResult::Denied)
            return false;
    }
#else
    for (auto iter = permissions.begin(); iter != permissions.end(); ++iter)
    {
        QFuture<QtAndroidPrivate::PermissionResult> result = QtAndroidPrivate::requestPermission(*iter);

        if (result.result() == QtAndroidPrivate::Denied)
            return false;
    }
#endif
    return true;
}
#endif

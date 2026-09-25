/*
 * Copyright (C) 2020 to 2026 by Andreas Theofilu <andreas@theosys.at>
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
#ifdef __MACH__
#include <json/json.h>
#else
#include <jsoncpp/json/json.h>
#endif
#include <unistd.h>

#include "tsettings.h"
#include "texpat++.h"
#include "terror.h"
#include "ttpinit.h"

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
using std::ifstream;
using namespace Expat;

const int FILE_VERSION = 1;

TSettings::TSettings(const string& path)
    : mPath(path)
{
    DECL_TRACER("TSettings::TSettings(const string& path)");

    MSG_DEBUG("Loading from path: " << path);

    if (TTPInit::isTsf())
        loadSettingsJson(true);
    else
        loadSettings(true);
}

bool TSettings::loadSettings(bool initial)
{
    DECL_TRACER("TSettings::loadSettings()");

    if (!initial)
    {
        mResourceLists.clear();
    }

    TError::clear();
    string fname = makeFileName(mPath, "prj.xma");

    if (!isValidFile())
    {
        MSG_ERROR("Error: File " << fname << " doesn't exist or can't be opened!");
        SET_ERROR_MSG("Error opening file!");
        return false;
    }

    TExpat xml(fname);

    if (!TTPInit::isG5())
        xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
    {
        SET_ERROR_MSG("Error parsing file " + fname);
        return false;
    }

    int depth = 0;
    size_t index = 0;
    MSG_DEBUG("Reading version info ...");

    if (xml.getElementIndex("versionInfo", &depth) == TExpat::npos)
    {
        SET_ERROR_MSG("Couldn't find the project version information! Broken surface?");
        return false;
    }

    depth++;
    bool valid = false;

    mSetup.versionInfo.formatVersion = xml.getElementInt("formatVersion", depth);
    mSetup.versionInfo.graphicsVersion = xml.getElementInt("graphicsVersion", depth);
    mSetup.versionInfo.fileVersion = xml.getElement("fileVersion", depth);
    mSetup.versionInfo.designVersion = xml.getElement("designVersion", depth);
    mSetup.versionInfo.g5appsVersion = xml.getElementInt("g5appsVersion", depth, &valid);
    MSG_DEBUG("formatVersion: " << mSetup.versionInfo.formatVersion << ", graphicsVersion: " << mSetup.versionInfo.graphicsVersion << ", valid: " << (valid ? "TRUE" : "FALSE"));

    if (mSetup.versionInfo.formatVersion >= 20 && mSetup.versionInfo.graphicsVersion >= 20)
    {
        mIsG5 = true;
        MSG_INFO("Detected a G5 file");

        if (!valid)
            mSetup.versionInfo.g5appsVersion = 1;
    }
    else
    {
        mSetup.versionInfo.g5appsVersion = 0;   // No G5 file
        mIsG5 = false;
        MSG_INFO("Detected a G4 file");
    }

    MSG_DEBUG("Reading project info ...");

    if (xml.getElementIndex("projectInfo", &depth) == TExpat::npos)
    {
        SET_ERROR_MSG("Couldn't find the project information! Broken surface?");
        return false;
    }

    depth++;
    mProject.protection = xml.getElement("protection", depth);
    mProject.password = xml.getElement("password", depth);
    vector<Expat::ATTRIBUTE_t> attr = xml.getAttributes();
    mProject.encrypted = xml.getAttributeInt("encrypted", attr);
    mProject.panelType = xml.getElement("panelType", depth);
    mProject.fileRevision = xml.getElement("fileRevision", depth);
    mProject.dealerID = xml.getElement("dealerId", depth);
    mProject.jobName = xml.getElement("jobName", depth);
    mProject.salesOrder = xml.getElement("salesOrder", depth);
    mProject.purchaseOrder = xml.getElement("purchaseOrder", depth);
    mProject.jobComment = xml.getElement("jobComment", depth);
    mProject.designerID = xml.getElement("designerId", depth);
    mProject.creationDate = xml.getElement("creationDate", depth);
    mProject.revisionDate = xml.getElement("revisionDate", depth);
    mProject.lastSaveDate = xml.getElement("lastSaveDate", depth);
    mProject.fileName = xml.getElement("fileName", depth);
    mProject.colorChoice = xml.getElement("colorChoice", depth);
    mProject.specifyPortCount = xml.getElementInt("specifyPortCount", depth);
    mProject.specifyChanCount = xml.getElementInt("specifyChanCount", depth);

    MSG_DEBUG("Reading support file list ...");

    if (xml.getElementIndex("supportFileList", &depth) == TExpat::npos)
    {
        SET_ERROR_MSG("Couldn't find the support file list! Broken surface?");
        return false;
    }

    depth++;
    valid = false;

    mSetup.supportFiles.mapFile = xml.getElement("mapFile", depth);
    mSetup.supportFiles.colorFile = xml.getElement("colorFile", depth);
    mSetup.supportFiles.fontFile = xml.getElement("fontFile", depth);
    mSetup.supportFiles.themeFile = xml.getElement("themeFile", depth);

    if (!mIsG5)
        mSetup.supportFiles.iconFile = xml.getElement("iconFile", depth);

    mSetup.supportFiles.externalButtonFile = xml.getElement("externalButtonFile", depth);

    if (mIsG5)
    {
        mSetup.supportFiles.appFile = xml.getElement("appFile", depth);
        mSetup.supportFiles.logFile = xml.getElement("logFile", depth);
    }

    MSG_DEBUG("Map file:     " << mSetup.supportFiles.mapFile);
    MSG_DEBUG("Color file:   " << mSetup.supportFiles.colorFile);
    MSG_DEBUG("Font file:    " << mSetup.supportFiles.fontFile);
    MSG_DEBUG("Theme file:   " << mSetup.supportFiles.themeFile);

    if (!mIsG5)
        MSG_DEBUG("IconFile:     " << mSetup.supportFiles.iconFile);

    MSG_DEBUG("Ext. buttons: " << mSetup.supportFiles.externalButtonFile);

    if (mIsG5)
    {
        MSG_DEBUG("App file:     " << mSetup.supportFiles.appFile);
        MSG_DEBUG("Log file:     " << mSetup.supportFiles.logFile);
    }

    MSG_DEBUG("Reading panel setup ...");

    if ((index = xml.getElementIndex("panelSetup", &depth)) == TExpat::npos)
    {
        SET_ERROR_MSG("Couldn't find the section \"panelSetup\" in file!");
        return false;
    }

    depth++;
    string value;
    mSetup.portCount = xml.getElementInt("portCount", depth);
    mSetup.setupPort = xml.getElementInt("setupPort", depth);
    mSetup.addressCount = xml.getElementInt("addressCount", depth);
    mSetup.channelCount = xml.getElementInt("channelCount", depth);
    mSetup.levelCount = xml.getElementInt("levelCount", depth);
    mSetup.powerUpPage = xml.getElement("powerUpPage", depth);

    value = xml.getElement("powerUpPopup", depth);

    if (!value.empty())
    {
        mSetup.powerUpPopup.push_back(value);
        bool valid;

        do
        {
            value = xml.getNextElement("powerUpPopup", depth, &valid);

            if (valid)
            {
                mSetup.powerUpPopup.push_back(value);
                MSG_DEBUG("powerUpPopup: " << value);
            }
        }
        while (valid);
    }

    xml.setIndex(index);
    mSetup.feedbackBlinkRate = xml.getElementInt("feedbackBlinkRate", depth);
    mSetup.startupString = xml.getElement("startupString", depth);
    mSetup.wakeupString = xml.getElement("wakeupString", depth);
    mSetup.sleepString = xml.getElement("sleepString", depth);
    mSetup.standbyString = xml.getElement("standbyString", depth);
    mSetup.shutdownString = xml.getElement("shutdownString", depth);
    mSetup.idlePage = xml.getElement("idlePage", depth);
    mSetup.idleTimeout = xml.getElementInt("idleTimeout", depth);
    mSetup.extButtonsKey = xml.getElementInt("extButtonsKey", depth);
    mSetup.screenWidth = xml.getElementInt("screenWidth", depth);
    mSetup.screenHeight = xml.getElementInt("screenHeight", depth);
    mSetup.screenRefresh = xml.getElementInt("screenRefresh", depth);
    mSetup.screenRotate = xml.getElementInt("screenRotate", depth);
    mSetup.screenDescription = xml.getElement("screenDescription", depth);
    mSetup.pageTracking = xml.getElementInt("pageTracking", depth);
    mSetup.cursor = xml.getElementInt("cursor", depth);
    mSetup.brightness = xml.getElementInt("brightness", depth);
    mSetup.lightSensorLevelPort = xml.getElementInt("lightSensorLevelPort", depth);
    mSetup.lightSensorLevelCode = xml.getElementInt("lightSensorLevelCode", depth);
    mSetup.lightSensorChannelPort = xml.getElementInt("lightSensorChannelPort", depth);
    mSetup.lightSensorChannelCode = xml.getElementInt("lightSensorChannelCode", depth);
    mSetup.motionSensorChannelPort = xml.getElementInt("motionSensorChannelPort", depth);
    mSetup.motionSensorChannelCode = xml.getElementInt("motionSensorChannelCode", depth);
    mSetup.batteryLevelPort = xml.getElementInt("batteryLevelPort", depth);
    mSetup.batteryLevelCode = xml.getElementInt("batteryLevelCode", depth);
    mSetup.irPortAMX38Emit = xml.getElementInt("irPortAMX38Emit", depth);
    mSetup.irPortAMX455Emit = xml.getElementInt("irPortAMX455Emit", depth);
    mSetup.irPortAMX38Recv = xml.getElementInt("irPortAMX38Recv", depth);
    mSetup.irPortAMX455Recv = xml.getElementInt("irPortAMX455Recv", depth);
    mSetup.irPortUser1 = xml.getElementInt("irPortUser1", depth);
    mSetup.irPortUser2 = xml.getElementInt("irPortUser2", depth);
    mSetup.cradleChannelPort = xml.getElementInt("cradleChannelPort", depth);
    mSetup.cradleChannelCode = xml.getElementInt("cradleChannelCode", depth);
    mSetup.uniqueID = xml.getElementInt("uniqueID", depth);
    mSetup.appCreated = xml.getElementInt("appCreated", depth);
    mSetup.buildNumber = xml.getElementInt("buildNumber", depth);
    mSetup.appModified = xml.getElement("appModified", depth);
    mSetup.buildNumberMod = xml.getElementInt("buildNumberMod", depth);
    mSetup.buildStatusMod = xml.getElement("buildStatusMod", depth);
    mSetup.activePalette = xml.getElementInt("activePalette", depth);
    mSetup.marqueeSpeed = xml.getElementInt("marqueeSpeed", depth);
    mSetup.setupPagesProject = xml.getElementInt("setupPagesProject", depth);
    mSetup.voipCommandPort = xml.getElementInt("voipCommandPort", depth);

    MSG_DEBUG("Reading resource list ...");

    if ((index = xml.getElementIndex("resourceList", &depth)) == TExpat::npos)
    {
        MSG_WARNING("Missing element \"resourceList\" in file!");
    }

    string name, content;
    vector<ATTRIBUTE_t> attrs;

    if (index != TExpat::npos)
    {
        depth++;
        size_t oldIndex = 0;
        MSG_DEBUG("Index " << index << " and depth " << depth << " and entity " << xml.getElementName());

        do
        {
            attrs = xml.getAttributes();
            string type = xml.getAttribute("type", attrs);
            RESOURCE_LIST_T list = findResourceType(type);
            MSG_DEBUG("resource type: " << type);

            if (mResourceLists.size() == 0 || list.type.empty())
            {
                list.type = type;
                list.ressource.clear();
                mResourceLists.push_back(list);
            }

            RESOURCE_T resource;

            while ((index = xml.getNextElementIndex("resource", depth)) != TExpat::npos)
            {
                while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
                {
                    string e = name;

                    if (e.compare("name") == 0)
                        resource.name = content;
                    else if (e.compare("protocol") == 0)
                        resource.protocol = content;
                    else if (e.compare("host") == 0)
                        resource.host = content;
                    else if (e.compare("file") == 0)
                        resource.file = content;
                    else if (e.compare("password") == 0)
                    {
                        resource.password = content;
                        int enc = xml.getAttributeInt("encrypted", attrs);

                        if (enc != 0)
                            resource.encrypted = true;
                        else
                            resource.encrypted = false;
                    }
                    else if (e.compare("user") == 0)
                        resource.user = content;
                    else if (e.compare("path") == 0)
                        resource.path = content;
                    else if (e.compare("refresh") == 0)
                        resource.refresh = xml.convertElementToInt(content);
                    else if (e.compare("dynamo") == 0)
                        resource.dynamo = ((xml.convertElementToInt(content) == 0) ? false : true);
                    else if (e.compare("preserve") == 0)
                        resource.preserve = ((xml.convertElementToInt(content) == 0) ? false : true);

                    oldIndex = index;
                }

                list.ressource.push_back(resource);
                MSG_DEBUG("Scheme: " << resource.protocol << ", Host: " << resource.host << ", Path: " << resource.path << ", File: " << resource.file << ", Name: " << resource.name);
                resource.clear();

                if (index == TExpat::npos)
                    index = oldIndex + 2;
            }

            vector<RESOURCE_LIST_T>::iterator itResList;

            for (itResList = mResourceLists.begin(); itResList != mResourceLists.end(); ++itResList)
            {
                if (itResList->type.compare(type) == 0)
                {
                    mResourceLists.erase(itResList);
                    mResourceLists.push_back(list);
                    break;
                }
            }
        }
        while ((index = xml.getNextElementIndex("resourceList", depth)) != TExpat::npos);
    }

    MSG_DEBUG("Reading palette list ...");

    if (xml.getElementIndex("paletteList", &depth) == TExpat::npos)
    {
        if (!isG5())
        {
            MSG_WARNING("There exists no color palette! There will be only the system colors available.");
        }
        else
        {
            PALETTE_SETUP ps;
            ps.name = ps.file = mSetup.supportFiles.colorFile;
            ps.paletteID = 1;
            mSetup.palettes.push_back(ps);
        }

        return true;
    }

    depth++;

    while ((index = xml.getNextElementIndex("palette", depth)) != TExpat::npos)
    {
        PALETTE_SETUP ps;

        while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
        {
            if (name.compare("name") == 0)
                ps.name = content;
            else if (name.compare("file") == 0)
                ps.file = content;
            else if (name.compare("paletteID") == 0)
                ps.paletteID = xml.convertElementToInt(content);
        }

        mSetup.palettes.push_back(ps);
    }

    return true;
}

bool TSettings::loadSettingsJson(bool initial)
{
    DECL_TRACER("TSettings::loadSettingsJson(bool initial)");

    if (!initial)
    {
        mResourceLists.clear();
    }

    TError::clear();
    string fname = makeFileName(mPath, "prj_.json");

    if (!isValidFile())
    {
        MSG_ERROR("Error: File " << fname << " doesn't exist or can't be opened!");
        SET_ERROR_MSG("Error opening file!");
        return false;
    }

    mIsG5 = true;       // .tsf files are always G5!
    Json::Value root;
    ifstream config_doc(fname, ifstream::binary);
    config_doc >> root;


    mSetup.versionInfo.fsfVersion = root["versionInfo"].get("fileVersion", "").asInt();

    if (mSetup.versionInfo.fsfVersion != FILE_VERSION)
    {
        MSG_ERROR("Invalid file version " << mSetup.versionInfo.fileVersion << "!");
        SET_ERROR();
        return false;
    }

    mSetup.versionInfo.fileVersion = root["versionInfo"].get("fileVersion", "").asString();
    mSetup.versionInfo.formatVersion = root["versionInfo"].get("formatVersion", 0).asInt();
    mSetup.versionInfo.graphicsVersion = root["versionInfo"].get("graphicsVersion", 0).asInt();
    mSetup.versionInfo.designVersion = root["versionInfo"].get("designVersion", "").asString();
    mSetup.versionInfo.g5appsVersion = root["versionInfo"].get("g5appsVersion", 0).asInt();

    mProject.protection = root["projectInfo"].get("protection", "").asString();
    mProject.password = root["projectInfo"].get("password", "").asString();
    mProject.encrypted = true;  // This format encrypts password in any case!
    mProject.panelType = root["projectInfo"].get("panelType", "").asString();
    mProject.fileRevision = root["projectInfo"].get("revision", "").asString();
    mProject.dealerID = root["projectInfo"].get("dealer", "").asString();
    mProject.jobName = root["projectInfo"].get("jobName", "").asString();
    mProject.salesOrder = root["projectInfo"].get("salesOrder", "").asString();
    mProject.purchaseOrder = root["projectInfo"].get("purchaseOrder", "").asString();
    mProject.jobComment = root["projectInfo"].get("comment", "").asString();
    mProject.designerID = root["projectInfo"].get("designer", "").asString();
    mProject.creationDate = root["projectInfo"].get("date", "").asString();
    mProject.revisionDate = mProject.creationDate; // root["projectInfo"].get("revisionDate", "").asString();
    mProject.lastSaveDate = root["projectInfo"].get("lastDate", "").asString();
    mProject.fileName = root["projectInfo"].get("fileName", "").asString();
    mProject.colorChoice = root["projectInfo"].get("colorChoice", "").asString();
    mProject.specifyPortCount = root["projectInfo"].get("specifyPortCount", 0).asInt();
    mProject.specifyChanCount = root["projectInfo"].get("specifyChanCount", 0).asInt();

    mSetup.supportFiles.mapFile = root["fileInfo"].get("mapFile", "").asString();
    mSetup.supportFiles.colorFile = root["fileInfo"].get("colorFile", "").asString();
    mSetup.supportFiles.fontFile = root["fileInfo"].get("fontFile", "").asString();
    mSetup.supportFiles.themeFile = root["fileInfo"].get("themeFile", "").asString();
    mSetup.supportFiles.externalButtonFile = root["fileInfo"].get("buttonFile", "").asString();
    mSetup.supportFiles.appFile = root["fileInfo"].get("appFile", "").asString();
    mSetup.supportFiles.logFile = root["fileInfo"].get("logFile", "").asString();

    mSetup.portCount = root["setup"].get("portCount", 0).asInt();
    mSetup.setupPort = root["setup"].get("setupPort", 0).asInt();
    mSetup.addressCount = root["setup"].get("addressCount", 0).asInt();
    mSetup.channelCount = root["setup"].get("channelCount", 0).asInt();
    mSetup.levelCount = root["setup"].get("levelCount", 0).asInt();
    mSetup.powerUpPage = root["setup"].get("powerUpPage", "").asString();

    const Json::Value panelSetup = root["setup"];
    const Json::Value powerUpPopups = panelSetup["powerUpPopups"];

    if (powerUpPopups.isArray())
    {
        for (size_t i = 0; i < powerUpPopups.size(); ++i)
            mSetup.powerUpPopup.push_back(powerUpPopups[(int)i].asString());
    }

    mSetup.startupString = panelSetup.get("startupString", "").asString();
    mSetup.wakeupString = panelSetup.get("wakeupString", "").asString();
    mSetup.sleepString = panelSetup.get("sleepString", "").asString();
    mSetup.shutdownString = panelSetup.get("shutdownString", "").asString();
    mSetup.idlePage = panelSetup.get("idlePage", "").asString();
    mSetup.inactivityPage = panelSetup.get("inactivityPage", "").asString();
    mSetup.idleTimeout = panelSetup.get("idleTimeout", 0).asInt();
    mSetup.screenWidth = panelSetup.get("screenWidth", 0).asInt();
    mSetup.screenHeight = panelSetup.get("screenWidth", 0).asInt();
    mSetup.screenRotate = panelSetup.get("screenRotate", 0).asInt();
    mSetup.batteryLevelPort = panelSetup.get("batteryLevelPort", 0).asInt();
    mSetup.batteryLevelCode = panelSetup.get("batteryLevelCode", 0).asInt();
    mSetup.marqueeSpeed = panelSetup.get("marqeeSpeed", 1).asInt();
    mSetup.fontName = panelSetup.get("fontName", "Arial").asString();
    mSetup.fontSize = panelSetup.get("fontSize", 10).asInt();

    const Json::Value resourceList = root["resourceList"];
    RESOURCE_LIST_T list = findResourceType("image");

    if (mResourceLists.size() == 0 || list.type.empty())
    {
        list.type = "image";
        list.ressource.clear();
        mResourceLists.push_back(list);
    }

    for (size_t i = 0; i < resourceList.size(); ++i)
    {
        int index = static_cast<int>(i);
        RESOURCE_T res;
        res.encrypted = true;
        res.name = resourceList[index].get("name", "").asString();
        res.protocol = resourceList[index].get("protocol", "").asString();
        res.host = resourceList[index].get("host", "").asString();
        res.path = resourceList[index].get("path", "").asString();
        res.file = resourceList[index].get("file", "").asString();
        res.password = resourceList[index].get("password", "").asString();
        res.user = resourceList[index].get("user", "").asString();
        res.refresh = resourceList[index].get("refresh", 0).asInt();
        res.dynamo = resourceList[index].get("dynamo", false).asBool();
        list.ressource.push_back(res);
    }

    vector<RESOURCE_LIST_T>::iterator itResList;

    for (itResList = mResourceLists.begin(); itResList != mResourceLists.end(); ++itResList)
    {
        if (itResList->type.compare("image") == 0)
        {
            mResourceLists.erase(itResList);
            mResourceLists.push_back(list);
            break;
        }
    }

    const Json::Value dataSource = root["dataSourceList"];
    list = findResourceType("data");

    if (mResourceLists.size() == 0 || list.type.empty())
    {
        list.type = "data";
        list.ressource.clear();
        mResourceLists.push_back(list);
    }

    for (size_t i = 0; i < dataSource.size(); ++i)
    {
        int index = static_cast<int>(i);
        RESOURCE_T res;
        res.encrypted = true;
        res.name = dataSource[index].get("name", "").asString();
        res.protocol = dataSource[index].get("protocol", "").asString();
        res.host = dataSource[index].get("host", "").asString();
        res.path = dataSource[index].get("path", "").asString();
        res.file = dataSource[index].get("file", "").asString();
        res.password = dataSource[index].get("password", "").asString();
        res.user = dataSource[index].get("user", "").asString();
        res.refresh = dataSource[index].get("refresh", 0).asInt();
        res.delimiter = dataSource[index].get("delimiter", ";").asString();
        res.force = dataSource[index].get("force", false).asBool();
        res.format = dataSource[index].get("format", "").asString();
        res.headlines = dataSource[index].get("headlines", 0).asInt();
        res.mapIdI1 = dataSource[index].get("mapIdI1", "").asString();
        res.mapIdT1 = dataSource[index].get("mapIdT1", "").asString();
        res.mapIdT2 = dataSource[index].get("mapIdT2", "").asString();
        res.quoted = dataSource[index].get("quoted", false).asBool();
        res.sort = dataSource[index].get("sort", 0).asInt();
        res.sortAdv = dataSource[index].get("sortAdv", "").asString();
        const Json::Value sortList = dataSource[index]["sortList"];

        for (size_t j = 0; j < sortList.size(); ++j)
            res.sortList.push_back(sortList[static_cast<int>(j)].asString());

        list.ressource.push_back(res);
    }

    for (itResList = mResourceLists.begin(); itResList != mResourceLists.end(); ++itResList)
    {
        if (itResList->type.compare("data") == 0)
        {
            mResourceLists.erase(itResList);
            mResourceLists.push_back(list);
            break;
        }
    }

    // TODO: Read palette file. Currently not available for TSF format.
    return false;
}

RESOURCE_LIST_T TSettings::findResourceType(const string& type)
{
    DECL_TRACER ("TSettings::findResourceType(const string& type)");

    vector<RESOURCE_LIST_T>::iterator iter;

    for (iter = mResourceLists.begin(); iter != mResourceLists.end(); iter++)
    {
        if (iter->type.compare(type) == 0)
            return *iter;
    }

    return RESOURCE_LIST_T();
}

bool TSettings::isPortrait()
{
    DECL_TRACER("TSettings::isPortrait()");

    return mSetup.screenWidth < mSetup.screenHeight;
}

bool TSettings::isLandscape()
{
    DECL_TRACER("TSettings::isLandscape()");

    return mSetup.screenWidth > mSetup.screenHeight;
}

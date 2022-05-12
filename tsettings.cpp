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

#include <unistd.h>
#include "tsettings.h"
#include "texpat++.h"
#include "tconfig.h"
#include "ttpinit.h"

using std::string;
using std::vector;
using namespace Expat;

TSettings::TSettings(const string& path)
    : mPath(path)
{
    DECL_TRACER("TSettings::TSettings(const string& path)");
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
        TError::setError();
        return false;
    }

    TExpat xml(fname);
    xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return false;

    int depth = 0;
    size_t index = 0;

    if ((index = xml.getElementIndex("panelSetup", &depth)) == TExpat::npos)
    {
        MSG_ERROR("Couldn't find the section \"panelSetup\" in file!");
        TError::setError();
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

            for (itResList = mResourceLists.begin(); itResList != mResourceLists.end(); itResList++)
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

    if ((index = xml.getElementIndex("paletteList", &depth)) == TExpat::npos)
    {
        MSG_WARNING("There exists no color palette! There will be only the system colors available.");
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

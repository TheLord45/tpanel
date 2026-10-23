/*
 * Copyright (C) 2022 to 2026 by Andreas Theofilu <andreas@theosys.at>
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

#include "tmap.h"
#include "texpat++.h"
#include "tpagemanager.h"
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

TMap::TMap(const std::string& file, bool tp)
    : mFile(file),
      mIsG5(tp)
{
    DECL_TRACER("TMap::TMap(std::string& file, bool tp)");

    if (!fs::exists(file))
    {
        MSG_ERROR("File " << file << " does not exist!");
        mError = true;
        return;
    }

    bool e;

    if (TTPInit::isTsf())
        e = readMapJson();
    else
        e = readMap();

    mError = !e;
}

bool TMap::readMap()
{
    DECL_TRACER("TMap::readMap()");

    string lfile = "map.xma";

    if (TTPInit::isG5() && gPageManager)
        lfile = gPageManager->getSettings()->getMapFileName();

    string path = makeFileName(mFile, lfile);
    vector<string> elements = { "cm", "am", "lm", "bm" };

    if (mIsG5)
        elements.push_back("evpf");
    else
    {
        elements.push_back("sm");
        elements.push_back("strm");
        elements.push_back("pm");
    }

    if (!isValidFile())
    {
        MSG_ERROR("File \"" << path << "\" is not a regular readable file!");
        return false;
    }

    TExpat xml(path);

    if (!TTPInit::isG5())
        xml.setEncoding(ENC_CP1250);

    if (!xml.parse())
        return false;

    int depth = 0;
    size_t index = 0;
    vector<string>::iterator mapIter;
    size_t oldIndex = 0;

    if (elements.size() == 0)
        return false;

    for (mapIter = elements.begin(); mapIter != elements.end(); ++mapIter)
    {
        if ((index = xml.getElementIndex(*mapIter, &depth)) == TExpat::npos)
        {
            MSG_WARNING("Element \"" << *mapIter << "\" was not found!");
            continue;
        }

        MAP_T map;
        MAP_BM_T mapBm;
        MAP_PM_T mapPm;
        MAP_EVPF_T mapEvpf;
        string name, content;

        while ((index = xml.getNextElementFromIndex(index, &name, nullptr, nullptr)) != TExpat::npos)
        {
            string el = name;

            if (el.compare("me") == 0)
            {
                while ((index = xml.getNextElementFromIndex(index, &name, &content, nullptr)) != TExpat::npos)
                {
                    string e = name;

                    if (mapIter->compare("cm") == 0 || mapIter->compare("am") == 0 ||
                        mapIter->compare("lm") == 0 || mapIter->compare("strm") == 0)
                    {
                        if (e.compare("p") == 0)
                            map.p = xml.convertElementToInt(content);
                        else if (e.compare("c") == 0)
                            map.c = xml.convertElementToInt(content);
                        else if (e.compare("ax") == 0)
                            map.ax = xml.convertElementToInt(content);
                        else if (e.compare("pg") == 0)
                            map.pg = xml.convertElementToInt(content);
                        else if (e.compare("bt") == 0)
                            map.bt = xml.convertElementToInt(content);
                        else if (e.compare("pn") == 0)
                            map.pn = content;
                        else if (e.compare("bn") == 0)
                            map.bn = content;
                    }
                    else if (mapIter->compare("bm") == 0)
                    {
                        while ((index = xml.getNextElementFromIndex(index, &name, &content, nullptr)) != TExpat::npos)
                        {
                            string im = name;

                            if (im.compare("i") == 0)
                                mapBm.i = content;
                            else if (im.compare("id") == 0)
                                mapBm.id = xml.convertElementToInt(content);
                            else if (im.compare("rt") == 0)
                                mapBm.rt = xml.convertElementToInt(content);
                            else if (im.compare("pg") == 0)
                                mapBm.pg = xml.convertElementToInt(content);
                            else if (im.compare("bt") == 0)
                                mapBm.bt = xml.convertElementToInt(content);
                            else if (im.compare("st") == 0)
                                mapBm.st = xml.convertElementToInt(content);
                            else if (im.compare("sl") == 0)
                                mapBm.sl = xml.convertElementToInt(content);
                            else if (im.compare("pn") == 0)
                                mapBm.pn = content;
                            else if (im.compare("bn") == 0)
                                mapBm.bn = content;
                            else if (im.compare("rc") == 0)
                                mapBm.rc = xml.convertElementToInt(content);

                            oldIndex = index;
                        }

                        mMap.map_bm.push_back(mapBm);

                        if (index == TExpat::npos)
                            index = oldIndex + 1;
                    }
                    else if (mapIter->compare("sm") == 0)
                    {
                        if (e.compare("i") == 0)
                            mMap.map_sm.push_back(content);
                    }
                    else if (mapIter->compare("pm") == 0)
                    {
                        if (e.compare("a") == 0)
                            mapPm.a = xml.convertElementToInt(content);
                        else if (e.compare("t") == 0)
                            mapPm.t = content;
                        else if (e.compare("pg") == 0)
                            mapPm.pg = xml.convertElementToInt(content);
                        else if (e.compare("bt") == 0)
                            mapPm.bt = xml.convertElementToInt(content);
                        else if (e.compare("pn") == 0)
                            mapPm.pn = content;
                        else if (e.compare("bn") == 0)
                            mapPm.bn = content;
                    }
                    else if (mapIter->compare("evpf") == 0)
                    {
                        if (e.compare("a") == 0)
                            mapEvpf.a = xml.convertElementToInt(content);
                        else if (e.compare("t") == 0)
                            mapEvpf.t = content;
                        else if (e.compare("pg") == 0)
                            mapEvpf.pg = xml.convertElementToInt(content);
                        else if (e.compare("bt") == 0)
                            mapEvpf.bt = xml.convertElementToInt(content);
                        else if (e.compare("ev") == 0)
                            mapEvpf.ev = content;
                        else if (e.compare("ai") == 0)
                            mapEvpf.ai = xml.convertElementToInt(content);
                    }

                    oldIndex = index;
                }

                if (mapIter->compare("cm") == 0)
                    mMap.map_cm.push_back(map);
                else if (mapIter->compare("am") == 0)
                    mMap.map_am.push_back(map);
                else if (mapIter->compare("lm") == 0)
                    mMap.map_lm.push_back(map);
                else if (mapIter->compare("strm") == 0)
                    mMap.map_strm.push_back(map);
                else if (mapIter->compare("pm") == 0)
                    mMap.map_pm.push_back(mapPm);
                else if (mapIter->compare("evpf") == 0)
                    mMap.map_evpf.push_back(mapEvpf);

                if (index == TExpat::npos)
                    index = oldIndex + 1;
            }

            oldIndex = index;
        }
    }

    return true;
}

bool TMap::readMapJson()
{
    DECL_TRACER("TMap::readMapJson()");

    string lfile;

    if (gPageManager)
        lfile = gPageManager->getSettings()->getMapFileName();
    else
        return false;

    string path = makeFileName(mFile, lfile);

    if (!isValidFile())
    {
        MSG_ERROR("File \"" << lfile << "\" doesn't exist or is not a regular file!");
        return false;
    }

    MAP_T map;
    MAP_BM_T mapBm;
    MAP_PM_T mapPm;
    MAP_EVPF_T mapEvpf;
    MAP_DDSM_T mapDdsm;

    const vector<string> elements = { "cm", "am", "lm", "bm", "evpf", "ddsm" };
    vector<string>::const_iterator mapIter;

    Json::Value root;
    ifstream config_doc(path, ifstream::binary);
    config_doc >> root;

    const Json::Value pageList = root["pageList"];

    for (mapIter = elements.cbegin(); mapIter != elements.cend(); ++mapIter)
    {
        const Json::Value array = root[mapIter->c_str()];

        for (size_t i = 0; i < array.size(); ++i)
        {
            int index = static_cast<int>(i);

            if (mapIter->compare("cm") == 0 || mapIter->compare("am") == 0 ||
                mapIter->compare("lm") == 0 || mapIter->compare("strm") == 0)
            {
                map.p = array[index].get("p", "0").asInt();
                map.c = array[index].get("c", "0").asInt();
                map.ax = array[index].get("ax", "0").asInt();
                map.pg = array[index].get("pg", "0").asInt();
                map.bt = array[index].get("bt", "0").asInt();
                map.pn = array[index].get("pn", "").asString();
                map.bn = array[index].get("bn", "").asString();

                if (mapIter->compare("cm") == 0)
                    mMap.map_cm.push_back(map);
                else if (mapIter->compare("am") == 0)
                    mMap.map_am.push_back(map);
                else if (mapIter->compare("lm") == 0)
                    mMap.map_lm.push_back(map);
                else if (mapIter->compare("strm") == 0)
                    mMap.map_strm.push_back(map);
            }
            else if (mapIter->compare("bm") == 0)
            {
                mapBm.i = array[index].get("i", "").asString();
                mapBm.id = array[index].get("id", "0").asInt();
                mapBm.rt = array[index].get("rt", "0").asInt();
                mapBm.pg = array[index].get("pg", "0").asInt();
                mapBm.bt = array[index].get("bt", "0").asInt();
                mapBm.st = array[index].get("st", "0").asInt();
                mapBm.sl = array[index].get("sl", "0").asInt();
                mapBm.pn = array[index].get("pn", "").asString();
                mapBm.bn = array[index].get("bn", "").asString();
                mapBm.rc = array[index].get("rc", "0").asInt();
                mMap.map_bm.push_back(mapBm);
            }
            else if (mapIter->compare("sm") == 0)
            {
                Json::Value sm = array[index]["sm"];

                for (int j = 0; j < static_cast<int>(sm.size()); ++j)
                    mMap.map_sm.push_back(sm[j].asString());
            }
            else if (mapIter->compare("pm") == 0)
            {
                mapPm.a = array[index].get("a", "0").asInt();
                mapPm.t = array[index].get("t", "").asString();
                mapPm.pg = array[index].get("pg", "0").asInt();
                mapPm.bt = array[index].get("bt", "0").asInt();
                mapPm.pn = array[index].get("pn", "").asString();
                mapPm.bn = array[index].get("bn", "").asString();
                mMap.map_pm.push_back(mapPm);
            }
            else if (mapIter->compare("evpf") == 0)
            {
                mapEvpf.a = array[index].get("a", "0").asInt();
                mapEvpf.t = array[index].get("t", "").asString();
                mapEvpf.pg = array[index].get("pg", "0").asInt();
                mapEvpf.bt = array[index].get("bt", "0").asInt();
                mapEvpf.ev = array[index].get("ev", "").asString();
                mapEvpf.ai = array[index].get("ai", "0").asInt();
                mMap.map_evpf.push_back(mapEvpf);
            }
            else if (mapIter->compare("ddsm") == 0)
            {
                mapDdsm.bt = array[index].get("bt", "0").asInt();
                mapDdsm.ddmn = array[index].get("ddmn", "").asString();
                mapDdsm.ddmt = array[index].get("ddmt", "").asString();
                mapDdsm.dds = array[index].get("dds", "").asString();
                mapDdsm.pg = array[index].get("pg", "0").asInt();
                mMap.map_ddsm.push_back(mapDdsm);
            }
        }
    }

    return true;
}

vector<TMap::MAP_T> TMap::findButtons(int port, vector<int>& channels, MAP_TYPE mt)
{
    DECL_TRACER("TMap::findButtons(int port, vector<int>& channels, MAP_TYPE mt)");

    vector<MAP_T> map;
    vector<int>::iterator iter;

    if (channels.empty())
    {
        MSG_WARNING("Got empty channel list!");
        return map;
    }

    vector<MAP_T> localMap;

    switch (mt)
    {
        case TYPE_AM:   localMap = mMap.map_am; break;
        case TYPE_CM:   localMap = mMap.map_cm; break;
        case TYPE_LM:   localMap = mMap.map_lm; break;
    }

    if (localMap.empty())
    {
        MSG_WARNING("The internal list of elements is empty!")
        return map;
    }

    for (iter = channels.begin(); iter != channels.end(); ++iter)
    {
        vector<MAP_T>::iterator mapIter;

        for (mapIter = localMap.begin(); mapIter != localMap.end(); ++mapIter)
        {
            if (mapIter->p == port && mapIter->c == *iter)
                map.push_back(*mapIter);
        }
    }

    MSG_DEBUG("Found " << map.size() << " buttons.");
    return map;
}

vector<TMap::MAP_T> TMap::findButtonByName(const string& name)
{
    DECL_TRACER("TAmxCommands::findButtonByName(const string& name)");

    vector<MAP_T> map;

    if (mMap.map_cm.empty())
    {
        MSG_WARNING("The internal list of elements is empty!")
        return map;
    }

    vector<MAP_T>::iterator mapIter;

    for (mapIter = mMap.map_cm.begin(); mapIter != mMap.map_cm.end(); ++mapIter)
    {
        if (mapIter->bn == name)
            map.push_back(*mapIter);
    }

    MSG_DEBUG("Found " << map.size() << " buttons.");
    return map;
}

string TMap::findImage(int bt, int page, int instance)
{
    DECL_TRACER("TAmxCommands::findImage(int bt, int page, int instance)");

    vector<MAP_BM_T> mapBm = mMap.map_bm;
    vector<MAP_BM_T>::iterator iter;

    if (mapBm.empty())
        return string();

    for (iter = mapBm.begin(); iter != mapBm.end(); ++iter)
    {
        if (iter->bt == bt && iter->pg == page && !iter->i.empty())
        {
            if (instance >= 0 && iter->st == (instance + 1))
                return iter->i;
            else if (instance < 0)
                return iter->i;
        }
    }

    return string();
}

string TMap::findImage(const string& name)
{
    DECL_TRACER("TAmxCommands::findImage(const string& name)");

    vector<MAP_BM_T> mapBm = mMap.map_bm;
    vector<MAP_BM_T>::iterator iter;

    if (mapBm.empty() || name.empty())
        return string();

    size_t cnt = 0;

    for (iter = mapBm.begin(); iter != mapBm.end(); ++iter)
    {
        if (!iter->i.empty() && iter->i.find(name) != string::npos)
        {
            size_t pos = iter->i.find_last_of(".");

            if (pos != string::npos)
            {
                string left = iter->i.substr(0, pos);

                if (left == name)
                    return iter->i;
            }
        }

        cnt++;
    }

    MSG_WARNING("No image with name " << name << " in table found!");
    return string();
}

vector<TMap::MAP_T> TMap::findBargraphs(int port, vector<int>& channels)
{
    DECL_TRACER("TAmxCommands::findBargraphs(int port, vector<int>& channels)");

    vector<MAP_T> map;
    vector<int>::iterator iter;

    if (channels.empty())
        return map;

    for (iter = channels.begin(); iter != channels.end(); ++iter)
    {
        vector<MAP_T>::iterator mapIter;

        if (!mMap.map_lm.empty())
        {
            for (mapIter = mMap.map_lm.begin(); mapIter != mMap.map_lm.end(); ++mapIter)
            {
                // To find also the joysticks, we must test for level codes
                // less then and grater then *iter.
                if (mapIter->p == port && (mapIter->c == *iter || mapIter->c == (*iter-1) || mapIter->c == (*iter+1)))
                    map.push_back(*mapIter);
            }
        }
    }

    MSG_DEBUG("Found " << map.size() << " buttons.");
    return map;
}

vector<string> TMap::findSounds()
{
    DECL_TRACER("TAmxCommands::findSounds()");

    return mMap.map_sm;
}

bool TMap::soundExist(const string& sname)
{
    DECL_TRACER("TAmxCommands::soundExist(const string sname)");

    if (mIsG5)
        return false;

    if (mMap.map_sm.size() == 0)
        return false;

    vector<string>::iterator iter;

    for (iter = mMap.map_sm.begin(); iter != mMap.map_sm.end(); ++iter)
    {
        if (iter->compare(sname) == 0)
            return true;
    }

    return false;
}

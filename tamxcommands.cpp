/*
 * Copyright (C) 2020 to 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include "tamxcommands.h"
#include "terror.h"
#include "tresources.h"
#include "tconfig.h"
#include "texpat++.h"

#include <string>
#include <vector>

using std::string;
using std::vector;
using namespace Expat;

typedef struct CMD_DEFINATIONS
{
    string cmd;
    bool hasChannels{false};
    bool hasPars{false};
    char separator{0};
}CMD_DEFINATIONS;

CMD_DEFINATIONS cmdDefinations[] = {
    { "@WLD", false, true, ',' },
    { "@AFP", false, true, ',' },
    { "^AFP", false, true, ',' },
    { "@GCE", false, true, ',' },
    { "^GCE", false, true, ',' },
    { "@APG", false, true, ';' },
    { "@CPG", false, true, ',' },
    { "@DPG", false, true, ';' },
    { "@PDR", false, true, ';' },
    { "@PHE", false, true, ';' },
    { "@PHP", false, true, ';' },
    { "@PHT", false, true, ';' },
    { "@PPA", false, true, ',' },
    { "^PPA", false, true, ',' },
    { "@PPF", false, true, ';' },
    { "^PPF", false, true, ';' },
    { "@PPG", false, true, ';' },
    { "^PPG", false, true, ';' },
    { "@PPK", false, true, ',' },
    { "^PPK", false, true, ',' },
    { "@PPM", false, true, ';' },
    { "^PPM", false, true, ';' },
    { "@PPN", false, true, ';' },
    { "^PPN", false, true, ';' },
    { "@PPT", false, true, ';' },
    { "^PPT", false, true, ';' },
    { "@PPX", false, false, '\0' },
    { "^PPX", false, false, '\0' },
    { "@PSE", false, true, ';' },
    { "@PSP", false, true, ';' },
    { "@PST", false, true, ';' },
    { "PAGE", false, true, ',' },
    { "^PGE", false, true, ',' },
    { "PPOF", false, true, ';' },
    { "PPOG", false, true, ';' },
    { "PPON", false, true, ';' },
    { "^ANI", true, true, ',' },
    { "^APF", true, true, ',' },
    { "^BAT", true, true, ',' },
    { "^BAU", true, true, ',' },
    { "^BCB", true, true, ',' },
    { "?BCB", true, true, ',' },
    { "^BCF", true, true, ',' },
    { "?BCF", true, true, ',' },
    { "^BCT", true, true, ',' },
    { "?BCT", true, true, ',' },
    { "^BDO", true, true, ',' },
    { "^BFB", true, true, ',' },
    { "^BIM", true, true, ',' },
    { "^BLN", true, true, ',' },
    { "^BMC", true, true, ',' },
    { "^BMF", true, true, ',' },
    { "^BMI", true, true, ',' },
    { "^BML", true, true, ',' },
    { "^BMP", true, true, ',' },
    { "?BMP", true, true, ',' },
    { "^BNC", true, true, ',' },
    { "^BNN", true, true, ',' },
    { "^BNT", true, true, ',' },
    { "^BOP", true, true, ',' },
    { "?BOP", true, true, ',' },
    { "^BOR", true, true, ',' },
    { "^BOS", true, true, ',' },
    { "^BPP", true, true, ',' },
    { "^BRD", true, true, ',' },
    { "?BRD", true, true, ',' },
    { "^BSF", true, true, ',' },
    { "^BSP", true, true, ',' },
    { "^BSM", true, false, ',' },
    { "^BSO", true, true, ',' },
    { "^BVL", true, true, ',' },
    { "^BVN", false, true, ',' },
    { "^BVP", true, true, ',' },
    { "^BVT", true, true, ',' },
    { "^BWW", true, true, ',' },
    { "?BWW", true, true, ',' },
    { "^CPF", true, false, ',' },
    { "^DLD", false, false, ',' },
    { "^DPF", true, true, ',' },
    { "^ENA", true, true, ',' },
    { "^FON", true, true, ',' },
    { "?FON", true, true, ',' },
    { "^GDI", true, true, ',' },
    { "^GIV", true, true, ',' },
    { "^GLH", true, true, ',' },
    { "^GLL", true, true, ',' },
    { "^GRD", true, true, ',' },
    { "^GRU", true, true, ',' },
    { "^GSC", true, true, ',' },
    { "^GSN", true, true, ',' },
    { "^ICO", true, true, ',' },
    { "?ICO", true, true, ',' },
    { "^IRM", false, true, ',' },
    { "^JSB", true, true, ',' },
    { "?JSB", true, true, ',' },
    { "^JSI", true, true, ',' },
    { "?JSI", true, true, ',' },
    { "^JST", true, true, ',' },
    { "?JST", true, true, ',' },
    { "^MBT", false, false, ',' },
    { "^MDC", false, false, ',' },
    { "^SHO", true, true, ',' },
    { "^TEC", true, true, ',' },
    { "?TEC", true, true, ',' },
    { "^TEF", true, true, ',' },
    { "?TEF", true, true, ',' },
    { "^TOP", false, true, ',' },
    { "^TXT", true, true, ',' },
    { "?TXT", true, true, ',' },
    { "^UNI", true, true, ',' },
    { "^UTF", true, true, ',' },
    { "^LPC", false, true, ',' },
    { "^LPR", false, true, ',' },
    { "^LPS", false, true, ',' },
    { "ABEEP", false, false, ',' },
    { "ADBEEP", false, false, ',' },
    { "@AKB", false, true, ';' },
    { "AKEYB", false, true, ',' },
    { "AKEYP", false, true, ',' },
    { "AKEYR", false, true, ',' },
    { "@AKP", false, true, ';' },
    { "@AKR", false, false, ',' },
    { "BEEP", false, false, ',' },
    { "^ABP", false, false, ',' },
    { "BRIT", false, true, ',' },
    { "@BRT", false, true, ',' },
    { "DBEEP", false, false, ',' },
    { "^ADP", false, false, ',' },
    { "@EKP", false, true, ';' },
    { "PKEYP", false, true, ',' },
    { "@PKB", false, true, ';' },
    { "^PKB", false, true, ';' },
    { "@PKP", false, true, ';' },
    { "^PKP", false, true, ';' },
    { "SETUP", false, false, ',' },
    { "^STP", false, false, ',' },
    { "SHUTDOWN", false, false, ',' },
    { "SLEEP", false, false, ',' },
    { "@SOU", false, true, ',' },
    { "^SOU", false, true, ',' },
    { "@TKP", false, true, ';' },
    { "^TKP", false, true, ';' },
    { "TPAGEON", false, false, ',' },
    { "TPAGEOFF", false, false, ',' },
    { "@VKB", false, false, ',' },
    { "^VKB", false, false, ',' },
    { "WAKE", false, false, ',' },
    { "^CAL", false, false, ',' },
    { "^KPS", false, true, ',' },
    { "^VKS", false, true, ',' },
    { "^WCN?", false, true, ',' },
    { "@PWD", false, true, ',' },
    { "^PWD", false, true, ',' },
    { "^BBR", true, true, ',' },
    { "^RAF", false, true, ',' },
    { "^RFR", false, true, ',' },
    { "^RMF", false, true, ',' },
    { "^RSR", false, true, ',' },
    { "^MODEL?", false, false, ',' },
    { "^MOD", false, false, ',' },
    { "^VER?", false, false, ',' },
    { "^ICS", false, true, ',' },
    { "^ICE", false, false, ',' },
    { "^ICM", false, true, ',' },
    { "^PHN", false, true, ',' },
    { "?PHN", false, true, ',' },
    { "GET ", false, false, ',' },
    { "SET ", false, false, ',' },
    { "LEVON", false, false, ',' },
    { "RXON", false, false, ',' },
    { "ON", false, true, ',' },
    { "OFF", false, true, ',' },
    { "LEVEL", false, true, ',' },
    { "BLINK", false, true, ',' },
    { "#FTR", false, true, ':' },
    { "TPCCMD", false, true, ',' },
    { "TPCACC", false, true, ',' },
    { "", false, false, '\0' }
};

CMD_DEFINATIONS& findCmdDefines(const string& cmd)
{
    DECL_TRACER("findCmdDefines(const string& cmd)");

    int i = 0;
    string uCmd = cmd;
    uCmd = toUpper(uCmd);

    while (cmdDefinations[i].cmd.length() > 0)
    {
        if (cmdDefinations[i].cmd.compare(uCmd) == 0)
            return cmdDefinations[i];

        i++;
    }

    return cmdDefinations[i];
}

TAmxCommands::TAmxCommands()
{
    DECL_TRACER("TAmxCommands::TAmxCommands()");
}

TAmxCommands::~TAmxCommands()
{
    DECL_TRACER("TAmxCommands::~TAmxCommands()");
}

bool TAmxCommands::readMap()
{
    DECL_TRACER("TAmxCommands::readMap()");

    string path = makeFileName(TConfig::getProjectPath(), "map.xma");
    vector<string> elements = { "cm", "am", "lm", "bm", "sm", "strm", "pm" };

    if (!isValidFile())
    {
        MSG_ERROR("File \"" << path << "\" is not a regular readable file!");
        return false;
    }

    TExpat xml(path);
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

                if (index == TExpat::npos)
                    index = oldIndex + 1;
            }

            oldIndex = index;
        }
    }

    return true;
}

vector<string> TAmxCommands::getFields(string& msg, char sep)
{
    DECL_TRACER("TAmxCommands::getFields(string& msg, char sep)");

    vector<string> flds;
    bool bStr = false;
    string part;

    for (size_t i = 0; i < msg.length(); i++)
    {
        if (msg.at(i) == sep && !bStr)
        {
            flds.push_back(part);
            part.clear();
            continue;
        }
        else if (msg.at(i) == '\'' && !bStr)
            bStr = true;
        else if (msg.at(i) == '\'' && bStr)
            bStr = false;
        else
            part.append(msg.substr(i, 1));
    }

    if (!part.empty())
        flds.push_back(part);

    if (flds.size() > 0 && TStreamError::checkFilter(HLOG_DEBUG))
    {
        MSG_DEBUG("Found fields:");
        vector<string>::iterator iter;
        int i = 1;

        for (iter = flds.begin(); iter != flds.end(); ++iter)
        {
            MSG_DEBUG("    " << i << ": " << *iter);
            i++;
        }
    }

    return flds;
}

vector<MAP_T> TAmxCommands::findButtons(int port, vector<int>& channels, MAP_TYPE mt)
{
    DECL_TRACER("TAmxCommands::findButtons(int port, vector<int>& channels, MAP_TYPE mt)");

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

vector<MAP_T> TAmxCommands::findButtonByName(const string& name)
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

vector<MAP_T> TAmxCommands::findBargraphs(int port, vector<int>& channels)
{
    DECL_TRACER("TAmxCommands::findBargraphs(int port, vector<int>& channels)");

    vector<MAP_T> map;
    vector<int>::iterator iter;

    if (channels.size() == 0)
        return map;

    for (iter = channels.begin(); iter != channels.end(); ++iter)
    {
        vector<MAP_T>::iterator mapIter;

        if (mMap.map_lm.size() > 0)
        {
            for (mapIter = mMap.map_lm.begin(); mapIter != mMap.map_lm.end(); ++mapIter)
            {
                if (mapIter->p == port && mapIter->c == *iter)
                    map.push_back(*mapIter);
            }
        }
    }

    MSG_DEBUG("Found " << map.size() << " buttons.");
    return map;
}

vector<string> TAmxCommands::findSounds()
{
    DECL_TRACER("TAmxCommands::findSounds()");

    return mMap.map_sm;
}

bool TAmxCommands::soundExist(const string& sname)
{
    DECL_TRACER("TAmxCommands::soundExist(const string sname)");

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

bool TAmxCommands::parseCommand(int device, int port, const string& cmd)
{
    DECL_TRACER("TAmxCommands::parseCommand(int device, int port, const string& cmd)");
/*
    if (device != TConfig::getChannel())    // This happens on a filetransfer only!
    {
        MSG_WARNING("Command is for device " << device << ", but this device is " << TConfig::getChannel());
    }
*/
    vector<CMD_TABLE>::iterator iter;
    size_t pos = cmd.find_first_of("-");
    int system = TConfig::getSystem();
    string scmd = getCommand(cmd);

    MSG_TRACE("Parsing for device <" << device << ":" << port << ":" << system << "> the command: " << scmd);

    if (pos != string::npos)    // Command with parameters
    {
        string bef = cmd.substr(0, pos);
        string rest = cmd.substr(pos + 1);

        for (iter = mCmdTable.begin(); iter != mCmdTable.end(); ++iter)
        {
            iter->channels.clear();
            iter->pars.clear();

            if (iter->cmd.compare(bef) == 0 && iter->command)
            {
                CMD_DEFINATIONS cdef = findCmdDefines(bef);

                if (cdef.cmd.empty())
                {
                    MSG_WARNING("Command \"" << bef << "\" not found in command table! Ignoring it.");
                    continue;
                }

                if (cdef.hasChannels || cdef.hasPars)
                {
                    vector<string> parts = getFields(rest, cdef.separator);

                    if (cdef.hasChannels)
                        extractChannels(parts[0], &iter->channels);

                    if (cdef.hasPars)
                    {
                        MSG_DEBUG("Command may have parameters. Found " << parts.size() << " parameters.");

                        if (parts.size() > 0)
                        {
                            vector<string>::iterator piter;
                            int cnt = 0;

                            for (piter = parts.begin(); piter != parts.end(); ++piter)
                            {
                                if (cdef.hasChannels && !cnt)
                                {
                                    cnt++;
                                    continue;
                                }

                                iter->pars.push_back(*piter);
                                cnt++;
                            }
                        }
                        else
                            iter->pars.push_back(rest);
                    }
                }

                iter->command(port, iter->channels, iter->pars);
                return true;
            }
        }
    }
    else        // Command without parameter
    {
        for (iter = mCmdTable.begin(); iter != mCmdTable.end(); ++iter)
        {
            if (iter->cmd.compare(cmd) == 0 && iter->command)
            {
                iter->command(port, iter->channels, iter->pars);
                return true;
            }
        }
    }

    MSG_WARNING("Command \"" << cmd << "\" currently not supported!");
    return false;
}

bool TAmxCommands::extractChannels(const string& schan, vector<int>* ch)
{
    DECL_TRACER("TAmxCommands::extractChannels(const string& schan, vector<int>* ch)");

    if (!ch || schan.empty())
        return false;

    if (schan.find("&") == string::npos && schan.find(".") == string::npos)
    {
        int c = atoi(schan.c_str());
        ch->push_back(c);
        return true;
    }

    if (schan.find("&") != string::npos)
    {
        vector<string> parts = StrSplit(schan, "&");
        vector<string>::iterator iter;

        if (parts.size() > 0)
        {
            for (iter = parts.begin(); iter != parts.end(); ++iter)
            {
                if (iter->find(".") != string::npos)
                {
                    vector<string> p2 = StrSplit(*iter, ".");

                    if (p2.size() >= 2)
                    {
                        for (int i = atoi(p2[0].c_str()); i <= atoi(p2[1].c_str()); i++)
                            ch->push_back(i);
                    }
                    else if (p2.size() > 0)
                        ch->push_back(atoi(p2[0].c_str()));
                }
                else
                    ch->push_back(atoi(iter->c_str()));
            }
        }
    }
    else
    {
        vector<string> parts = StrSplit(schan, ".");

        if (parts.size() >= 2)
        {
            for (int i = atoi(parts[0].c_str()); i <= atoi(parts[1].c_str()); i++)
                ch->push_back(i);
        }
        else if (parts.size() > 0)
            ch->push_back(atoi(parts[0].c_str()));
    }

    return true;
}

void TAmxCommands::registerCommand(std::function<void (int port, vector<int>& channels, vector<string>& pars)> command, const string& name)
{
    DECL_TRACER("TAmxCommands::registerCommand(std::function<void (vector<int>& channels, vector<string>& pars)> command, const string& name)");

    vector<CMD_TABLE>::iterator iter;

    for (iter = mCmdTable.begin(); iter != mCmdTable.end(); ++iter)
    {
        if (iter->cmd.compare(name) == 0)
        {
            iter->command = command;
            iter->channels.clear();
            iter->pars.clear();
            return;
        }
    }

    CMD_TABLE ctbl;
    ctbl.cmd = name;
    ctbl.command = command;
    mCmdTable.push_back(ctbl);
}

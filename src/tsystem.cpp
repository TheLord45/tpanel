/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include "tsystem.h"
#include "tconfig.h"
#include "terror.h"

using std::string;
using std::to_string;

#define IDX_INVALID     -1

typedef enum SYSBTTYPE_t
{
    BT_UNKNOWN,
    BT_CHECKBOX,
    BT_BUTTON,
    BT_COMBOBOX,
    BT_TEXT,
    BT_SLIDER,
    BT_FUNCTION
}SYSBTTYPE_t;

typedef struct SYSBUTTONS_t
{
    int channel{0};                 // Channel number
    BUTTONTYPE type{NONE};          // Type of button
    int states{0};                  // Maximum number of states
    int ll{0};                      // Level low range
    int lh{0};                      // Level high range
    SYSBTTYPE_t button{BT_UNKNOWN}; // The technical type of the button (only internal use!)
}SYSBUTTONS_t;

SYSBUTTONS_t sysButtons[] = {
    {    6, BARGRAPH,              2, 0, 100, BT_SLIDER },      // System gain
    {    8, MULTISTATE_BARGRAPH,  12, 0,  11, BT_FUNCTION },    // Connection status
    {    9, BARGRAPH,              2, 0, 100, BT_SLIDER },      // System volume
    {   17, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Button sounds on/off
    {   25, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Controller: TP4 file name
    {   73, GENERAL,               2, 0,   0, BT_BUTTON },      // Enter setup page
    {   80, GENERAL,               2, 0,   0, BT_BUTTON },      // Shutdown program
    {   81, MULTISTATE_BARGRAPH,   6, 1,   6, BT_FUNCTION },    // Network signal stength
    {  122, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Controller: Controller: IP Address of server or domain name
    {  123, TEXT_INPUT,            2, 9,   0, BT_TEXT },        // Controller: Channel number of panel
    {  124, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Controller: The network port number (1319)
    {  128, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Controller: The type name of the panel
    {  141, GENERAL,               2, 0,   0, BT_FUNCTION },    // Standard time
    {  142, GENERAL,               2, 0,   0, BT_FUNCTION },    // Time AM/PM
    {  143, GENERAL,               2, 0,   0, BT_FUNCTION },    // 24 hour time
    {  144, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Network port of NetLinx
    {  151, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date weekday
    {  152, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date mm/dd
    {  153, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date dd/mm
    {  154, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date mm/dd/yyyy
    {  155, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date dd/mm/yyyy
    {  156, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date month dd, yyyy
    {  157, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date dd month, yyyy
    {  158, GENERAL,               2, 0,   0, BT_FUNCTION },    // Date yyyy-mm-dd
    {  159, GENERAL,               2, 0,   0, BT_BUTTON },      // Sound: Play test sound
    {  161, GENERAL,               2, 0,   0, BT_FUNCTION },    // GPS: Latitude --> TheoSys specific!
    {  162, GENERAL,               2, 0,   0, BT_FUNCTION },    // GPS: Longitude --> TheoSys specific!
    {  171, GENERAL,               2, 0,   0, BT_FUNCTION },    // Sytem volume up
    {  172, GENERAL,               2, 0,   0, BT_FUNCTION },    // Sytem volume down
    {  173, GENERAL,               2, 0,   0, BT_FUNCTION },    // Sytem mute toggle
    {  199, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Technical name of panel (from the settings)
    {  234, GENERAL,               2, 0,   0, BT_FUNCTION },    // Battery charging/not charging
    {  242, BARGRAPH,              2, 0, 100, BT_SLIDER },      // Battery level
    {  404, GENERAL,               2, 0,   0, BT_TEXT },        // Sound: Single beep
    {  405, GENERAL,               2, 0,   0, BT_TEXT },        // Sound: Double beep
    {  412, GENERAL,               2, 0,   0, BT_BUTTON },      // Button OK: Save changes of system dialogs
    {  413, GENERAL,               2, 0,   0, BT_BUTTON },      // Button Cancel: Cancel changes of system pages
    {  416, GENERAL,               2, 0,   0, BT_CHECKBOX },    // SIP: Enabled
    {  418, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // SIP: Proxy
    {  419, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // SIP: Network port
    {  420, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // SIP: STUN
    {  421, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // SIP: Domain
    {  422, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // SIP: User
    {  423, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // SIP: Password
    { 1143, GENERAL,               2, 0,   0, BT_TEXT },        // Sound: File name of system sound WAV
    { 2000, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Info
    { 2001, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Warning
    { 2002, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Error
    { 2003, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Trace
    { 2004, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Debug
    { 2005, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Protocol
    { 2006, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: all
    { 2007, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Profiling
    { 2008, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Logging: Long format
    { 2009, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Logging: Log file name
    { 2010, GENERAL,               2, 0,   0, BT_BUTTON },      // Logging: Button reset
    { 2011, GENERAL,               2, 0,   0, BT_BUTTON },      // Logging: Button file open
    { 2020, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Controller: FTP user name
    { 2021, TEXT_INPUT,            2, 0,   0, BT_TEXT },        // Controller: FTP password
    { 2023, LISTBOX,               1, 0,   0, BT_COMBOBOX },    // Controller: Listbox for file names of TP4 surfaces
    { 2024, LISTBOX,               1, 0,   0, BT_COMBOBOX },    // Sound: Button sound
    { 2025, LISTBOX,               1, 0,   0, BT_COMBOBOX },    // Sound: Single sound
    { 2026, LISTBOX,               1, 0,   0, BT_COMBOBOX },    // Sound: Double sound
    { 2030, GENERAL,               2, 0,   0, BT_BUTTON },      // Controller: Button for FTP download force
    { 2031, GENERAL,               2, 0,   0, BT_CHECKBOX },    // Controller: FTP passive mode (checkbox)
    { 2050, GENERAL,               2, 0,   0, BT_BUTTON },      // Sound: Play system sound
    { 2051, GENERAL,               2, 0,   0, BT_BUTTON },      // Sound: Play single beep
    { 2052, GENERAL,               2, 0,   0, BT_BUTTON },      // Sound: Play double beep
    { 2060, GENERAL,               2, 0,   0, BT_CHECKBOX },    // SIP: IPv4
    { 2061, GENERAL,               2, 0,   0, BT_CHECKBOX },    // SIP: IPV6
    { 2062, GENERAL,               2, 0,   0, BT_CHECKBOX },    // SIP: Use internal phone
    { 2070, GENERAL,               2, 0,   0, BT_CHECKBOX },    // View: scale to fit
    { 2071, GENERAL,               2, 0,   0, BT_CHECKBOX },    // View: show banner
    { 2072, GENERAL,               2, 0,   0, BT_CHECKBOX },    // View: suppress toolbar
    { 2073, GENERAL,               2, 0,   0, BT_CHECKBOX },    // View: force toolbar visible
    { 2074, GENERAL,               2, 0,   0, BT_CHECKBOX },    // View: lock rotation
    {    0, NONE,                  0, 0,   0, BT_UNKNOWN }      // Terminate
};

TSystem::TSystem()
{
    DECL_TRACER("TSystem::TSystem()");
}

string TSystem::fillButtonText(int ad, int ch)
{
    DECL_TRACER("TSystem::fillButtonText(int ad, int ch)");

    int idx = getSystemButtonIndex(ad > 0 ? ad : ch);

    if (idx == IDX_INVALID)
        return string();

    if ((sysButtons[idx].type == GENERAL || sysButtons[idx].type == TEXT_INPUT) && sysButtons[idx].button == BT_TEXT)
    {
        switch(sysButtons[idx].channel)
        {
            case 25:    return TConfig::getFtpSurface();
            case 122:   return TConfig::getController();
            case 123:   return to_string(TConfig::getChannel());
            case 124:   return to_string(TConfig::getPort());
            case 128:   return TConfig::getPanelType();
            case 144:   return to_string(TConfig::getPort());
            case 199:   return TConfig::getPanelType();
            case 404:   return TConfig::getSingleBeepSound();
            case 405:   return TConfig::getDoubleBeepSound();
            case 418:   return TConfig::getSIPproxy();
            case 419:   return to_string(TConfig::getSIPport());
            case 420:   return TConfig::getSIPstun();
            case 421:   return TConfig::getSIPdomain();
            case 422:   return TConfig::getSIPuser();
            case 423:   return TConfig::getSIPpassword();
            case 1143:  return TConfig::getSystemSound();
            case 2009:  return TConfig::getLogFile();
            case 2020:  return TConfig::getFtpUser();
            case 2021:  return TConfig::getFtpPassword();
        }
    }

    return string();
}

int TSystem::getButtonInstance(int ad, int ch)
{
    DECL_TRACER("TSystem::getButtonInstance(int ad, int ch)");

    int idx = getSystemButtonIndex(ad > 0 ? ad : ch);

    if (idx == IDX_INVALID)
        return idx;

    if (sysButtons[idx].type == GENERAL && sysButtons[idx].button == BT_CHECKBOX)
    {
        bool s = false;

        switch(sysButtons[idx].channel)
        {
            case 17:    s = TConfig::getSystemSoundState(); break;
            case 416:   s = TConfig::getSIPstatus(); break;
            case 2000:  s = (TConfig::getLogLevelBits() & HLOG_INFO); break;
            case 2001:  s = (TConfig::getLogLevelBits() & HLOG_WARNING); break;
            case 2002:  s = (TConfig::getLogLevelBits() & HLOG_ERROR); break;
            case 2003:  s = (TConfig::getLogLevelBits() & HLOG_TRACE); break;
            case 2004:  s = (TConfig::getLogLevelBits() & HLOG_DEBUG); break;
            case 2005:  s = (TConfig::getLogLevelBits() & HLOG_PROTOCOL) == HLOG_PROTOCOL; break;
            case 2006:  s = (TConfig::getLogLevelBits() & HLOG_ALL) == HLOG_ALL; break;
            case 2007:  s = TConfig::getProfiling(); break;
            case 2008:  s = TConfig::isLongFormat(); break;
            case 2031:  s = TConfig::getFtpPassive(); break;
            case 2060:  s = TConfig::getSIPnetworkIPv4(); break;
            case 2061:  s = TConfig::getSIPnetworkIPv6(); break;
            case 2062:  s = TConfig::getSIPiphone(); break;
            case 2070:  s = TConfig::getScale(); break;
            case 2071:  s = TConfig::showBanner(); break;
            case 2072:  s = TConfig::getToolbarSuppress(); break;
            case 2073:  s = TConfig::getToolbarForce(); break;
            case 2074:  s = TConfig::getRotationFixed(); break;

            default:
                return IDX_INVALID;
        }

        if (s)
            return 1;

        return 0;
    }

    return IDX_INVALID;
}

bool TSystem::isSystemButton(int channel)
{
    DECL_TRACER("TSystem::isSystemButton(int channel)");

    int i = getSystemButtonIndex(channel);

    if (i >= 0)
        return true;

    return false;
}

bool TSystem::isSystemCheckBox(int channel)
{
    DECL_TRACER("TSystem::isSystemCheckBox(int channel)");

    int i = getSystemButtonIndex(channel);

    if (i >= 0 && sysButtons[i].button == BT_CHECKBOX)
        return true;

    return false;
}

bool TSystem::isSystemTextLine(int channel)
{
    DECL_TRACER("TSystem::isSystemTextLine(int channel)");

    int i = getSystemButtonIndex(channel);

    if (i >= 0 && sysButtons[i].button == BT_TEXT)
        return true;

    return false;
}

bool TSystem::isSystemPushButton(int channel)
{
    DECL_TRACER("TSystem::isSystemPushButton(int channel)");

    int i = getSystemButtonIndex(channel);

    if (i >= 0 && sysButtons[i].button == BT_BUTTON)
        return true;

    return false;
}

bool TSystem::isSystemFunction(int channel)
{
    DECL_TRACER("TSystem::isSystemFunction(int channel)");

    int i = getSystemButtonIndex(channel);

    if (i >= 0 && sysButtons[i].button == BT_FUNCTION)
        return true;

    return false;
}

bool TSystem::isSystemSlider(int channel)
{
    DECL_TRACER("TSystem::isSystemSlider(int channel)");

    int i = getSystemButtonIndex(channel);

    if (i >= 0 && sysButtons[i].button == BT_SLIDER)
        return true;

    return false;
}

bool TSystem::isSystemComboBox(int channel)
{
    DECL_TRACER("TSystem::isSystemComboBox(int channel)");

    int i = getSystemButtonIndex(channel);

    if (i >= 0 && sysButtons[i].button == BT_COMBOBOX)
        return true;

    return false;
}

int TSystem::getSystemButtonIndex(int channel)
{
    DECL_TRACER("TSystem::getSystemButtonIndex(int channel)");

    int i = 0;

    while (sysButtons[i].channel > 0)
    {
        if (sysButtons[i].channel == channel)
            return i;

        i++;
    }

    return IDX_INVALID;
}

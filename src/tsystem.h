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

#ifndef __TSYSTEM_H__
#define __TSYSTEM_H__

#include <string>

typedef enum BUTTONTYPE
{
    NONE,
    GENERAL,
    MULTISTATE_GENERAL,
    BARGRAPH,
    MULTISTATE_BARGRAPH,
    JOYSTICK,
    TEXT_INPUT,
    LISTBOX,
    COMPUTER_CONTROL,
    TAKE_NOTE,
    SUBPAGE_VIEW
} BUTTONTYPE;

class TSystem
{
    public:
        TSystem();

        std::string fillButtonText(int ad, int ch);
        int getButtonInstance(int ad, int ch);
        bool isSystemButton(int channel);
        bool isSystemCheckBox(int channel);
        bool isSystemTextLine(int channel);
        bool isSystemPushButton(int channel);
        bool isSystemFunction(int channel);
        bool isSystemSlider(int channel);
        bool isSystemComboBox(int channel);

    private:
        int getSystemButtonIndex(int channel);
};

#endif


/*
 * Copyright (C) 2023 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TBUTTONSTATES_H__
#define __TBUTTONSTATES_H__

#include <cstdint>
#include "tsystem.h"

class TButtonStates
{
    public:
        TButtonStates(const TButtonStates& bs);
        TButtonStates(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv);
        ~TButtonStates();

        bool isButton(uint32_t ID) { return ID == mID; }
        bool isButton(BUTTONTYPE t, uint32_t ID);
        bool isButton(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv);
        bool isButton(const TButtonStates& bs);
        bool isButton(BUTTONTYPE t, int rap, int rad, int rch, int rcp);
        bool isButton(BUTTONTYPE t, int rlp, int rlv);
        void setLastLevel(int level) { mLastLevel = level; }
        int getLastLevel() { return mLastLevel; }
        void setLastJoyX(int x) { mLastJoyX = x; }
        int getLastJoyX() { return mLastJoyX; }
        void setLastJoyY(int y) { mLastJoyY = y; }
        int getLastJoyY() { return mLastJoyY; }
        void setLastSendLevelX(int x) { mLastSendLevelX = x; }
        int getLastSendLevelX() { return mLastSendLevelX; }
        void setLastSendLevelY(int y) { mLastSendLevelY = y; }
        int getLastSendLevelY() { return mLastSendLevelY; }

        uint32_t getID() { return mID; }
        BUTTONTYPE getType() { return type; }

    private:
        void init();

        uint32_t mID{0};        // The button ID
        BUTTONTYPE type;        // The type of the button

        int ap{1};              // Address port (default: 1)
        int ad{0};              // Address channel
        int ch{0};              // Channel number
        int cp{1};              // Channel port (default: 1)
        int lp{1};              // Level port (default: 1)
        int lv{0};              // Level code

        int mLastLevel{0};      // The last level value for a bargraph
        int mLastJoyX{0};       // The last x position of the joystick curser
        int mLastJoyY{0};       // The last y position of the joystick curser
        int mLastSendLevelX{0}; // This is the last level send from a bargraph or the X level of a joystick
        int mLastSendLevelY{0}; // This is the last Y level from a joystick

};

#endif  // __TBUTTONSTATES_H__

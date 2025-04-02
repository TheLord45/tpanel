/*
 * Copyright (C) 2023 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

/**
 * @brief The TButtonStates class
 * The class holds the initial state of a button. This is needed because
 * the states could be changed by commands. There exists also a command to
 * restore the states of a button. Then the default values are taken from
 * a list containing pointers of this class. So the original button state
 * can easily be restored.
 * To make each state collection unique, a CRC checksum is calculated. This
 * unique number identifies each button over popups. Means, if an idendical
 * button exists on more then 1 popup or page, the CRC will be the same and
 * the button can be identified.
 */
class TButtonStates
{
    public:
    /**
         * @brief TButtonStates
         * Constructor.
         *
         * @param bs    A class of this type
         */
        TButtonStates(const TButtonStates& bs);
        /**
         * @brief TButtonStates
         * Constructor.
         *
         * @param t     Button type
         * @param rap   Address port
         * @param rad   Address channel
         * @param rch   Channel number
         * @param rcp   Channel port
         * @param rlp   Level port
         * @param rlv   Level code
         */
        TButtonStates(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv);
        ~TButtonStates();

        /**
         * @brief isButton
         * Test the CRC.
         *
         * @param ID    CRC
         * @return Returns TRUE if the CRC \b ID is dentical to the internal CRC.
         */
        bool isButton(uint32_t ID) { return ID == mID; }
        /**
         * @brief isButton
         * Test the type and CRC.
         *
         * @param t     Type of button
         * @param ID    CRC
         *
         * @return Returns TRUE if the type of button \b t and the CRC \b ID is
         * identical to the internal values.
         */
        bool isButton(BUTTONTYPE t, uint32_t ID);
        /**
         * @brief isButton
         * Test for all values.
         *
         * @param t     Button type
         * @param rap   Address port
         * @param rad   Address channel
         * @param rch   Channel number
         * @param rcp   Channel port
         * @param rlp   Level port
         * @param rlv   Level code
         *
         * @return If all parameters are identical to the internal values the method
         * return TRUE.
         */
        bool isButton(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv);
        /**
         * @brief isButton
         * Test for all values.
         *
         * @param bs    A TbuttonStates class
         *
         * @return If all parameters contained in \b bs are identical to the
         * internal values of the method, it returns TRUE.
         */
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

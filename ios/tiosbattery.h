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
#ifndef TIOSBATTERY_H
#define TIOSBATTERY_H

#include "tpagemanager.h"

class TIOSBattery
{
    public:
        typedef enum BSTATE
        {
            BS_UNKNOWN,
            BS_UNPLUGGED,
            BS_CHARGING,
            BS_FULL
        }BSTATE;

        TIOSBattery();
        ~TIOSBattery();

        void update();
        int getBatteryLeft() { return mLeft; }
        int getBatteryState() { return mState; }

        static void informStatus(int left, int state)
        {
            if (gPageManager)
                gPageManager->informBatteryStatus(left, state);
        }

    private:
        int mLeft{0};               // The left battery %
        BSTATE mState{BS_UNKNOWN};  // The state (plugged, charging, ...)
};

#endif // TIOSBATTERY_H

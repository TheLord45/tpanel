/*
 * Copyright (C) 2025 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef TBATTERY_H
#define TBATTERY_H

#include <thread>
#include <map>
#include <functional>

class TBattery
{
    public:
        TBattery();
        ~TBattery();

        int getLoad();
        bool isCharging();
        void regCallback(std::function<void (int load, bool charge)> func) { _callback = func; }

    protected:
        typedef struct BATTERY_t
        {
            int ID{0};          // The number of the battery class
            int load{0};        // load of battery in %
            bool ac{false};     // TRUE = Device is charging
        }BATTERY_t;

        int batteryState();
#ifdef __linux__
        int linuxBattery();
#endif
#ifdef __OSX_AVAILABLE
        int macBattery();
#endif
#if defined(__linux__) || defined(__OSX_AVAILABLE)
        void runTimer();
#endif
    private:
        std::function<void (int load, bool charge)> _callback{nullptr};

        std::map<int, BATTERY_t> mBState; // The battery load in %
        bool mHaveBattery{false};       // TRUE: Device has a battery
        int mBatteryNumber{0};          // Number of batteries.
        int mOldLoad{0};                // The previous load in %
        bool mOldAc{false};             // The old state of charging
#if defined(__linux__) || defined(__OSX_AVAILABLE)
        bool mTimerRun{false};          // TRUE = Timer runs; FALSE = Timer stops
        std::thread mTimerThread;       // Thread for timer
#endif
};

#endif // TBATTERY_H

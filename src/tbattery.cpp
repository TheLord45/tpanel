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
#include <string>
#include <fstream>
#include <thread>
#ifdef __OSX_AVAILABLE
#include <cstring>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFString.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#endif
#include "tbattery.h"
#include "terror.h"
#include "tresources.h"

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
using std::exception;
using std::map;
using std::pair;
using std::ifstream;
using std::thread;

TBattery::TBattery()
{
    DECL_TRACER("TBattery::TBattery()");
#ifdef __linux__
    mTimerRun = true;
    linuxBattery();
    runTimer();
#endif
#ifdef __OSX_AVAILABLE
    mTimerRun = true;
    macBattery();
    runTimer();
#endif
}

TBattery::~TBattery()
{
    DECL_TRACER("TBattery::~TBattery()");
#if defined(__linux__) || defined(__OSX_AVAILABLE)
    mTimerRun = false;
#endif
}

int TBattery::batteryState()
{
    DECL_TRACER("TBattery::batteryState()");

#ifdef __linux__
    return linuxBattery();
#elif defined(__OSX_AVAILABLE)
    return macBattery();
#else
    return 0;
#endif
}
#ifdef __linux__
int TBattery::linuxBattery()
{
    DECL_TRACER("TBattery::linuxBattery()");

    string path = "/sys/class/power_supply";
    string file;

    if (!fs::exists(path))
    {
        MSG_WARNING("This device has no battery!");
        return 0;
    }

    try
    {
        for(auto& p: fs::directory_iterator(path))
        {
            string f = fs::path(p.path()).filename();
            MSG_DEBUG("Found file: " << f);

            if (f.at(0) == '.')
                continue;

            if (startsWith(f, "AC"))
            {
                int num = atoi(f.substr(2).c_str());
                map<int, BATTERY_t>::iterator entry;
                BATTERY_t bat;
                ifstream in;

                try
                {
                    char charge[64];
                    file = p.path().u8string() + "/online";

                    memset(charge, 0, sizeof(charge));
                    in.open(file);
                    in.read(charge, sizeof(charge));
                    in.close();
                    bat.ID = num;
                    bat.ac = charge[0] == '1';
                }
                catch (exception &e)
                {
                    MSG_ERROR("Error reading file \"" << file << "\": " << e.what());

                    if (in.is_open())
                        in.close();
                }

                MSG_DEBUG("Charging state " << num << ": " << (bat.ac ? "Charging" : "Not charging"));

                if (mBState.empty() || (entry = mBState.find(num)) == mBState.end())
                    mBState.insert(pair<int, BATTERY_t>(num, bat));
                else
                    entry->second.ac = bat.ac;
            }
            else if (startsWith(f, "BAT"))
            {
                int num = atoi(f.substr(3).c_str());
                map<int, BATTERY_t>::iterator entry;
                BATTERY_t bat;
                ifstream in;
                mHaveBattery = true;
                mBatteryNumber++;

                try
                {
                    char ld[64];
                    file = p.path().u8string() + "/capacity";

                    memset(ld, 0, sizeof(ld));
                    in.open(file);
                    in.read(ld, sizeof(ld));
                    in.close();
                    bat.ID = num;
                    bat.load = atoi(ld);
                }
                catch (exception &e)
                {
                    MSG_ERROR("Error reading file \"" << file << "\": " << e.what());

                    if (in.is_open())
                        in.close();
                }

                MSG_DEBUG("Loading state " << num << ": " << bat.load << "%");

                if (mBState.empty() || (entry = mBState.find(num)) == mBState.end())
                    mBState.insert(pair<int, BATTERY_t>(num, bat));
                else
                    entry->second.load = bat.load;
            }
        }
    }
    catch(exception& e)
    {
        MSG_ERROR("Error: " << e.what());
        return 0;
    }

    return getLoad();
}
#endif  // __linux__

#if defined(__linux__) || defined(__OSX_AVAILABLE)
void TBattery::runTimer()
{
    DECL_TRACER("TBattery::runTimer()");

    try
    {
        mTimerThread = thread([=] {
            MSG_PROTOCOL("Thread \"TBattery::runTimer()\" was started.");

            while (mTimerRun)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                if (!mTimerRun)
                    break;
#ifdef __linux__
                int load = linuxBattery();
#elif defined(__OSX_AVAILABLE)
                int load = macBattery();
#endif
                bool charge = isCharging();

                if (_callback && (mOldLoad != load || mOldAc != charge))
                {
                    mOldLoad = load;
                    mOldAc = charge;
                    _callback(load, charge);
                }
            }
        });

        mTimerThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting a thread to handle the battery timer: " << e.what());
    }
}
#endif

#ifdef __OSX_AVAILABLE
int TBattery::macBattery()
{
    DECL_TRACER("TBattery::macBattery()");

    CFTypeRef src = IOPSCopyPowerSourcesInfo();

    if (src == nullptr)
    {
        MSG_ERROR("Error getting power source info!");
        return 0;
    }

    CFArrayRef list = IOPSCopyPowerSourcesList(src);

    if (list == nullptr)
    {
        MSG_ERROR("Error getting power source list!");
        CFRelease(src);
        return 0;
    }

    long count = CFArrayGetCount(list);
    MSG_DEBUG("Searching for " << count << " entries ...");

    if (count == 0)
    {
        CFRelease(src);
        CFRelease(list);
        return 0;
    }

    BATTERY_t battery;
    map<int, BATTERY_t>::iterator entry;

    for(long i = 0; i < count; i++)
    {
        CFDictionaryRef ps = IOPSGetPowerSourceDescription(src, CFArrayGetValueAtIndex(list, i));

        if (ps == nullptr)
        {
            MSG_ERROR("Error getting power source description on loop " << i << " from " << count << "!");
            continue;
        }

        CFStringRef powerSource = static_cast<CFStringRef>(CFDictionaryGetValue(ps, CFSTR(kIOPSPowerSourceStateKey)));

        if (powerSource != nullptr)
        {
            char buffer[64];

            CFStringGetCString(powerSource, buffer, sizeof(buffer), CFStringEncodings::kCFStringEncodingDOSLatin1);

            if (strncmp(buffer, kIOPSBatteryPowerValue, sizeof(kIOPSBatteryPowerValue)) == 0)
                battery.ac = true;
        }
        else
        {
            MSG_ERROR("Error getting battery power value on loop " << i << " from " << count << "!");
        }

        CFStringRef load = static_cast<CFStringRef>(CFDictionaryGetValue(ps, CFSTR(kIOPSCurrentCapacityKey)));

        if (load != nullptr)
        {
            char buffer[64];

            CFStringGetCString(load, buffer, sizeof(buffer), CFStringEncodings::kCFStringEncodingDOSLatin1);
            battery.load = atoi(buffer);
        }
        else
        {
            MSG_ERROR("Error getting current capacity on loop " << i << " from " << count << "!");
        }

        MSG_DEBUG("(" << i << ")Battery load: " << battery.load << "%, Charge state: " << (battery.ac ? "Charging" : "Not charging"));

        if (mBState.empty() || (entry = mBState.find(i)) == mBState.end())
            mBState.insert(pair<int, BATTERY_t>(i, battery));
        else
        {
            entry->second.ac = battery.ac;
            entry->second.load = battery.load;
        }
    }

    CFRelease(src);
    CFRelease(list);

    return battery.load;
}
#endif

int TBattery::getLoad()
{
    DECL_TRACER("TBattery::getLoad()");

    if (mBState.empty())
        return 0;

    int load = 0;
    int num = 0;
    map<int, BATTERY_t>::iterator iter;

    for (iter = mBState.begin(); iter != mBState.end(); ++iter)
    {
        load += iter->second.load;
        num++;
    }

    double l = static_cast<double>(load) / static_cast<double>(num);

    if (l > 100.0)
        l = 0;

    return static_cast<int>(l);
}

bool TBattery::isCharging()
{
    DECL_TRACER("TBattery::isCharging()");

    if (mBState.empty())
        return false;

    map<int, BATTERY_t>::iterator iter;

    for (iter = mBState.begin(); iter != mBState.end(); ++iter)
    {
        if (iter->second.ac)
            return true;
    }

    return false;
}

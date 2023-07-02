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

#ifndef __TESTMODE_H__
#define __TESTMODE_H__

#include <string>
#include <thread>
#include <atomic>
#include <vector>

#include <QtGlobal>

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#define TESTMODE    1
#endif

extern std::atomic<bool> __success;
extern std::atomic<bool> __done;
extern bool _testmode;

class _TestMode
{
    public:
        _TestMode(const std::string& path);
        ~_TestMode();

        void run();
        void setResult(const std::string& res) { verify = res; }

    private:
        typedef struct _TESTCMD
        {
            std::string command;    // The command to execute
            std::string result;     // The expected result
            bool compare{false};    // TRUE: Compare expected result with real result
        }_TESTCMD;

        void inject(int port, const std::string& c);
        void testSuccess(_TESTCMD& tc);
        void start();
        void inform(const std::string& msg);

        std::thread mThread;
        bool isRunning{false};
        std::string mPath;
        std::vector<std::string> mCmdFiles;
        std::string verify;         // The real result
};

extern _TestMode *_gTestMode;

#ifndef TESTMODE
extern bool _testmode;
#elif TESTMODE != 1
extern bool _testmode;
#endif  // TESTMODE

#endif

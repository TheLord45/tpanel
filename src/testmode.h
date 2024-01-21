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

#if TESTMODE == 1

#include <string>
#include <thread>
#include <atomic>
#include <vector>

#include <QtGlobal>

extern bool __success;
extern bool __done;
extern bool _test_screen;
extern bool _testmode;
extern bool _run_test_ready;
extern bool _block_screen;
class _TestMode
{
    public:
        _TestMode(const std::string& path);
        ~_TestMode();

        void run();
        void setMouseClick(int x, int y, bool pressed);
        void setResult(const std::string& res) { mVerify = res; }

    private:
        typedef struct _TESTCMD
        {
            std::string command;    // The command to execute
            std::string result;     // The expected result
            bool compare{false};    // TRUE: Compare expected result with real result
            bool nowait{false};     // TRUE: Don't wait until the command finished (no compare!)
            bool reverse{false};    // TRUE: This changes the meaning of success and failure.
            bool waitscreen{false}; // TRUE: Wait for screen finished even if no comparison is made.
            bool saveresult{false}; // TRUE: The result is saved into the named variable
            bool compresult{false}; // TRUE: The result is compared against the named variable
        }_TESTCMD;

        typedef struct _VARS
        {
            std::string name;
            std::string content;
        }_VARS;

        void inject(int port, const std::string& c);
        void testSuccess(_TESTCMD& tc);
        void start();
        void inform(const std::string& msg);
        void saveVariable(const std::string& name, const std::string& content);
        std::string getVariable(const std::string& name);
        void deleteVariable(const std::string& name);

        std::thread mThread;
        bool isRunning{false};
        std::string mPath;
        std::vector<std::string> mCmdFiles;
        std::string mVerify;         // The real result
        int mX{0};
        int mY{0};
        bool mPressed{false};
        int mCaseNumber{0};         // The number of the test case
        std::string mVarName;
        std::vector<_VARS> mVariables;
};

extern _TestMode *_gTestMode;

#define setDone()       if (_testmode) { __done = true; }
#define setScreenDone() if (_testmode && !_block_screen) { MSG_DEBUG("setScreenDone(); at module " << __FILE__ << ": " << __LINE__); _test_screen = true; }
#define setAllDone()    if (_testmode) { MSG_DEBUG("setAllDone(); at module " << __FILE__ << ": " << __LINE__); _test_screen = true; __done = true; }

#else
extern bool _testmode;
#endif  // TESTMODE == 1
#endif  // __TESTMODE_H__

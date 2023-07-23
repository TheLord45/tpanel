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
#if TESTMODE == 1
#include <cstring>
#include <iomanip>
#include <fstream>

#include "testmode.h"
#include "tpagemanager.h"
#include "tdirectory.h"
#include "tconfig.h"
#include "tresources.h"
#include "terror.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif

#define TIMEOUT     5000000         // Microseconds (5 seconds)
#define DELAY       100             // Wait this amount of microseconds

using std::string;
using std::strncpy;
using std::min;
using std::atomic;
using std::vector;
using std::cout;
using std::endl;
using std::ifstream;
using namespace dir;

bool __success{false};              // TRUE indicates a successful test
bool __done{false};                 // TRUE marks a test case as done
bool _test_screen{false};           // TRUE: The graphical part of a test case is done.
bool _testmode{false};              // TRUE: Test mode is active. Activated by command line.
bool _run_test_ready{false};        // TRUE: The test cases are allowd to start.
string gLastCommand;                // The last command to be tested.
_TestMode *_gTestMode{nullptr};     // Pointer to class _TestMode. This class must exist only once!

extern amx::TAmxNet *gAmxNet;

_TestMode::_TestMode(const string& path)
    : mPath(path)
{
#if TESTMODE == 1
    DECL_TRACER("_TestMode::_TestMode(const string& path)");

    TDirectory dir(mPath);
    dir.scanFiles(".tst");
    size_t num = dir.getNumEntries();
    size_t pos = 0;

    while (pos < num)
    {
        DFILES_T f = dir.getEntry(pos);

        if (dir.testText(f.attr))
            mCmdFiles.push_back(f.name);

        pos++;
    }

    sort(mCmdFiles.begin(), mCmdFiles.end());
#endif
}

_TestMode::~_TestMode()
{
    DECL_TRACER("_TestMode::~_TestMode()");

    _gTestMode = nullptr;
    gLastCommand.clear();
    prg_stopped = true;
    killed = true;

    if (gAmxNet)
        gAmxNet->stop();

    if (gPageManager)
        gPageManager->callShutdown();
}

void _TestMode::run()
{
#if TESTMODE == 1
    DECL_TRACER("_TestMode::run()");

    if (isRunning)
        return;

    try
    {
        mThread = std::thread([=] { this->start(); });
        mThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error spawning thread: " << e.what());
    }
#endif
}

void _TestMode::start()
{
    isRunning = true;
    DECL_TRACER("_TestMode::start()");

    while (!_run_test_ready)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    vector<string>::iterator iter;

    for (iter = mCmdFiles.begin(); iter != mCmdFiles.end(); ++iter)
    {
        _TESTCMD tcmd;
        inform("------ File: " + *iter + " ------");

        try
        {
            ifstream is(*iter);
            char buffer[4096];
            int line = 1;
            int port = 1;
            string command, content;
            tcmd.command.clear();
            tcmd.result.clear();
            tcmd.compare = false;

            while (is.getline(buffer, sizeof(buffer)))
            {
                if (buffer[0] == '#' || buffer[0] == '\0')
                {
                    line++;
                    continue;
                }

                string scmd(buffer);
                command.clear();
                content.clear();
                size_t pos = scmd.find("=");

                // Parse the command line and extract the command and the content
                if (scmd == "exec")
                    command = scmd;
                else if (pos == string::npos)
                {
                    MSG_WARNING("(" << *iter << ") Line " << line << ": Invalid or malformed command!");
                    line++;
                    continue;
                }
                else
                {
                    command = scmd.substr(0, pos);
                    content = scmd.substr(pos + 1);
                }

                // Test for the command and take the desired action.
                if (command == "command")
                    tcmd.command = content;
                else if (command == "port")
                {
                    int p = atoi(content.c_str());

                    if (p > 0)
                        port = p;
                }
                else if (command == "result")
                    tcmd.result = content;
                else if (command == "compare")
                    tcmd.compare = isTrue(content);
                else if (command == "reverse")
                    tcmd.reverse = isTrue(content);
                else if (command == "wait")
                {
                    unsigned long ms = atol(content.c_str());

                    if (ms != 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
                }
                else if (command == "nowait")
                    tcmd.nowait = isTrue(content);
                else if (command == "screenwait")
                    tcmd.waitscreen = isTrue(content);
                else if (command == "saveresult")
                {
                    mVarName = content;
                    tcmd.saveresult = true;
                }
                else if (command == "compresult")
                {
                    mVarName = content;
                    tcmd.compresult = true;
                }
                else if (command == "exec")
                {
                    if (tcmd.command.empty())
                    {
                       MSG_WARNING("(" << *iter << ") Nothing to execute on line " << line << "!");
                       line++;
                       continue;
                    }

                    mCaseNumber++;
                    inform("\x1b[1;37mCase " + intToString(mCaseNumber) + "\x1b[0m: \x1b[0;33m" + tcmd.command + "\x1b[0m");
                    __done = false;
                    __success = false;
                    _test_screen = false;
                    inject(port, tcmd.command);

                    if (!tcmd.nowait)
                        testSuccess(tcmd);
                    else
                    {
                        MSG_WARNING("Skipped result check!");
                        inform("   Result check skipped!");
                    }

                    tcmd.command.clear();
                    tcmd.result.clear();
                    tcmd.compare = false;
                    tcmd.nowait = false;
                    tcmd.reverse = false;
                    tcmd.waitscreen = false;
                    tcmd.saveresult = false;
                    tcmd.compresult = false;
                    mVerify.clear();
                    port = 1;
                }

                line++;
            }

            is.close();
        }
        catch (std::exception& e)
        {
            MSG_ERROR("Error on file " << *iter << ": " << e.what());
            continue;
        }
    }

    isRunning = false;
    delete this;
}

void _TestMode::inject(int port, const string& c)
{
    if (!gPageManager)
        return;

    gLastCommand = c;
    int channel = TConfig::getChannel();
    int system = TConfig::getSystem();

    string bef = c;

    if (startsWith(bef, "PUSH-"))
    {
        amx::ANET_SEND scmd;
        size_t pos = bef.find("-");
        string bt = bef.substr(pos + 1);
        vector<string> parts = StrSplit(bt, ",");

        if (parts.size() < 2)
        {
            MSG_ERROR("Command PUSH has syntax: PUSH-<x>,<y>!");
            return;
        }

        mX = atoi(parts[0].c_str());
        mY = atoi(parts[1].c_str());
        mPressed = true;

        if (gPageManager)
            gPageManager->mouseEvent(mX, mY, mPressed);
    }
    else if (startsWith(bef, "RELEASE-"))
    {
        amx::ANET_SEND scmd;
        size_t pos = bef.find("-");
        string bt = bef.substr(pos + 1);
        vector<string> parts = StrSplit(bt, ",");

        if (parts.size() < 2)
        {
            MSG_ERROR("Command RELEASE has syntax: RELEASE-<x>,<y>!");
            return;
        }

        mX = atoi(parts[0].c_str());
        mY = atoi(parts[1].c_str());
        mPressed = false;

        if (gPageManager)
            gPageManager->mouseEvent(mX, mY, mPressed);
    }
    else if (startsWith(bef, "CLICK-"))
    {
        amx::ANET_SEND scmd;
        size_t pos = bef.find("-");
        string bt = bef.substr(pos + 1);
        vector<string> parts = StrSplit(bt, ",");

        if (parts.size() < 2)
        {
            MSG_ERROR("Command CLICK has syntax: CLICK-<x>,<y>!");
            return;
        }

        mX = atoi(parts[0].c_str());
        mY = atoi(parts[1].c_str());

        if (gPageManager)
        {
            mPressed = true;
            gPageManager->mouseEvent(mX, mY, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            mPressed = false;
            gPageManager->mouseEvent(mX, mY, false);
        }
    }
    else
    {
        amx::ANET_COMMAND cmd;
        cmd.MC = 0x000c;
        cmd.device1 = channel;
        cmd.port1 = port;
        cmd.system = system;
        cmd.data.message_string.system = system;
        cmd.data.message_string.device = channel;
        cmd.data.message_string.port = port;    // Must be the address port of button
        cmd.data.message_string.type = DTSZ_STRING;    // Char string
        cmd.data.message_string.length = min(c.length(), sizeof(cmd.data.message_string.content));
        memset(cmd.data.message_string.content, 0, sizeof(cmd.data.message_string.content));
        memcpy ((char *)cmd.data.message_string.content, c.c_str(), cmd.data.message_string.length);
        gPageManager->doCommand(cmd);
    }
}

void _TestMode::setMouseClick(int x, int y, bool pressed)
{
    DECL_TRACER("_TestMode::setMouseClick(int x, int y, bool pressed)");

    if (mX == 0 && mY == 0 && mPressed == false)
        return;

    if (mX == x && mY == y && mPressed == pressed)
    {
        __success = true;
        _test_screen = true;
        __done = true;
        mX = mY = 0;
        mPressed = false;
    }
}

void _TestMode::testSuccess(_TESTCMD& tc)
{
    ulong total = 0;
    bool ready = ((tc.compare || tc.waitscreen) ? (__done && _test_screen) : __done);

    while (!ready && total < TIMEOUT)
    {
        usleep(DELAY);
        total += DELAY;
        ready = ((tc.compare || tc.waitscreen) ? (__done && _test_screen) : __done);
    }

    if (!mVarName.empty())
    {
        if (tc.saveresult)
            saveVariable(mVarName, mVerify);

        if (tc.compresult)
            mVerify = getVariable(mVarName);

        mVarName.clear();
    }

    if (total >= TIMEOUT)
    {
        MSG_WARNING("Command \"" << gLastCommand << "\" timed out!");
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "tpanel", "Command \"%s\" timed out!", gLastCommand.c_str());
#else
        cout << "Command \"" << gLastCommand << "\" timed out!" << endl;
#endif
        __success = false;
        tc.compare = false;
    }

    if (tc.compare && tc.result.compare(mVerify) == 0)
    {
        MSG_WARNING("The result \"" << mVerify << "\" is equal to \"" << tc.result << "\"!");
        __success = tc.reverse ? false : true;
    }
    else if (tc.compare)
    {
        MSG_WARNING("The result \"" << mVerify << "\" does not match the expected result of \"" << tc.result << "\"!");
        __success = tc.reverse ? true : false;
    }

    if (__success)
    {
        MSG_INFO("SUCCESS (" << gLastCommand << ")");
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "tpanel", "SUCCESS");
#else
        cout << "   \x1b[0;32mSUCCESS\x1b[0m" << endl;
#endif
    }
    else
    {
        MSG_ERROR("FAILED (" << gLastCommand << ")");
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "FAILED");
#else
        cout << "   \x1b[0;31mFAILED\x1b[0m" << endl;
#endif
    }

    __success = false;
    _test_screen = false;
    __done = false;
}

void _TestMode::inform(const string& msg)
{
    MSG_INFO(msg);
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_ERROR, "tpanel", "%s", msg.c_str());
#else
    cout << msg << endl;
#endif
}

void _TestMode::saveVariable(const string &name, const string &content)
{
    DECL_TRACER("_TestMode::saveVariable(const string &name, const string &content)");

    if (name.empty())
        return;

    _VARS v;

    if (mVariables.empty())
    {
        v.name = name;
        v.content = content;
        mVariables.push_back(v);
        return;
    }

    vector<_VARS>::iterator iter;

    for (iter = mVariables.begin(); iter != mVariables.end(); ++iter)
    {
        if (iter->name == name)
        {
            iter->content = content;
            return;
        }
    }

    v.name = name;
    v.content = content;
    mVariables.push_back(v);
}

string _TestMode::getVariable(const string &name)
{
    DECL_TRACER("_TestMode::getVariable(const string &name)");

    if (mVariables.empty())
        return string();

    vector<_VARS>::iterator iter;

    for (iter = mVariables.begin(); iter != mVariables.end(); ++iter)
    {
        if (iter->name == name)
            return iter->content;
    }

    return string();
}

void _TestMode::deleteVariable(const string &name)
{
    DECL_TRACER("_TestMode::deleteVariable(const string &name)");

    if (mVariables.empty())
        return;

    vector<_VARS>::iterator iter;

    for (iter = mVariables.begin(); iter != mVariables.end(); ++iter)
    {
        if (iter->name == name)
        {
            mVariables.erase(iter);
            return;
        }
    }
}

#else
bool _testmode{false};              // TRUE: Test mode is active. Activated by command line.
#endif
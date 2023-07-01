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

#define TIMEOUT     5000000  // Microseconds (5 seconds)
#define DELAY       100      // Wait this amount of microseconds

using std::string;
using std::strncpy;
using std::min;
using std::atomic;
using std::vector;
using std::cout;
using std::endl;
using std::ifstream;
using namespace dir;

atomic<bool> __success{false};
atomic<bool> __done{false};
bool _testmode{false};
string gLastCommand;
_TestMode *_gTestMode{nullptr};

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

    vector<string>::iterator iter;

    for (iter = mCmdFiles.begin(); iter != mCmdFiles.end(); ++iter)
    {
        _TESTCMD tcmd;

        try
        {
            ifstream is(*iter);
            char buffer[4096];
            int line = 1;
            tcmd.command.clear();
            tcmd.result.clear();
            tcmd.compare = false;

            while (is.getline(buffer, sizeof(buffer)))
            {
                if (buffer[0] == '#')
                    continue;

                if (startsWith(buffer, "command="))
                {
                    string scmd(buffer);

                    size_t pos = scmd.find("=");
                    tcmd.command = scmd.substr(pos + 1);
                }
                else if (startsWith(buffer, "result="))
                {
                    string scmd(buffer);

                    size_t pos = scmd.find("=");
                    tcmd.result = scmd.substr(pos + 1);
                }
                else if (startsWith(buffer, "compare="))
                {
                    string scmd(buffer);

                    size_t pos = scmd.find("=");
                    string res = scmd.substr(pos + 1);
                    tcmd.compare = isTrue(res);
                }
                else if (startsWith(buffer, "exec"))
                {
                    if (tcmd.command.empty())
                    {
                       MSG_WARNING("Nothing to execute on line " << line << "!");
                       line++;
                       continue;
                    }

                    inform("Testing command: " + tcmd.command);
                    inject(1, tcmd.command);
                    testSuccess(tcmd);

                    tcmd.command.clear();
                    tcmd.result.clear();
                    tcmd.compare = false;
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
/*
    inform("Testing command: ^MUT-1");
    inject(1, "^MUT-1");
    testSuccess();

    inform("Testing command: ^MUT-0");
    inject(1, "^MUT-0");
    testSuccess();
*/
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

void _TestMode::testSuccess(_TESTCMD& tc)
{
    ulong total = 0;

    while (!__done && total < TIMEOUT)
    {
        usleep(DELAY);
        total += DELAY;
    }

    if (total >= TIMEOUT)
    {
        MSG_WARNING("Command \"" << gLastCommand << "\" timed out!");
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "tpanel", "Command \"%s\" timed out!", gLastCommand.c_str());
#else
        cout << "Command \"" << gLastCommand << "\" timed out!" << endl;
#endif
    }

    if (tc.compare && tc.result == verify)
    {
        MSG_WARNING("The result \"" << verify << "\" is equal to \"" << tc.result << "\"!");
        __success = true;
    }
    else if (tc.compare)
    {
        MSG_WARNING("The result \"" << verify << "\" does not match the expected result of \"" << tc.result << "\"!");
        __success = false;
    }

    if (__success)
    {
        MSG_INFO("SUCCESS (" << gLastCommand << ")");
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, "tpanel", "SUCCESS (%s)", gLastCommand.c_str());
#else
        cout << "   SUCCESS (" << gLastCommand << ")" << endl;
#endif
    }
    else
    {
        MSG_ERROR("FAILED (" << gLastCommand << ")");
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "FAILED (%s)", gLastCommand.c_str());
#else
        cout << "   FAILED (" << gLastCommand << ")" << endl;
#endif
    }

    __success = false;
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

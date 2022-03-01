/*
 * Copyright (C) 2020, 2021 by Andreas Theofilu <andreas@theosys.at>
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
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include "tconfig.h"
#include "tsettings.h"
#include "tpagelist.h"
#include "tpage.h"
#include "tsubpage.h"
#include "tpagemanager.h"
#include "tqtmain.h"

#ifdef Q_OS_ANDROID
#include <QStandardPaths>
#endif
#ifdef __ANDROID__
#include <android/log.h>
#endif

using std::string;
using std::find;
using std::vector;
using std::cout;
using std::endl;

bool _restart_ = false;
#ifdef __ANDROID__
extern std::atomic<bool> killed;
extern std::atomic<bool> _netRunning;
#endif

/**
 * @class InputParser
 * @brief The InputParser class parses the command line.
 *
 * This class takes the command line arguments and parses them. It creates an
 * internal vector array to hold the parameters.
 */
class InputParser
{
    public:
        /**
         * @brief InputParser is the constructor.
         *
         * The constructor requires the command line parameters. It immediately
         * starts to parse each parameter. If it finds the string `--` it stops
         * and ignores the rest of parameters. \p argc contains the rest of
         * the parameters after `--`, if there are any.
         *
         * @param argc  A pointer to the numbers of command line parameters.
         *              This parameter must not be `NULL`.
         * @param argv  The 2 dimensional array of command line parameters.
         *              This parameter must not be `NULL`.
         */
        InputParser(int *argc, char **argv)
        {
            int i;

            for (i = 1; i < *argc; ++i)
            {
                if (string(argv[i]).compare("--") == 0)
                    break;

                this->tokens.push_back(string(argv[i]));
            }

            *argc -= i;

            if (*argc <= 1)
                *argc = 1;
            else
            {
                *argc = *argc + 1;
                *(argv + i - 1) = *argv;
            }
        }
        /**
         * @brief getCmdOption searches for the command line option \p option.
         * @author iain
         *
         * The method searches for the command line option \p option and
         * returns the parameter after the option, if there is one.
         *
         * @param option    The name of a command line option. This is any
         *                  name starting with 1 or 2 dash (-). The name in
         *                  the parameter must include the dash(es) in front
         *                  of the name.\n
         *                  If the option was found and the next parameter on
         *                  the command line doesn't start with a dash, the
         *                  parameter is returned.
         *
         * @return Tf the option was found and the parameter on the command
         * line following the option doesn't start with a dash, it is returned.
         * Otherwise an empty string is returned.
         */
        const string& getCmdOption(const string &option) const
        {
            vector<string>::const_iterator itr;
            itr = find(this->tokens.begin(), this->tokens.end(), option);

            if (itr != this->tokens.end() && ++itr != this->tokens.end())
                return *itr;

            static const string empty_string("");
            return empty_string;
        }

        /**
         * @brief cmdOptionExists tests for an existing option.
         * @author iain
         *
         * This function tests whether the option \p option exists or not. If
         * the option was found, it returnes `true`.
         *
         * @param option    The name of a command line option. This is any
         *                  name starting with 1 or 2 dash (-). The name in
         *                  the parameter must include the dash(es) in front
         *                  of the name.\n
         *
         * @return If the command line option was found in the internal vector
         * array `true` is returned. Otherwise it returnes `false`.
         */
        bool cmdOptionExists(const string &option) const
        {
            return find(this->tokens.begin(), this->tokens.end(), option) != this->tokens.end();
        }

    private:
        vector <string> tokens;
};

/**
 * @brief usage displays on the standard output a small help.
 *
 * This function shows a short help with all available parameters and a brief
 * description of them.
 * \verbatim
 * NOTE: This function is not available on Android systems.
 * \endverbatim
 */
void usage()
{
#ifndef __ANDROID__
    cout << TConfig::getProgName() << " version " <<  V_MAJOR << "."  << V_MINOR << "." << V_PATCH << endl << endl;
    cout << "Usage: tpanel [-c <config file>]" << endl;
    cout << "-c | --config-file <file> The path and name of the configuration file." << endl;
    cout << "                          This parameter is optional. If it is omitted," << endl;
    cout << "                          The standard path is searched for the" << endl;
    cout << "                          configuration file." << endl << endl;
    cout << "-h | --help               This help." << endl << endl;
#endif
}

#ifndef __ANDROID__
/**
 * @brief banner displays a shor banner with informations about this application.
 *
 * This function shows a short information about this application. It prints
 * this on the standard output.
 * \verbatim
 * NOTE: This function is not available on Android systems.
 * \endverbatim
 *
 * @param pname The name of this application.
 */
void banner(const string& pname)
{
    if (!TConfig::showBanner())
        return;

    cout << pname << " v" << V_MAJOR << "."  << V_MINOR << "." << V_PATCH << endl;
    cout << "(C) Andreas Theofilu <andreas@theosys.at>" << endl;
    cout << "This program is under the terms of GPL version 3" << endl << endl;
}
#endif

/**
 * @brief Is called whenever the program starts up.
 *
 * This method is called to start up the program. It initializes the main
 * classes and waits until the main loop ends.
 * It is also called if the program have to start over. This happens when
 * the settings change the host, port or channel ID, or after receiving a new
 * surface.
 *
 * @param oldArgc   This is the previous parameter counter (only for desktop).
 * @param argc      This is the actual parameter counter.
 * @param argv      This is the pointer array to the environment.
 *
 * @return of success TRUE is returned. Otherwise FALSE.
 */
bool _startUp(int oldArgc, int argc, char *argv[])
{
    DECL_TRACER("_startUp(int oldArgc, int argc, char *argv[])");

    TPageManager *pageManager = new TPageManager;

    if (TError::isError())
    {
        delete pageManager;
        return false;
    }

    // Prepare command line stack
#ifndef __ANDROID__
    int pt = oldArgc - argc;
#else
    int pt = 0;
#endif
    // Start the graphical environment
    int ret = 0;

    // The _restart_ variable is reset in class initialization MainWindow.
    ret = qtmain(argc, &argv[pt], pageManager);
    delete pageManager;

    if (ret != 0)
        return false;

    return true;
}

/**
 * @brief main is the main entry function.
 *
 * This is where the program starts.
 *
 * @param argc  The number of command line arguments.
 * @param argv  A pointer to a 2 dimensional array containing the command line
 *              parameters.
 *
 * @return 0 on success. This means that no errors occured.\n
 * In case of an error a number grater than 0 is returned.
 */
int main(int argc, char *argv[])
{
    string configFile;
    int oldArgc = argc;
#ifndef __ANDROID__
    string pname = *argv;
    size_t pos = pname.find_last_of("/");

    if (pos != string::npos)
        pname = pname.substr(pos + 1);
#else
    string pname = "tpanel";
    killed = false;
    _netRunning = false;
#endif
    TConfig::setProgName(pname);    // Remember the name of this application.
#ifndef Q_OS_ANDROID
    InputParser input(&argc, argv); // Parse the command line parameters.

    // Evaluate the command line parameters.
    if (input.cmdOptionExists("-h") || input.cmdOptionExists("--help"))
    {
        banner(pname);
        usage();
        return 0;
    }

    if (input.cmdOptionExists("-c") || input.cmdOptionExists("--config-file"))
    {
        configFile = input.getCmdOption("-c");

        if (configFile.empty())
            configFile = input.getCmdOption("--config-file");

        if (configFile.empty())
        {
            banner(pname);
            std::cerr << "Missing the path and name of the configuration file!" << std::endl;
            usage();
            return 1;
        }
    }
#endif
    TError::clear();                    // Clear all errors (initialize)
    TConfig config(configFile);         // Read the configuration file.

    if (TError::isError())              // Exit if the previous command failed.
    {
        TError::displayMessage(TError::getErrorMsg());
        return 1;
    }
#ifndef __ANDROID__
    banner(pname);
#endif
    TError::clear();
    // Start the page manager. This is the core class handling everything.
    try
    {
        bool ret;

        do
        {
            ret = _startUp(oldArgc, argc, argv);

            if (_restart_)
            {
                MSG_INFO("Starting over ...");
                prg_stopped = false;
                killed = false;
            }
        }
        while (_restart_);

        if (!ret)
        {
            MSG_ERROR("Terminating because of a previous fatal error!");
            return 1;
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Fatal: " << e.what());
        return 1;
    }

    return 0;
}

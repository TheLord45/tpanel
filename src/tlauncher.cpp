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

#include <cstring>
#include <unistd.h>

#include "tlauncher.h"
#include "terror.h"

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
using std::vector;

bool TLauncher::mInitialized{false};
vector<APPDEF_t> TLauncher::APPS;

bool TLauncher::launch(const string& name)
{
    DECL_TRACER("TLauncher::launch(const string& name)");

    if (name.empty())
        return false;

    initialize();

    if (APPS.empty())
    {
        MSG_WARNING("No application found for " << name << "!");
        return false;
    }

    vector<APPDEF_t>::iterator iter;
    bool started = false;

    for (iter = APPS.begin(); iter != APPS.end(); ++iter)
    {
        if (iter->name == name)
        {
            string p = "/usr/bin/";
            string exe, prg;
            vector<string>::iterator pgIter;

            for (pgIter = iter->executeables.begin(); pgIter != iter->executeables.end(); ++pgIter)
            {
                string p = "/usr/bin/";
                string exe;

                if (fs::exists(p + *pgIter))
                {
                    exe = p + *pgIter;
                    pid_t pid;
                    started = true;

                    if (0 == (pid = fork()))
                    {
                        if (-1 == execl(exe.c_str(), pgIter->c_str(), (char *)NULL))
                        {
                            MSG_ERROR("Child process execl failed: " << strerror(errno));
                            return -1;
                        }
                    }

                    break;
                }
            }

            break;
        }
    }

    if (!started)
    {
        MSG_WARNING("No application found for " << name << "!");
    }

    return started;
}

void TLauncher::initialize()
{
    DECL_TRACER("TLauncher::initialize()");
#ifdef __linux__
    if (mInitialized)
        return;

    APPDEF_t a;

    a.name = "PDF Viewer";
    a.executeables.push_back("okular"),
    a.executeables.push_back("evince");
    APPS.push_back(a);
    a.executeables.clear();

    a.name = "Browser";
    a.executeables.push_back("firefox");
    a.executeables.push_back("chromium");
    a.executeables.push_back("konqueror");
    APPS.push_back(a);
    a.executeables.clear();

    a.name = "Calculator";
    a.executeables.push_back("kcalc");
    a.executeables.push_back("gnome-calculator");
    a.executeables.push_back("xcalc");
    APPS.push_back(a);
    a.executeables.clear();

    a.name = "Calendar";
    a.executeables.push_back("korganizer");
    a.executeables.push_back("calindori");
    a.executeables.push_back("gnome-calendar");
    a.executeables.push_back("evolution");
    APPS.push_back(a);
    a.executeables.clear();

    a.name = "Contacts";
    a.executeables.push_back("kaddressbook");
    a.executeables.push_back("gnome-contacts");
    APPS.push_back(a);
    a.executeables.clear();

    a.name = "Email";
    a.executeables.push_back("kmail");
    a.executeables.push_back("balsa");
    APPS.push_back(a);
    a.executeables.clear();

    a.name = "FileBrowser";
    a.executeables.push_back("dolphin");
    a.executeables.push_back("nautilus");
    APPS.push_back(a);
    a.executeables.clear();

    mInitialized = true;
#endif
}

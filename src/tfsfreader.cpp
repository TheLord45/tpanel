/*
 * Copyright (C) 2022 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

#include <fstream>
#include <functional>
#include <iostream>
#include <cstdint>
#include <filesystem>

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifndef __MACH__
#include <sys/sendfile.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "tfsfreader.h"
#include "tconfig.h"
#include "terror.h"
#include "readtp4.h"

#if __cplusplus < 201402L
#   error "This module requires at least C++ 14 standard!"
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

#define FTP_PORT    21

#define FTP_CMD_USER    0
#define FTP_CMD_PASS    1
#define FTP_CMD_PASV    2
#define FTP_CMD_TYPE    3
#define FTP_CMD_RETR    4
#define FTP_CMD_QUIT    5

using std::string;
using std::vector;
using std::bind;
/*
#if __GNUC__ < 9 && !defined(__ANDROID__)
   #if __cplusplus < 201703L
      #warning "Your C++ compiler seems to have no support for C++17 standard!"
   #endif
   #include <experimental/filesystem>
   namespace fs = std::experimental::filesystem;
#else
#  ifdef __ANDROID__
#   if _LIBCPP_STD_VER >= 17
#       include <filesystem>
        namespace fs = std::__fs::filesystem;
#   else
#       include <experimental/filesystem>
        namespace fs = std::__fs::filesystem;
#   endif
#  else
#   include <filesystem>
    namespace fs = std::filesystem;
#  endif
#endif
*/

std::function<int (off64_t xfered)> TFsfReader::_progress{nullptr};

TFsfReader::TFsfReader()
{
    DECL_TRACER("TFsfReader::TFsfReader()");

}

TFsfReader::~TFsfReader()
{
    DECL_TRACER("TFsfReader::~TFsfReader()");

    if (mFtpLib)
        delete mFtpLib;
}

bool TFsfReader::copyOverFTP(const string& fname, const string& target)
{
    DECL_TRACER("TFsfReader::copyOverFTP(const string& fname, const string& target)");

    if (mFtpLib)
        delete mFtpLib;

    mFtpLib = new ftplib();
    mFtpLib->regLogging(bind(&TFsfReader::logging, this, std::placeholders::_1, std::placeholders::_2));

    if (TConfig::getFtpPassive())
        mFtpLib->SetConnmode(ftplib::pasv);
    else
        mFtpLib->SetConnmode(ftplib::port);

    mFtpLib->SetCallbackLogFunction(&TFsfReader::callbackLog);      // Print some debugging messages
    mFtpLib->SetCallbackErrorFunction(&TFsfReader::callbackError);  // Print errors or info's
    mFtpLib->SetCallbackXferFunction(&TFsfReader::callbackXfer);    // This is the progress
    mFtpLib->SetCallbackBytes(10000L);                              // This tells the progress to be called every 10Kb
    string scon = TConfig::getController() + ":21";
    MSG_DEBUG("Trying to connect to " << scon);

    if (!mFtpLib->Connect(scon.c_str()))
    {
        delete mFtpLib;
        mFtpLib = nullptr;
        return false;
    }

    string sUser = TConfig::getFtpUser();
    string sPass = TConfig::getFtpPassword();
    MSG_DEBUG("Trying to login <" << sUser << ", ********>");

    if (!mFtpLib->Login(sUser.c_str(), sPass.c_str()))
    {
        delete mFtpLib;
        mFtpLib = nullptr;
        return false;
    }

    MSG_DEBUG("Trying to download file " << fname << " to " << target);

    if (!mFtpLib->Get(target.c_str(), fname.c_str(), ftplib::image))
    {
        MSG_ERROR("Error downloading file " << fname);
        delete mFtpLib;
        mFtpLib = nullptr;
        return false;
    }

    MSG_INFO("File " << fname << " successfully downloaded to " << target << ".");
    mFtpLib->Quit();
    return true;
}

bool TFsfReader::unpack(const string& fname, const string& path)
{
    DECL_TRACER("TFsfReader::unpack(const string& fname, const string& path)");

    if (fname.empty() || path.empty())
    {
        MSG_ERROR("Invalid parameters!");
        return false;
    }

    reader::ReadTP4 readtp4(fname, path);

    if (!readtp4.isReady())
        return false;

    mG5Type = readtp4.isG5();
    // We must delete the old files first
    std::uintmax_t n = fs::remove_all(path);
    MSG_TRACE("Deleted " << n << " files/directories from " << path);
    return readtp4.doRead();
}

void TFsfReader::callbackLog(char* str, void*, bool out)
{
    DECL_TRACER("TFsfReader::callbackLog(char* str, void* arg, bool out)");

    if (!str)
        return;

    string msg = str;
    size_t pos = 0;

    if ((pos = msg.find("\r")) != string::npos)
        msg = msg.substr(0, pos);

    if (!out)
    {
        MSG_DEBUG("Output: " << msg);
    }
    else
    {
        MSG_DEBUG("Input: " << msg);
    }
}

void TFsfReader::callbackError(char* msg, void*, int err)
{
    DECL_TRACER("TFsfReader::callbackError(char* msg, void* arg, int err)");

    if (!msg)
        return;

    if (err)
    {
        MSG_ERROR(msg);
    }
    else if (*msg >= '0' && *msg <= '9')
    {
        MSG_INFO(msg);
    }
    else
    {
        MSG_DEBUG(msg);
    }
}

int TFsfReader::callbackXfer(off64_t xfered, void*)
{
    DECL_TRACER("TFsfReader::callbackXfer(off64_t xfered, void*)");

    if (_progress)
        return _progress(xfered);

    return 1;
}

void TFsfReader::logging(int level, const std::string &msg)
{
    switch(level)
    {
        case LOG_INFO:      MSG_INFO(msg); break;
        case LOG_WARNING:   MSG_WARNING(msg); break;
        case LOG_ERROR:     MSG_ERROR(msg); break;
        case LOG_TRACE:     MSG_TRACE(msg); break;
        case LOG_DEBUG:     MSG_DEBUG(msg); break;
    }
}

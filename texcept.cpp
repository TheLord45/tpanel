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

#include "texcept.h"
#include "terror.h"

#include <sstream>

using namespace std;

TXceptBase::TXceptBase(const string& message, const string& categorie, const string& file, const int& iLine, bool bFatal)
        : mMessage(message),
          mCategory(categorie),
          mFile(file),
          mLine(iLine),
          mbFatal(bFatal)
{
}

TXceptBase::~TXceptBase()
{
}

void TXceptBase::logIt (void)
{
    string sFehlertyp = "*** an EXCEPTION occured ***";
    string sAbbruch;

    if (mbFatal)
    {
        sFehlertyp = "*** a FATAL EXCEPTION occured ***";
        sAbbruch = "FATAL ERROR, PROGRAM ABORT!";
    }

    time_t t = time(NULL);
    struct tm *tmp;
    tmp = localtime(&t);
    char timeStr[200];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tmp);

    stringstream str;
    str <<  sFehlertyp                      << std::endl
    << "msg       :     " << getMessage()   << std::endl
    << "category  :     " << getCategorie() << std::endl
    << "time      :     " << timeStr        << std::endl
    << "file      :     " << getFile()      << std::endl
    << "line      :     " << getLine()      << std::endl
    << sAbbruch                             << std::endl;

    mMessage = str.str();
    MSG_ERROR(mMessage);
}

TXceptNetwork::TXceptNetwork (const std::string& message, const std::string& file, int iLine, bool bFatal)
    : TXceptBase(message, "Network error", file, iLine, bFatal)
{
    logIt();
}

TXceptNetwork::TXceptNetwork (const std::string &  file, int iLine, bool bFatal)
: TXceptBase("", "Network error", file, iLine, bFatal)
{
    logIt();
}

TXceptComm::TXceptComm (const std::string& message, const std::string& file, int iLine, bool bFatal)
: TXceptBase(message, "Controller communication error", file, iLine, bFatal)
{
    logIt();
}

TXceptComm::TXceptComm (const std::string &  file, int iLine, bool bFatal)
: TXceptBase("", "Controller communication error", file, iLine, bFatal)
{
    logIt();
}

TXceptFatal::TXceptFatal (const std::string& message, const std::string& file, int iLine, bool bFatal)
: TXceptBase(message, "Fatal error", file, iLine, bFatal)
{
    logIt();
    exit(1);
}

TXceptFatal::TXceptFatal (const std::string &  file, int iLine, bool bFatal)
: TXceptBase("", "Fatal error", file, iLine, bFatal)
{
    logIt();
    exit(1);
}

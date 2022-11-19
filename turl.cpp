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

#include <algorithm>
#include <cctype>
#include <functional>

#include "turl.h"
#include "terror.h"

using std::string;
using std::transform;
using std::distance;

TUrl::TUrl()
{
    DECL_TRACER("TUrl::TUrl()");
}

TUrl::TUrl(const string &url)
{
    DECL_TRACER("TUrl::TUrl(const string &url)");

    if (!url.empty())
    {
        mUrl = url;
        parse();
    }
}

TUrl::~TUrl()
{
    DECL_TRACER("TUrl::~TUrl()");
}

bool TUrl::setUrl(const string &url)
{
    DECL_TRACER("TUrl::setUrl(const string &url)");

    mUrl = url;
    return parse();
}

bool TUrl::parse()
{
    DECL_TRACER("TUrl::parse()");

    string x, port;
    size_t offset = 0;
    size_t pos1, pos2, pos3, pos4, pos5;
    x = _trim(mUrl);

    offset = mUrl.find_first_of("://");

    if (offset == string::npos)
        return false;

    offset += 3;

    pos1 = x.find_first_of('/', offset + 1);
    mPath = pos1 == string::npos ? "" : x.substr(pos1);
    mDomain = string(x.begin() + offset, pos1 != string::npos ? x.begin() + pos1 : x.end());

    if ((pos5 = mDomain.find_first_of('@')) != string::npos)
    {
        mUser = mDomain.substr(0, pos5);

        if ((pos5 = mUser.find(':')) != string::npos)
        {
            mPassword = mUser.substr(pos5 + 1);
            mUser = mUser.substr(0, pos5);
        }
    }

    mPath = (pos2 = mPath.find("#")) != string::npos ? mPath.substr(0, pos2) : mPath;
    port = (pos3 = mDomain.find(":"))!=string::npos ? mDomain.substr(pos3+1) : "";
    mDomain = mDomain.substr(0, pos3!=string::npos ? pos3 : mDomain.length());
    mProtocol = offset > 0 ? x.substr(0,offset-3) : "";
    mQuery = (pos4 = mPath.find("?")) != string::npos ? mPath.substr(pos4+1) : "";
    mPath = pos4 != string::npos ? mPath.substr(0,pos4) : mPath;

    if (!port.empty())
        mPort = atoi(port.c_str());

    if (mPort == 0)
    {
        if (mProtocol == "https")           mPort = 443;
        else if (mProtocol == "http")       mPort = 80;
        else if (mProtocol == "ssh" ||
                 mProtocol == "sftp")       mPort = 22;
        else if (mProtocol == "ftp")        mPort = 21;
        else if (mProtocol == "mysql")      mPort = 3306;
        else if (mProtocol == "mongo")      mPort = 27017;
        else if (mProtocol == "mongo+srv")  mPort = 27017;
        else if (mProtocol == "kafka")      mPort = 9092;
        else if (mProtocol == "postgres")   mPort = 5432;
        else if (mProtocol == "postgresql") mPort = 5432;
        else if (mProtocol == "redis")      mPort = 6379;
        else if (mProtocol == "zookeeper")  mPort = 2181;
        else if (mProtocol == "ldap")       mPort = 389;
        else if (mProtocol == "ldaps")      mPort = 636;
    }

    return true;
}

string TUrl::_trim(const string& str)
{
    DECL_TRACER("TUrl::_trim(const string& str)");

    size_t start = str.find_first_not_of(" \n\r\t");
    size_t until = str.find_last_not_of(" \n\r\t");
    string::const_iterator i = start==string::npos ? str.begin() : str.begin() + start;
    string::const_iterator x = until==string::npos ? str.end()   : str.begin() + until+1;
    return string(i,x);
}

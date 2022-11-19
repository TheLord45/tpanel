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

#ifndef __TURL_H__
#define __TURL_H__

#include <string>

class TUrl
{
    public:
        TUrl();
        TUrl(const std::string& url);
        ~TUrl();

        bool setUrl(const std::string& url);
        std::string& getUrl() { return mUrl; }

        std::string getProtocol() { return mProtocol; } // The protocol of the URL (https, sftp, ...)
        std::string getUser() { return mUser; }         // The user name, if any
        std::string getPassword() { return mPassword; } // The password, if any --> should not be set in a URL for security reasons!
        std::string getDomain() { return mDomain; }     // The host name; either a FQDN name or an IP address.
        int getPort() { return mPort; }                 // The port number; If no port number was set it depends on the protocol.
        std::string getPath() { return mPath; }         // The path of the URL.
        std::string getQuery() { return mQuery; }       // The query part appended to the path, if any

    protected:
        bool parse();
        std::string _trim(const std::string& str);

    private:
        std::string mUrl;
        std::string mProtocol;
        std::string mDomain;
        int mPort{0};
        std::string mUser;
        std::string mPassword;
        std::string mPath;
        std::string mQuery;
};

#endif

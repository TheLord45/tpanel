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

#ifndef __TPRJRESOURCES_H__
#define __TPRJRESOURCES_H__

#include <string>
#include <vector>

#include "thttpclient.h"

typedef struct RESOURCE_T
{
    std::string name;           // Name of resource
    std::string protocol;       // With TP4 this is either HTTP of FTP
    std::string user;           // Optional: User name
    std::string password;       // Optional: Password (usualy encrypted)
    bool encrypted{false};      // If a password exist and this is TRUE the password is encrypted
    std::string host;           // The name of the the machine where the source is located (<host>:<port>)
    std::string path;           // Optional: The path of the URL
    std::string file;           // The file or path and file of the URL
    int refresh{0};             // if > 0 then it defines the time in seconds to reread the source
    bool dynamo{false};         // If this is TRUE the source is a MJPEG
    bool preserve{false};       // If this is TRUE the source is read only once at startup.

    void clear()
    {
        name.clear();
        protocol.clear();
        user.clear();
        password.clear();
        encrypted = false;
        host.clear();
        path.clear();
        file.clear();
        refresh = 0;
        dynamo = false;
        preserve = false;
    }
}RESOURCE_T;

typedef struct RESOURCE_LIST_T
{
    std::string type;
    std::vector<RESOURCE_T> ressource;
}RESOURCE_LIST_T;

class TPrjResources
{
    public:
        TPrjResources(std::vector<RESOURCE_LIST_T>& list);
        ~TPrjResources();

        void setResourcesList(std::vector<RESOURCE_LIST_T>& list) { mResources = list; }
        RESOURCE_LIST_T findResourceType(const std::string& type);
        RESOURCE_T findResource(int idx, const std::string& name);
        RESOURCE_T findResource(const std::string& type, const std::string& name);
        RESOURCE_T findResource(const std::string& name);
        size_t getResourceIndex(const std::string& type);
        bool setResource(const std::string& name, const std::string& scheme, const std::string& host, const std::string& path, const std::string& file, const std::string& user, const std::string& pw, int refresh=-1);
        bool addResource(const std::string& name, const std::string& scheme, const std::string& host, const std::string& path, const std::string& file, const std::string& user, const std::string& pw, int refresh=-1);
        std::string decryptPassword(const std::string& pw);
        static const size_t npos = static_cast<size_t>(-1);

    private:
        std::vector<RESOURCE_LIST_T> mResources;
};

#endif

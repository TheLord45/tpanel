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

#include "tprjresources.h"
#include "terror.h"

using std::string;
using std::vector;

TPrjResources::TPrjResources(vector<RESOURCE_LIST_T>& list)
        : mResources(list)
{
    DECL_TRACER("TPrjResources::TPrjResources(vector<RESOURCE_LIST_T>& list)");
}

TPrjResources::~TPrjResources()
{
    DECL_TRACER("TPrjResources::~TPrjResources()");
}

RESOURCE_LIST_T TPrjResources::findResourceType(const std::string& type)
{
    DECL_TRACER("TPrjResources::findResourceType(const std::string& type)");

    if (mResources.size() == 0)
        return RESOURCE_LIST_T();

    vector<RESOURCE_LIST_T>::iterator iter;

    for (iter = mResources.begin(); iter != mResources.end(); iter++)
    {
        if (iter->type.compare(type) == 0)
            return *iter;
    }

    return RESOURCE_LIST_T();
}

RESOURCE_T TPrjResources::findResource(int idx, const std::string& name)
{
    DECL_TRACER("TPrjResources::findResource(int idx, const std::string& name)");

    if (idx < 1 || mResources.size() < (size_t)idx)
    {
        MSG_ERROR("Invalid index " << idx << "!");
        return RESOURCE_T();
    }

    RESOURCE_LIST_T list = mResources[idx-1];

    if (list.type.empty())
    {
        MSG_ERROR("Resource list " << idx << " is empty!");
        return RESOURCE_T();
    }

    vector<RESOURCE_T>::iterator iter;

    for (iter = list.ressource.begin(); iter != list.ressource.end(); iter++)
    {
        MSG_DEBUG("Resource: " << iter->name);
        if (iter->name.compare(name) == 0)
            return *iter;
    }

    MSG_WARNING("Resource " << name << " not found!");
    return RESOURCE_T();
}

RESOURCE_T TPrjResources::findResource(const std::string& type, const std::string& name)
{
    DECL_TRACER("TPrjResources::findResource(const std::string& type, const std::string& name)");

    RESOURCE_LIST_T list = findResourceType(type);

    if (list.type.empty())
        return RESOURCE_T();

    vector<RESOURCE_T>::iterator iter;

    for (iter = list.ressource.begin(); iter != list.ressource.end(); iter++)
    {
        if (iter->name.compare(name) == 0)
            return *iter;
    }

    return RESOURCE_T();
}

RESOURCE_T TPrjResources::findResource(const std::string& name)
{
    DECL_TRACER("TPrjResources::findResource(const std::string& name)");

    if (mResources.size() == 0 || mResources[0].ressource.size() == 0)
        return RESOURCE_T();

    vector<RESOURCE_T> list = mResources[0].ressource;
    vector<RESOURCE_T>::iterator iter;

    for (iter = list.begin(); iter != list.end(); iter++)
    {
        if (iter->name.compare(name) == 0)
            return *iter;
    }

    return RESOURCE_T();
}

size_t TPrjResources::getResourceIndex(const string& type)
{
    DECL_TRACER("TPrjResources::getResourceIndex(const string& type)");

    if (mResources.size() == 0)
        return npos;

    vector<RESOURCE_LIST_T>::iterator iter;
    size_t idx = 0;

    for (iter = mResources.begin(); iter != mResources.end(); iter++)
    {
        idx++;

        if (iter->type.compare(type) == 0)
            return idx;
    }

    return npos;
}

bool TPrjResources::setResource(const string& name, const string& scheme, const string& host, const string& path, const string& file, const string& user, const string& pw, int refresh)
{
    DECL_TRACER("TPrjResources::setResource(const string& name, const string& scheme, const string& host, const string& path, const string& file, const string& user, const string& pw, int refresh)");

    if (mResources.size() == 0)
        return false;

    vector<RESOURCE_LIST_T>::iterator itRlist;

    for (itRlist = mResources.begin(); itRlist != mResources.end(); ++itRlist)
    {
        if (itRlist->type.compare("image") == 0)
            break;
    }

    if (itRlist == mResources.end())
    {
        MSG_ERROR("There was no resource type \"image\" found in the resources!");
        return false;
    }

    vector<RESOURCE_T> list = itRlist->ressource;
    vector<RESOURCE_T>::iterator iter;

    for (iter = list.begin(); iter != list.end(); iter++)
    {
        if (iter->name.compare(name) == 0)
        {
            RESOURCE_T res = *iter;

            if (!scheme.empty())
                res.protocol = scheme;

            if (!host.empty())
                res.host = host;

            if (!path.empty())
                res.path = path;

            if (!file.empty())
                res.file = file;

            if (!user.empty())
                res.user = user;

            if (!pw.empty())
                res.password = pw;

            if (refresh >= 0)
                res.refresh = refresh;

            list.erase(iter);
            list.push_back(res);

            itRlist->ressource.clear();
            itRlist->ressource = list;
            return true;
        }
    }

    return false;
}

bool TPrjResources::addResource(const string& name, const string& scheme, const string& host, const string& path, const string& file, const string& user, const string& pw, int refresh)
{
    DECL_TRACER("TPrjResources::addResource(const string& name, const string& scheme, const string& host, const string& path, const string& file, const string& user, const string& pw, int refresh)");

    vector<RESOURCE_LIST_T>::iterator itRlist;

    if (mResources.size() == 0)
    {
        RESOURCE_LIST_T rl;
        rl.type = "image";
        mResources.push_back(rl);
        itRlist = mResources.begin();
    }
    else
    {
        // Find the resource container
        for (itRlist = mResources.begin(); itRlist != mResources.end(); ++itRlist)
        {
            if (itRlist->type.compare("image") == 0)
                break;
        }
    }

    if (itRlist == mResources.end())
    {
        MSG_ERROR("There is no resouce container called \"image\"!");
        return false;
    }

    RESOURCE_T r;
    r.name = name;
    r.protocol = scheme;
    r.host = host;
    r.path = path;
    r.file = file;
    r.user = user;
    r.password = pw;
    r.refresh = refresh;

    // Make sure the resource does not already exist
    if (itRlist->ressource.size() == 0)
    {
        itRlist->ressource.push_back(r);
        return true;
    }

    vector<RESOURCE_T>::iterator iter;

    for (iter = itRlist->ressource.begin(); iter != itRlist->ressource.end(); iter++)
    {
        if (iter->name == name)
        {
            iter->protocol = scheme;
            iter->host = host;
            iter->path = path;
            iter->file = file;
            iter->user = user;
            iter->password = pw;
            iter->refresh = refresh;
            return true;
        }
    }

    itRlist->ressource.push_back(r);
    return true;
}

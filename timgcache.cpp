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

#include <mutex>

#include "timgcache.h"
#include "terror.h"

using std::string;
using std::vector;
using std::mutex;

vector<_IMGCACHE> TImgCache::mImgCache;

mutex _imgCache;

TImgCache::~TImgCache()
{
    DECL_TRACER("TImgCache::~TImgCache()");

    mImgCache.clear();
}

bool TImgCache::addImage(const string& name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::addImage(const string& name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)");

    _imgCache.lock();
    _IMGCACHE ic;

    ic.name = name;
    ic.bmType = bmType;
    ic.bitmap = bm;

    if (mImgCache.size() == 0)
    {
        mImgCache.push_back(ic);
        MSG_DEBUG("Bitmap \"" << name << "\" was freshly added.");
        _imgCache.unlock();
        return true;
    }

    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->name == name)
        {
            MSG_DEBUG("Bitmap \"" << name << "\" already in cache.");
            _imgCache.unlock();
            return true;
        }
    }

    // Here we know that the image is not yet in the cache. So we add it now.
    mImgCache.push_back(ic);
    MSG_DEBUG("Bitmap \"" << name << "\" was added.");
    _imgCache.unlock();
    return true;
}

bool TImgCache::getBitmap(const string& name, SkBitmap *bm, _IMGCACHE_BMTYPE bmType, int *width, int *height)
{
    DECL_TRACER("TImgCache::getBitmap(const string& name, _IMGCACHE_BMTYPE bmType, int *width, int *height)");

    if (mImgCache.size() == 0 || !bm)
        return false;

    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->name == name && iter->bmType == bmType)
        {
            *bm = iter->bitmap;

            if (width && !iter->bitmap.empty())
                *width = iter->bitmap.info().width();

            if (height && !iter->bitmap.empty())
                *height = iter->bitmap.info().height();

            MSG_DEBUG("Bitmap \"" << iter->name << "\" was found.");
            return true;
        }
    }

    if (width)
        *width = 0;

    if (height)
        *height = 0;

    return false;
}

bool TImgCache::delBitmap(const string& name, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::delBitmap(const string& name, _IMGCACHE_BMTYPE bmType)");

    if (name.empty() || mImgCache.size() == 0)
        return false;

    _imgCache.lock();
    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->name == name && iter->bmType == bmType)
        {
            MSG_DEBUG("Bitmap \"" << iter->name << "\" will be erased.");
            mImgCache.erase(iter);
            _imgCache.unlock();
            return true;
        }
    }

    _imgCache.unlock();
    return false;
}

bool TImgCache::existBitmap(const string& name, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::existBitmap(const string& name, _IMGCACHE_BMTYPE bmType)");

    if (mImgCache.size() == 0)
        return false;

    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->name == name && iter->bmType == bmType)
            return true;
    }

    return false;
}

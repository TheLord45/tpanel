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
#include "tconfig.h"

using std::string;
using std::vector;
using std::mutex;

vector<_IMGCACHE> TImgCache::mImgCache;
size_t TImgCache::size{0};

mutex _imgCache;

TImgCache::~TImgCache()
{
    DECL_TRACER("TImgCache::~TImgCache()");

    mImgCache.clear();
}

bool TImgCache::addImage(const string& name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::addImage(const string& name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)");

    if (name.empty() || bm.empty())
        return false;

    _imgCache.lock();
    _IMGCACHE ic;

    ic.name = name;
    ic.bmType = bmType;
    ic.bitmap = bm;
    ic.handle = 0;

    bool ret = addImage(ic);
    _imgCache.unlock();
    return ret;
}

bool TImgCache::addImage(const string& name, SkBitmap& bm, ulong handle, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::addImage(const string& name, SkBitmap& bm, uint32_t handle, _IMGCACHE_BMTYPE bmType)");

    if (bm.empty() || handle == 0)
        return false;

    _imgCache.lock();
    _IMGCACHE ic;

    if (name.empty())
        ic.name = handleToString(handle);
    else
        ic.name = name;

    ic.bmType = bmType;
    ic.bitmap = bm;
    ic.handle = handle;

    bool ret = addImage(ic);
    _imgCache.unlock();
    return ret;
}

bool TImgCache::addImage(_IMGCACHE ic)
{
    DECL_TRACER("TImgCache::addImage(_IMGCACHE ic)");

    if (mImgCache.size() == 0)
    {
        mImgCache.push_back(ic);
        size += sizeof(_IMGCACHE);
        MSG_DEBUG("Bitmap \"" << ic.name << "\" was freshly added.");
        return true;
    }

    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->name == ic.name)
        {
            MSG_DEBUG("Bitmap \"" << ic.name << "\" already in cache.");
            return true;
        }
    }

    // Here we know that the image is not yet in the cache. So we add it now.
    mImgCache.push_back(ic);
    size += sizeof(_IMGCACHE);
    MSG_DEBUG("Bitmap \"" << ic.name << "\" was added.");

    if (size > TConfig::getButttonCache())
        shrinkCache();

    return true;
}

bool TImgCache::getBitmap(const string& name, SkBitmap *bm, _IMGCACHE_BMTYPE bmType, int *width, int *height)
{
    DECL_TRACER("TImgCache::getBitmap(const string& name, SkBitmap *bm, _IMGCACHE_BMTYPE bmType, int *width, int *height)");

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
            else if (width)
                *width = 0;

            if (height && !iter->bitmap.empty())
                *height = iter->bitmap.info().height();
            else if (height)
                *height = 0;

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

bool TImgCache::getBitmap(SkBitmap *bm, ulong handle, int *width, int *height, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::getBitmap(SkBitmap *bm, uint32_t handle, int *width, int *height, _IMGCACHE_BMTYPE bmType)");

    if (mImgCache.size() == 0 || !bm || handle == 0)
        return false;

    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            if (bmType != _BMTYPE_NONE && iter->bmType != bmType)
                return false;

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

bool TImgCache::delBitmap(ulong handle, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::delBitmap(uint32_t handle, _IMGCACHE_BMTYPE bmType)");

    if (!handle)
        return false;

    _imgCache.lock();
    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            if (bmType != _BMTYPE_NONE && iter->bmType != bmType)
                return false;

            MSG_DEBUG("Bitmap \"" << iter->name << "\" will be erased.");
            mImgCache.erase(iter);
            size -= sizeof(_IMGCACHE);
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

bool TImgCache::existBitmap(ulong handle, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::existBitmap(uint32_t handle, _IMGCACHE_BMTYPE bmType)");

    if (!handle)
        return false;

    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            if (bmType != _BMTYPE_NONE && iter->bmType != bmType)
                return false;

            return true;
        }
    }

    return false;
}

bool TImgCache::replaceBitmap(const std::string &name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::replaceBitmap(const std::string &name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)");

    if (name.empty() || bm.empty())
        return false;

    _imgCache.lock();
    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->name == name)
        {
            if (bmType != _BMTYPE_NONE && iter->bmType != bmType)
            {
                _imgCache.unlock();
                return false;
            }

            _IMGCACHE ic = *iter;
            ic.bitmap = bm;
            mImgCache.erase(iter);
            mImgCache.push_back(ic);
            _imgCache.unlock();
            return true;
        }
    }

    _imgCache.unlock();
    return false;
}

bool TImgCache::replaceBitmap(ulong handle, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)
{
    DECL_TRACER("TImgCache::replaceBitmap(ulong handle, SkBitmap& bm, _IMGCACHE_BMTYPE bmType)");

    if (handle == 0 || bm.empty())
        return false;

    _imgCache.lock();
    vector<_IMGCACHE>::iterator iter;

    for (iter = mImgCache.begin(); iter != mImgCache.end(); ++iter)
    {
        if (iter->handle == handle)
        {
            if (bmType != _BMTYPE_NONE && iter->bmType != bmType)
            {
                _imgCache.unlock();
                return false;
            }

            _IMGCACHE ic = *iter;
            ic.bitmap = bm;
            mImgCache.erase(iter);
            mImgCache.push_back(ic);
            _imgCache.unlock();
            return true;
        }
    }

    _imgCache.unlock();
    return false;
}

void TImgCache::shrinkCache()
{
    DECL_TRACER("TImgCache::shrinkCache()");

    if (mImgCache.size() == 0)
        return;

    size_t s = TConfig::getButttonCache();

    if (s > size || s <= sizeof(_IMGCACHE))
        return;

    while (size > s && mImgCache.size() > 0)
    {
        MSG_DEBUG("Erasing image " << mImgCache.begin()->name << " -- Size: " << size);
        mImgCache.erase(mImgCache.begin());
        size -= sizeof(_IMGCACHE);
    }
}

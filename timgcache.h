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

#ifndef __TIMGCACHE_H__
#define __TIMGCACHE_H__

#include <string>
#include <vector>

#include <include/core/SkBitmap.h>

typedef enum _IMGCACHE_BMTYPE
{
    _BMTYPE_NONE,
    _BMTYPE_CHAMELEON,
    _BMTYPE_BITMAP,
    _BMTYPE_ICON,
    _BMTYPE_URL
}_IMGCACHE_BMTYPE;

typedef struct _IMGCACHE
{
    _IMGCACHE_BMTYPE bmType{_BMTYPE_NONE};
    std::string name;
    SkBitmap bitmap;
}_IMGCACHE;

class TImgCache
{
    public:
        static bool addImage(const std::string& name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool getBitmap(const std::string& name, SkBitmap *bm, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool delBitmap(const std::string& name, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool existBitmap(const std::string& name, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);

    private:
        // This should never be used
        TImgCache() {}
        ~TImgCache();

        // Internal variables
        static std::vector<_IMGCACHE> mImgCache;
        SkBitmap mDummyBM;
};

#endif

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

#ifdef __ANDROID__
typedef unsigned long ulong;
#endif

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
    uint32_t handle{0};
}_IMGCACHE;

class TImgCache
{
    public:
        static bool addImage(const std::string& name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool addImage(const std::string& name, SkBitmap& bm, ulong handle=0, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool getBitmap(const std::string& name, SkBitmap *bm, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE, int *width=nullptr, int *height=nullptr);
        static bool getBitmap(SkBitmap *bm, ulong handle, int *width=nullptr, int *height=nullptr, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool delBitmap(const std::string& name, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool delBitmap(ulong handle, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool existBitmap(const std::string& name, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool existBitmap(ulong handle, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool replaceBitmap(const std::string& name, SkBitmap& bm, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);
        static bool replaceBitmap(ulong handle, SkBitmap& bm, _IMGCACHE_BMTYPE bmType=_BMTYPE_NONE);

    protected:
        static bool addImage(_IMGCACHE ic);

    private:
        static void shrinkCache();
        // This should never be used
        TImgCache() {}
        ~TImgCache();

        static std::string handleToString(ulong handle)
        {
            ulong part1 = (handle >> 16) & 0x0000ffff;
            ulong part2 = handle & 0x0000ffff;
            return std::to_string(part1)+":"+std::to_string(part2);
        }

        // Internal variables
        static std::vector<_IMGCACHE> mImgCache;
        static size_t size;
        SkBitmap mDummyBM;
};

#endif

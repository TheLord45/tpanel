/*
 * Copyright (C) 2026 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef TTILESCACHE_H
#define TTILESCACHE_H

#include <functional>
#include <mutex>

#include <core/SkImage.h>

enum MapSource
{
    GOOGLE,
    OSM
};

struct TileKey
{
    int x, y, z;
    MapSource source;

    bool operator==(const TileKey& o) const
    {
        return x == o.x && y == o.y && z == o.z && source == o.source;
    }
};

namespace std
{
    template <>
    struct hash<TileKey>
    {
        size_t operator()(const TileKey& k) const
        {
            return ((std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1)) >> 1) ^ (std::hash<int>()(k.z) << 1) ^ (std::hash<int>()(k.source) << 2);
        }
    };
}

class TTileCache
{
    std::unordered_map<TileKey, sk_sp<SkImage>> cache;
    std::mutex mtx;

    public:
        TTileCache();
        TTileCache(const std::string& path);
        ~TTileCache();

        sk_sp<SkImage> getTile(int x, int y, int z, MapSource source);
        void setTilePath(const std::string& path);
        void clearCache() { cache.clear(); }

    protected:
        sk_sp<SkImage> getTileFromFile(int x, int y, int z, MapSource source);
        bool putTileToFile(int x, int y, int z, MapSource source, const sk_sp<SkImage>& img, bool overwrite=true);
        sk_sp<SkImage> makeImage(const char *content, size_t size);

    private:
        std::string mTileCache;
};

#endif // TTILESCACHE_H

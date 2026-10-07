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
#ifndef TGRMAP_H
#define TGRMAP_H

#include <functional>
#include <mutex>
#include <unordered_map>
#include <cmath>

#include <core/SkImage.h>
#include <core/SkSurface.h>
#include <core/SkPaint.h>

namespace GMap
{
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

    class TileCache
    {
        ::std::unordered_map<TileKey, sk_sp<SkImage>> cache;
        ::std::mutex mtx;

        public:
            sk_sp<SkImage> getTile(int x, int y, int z, MapSource source);
    };

    class TGrMap
    {
        public:
            TGrMap();

            void createMap();
            void setLatitute(double lat) { mMarkerLat = lat; }
            void setLongitude(double lon) { mMarkerLon = lon; }
            void setZoom(int z) { zoom = z; }
            void setSource(MapSource src) { mMapSource = src; }


        protected:
            // Converts lat/lon to tile x,y at zoom z
            void latLonToTileXY(double lat, double lon, int zoom, int& x, int& y);
            // Converts lat/lon to pixel coordinates at zoom level
            void latLonToPixelXY(double lat, double lon, int zoom, double& px, double& py);
            // Converts pixel coordinates to lat/lon at zoom level
            void pixelXYToLatLon(double px, double py, int zoom, double& lat, double& lon);

            void centerOnMarker();
            void draw();

        private:
            int TILE_SIZE{256};
            int WINDOW_WIDTH{800};
            int WINDOW_HEIGHT{600};

            sk_sp<SkSurface> mSurface;
            TileCache mTileCache;
            bool changed = true;

            double mOffsetX{0}; // pixel offset for panning
            double mOffsetY{0};
            int zoom = 3;

            double mMarkerLat{0};
            double mMarkerLon{0};

            MapSource mMapSource = GOOGLE;
            SkPaint mPaint;


    };
}

#endif // TGRMAP_H

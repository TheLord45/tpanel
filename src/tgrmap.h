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

#include <core/SkPaint.h>

#include "ttilescache.h"

class SkImage;
class SkBitmap;
class SkSurface;

class TGrMap
{
    public:
        TGrMap();
        TGrMap(const std::string& tilecache);
        ~TGrMap();

        void createMap(SkBitmap& bmp);
        void setLatitute(double lat) { mMarkerLat = lat; }
        void setLongitude(double lon) { mMarkerLon = lon; }
        void setZoom(int z) { mZoom = z; }
        void setSource(MapSource src) { mMapSource = src; }
        void setTileCachePath(const std::string& path);
        void setWindowSize(int width, int height) { WINDOW_WIDTH = width; WINDOW_HEIGHT = height; }


    protected:
        // Converts lat/lon to tile x,y at zoom z
        void latLonToTileXY(double lat, double lon, int zoom, int& x, int& y);
        // Converts lat/lon to pixel coordinates at zoom level
        void latLonToPixelXY(double lat, double lon, int zoom, double& px, double& py);
        // Converts pixel coordinates to lat/lon at zoom level
        void pixelXYToLatLon(double px, double py, int zoom, double& lat, double& lon);

        void centerOnMarker();
        SkBitmap draw();

    private:
        int TILE_SIZE{256};
        int WINDOW_WIDTH{800};
        int WINDOW_HEIGHT{600};

        sk_sp<SkSurface> mSurface;
        TTileCache *mTileCache{nullptr};

        double mOffsetX{0}; // pixel offset for panning
        double mOffsetY{0};
        int mZoom{10};

        double mMarkerLat{0};
        double mMarkerLon{0};

        MapSource mMapSource{GOOGLE};
        SkPaint mPaint;
};

#endif // TGRMAP_H

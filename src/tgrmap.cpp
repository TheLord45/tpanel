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
#include <string>
#include <vector>

#include <core/SkBitmap.h>
#include <core/SkCanvas.h>
#include <core/SkSurface.h>
#include <codec/SkCodec.h>

#include "tgrmap.h"
#include "tresources.h"
#include "terror.h"

using std::string;
using std::to_string;
using std::vector;
using std::mutex;
using std::lock_guard;
using std::move;
using std::unique_ptr;
using std::unordered_map;


TGrMap::TGrMap()
{
    DECL_TRACER("TGrMap::TGrMap()");
}

TGrMap::~TGrMap()
{
    DECL_TRACER("TGrMap::~TGrMap()");
}

void TGrMap::createMap(SkBitmap& bmp)
{
    DECL_TRACER("TGrMap::createMap(const SkBitmap& bmp)");

    mSurface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(WINDOW_WIDTH, WINDOW_HEIGHT));
    // Initial marker position
    if (mMarkerLat == 0.0)
        mMarkerLat = 48.199453;

    if (mMarkerLon == 0.0)
        mMarkerLon = 16.328604;

    centerOnMarker();
    bmp = draw();
}

void TGrMap::centerOnMarker()
{
    DECL_TRACER("TGrMap::centerOnMarker()");

    double px, py;

    latLonToPixelXY(mMarkerLat, mMarkerLon, mZoom, px, py);
    mOffsetX = WINDOW_WIDTH / 2 - px;
    mOffsetY = WINDOW_HEIGHT / 2 - py;
}

SkBitmap TGrMap::draw()
{
    DECL_TRACER("TGrMap::draw()");

    SkCanvas *canvas = mSurface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    // Draw tiles
    int tilesCount = 1 << mZoom;
    int startX = int(floor(-mOffsetX / TILE_SIZE));
    int startY = int(floor(-mOffsetY / TILE_SIZE));
    int endX = int(ceil((WINDOW_WIDTH - mOffsetX) / TILE_SIZE));
    int endY = int(ceil((WINDOW_HEIGHT - mOffsetY) / TILE_SIZE));

    for (int tx = startX; tx <= endX; ++tx)
    {
        for (int ty = startY; ty <= endY; ++ty)
        {
            int x = (tx % tilesCount + tilesCount) % tilesCount;
            int y = (ty % tilesCount + tilesCount) % tilesCount;
            sk_sp<SkImage> img = mTileCache.getTile(x, y, mZoom, mMapSource);

            if (img)
            {
                SkRect dst = SkRect::MakeXYWH(mOffsetX + tx * TILE_SIZE, mOffsetY + ty * TILE_SIZE, TILE_SIZE, TILE_SIZE);
                canvas->drawImageRect(img, dst, SkSamplingOptions(SkFilterMode::kLinear));
            }
            else
            {
                // Draw placeholder
                mPaint.setColor(SK_ColorLTGRAY);
                canvas->drawRect(SkRect::MakeXYWH(mOffsetX + tx * TILE_SIZE, mOffsetY + ty * TILE_SIZE, TILE_SIZE, TILE_SIZE), mPaint);
            }
        }
    }

    // Draw marker
    double px, py;
    latLonToPixelXY(mMarkerLat, mMarkerLon, mZoom, px, py);
    float mx = (float)(mOffsetX + px);
    float my = (float)(mOffsetY + py);
    mPaint.setColor(SK_ColorGREEN);
    mPaint.setAntiAlias(true);
    canvas->drawCircle(mx, my, 10, mPaint);

    if (isBigEndian())
        mPaint.setColor(SK_ColorRED);
    else
        mPaint.setColor(SK_ColorBLUE);

    mPaint.setStrokeWidth(2);
    canvas->drawLine(mx - 5, my, mx + 5, my, mPaint);
    canvas->drawLine(mx, my - 5, mx, my + 5, mPaint);

    // The resulting map is upside down. Therefore we must turn it by 180°.
    SkPixmap pixmap;

    if (mSurface->peekPixels(&pixmap))
    {
        // int width = pixmap.width();
        int height = pixmap.height();
        int rowBytes = pixmap.rowBytes();
        const void* srcPixels = pixmap.addr();

        // Allocate buffer for flipped pixels
        vector<uint8_t> flippedPixels(rowBytes * height);

        for (int y = 0; y < height; ++y)
        {
            // Copy row y from bottom to top
            const uint8_t* srcRow = static_cast<const uint8_t*>(srcPixels) + (height - 1 - y) * rowBytes;
            uint8_t* dstRow = flippedPixels.data() + y * rowBytes;
            memcpy(dstRow, srcRow, rowBytes);
        }

        SkBitmap bmp;

        if (!bmp.tryAllocPixels(pixmap.info()))
        {
            MSG_ERROR("Error allocating pixels!");
            return SkBitmap();
        }

        bmp.setPixels(flippedPixels.data());
        return bmp;
    }

    return SkBitmap();
}

// Converts lat/lon to tile x,y at zoom z
void TGrMap::latLonToTileXY(double lat, double lon, int zoom, int& x, int& y)
{
    double latRad = lat * M_PI / 180.0;
    int n = 1 << zoom;
    x = int((lon + 180.0) / 360.0 * n);
    y = int((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n);
}

// Converts lat/lon to pixel coordinates at zoom level
void TGrMap::latLonToPixelXY(double lat, double lon, int zoom, double& px, double& py)
{
    double latRad = lat * M_PI / 180.0;
    int n = 1 << zoom;
    px = ((lon + 180.0) / 360.0 * n) * TILE_SIZE;
    py = ((1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / M_PI) / 2.0 * n) * TILE_SIZE;
}

// Converts pixel coordinates to lat/lon at zoom level
void TGrMap::pixelXYToLatLon(double px, double py, int zoom, double& lat, double& lon)
{
    int n = 1 << zoom;
    lon = px / (TILE_SIZE * n) * 360.0 - 180.0;
    double y = 0.5 - (py / (TILE_SIZE * n));
    lat = 90.0 - 360.0 * atan(exp(-y * 2 * M_PI)) / M_PI;
}

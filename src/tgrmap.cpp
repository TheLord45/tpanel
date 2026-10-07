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
#include <codec/SkCodec.h>

#include "tgrmap.h"

using namespace GMap;
using ::std::string;
using ::std::to_string;
using ::std::vector;
using ::std::mutex;
using ::std::lock_guard;
using ::std::move;
using ::std::unique_ptr;

sk_sp<SkImage> TileCache::getTile(int x, int y, int z, MapSource source)
{
    TileKey key { x, y, z, source };
    {
        lock_guard<mutex> lock(mtx);
        auto it = cache.find(key);

        if (it != cache.end())
            return it->second;
    }
    // Download tile
    string url;
    //        struct curl_slist* list = NULL;

    if (source == GOOGLE)
        url = "https://mt0.google.com/vt/lyrs=m&x=" + to_string(x) + "&y=" + to_string(y) + "&z=" + to_string(z);
    else
        url = "https://tile.openstreetmap.org/" + to_string(z) + "/" + to_string(x) + "/" + to_string(y) + ".png";

    CURL* curl = curl_easy_init();

    if (!curl)
        return nullptr;

    vector<unsigned char> buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
/*
    if (source == OSM) {
        list = curl_slist_append(list, "Referrer-Policy: origin");
        list = curl_slist_append(list, "Referer: https://www.theosys.at");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    }
*/
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || buffer.empty())
        return nullptr;

    auto data = SkData::MakeWithoutCopy(buffer.data(), buffer.size());
    unique_ptr<SkCodec> codec = SkCodec::MakeFromData(move(data));

    if (!codec)
        return nullptr;

    SkImageInfo info = codec->getInfo();
    SkBitmap dst;

    if (dst.tryAllocPixels(info)) {
        if (codec->getPixels(info, dst.getPixels(), dst.rowBytes()) != SkCodec::kSuccess)
            return nullptr;
    }

    auto img = dst.asImage();

    if (!img)
        return nullptr;

    {
        lock_guard<mutex> lock(mtx);
        cache[key] = img;
    }
    return img;
}

TGrMap::TGrMap()
{
}

void TGrMap::createMap()
{
    mSurface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(WINDOW_WIDTH, WINDOW_HEIGHT));
    // Initial marker position
    if (mMarkerLat == 0.0)
        mMarkerLat = 48.199453;

    if (mMarkerLon == 0.0)
        mMarkerLon = 16.328604;

    centerOnMarker();
    draw();
}

void TGrMap::centerOnMarker()
{
    double px, py;

    latLonToPixelXY(mMarkerLat, mMarkerLon, zoom, px, py);
    mOffsetX = WINDOW_WIDTH / 2 - px;
    mOffsetY = WINDOW_HEIGHT / 2 - py;
}

void TGrMap::draw()
{
    auto canvas = mSurface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    // Draw tiles
    int tilesCount = 1 << zoom;
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
            auto img = mTileCache.getTile(x, y, zoom, mMapSource);

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
    latLonToPixelXY(mMarkerLat, mMarkerLon, zoom, px, py);
    float mx = (float)(mOffsetX + px);
    float my = (float)(mOffsetY + py);
    mPaint.setColor(SK_ColorGREEN);
    mPaint.setAntiAlias(true);
    canvas->drawCircle(mx, my, 10, mPaint);
    mPaint.setColor(SK_ColorBLUE);
    mPaint.setStrokeWidth(2);
    canvas->drawLine(mx - 5, my, mx + 5, my, mPaint);
    canvas->drawLine(mx, my - 5, mx, my + 5, mPaint);

    SkPixmap pixmap;

    if (mSurface->peekPixels(&pixmap))
    {
        int width = pixmap.width();
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

    }
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

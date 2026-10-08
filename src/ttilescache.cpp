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
#include <fstream>
#include <chrono>

#include <core/SkBitmap.h>
#include <core/SkCanvas.h>
#include <core/SkSurface.h>
#include <codec/SkCodec.h>

#include "ttilescache.h"
#include "thttpclient.h"
#include "tconfig.h"
#include "terror.h"

#if __cplusplus < 201402L
#   error "This module requires at least C++14 standard!"
#else
#   if __cplusplus < 201703L
#       include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#       warning "Support for C++14 and experimental filesystem will be removed in a future version!"
#   else
#       include <filesystem>
#       ifdef __ANDROID__
namespace fs = std::__fs::filesystem;
#       else
namespace fs = std::filesystem;
#       endif
#   endif
#endif

using std::string;
using std::to_string;
using std::vector;
using std::mutex;
using std::lock_guard;
using std::unique_ptr;
using std::unordered_map;
using std::ifstream;
using std::ofstream;

TTileCache::TTileCache()
{
    DECL_TRACER("TTileCache::TTileCache()");

    string cfgPath = TConfig::getConfigPath() + "/.tiles";

    try
    {
        if (!fs::exists(cfgPath))
        {
            if (fs::create_directories(cfgPath))
                mTileCache = cfgPath;
        }
        else if (fs::exists(cfgPath) && fs::is_directory(cfgPath))
            mTileCache = cfgPath;
        else
        {
            char *home = getenv("HOME");

            if (home)
            {
                cfgPath = home;
                cfgPath.append("/.tiles");

                if (!fs::exists(cfgPath))
                    fs::create_directories(cfgPath);
                else if (!fs::is_directory(cfgPath))
                {
                    MSG_WARNING("Unable to find/create a path for saving map tiles! Will keep them in memory only.");
                    return;
                }

                mTileCache = cfgPath;
            }
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("A filesystem opration failed: " << e.what());
    }
}

TTileCache::TTileCache(const string& path)
{
    DECL_TRACER("TTileCache::TTileCache(const string& path)");

    setTilePath(path);
}

TTileCache::~TTileCache()
{
    DECL_TRACER("TTileCache::~TTileCache()");
}

sk_sp<SkImage> TTileCache::getTile(int x, int y, int z, MapSource source)
{
    DECL_TRACER("TileCache::getTile(int x, int y, int z, MapSource source)");

    TileKey key { x, y, z, source };
    {
        lock_guard<mutex> lock(mtx);
        unordered_map<TileKey, sk_sp<SkImage>>::iterator it = cache.find(key);

        if (it != cache.end())
            return it->second;
    }

    // Is the tile already on disk?
    sk_sp<SkImage> fimg;

    if ((fimg = getTileFromFile(x, y, z, source)) != nullptr)
        return fimg;

    // Download tile
    string url;

    if (source == GOOGLE)
        url = "https://mt0.google.com/vt/lyrs=m&x=" + to_string(x) + "&y=" + to_string(y) + "&z=" + to_string(z);
    else
        url = "https://tile.openstreetmap.org/" + to_string(z) + "/" + to_string(x) + "/" + to_string(y) + ".png";

    THTTPClient WEBClient;

    try
    {
        char *content = nullptr;
        size_t length = 0;
        size_t contentlen = 0;

        if ((content = WEBClient.tcall(&length, url, "", "")) == nullptr)
            return nullptr;

        contentlen = WEBClient.getContentSize();

        if (!content)
        {
            MSG_ERROR("Server returned no or invalid content!");
            return nullptr;
        }

        sk_sp<SkImage> img = makeImage(content, contentlen);

        if (!img)
            return nullptr;

        {
            lock_guard<mutex> lock(mtx);
            // Put the tile into the memory cache
            cache[key] = img;
            // Write the tile to the disk
            putTileToFile(x, y, z, source, img);
        }

        return img;
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error loading a map tile: " << e.what());
    }
    catch(...)
    {
        MSG_ERROR("Unexpected exception occured. [TileCache::getTile()]");
    }

    return nullptr;
}

sk_sp<SkImage> TTileCache::makeImage(const char *content, size_t size)
{
    DECL_TRACER("TTileCache::makeImage(const char *content, size_t size)");

    if (!content)
        return nullptr;

    sk_sp<SkData> data = SkData::MakeWithCopy(content, size);
    unique_ptr<SkCodec> codec = SkCodec::MakeFromData(data);

    if (!codec)
        return nullptr;

    SkImageInfo info = codec->getInfo();
    SkBitmap dst;

    if (dst.tryAllocPixels(info))
    {
        if (codec->getPixels(info, dst.getPixels(), dst.rowBytes()) != SkCodec::kSuccess)
            return nullptr;
    }

    sk_sp<SkImage> img = dst.asImage();
    return img;
}

void TTileCache::setTilePath(const string& path)
{
    DECL_TRACER("TTileCache::setTilePath(const string& path)");

    if (path.empty())
        return;

    try
    {
        if (fs::exists(path) && fs::is_directory(path) && path != mTileCache)
            mTileCache = path;
        else if (!fs::exists(path))
        {
            fs::create_directories(path);
            mTileCache = path;
        }
    }
    catch(std::exception& e)
    {
        MSG_ERROR("A filesystem operation failed: " << e.what());
    }
}

sk_sp<SkImage> TTileCache::getTileFromFile(int x, int y, int z, MapSource source)
{
    DECL_TRACER("TTileCache::getTileFromFile(int x, int y, int z, MapSource source)");

    if (mTileCache.empty() || !fs::exists(mTileCache))
        return nullptr;

    string fname = mTileCache + "/" + to_string(source) + "/" + to_string(x) + "." + to_string(y) + "." + to_string(z) + ".png";

    if (!fs::exists(fname))
        return nullptr;

    // Here we check whether the file is older then 7 days. If so, we delete the
    // file and return a nullptr.
    std::chrono::time_point ctt(fs::last_write_time(fname));

    if ((ctt + std::chrono::seconds(86400 * 7)) < fs::file_time_type::clock::now())
    {
        fs::remove(fname);
        return nullptr;
    }

    ifstream istr;
    char *content = nullptr;
    sk_sp<SkImage> img;

    try
    {
        size_t size = fs::file_size(fname);
        istr.open(fname);
        content = new char[size];
        istr.read(content, size);
        istr.close();
        img = makeImage(content, size);
        delete[] content;
    }
    catch(std::exception& e)
    {
        MSG_ERROR("Error reading the map tile " << fname << ": " << e.what());

        if (istr.is_open())
            istr.close();

        if (content)
            delete[] content;

        return nullptr;
    }

    return img;
}

bool TTileCache::putTileToFile(int x, int y, int z, MapSource source, const sk_sp<SkImage>& img, bool overwrite)
{
    DECL_TRACER("TTileCache::putTileToFile(int x, int y, int z, MapSource source, const sk_sp<SkImage>& img)");

    string fname = mTileCache + "/" + to_string(source) + "/" + to_string(x) + "." + to_string(y) + "." + to_string(z) + ".png";

    // Here we check whether the file is older then 7 days. If so, we overwrite
    // the file.
    if (fs::exists(fname) && !overwrite)
    {
        std::chrono::time_point ctt(fs::last_write_time(fname));

        if ((ctt + std::chrono::seconds(86400 * 7)) < fs::file_time_type::clock::now())
            overwrite = true;

        if (!overwrite)
            return true;
    }

    ofstream ostr;

    try
    {
        ostr.open(fname, std::ios::binary);
        sk_sp<const SkData> ref = img->refEncodedData();
        ostr.write(static_cast<const char *>(ref->data()), ref->size());
        ostr.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error writing map tile " << fname << ": " << e.what());

        if (ostr.is_open())
            ostr.close();

        return false;
    }

    return true;
}

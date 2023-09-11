/*
 * Copyright (C) 2023 by Andreas Theofilu <andreas@theosys.at>
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

#include <cstring>

#include "tbitmap.h"
#include "terror.h"

TBitmap::TBitmap()
{
    DECL_TRACER("TBitmap::TBitmap()");
}

TBitmap::TBitmap(const unsigned char* data, size_t size)
{
    DECL_TRACER("TBitmap::TBitmap(unsigned char* data, size_t size)");

    if (!data || !size)
        return;

    mData = new unsigned char[size];
    memmove(mData, data, size);
    mSize = size;
}

TBitmap::TBitmap(const unsigned char* data, int width, int height, int pixsize)
{
    DECL_TRACER("TBitmap::TBitmap(const unsigned char* data, int width, int height, int pixsize)");

    if (!data || width <= 0 || height <= 0 || pixsize < 1)
        return;

    mSize = ((size_t)width * (size_t)pixsize) * (size_t)height;
    mData = new unsigned char[mSize];
    memmove(mData, data, mSize);
    mPixelSize = pixsize;
    mPixline = width * pixsize;
    mWidth = width;
    mHeight = height;
}

TBitmap::TBitmap(const TBitmap& bm)
{
    DECL_TRACER("TBitmap::TBitmap(const TBitmap& bm)");

    if (bm.mSize > 0)
    {
        mData = new unsigned char[bm.mSize];
        memmove(mData, bm.mData, bm.mSize);
        mSize = bm.mSize;
    }

    mPixline = bm.mPixline;
    mPixelSize = bm.mPixelSize;
    mWidth = bm.mWidth;
    mHeight = bm.mHeight;
}

TBitmap::~TBitmap()
{
    DECL_TRACER("TBitmap::~TBitmap()");

    if (mData)
        delete[] mData;

    mData = nullptr;
    mSize = 0;
}

void TBitmap::setPixline(int pl)
{
    DECL_TRACER("TBitmap::setPixline(int pl)");

    if (pl < 1 || pl < mPixelSize)
        return;

    int width = pl / mPixelSize;
    int height = mSize / pl;

    if ((size_t)(height * pl) > mSize)
    {
        MSG_ERROR("TBitmap::setPixline: Number of pixels exceeds the allocated size of image!");
        return;
    }

    mPixline = pl;
    mWidth = width;
    mHeight = height;
}

void TBitmap::setBitmap(const unsigned char* data, size_t size)
{
    DECL_TRACER("TBitmap::setBitmap(unsigned char* data, size_t size)");

    clear();

    if (!data || !size)
        return;

    mData = new unsigned char [size];
    memmove(mData, data, size);
    mSize = size;
}

void TBitmap::setBitmap(const unsigned char* data, int width, int height, int pixsize)
{
    DECL_TRACER("TBitmap::setBitmap(const unsigned char* data, int width, int height, int pixsize)");

    clear();

    if (!data || width <= 0 || height <= 0 || pixsize < 1)
        return;

    mSize = ((size_t)width * (size_t)pixsize) * (size_t)height;
    mData = new unsigned char[mSize];
    memmove(mData, data, mSize);
    mPixelSize = pixsize;
    mPixline = width * pixsize;
    mWidth = width;
    mHeight = height;
}

unsigned char *TBitmap::getBitmap(size_t* size)
{
    DECL_TRACER("TBitmap::getBitmap(size_t* size)");

    if (size)
        *size = mSize;

    return mData;
}

void TBitmap::setWidth(int w)
{
    DECL_TRACER("TBitmap::setWidth(int w)");

    if (w < 1)
        return;

    if ((size_t)(w * mPixelSize * mHeight) > mSize)
    {
        MSG_ERROR("New width would exceed allocated image size!");
        return;
    }

    mPixline = w * mPixelSize;
    mWidth = w;
    mHeight = mSize / mPixline;
}

void TBitmap::setHeight(int h)
{
    DECL_TRACER("TBitmap::setHeight(int h)");

    if (h < 1)
        return;

    if ((mSize / mPixline) < (size_t)h)
    {
        MSG_ERROR("New height would exceed allocated image size!");
        return;
    }

    mHeight = h;
}

void TBitmap::setSize(int w, int h)
{
    DECL_TRACER("TBitmap::setSize(int w, int h)");

    if (w < 1 || h < 1)
        return;

    int pixline = w * mPixelSize;
    int maxHeight = mSize / pixline;

    if (h > maxHeight || (size_t)(pixline * h) > mSize)
    {
        MSG_ERROR("Width and height exceeds allocated image size!");
        return;
    }

    mPixline = pixline;
    mWidth = w;
    mHeight = h;
}

void TBitmap::setPixelSize(int ps)
{
    DECL_TRACER("TBitmap::setPixelSize(int ps)");

    if (ps < 1)
        return;

    int pixline = mWidth * ps;

    if ((size_t)pixline > mSize)
    {
        MSG_ERROR("New pixel size would exceed allocated image size!");
        return;
    }

    mPixelSize = ps;
    mPixline = pixline;
    mHeight = mSize / pixline;
}

bool TBitmap::isValid()
{
    DECL_TRACER("TBitmap::isValid()");

    if (!mData)         // If there is no data, the content is invalid.
        return false;

    // Here we make sure the stored values are totaly valid.
    // First we check that there is potential content.
    if (mSize > 0 && mPixline > 0 && mPixelSize > 0)    // Content?
    {                                                   // Yes, then validate it ...
        int pxl = mWidth * mPixelSize;                  // Calculate the pixels per line
        size_t s = (size_t)(pxl * mHeight);             // Calculate the minimum size of buffer

        if (pxl == mPixline && s <= mSize)              // Compare the pixels per line and the allocated buffer size
            return true;                                // Everything is plausible.
    }

    return false;                                       // Some values failed!
}

void TBitmap::clear()
{
    DECL_TRACER("TBitmap::clear()");

    if (mData)
    {
        delete[] mData;
        mData = nullptr;
    }

    mSize = 0;
    mPixline = mWidth = mHeight = 0;
    mPixelSize = 4;
}

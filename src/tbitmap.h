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

#ifndef __TBITMAP_H__
#define __TBITMAP_H__

#include <cstring>

class TBitmap
{
    public:
        TBitmap();
        TBitmap(const TBitmap& bm);
        TBitmap(const unsigned char *data, size_t size);
        TBitmap(const unsigned char *data, int width, int height, int pixsize=4);
        ~TBitmap();

        void setBitmap(const unsigned char *data, size_t size);
        void setBitmap(const unsigned char *data, int width, int height, int pixsize=4);
        unsigned char *getBitmap(size_t *size=nullptr);
        size_t getSize() { return mSize; }
        bool isValid();

        int getPixline() { return mPixline; }
        void setPixline(int pl);
        int getWidth() { return mWidth; }
        void setWidth(int w);
        int getHeight() { return mHeight; }
        void setHeight(int h);

        void setSize(int w, int h);
        void clear();

        int getPixelSize() { return mPixelSize; }
        void setPixelSize(int ps);

        TBitmap& operator=(const TBitmap& bm)
        {
            this->clear();

            if (bm.mSize > 0)
            {
                this->mData = new unsigned char[bm.mSize];
                memmove(this->mData, bm.mData, bm.mSize);
                this->mSize = bm.mSize;
            }

            this->mPixline = bm.mPixline;
            this->mPixelSize = bm.mPixelSize;
            this->mWidth = bm.mWidth;
            this->mHeight = bm.mHeight;

            return *this;
        }

    private:
        unsigned char *mData{nullptr};
        size_t mSize{0};
        int mPixline{0};
        int mWidth{0};
        int mHeight{0};
        int mPixelSize{4};
};

#endif

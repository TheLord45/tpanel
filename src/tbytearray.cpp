/*
 * Copyright (C) 2024 by Andreas Theofilu <andreas@theosys.at>
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

#include "tbytearray.h"
#include "terror.h"

using std::string;

TByteArray::TByteArray()
{
    DECL_TRACER("TByteArray::TByteArray()");
}

TByteArray::TByteArray(TByteArray& arr)
{
    DECL_TRACER("TByteArray::TByteArray(TByteArray& arr)");

    if (!arr.mSize)
        return;

    mBuffer = new unsigned char[arr.mSize];
    memcpy(mBuffer, arr.mBuffer, arr.mSize);
    mSize = arr.mSize;
}

TByteArray::TByteArray(const unsigned char* data, size_t len)
{
    DECL_TRACER("TByteArray::TByteArray(const unsigned char* data, size_t len)");

    if (!len)
        return;

    mBuffer = new unsigned char[len];
    memcpy(mBuffer, data, len);
    mSize = len;
}

TByteArray::TByteArray(const string& data)
{
    DECL_TRACER("TByteArray::TByteArray(const string& data)");

    if (data.empty())
        return;

    mBuffer = new unsigned char[data.length()];
    memcpy(mBuffer, data.c_str(), data.length());
    mSize = data.length();
}

TByteArray::~TByteArray()
{
    DECL_TRACER("TByteArray::~TByteArray()");

    if (mBuffer)
        delete[] mBuffer;
}

void TByteArray::assign(unsigned char *data, size_t len)
{
    DECL_TRACER("TByteArray::assign(unsigned char *data, size_t len)");

    if (!data)
        return;

    if (mBuffer)
    {
        delete[] mBuffer;
        mBuffer = nullptr;
    }

    mSize = len;

    if (!len)
        return;

    mBuffer = new unsigned char[len];
    memcpy(mBuffer, data, len);
}

void TByteArray::assign(const string& data)
{
    DECL_TRACER("TByteArray::assign(const string& data)");

    assign((unsigned char *)data.c_str(), data.length());
}

void TByteArray::assign(const TByteArray& arr)
{
    DECL_TRACER("TByteArray::assign(const TByteArray& arr)");

    assign(arr.mBuffer, arr.mSize);
}

void TByteArray::append(unsigned char *data, size_t len)
{
    DECL_TRACER("TByteArray::append(unsigned char *data, size_t len)");

    if (!data)
        return;

    if (len == 0)
        return;

    if (!mSize)
    {
        assign(data, len);
        return;
    }

    size_t newLen = mSize + len;
    unsigned char *buf = new unsigned char[newLen];

    memcpy(buf, mBuffer, mSize);
    memcpy(buf+mSize, data, len);
    mSize = newLen;
    delete[] mBuffer;
    mBuffer = buf;
}

void TByteArray::append(const string& data)
{
    DECL_TRACER("TByteArray::append(const string& data)");

    append((unsigned char *)data.c_str(), data.length());
}

void TByteArray::append(const TByteArray& arr)
{
    DECL_TRACER("TByteArray::append(const TByteArray& arr)");

    append(arr.mBuffer, arr.mSize);
}

unsigned char *TByteArray::hexStrToByte(const string& hstr, size_t *size)
{
    DECL_TRACER("TByteArray::hexStrToByte(const string& hstr)");

    if (hstr.empty())
        return nullptr;

    // Test string for hex digits and determine the translateable length
    size_t len = 0;

    for (size_t i = 0; i < hstr.length(); ++i)
    {
        if (!isxdigit(hstr[i]))
            break;

        len++;
    }

    if (!len || (len % 2) != 0) // Is the number of digits an equal number?
        return nullptr;         // No, then return. We do no padding.

    unsigned char *bytes = new unsigned char[len / 2];
    size_t pos = 0;

    if (size)
        *size = len / 2;

    for (size_t i = 0; i < len; i += 2)
    {
        char *endptr;
        string sbyte = hstr.substr(i, 2);
        unsigned long ch = strtoul(sbyte.c_str(), &endptr, 16);

        if (errno != 0)
        {
            MSG_ERROR("Error interpreting number " << sbyte << " into hex: " << strerror(errno));
            delete[] bytes;
            return nullptr;
        }

        if (endptr == sbyte.c_str())
        {
            MSG_ERROR("No digits were found!");
            delete[] bytes;
            return nullptr;
        }

        *(bytes + pos) = static_cast<int>(ch);
        pos++;
    }

    return bytes;
}

string TByteArray::toString()
{
    DECL_TRACER("TByteArray::toString()");

    return string(reinterpret_cast<char *>(mBuffer), mSize);
}

unsigned char *TByteArray::toSerial(unsigned char *erg, bool bigEndian)
{
    DECL_TRACER("TByteArray::toSerial(unsigned char *erg)");

    if (!mSize)
        return nullptr;

    size_t len = mSize + 4;
    unsigned char *buf = nullptr;

    if (!erg)
        buf = new unsigned char[len];
    else
        buf = erg;

    u_int32_t bsize = static_cast<u_int32_t>(mSize);

    if (bigEndian && !isBigEndian())
        swapBytes(reinterpret_cast<unsigned char *>(&bsize), sizeof(bsize));

    memcpy(buf, &bsize, sizeof(bsize));
    memcpy(buf+sizeof(bsize), mBuffer, mSize);
    return buf;
}

size_t TByteArray::serialSize()
{
    DECL_TRACER("TByteArray::serialSize()");

    return mSize + sizeof(u_int32_t);
}

unsigned char TByteArray::at(size_t pos) const
{
    DECL_TRACER("TByteArray::at(size_t pos) const");

    if (pos >= mSize)
        return 0;

    return *(mBuffer+pos);
}

void TByteArray::clear()
{
    DECL_TRACER("TByteArray::clear()");

    if (mBuffer)
        delete[] mBuffer;

    mSize = 0;
    mBuffer = nullptr;
}

bool TByteArray::isBigEndian()
{
    DECL_TRACER("TByteArray::isBigEndian()");

    u_int16_t var = 1;
    unsigned char snum[2];
    memcpy(snum, &var, sizeof(snum));

    if (snum[0] == 1)
        return false;

    return true;
}

unsigned char *TByteArray::swapBytes(unsigned char* b, size_t size)
{
    DECL_TRACER("TByteArray::swapBytes(unsigned char* b, size_t size)");

    unsigned char *num = new unsigned char[size];
    size_t cnt = size - 1;

    for (size_t i = 0; i < size; ++i)
    {
        *(num+cnt) = *(b+i);
        cnt--;
    }

    memcpy(b, num, size);
    delete[] num;
    return b;
}

TByteArray& TByteArray::operator=(const TByteArray& other)
{
    DECL_TRACER("TByteArray::operator=(const TByteArray& other)");

    assign(other.mBuffer, other.mSize);
    return *this;
}

TByteArray& TByteArray::operator+=(const TByteArray& other)
{
    DECL_TRACER("TByteArray::operator+=(const TByteArray& other)");

    append(other.mBuffer, other.mSize);
    return *this;
}

bool TByteArray::operator==(const TByteArray& other) const
{
    DECL_TRACER("TByteArray::operator==(const TByteArray& other) const");

    if (other.mSize != mSize)
        return false;

    for (size_t i = 0; i < mSize; ++i)
    {
        if (*(mBuffer+i) != *(other.mBuffer+i))
            return false;
    }

    return true;
}

bool TByteArray::operator!=(const TByteArray& other) const
{
    DECL_TRACER("TByteArray::operator!=(const TByteArray& other) const");

    if (other.mSize != mSize)
        return true;

    for (size_t i = 0; i < mSize; ++i)
    {
        if (*(mBuffer+i) != *(other.mBuffer+i))
            return true;
    }

    return false;
}

unsigned char TByteArray::operator[](size_t pos) const
{
    DECL_TRACER("TByteArray::operator[](size_t pos) const");

    return at(pos);
}

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

#ifndef TBYTEARRAY_H
#define TBYTEARRAY_H

#include <string>
#include <cstddef>

class TByteArray
{
    public:
        TByteArray();
        TByteArray(TByteArray& arr);
        TByteArray(const unsigned char *data, size_t len);
        TByteArray(const std::string& data);
        ~TByteArray();

        void assign(unsigned char *data, size_t len);
        void assign(const std::string& data);
        void assign(const TByteArray& arr);

        void append(unsigned char *data, size_t len);
        void append(const std::string& data);
        void append(const TByteArray& arr);

        size_t size() { return mSize; }
        bool empty() { return mSize == 0; }
        std::string toString();
        unsigned char *toSerial(unsigned char *erg=nullptr, bool bigEndian=false);
        static unsigned char *hexStrToByte(const std::string& hstr, size_t *size=nullptr);
        size_t serialSize();
        unsigned char *data() { return mBuffer; };
        unsigned char at(size_t pos) const;
        void clear();

        TByteArray& operator=(const TByteArray& other);
        TByteArray& operator+=(const TByteArray& other);
        bool operator==(const TByteArray& other) const;
        bool operator!=(const TByteArray& other) const;
        unsigned char operator[](size_t pos) const;

    protected:
        bool isBigEndian();
        unsigned char *swapBytes(unsigned char *b, size_t size);

    private:
        size_t mSize{0};
        unsigned char *mBuffer{nullptr};
};

#endif // BYTEARRAY_H

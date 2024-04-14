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

#include "taudioconvert.h"
#include "terror.h"

TAudioConvert::TAudioConvert()
{
    DECL_TRACER("TAudioConvert::TAudioConvert()");
}

TAudioConvert::~TAudioConvert()
{
    DECL_TRACER("TAudioConvert::~TAudioConvert()");
}

int16_t TAudioConvert::uLawDecodeDigital(int8_t number)
{
    const uint16_t MULAW_BIAS = 33;
    uint8_t sign = 0, position = 0;
    int16_t decoded = 0;
    number = ~number;

    if (number & 0x80)
    {
        number &= ~(1 << 7);
        sign = -1;
    }

    position = ((number & 0xF0) >> 4) + 5;
    decoded = ((1 << position) | ((number & 0x0F) << (position - 4)) | (1 << (position - 5))) - MULAW_BIAS;
    return (sign == 0) ? (decoded) : (-(decoded));
}

int8_t TAudioConvert::uLawEncodeDigital(int16_t number)
{
    const uint16_t MULAW_MAX = 0x1FFF;
    const uint16_t MULAW_BIAS = 33;
    uint16_t mask = 0x1000;
    uint8_t sign = 0;
    uint8_t position = 12;
    uint8_t lsb = 0;

    if (number < 0)
    {
        number = -number;
        sign = 0x80;
    }

    number += MULAW_BIAS;

    if (number > MULAW_MAX)
        number = MULAW_MAX;

    for (; ((number & mask) != mask && position >= 5); mask >>= 1, position--)
        ;

    lsb = (number >> (position - 4)) & 0x0f;
    return (~(sign | ((position - 5) << 4) | lsb));
}

int8_t TAudioConvert::linearToMuLaw(int16_t sample)
{
    //We get the sign
    int sign = (sample >> 8) & 0x0080;

    if (sign != 0)
        sample = -sample;

    if (sample > cClip)
        sample = cClip;

    sample += cBias;

    int exponent = static_cast<int>(MuLawCompressTable[(sample >> 7) & 0xFF]);
    int mantissa = (sample >> (exponent + 3)) & 0x0F;
    int compressedByte = ~(sign | (exponent << 4) | mantissa);

    return static_cast<int8_t>(compressedByte);
}

int8_t TAudioConvert::linearToALaw(int16_t sample)

{
    int16_t sign, exponent, mantissa;
    uint8_t compressedByte;

    sign = ((~sample) >> 8) & 0x80;

    if (!sign)
        sample = (short)-sample;

    if (sample > cClip)
        sample = cClip;

    if (sample >= 256)
    {
        exponent = static_cast<int16_t>(ALawCompressTable[(sample >> 8) & 0x7F]);
        mantissa = (sample >> (exponent + 3) ) & 0x0F;
        compressedByte = ((exponent << 4) | mantissa);
    }
    else
        compressedByte = static_cast<uint8_t>(sample >> 4);

    compressedByte ^= (sign ^ 0x55);
    return compressedByte;
}

int16_t TAudioConvert::muLawToLinear(uint8_t ulawbyte)
{
    return MuLawDecompressTable[ulawbyte];
}

int16_t TAudioConvert::aLawToLinear(uint8_t alawbyte)
{
    return ALawDecompressTable[alawbyte];
}

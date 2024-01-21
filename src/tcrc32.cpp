/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include "tcrc32.h"
#include "tresources.h"
#include "terror.h"

using std::vector;

TCrc32::TCrc32(const vector<uint8_t>& bytes)
{
    DECL_TRACER("TCrc32::TCrc32(const vector<uint8_t>& bytes)");

    uint32_t salt = 0;
    size_t pos = 0;

    for (size_t i = 0; i < bytes.size(); ++i)
    {
        uint32_t b = static_cast<uint32_t>(bytes[i]);

        if (b == 0 && bytes.size() > 4)
            continue;

        salt |= (b << (pos * 8));

        if (pos >= 4)
            break;

        pos++;
    }

    mCrc32 = calculate_crc32c(salt, bytes.data(), bytes.size());
}

uint32_t TCrc32::crc32c_sb8_64_bit(uint32_t crc, const unsigned char *p_buf, uint32_t length, uint32_t init_bytes)
{
    DECL_TRACER("TCrc32::crc32c_sb8_64_bit(uint32_t crc, const unsigned char *p_buf, uint32_t length, uint32_t init_bytes)");

    uint32_t li;
    uint32_t term1, term2;
    uint32_t running_length;
    uint32_t end_bytes;

    running_length = ((length - init_bytes) / 8) * 8;
    end_bytes = length - init_bytes - running_length;

    for (li = 0; li < init_bytes; li++)
        crc = sctp_crc_tableil8_o32[(crc ^ *p_buf++) & 0x000000FF] ^ (crc >> 8);

    for (li = 0; li < running_length / 8; li++)
    {
        if (isBigEndian())
        {
            crc ^= *p_buf++;
            crc ^= (*p_buf++) << 8;
            crc ^= (*p_buf++) << 16;
            crc ^= (*p_buf++) << 24;
        }
        else
        {
            crc ^= *(const uint32_t *) p_buf;
            p_buf += 4;
        }

        term1 = sctp_crc_tableil8_o88[crc & 0x000000FF] ^ sctp_crc_tableil8_o80[(crc >> 8) & 0x000000FF];
        term2 = crc >> 16;
        crc = term1 ^ sctp_crc_tableil8_o72[term2 & 0x000000FF] ^ sctp_crc_tableil8_o64[(term2 >> 8) & 0x000000FF];

        if (isBigEndian())
        {
            crc ^= sctp_crc_tableil8_o56[*p_buf++];
            crc ^= sctp_crc_tableil8_o48[*p_buf++];
            crc ^= sctp_crc_tableil8_o40[*p_buf++];
            crc ^= sctp_crc_tableil8_o32[*p_buf++];
        }
        else
        {
            term1 = sctp_crc_tableil8_o56[(*(const uint32_t *) p_buf) & 0x000000FF] ^ sctp_crc_tableil8_o48[((*(const uint32_t *) p_buf) >> 8) & 0x000000FF];

            term2 = (*(const uint32_t *) p_buf) >> 16;
            crc = crc ^ term1 ^ sctp_crc_tableil8_o40[term2 & 0x000000FF] ^ sctp_crc_tableil8_o32[(term2 >> 8) & 0x000000FF];
            p_buf += 4;
        }
    }

    for (li = 0; li < end_bytes; li++)
        crc = sctp_crc_tableil8_o32[(crc ^ *p_buf++) & 0x000000FF] ^ (crc >> 8);

    return crc;
}

uint32_t TCrc32::multitable_crc32c(uint32_t crc32c, const unsigned char *buffer, unsigned int length)
{
    DECL_TRACER("TCrc32::multitable_crc32c(uint32_t crc32c, const unsigned char *buffer, unsigned int length)");

    uint32_t to_even_word;

    if (length == 0)
        return (crc32c);

    to_even_word = (4 - (((uintptr_t) buffer) & 0x3));
    return (crc32c_sb8_64_bit(crc32c, buffer, length, to_even_word));
}

uint32_t TCrc32::singletable_crc32c(uint32_t crc, const void *buf, size_t size)
{
    DECL_TRACER("TCrc32::singletable_crc32c(uint32_t crc, const void *buf, size_t size)");

    const uint8_t *p = (uint8_t *)buf;

    while (size--)
        crc = crc32Table[(crc ^ *p++) & 0xff] ^ (crc >> 8);

    return crc;
}

uint32_t TCrc32::calculate_crc32c(uint32_t crc32c, const unsigned char *buffer, unsigned int length)
{
    DECL_TRACER("TCrc32::calculate_crc32c(uint32_t crc32c, const unsigned char *buffer, unsigned int length)");

    if (!buffer || !length)
        return 0;

    std::stringstream s;
    s << std::setw(8) << std::setfill('0') << std::hex << crc32c;
    MSG_DEBUG("Calculating CRC32 with the salt " << s.str() << " and a length of " << length << " bytes.");

    if (length < 4)
        return (singletable_crc32c(crc32c, buffer, length));
    else
        return (multitable_crc32c(crc32c, buffer, length));
}

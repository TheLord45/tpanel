/*
 * Copyright (C) 2019 to 2021 by Andreas Theofilu <andreas@theosys.at>
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

#include <iostream>
#include <fstream>

#if __cplusplus < 201402L
#   error "This module requires at least C++ 14 standard!"
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

#include <assert.h>

#include "texpand.h"
#include "terror.h"

using namespace std;

void TExpand::setFileName (const string &fn)
{
    DECL_TRACER("TExpand::setFileName (const string &fn)");
	fname.assign(fn);
}

int TExpand::unzip()
{
    DECL_TRACER("TExpand::unzip()");

	if (fname.empty())
		return -1;

	string target(fname+".temp");
	FILE *source, *dest;

	source = fopen(fname.c_str(), "rb");

	if (!source)
	{
		MSG_ERROR("Error opening file" << fname << "!");
		return -1;
	}

	dest = fopen(target.c_str(), "wb");

	if (!dest)
	{
		fclose(source);
		MSG_ERROR("Error creating the temporary file " << target << "!");
		return -1;
	}

	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	// allocate inflate state
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, 32+MAX_WBITS);

	if (ret != Z_OK)
	{
		zerr(ret);
		fclose(source);
		fclose(dest);
        fs::remove(target);
		return ret;
	}

	do
	{
		strm.avail_in = fread(in, 1, CHUNK, source);

		if (ferror(source))
		{
			(void)inflateEnd(&strm);
			MSG_ERROR("Error reading from file " << fname << "!");
			fclose(source);
			fclose(dest);
            fs::remove(target);
			return Z_ERRNO;
		}

		if (strm.avail_in == 0)
			break;

		strm.next_in = in;
		// run inflate() on input until output buffer not full
        do
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);	// state not clobbered

			switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;		// and fall through
					// fall through
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					fclose(source);
					fclose(dest);
                    fs::remove(target);
					zerr(ret);
					return ret;
			}

			have = CHUNK - strm.avail_out;

			if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				(void)inflateEnd(&strm);
				MSG_ERROR("Error on writing to file " << target << "!");
				fclose(source);
				fclose(dest);
                fs::remove(target);
				return Z_ERRNO;
			}
		}
		while (strm.avail_out == 0);
		// done when inflate() says it's done
	}
	while (ret != Z_STREAM_END);
	// clean up and return
	(void)inflateEnd(&strm);
	fclose(source);
	fclose(dest);
    fs::remove(fname);
    fs::rename(target, fname);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void TExpand::zerr(int ret)
{
    DECL_TRACER("TExpand::zerr(int ret)");

    switch (ret)
	{
		case Z_ERRNO:
			MSG_ERROR("Error reading or writing a file!");
		break;

		case Z_STREAM_ERROR:
			MSG_ERROR("invalid compression level");
		break;

		case Z_DATA_ERROR:
			MSG_ERROR("invalid or incomplete deflate data");
		break;

		case Z_MEM_ERROR:
			MSG_ERROR("out of memory");
		break;

		case Z_VERSION_ERROR:
			MSG_ERROR("zlib version mismatch!");
		break;

		default:
			if (ret != Z_OK)
            {
				MSG_ERROR("Unknown error " << ret << "!");
            }
    }
}

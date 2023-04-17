/*
 * Copyright (C) 2020 to 2023 by Andreas Theofilu <andreas@theosys.at>
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
#include <string.h>
#include <ios>
#include <iomanip>
#include <iconv.h>
#include <sstream>
#include "tnameformat.h"

#ifdef __ANDROID__
#if __ANDROID_API__ < 28
#error "This module needs Android API level 28 or newer!"
#endif
#endif

using namespace std;

std::string TNameFormat::mStr;

/**
 * @brief TNameFormat::TNameFormat constructor
 */
TNameFormat::TNameFormat()
{
	DECL_TRACER("TNameFormat::TNameFormat()");
}

/**
 * @brief TNameFormat::~TNameFormat destructor
 */
TNameFormat::~TNameFormat()
{
	DECL_TRACER("TNameFormat::~TNameFormat()");
}

/**
 * @brief TNameFormat::toValidName filters invalid characters.
 *
 * This method filters a string for invalid characters. Allowed are all
 * characters between 0 to 9, a to z, A to Z and the underline(_). All other
 * characters will be filtered out.
 *
 * @param str   The string to filter
 * @return A string containing only the valid characters.
 */
string TNameFormat::toValidName(string& str)
{
	DECL_TRACER("TNameFormat::toValidName(string& str)");
	string ret;

	for (size_t i = 0; i < str.length(); i++)
	{
		char c = str.at(i);

		if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z'))
			c = '_';

		ret.push_back(c);
	}

	return ret;
}

/**
 * @brief TNameFormat::cutNumbers filters everything but digit symbols.
 *
 * This method filters a string for digits. It filters out all characters
 * except digits.
 *
 * @param str   String to filter.
 * @return A string containing only digits or an empty string if there were
 * no digits in \p str.
 */
string TNameFormat::cutNumbers(string& str)
{
	DECL_TRACER("TNameFormat::cutNumbers(string& str)");
	string ret;

	for (size_t i = 0; i < str.length(); i++)
	{
		char c = str.at(i);

		if (c < '0' || c > '9')
			continue;

		ret.push_back(c);
	}

	return ret;
}

/**
 * @brief TNameFormat::toShortName filters a sequence starting with a blank and ends with a dot.
 *
 * This method filters any sequence starting with one or more blanks (' ') and
 * end with a dot (.).
 *
 * @param str   The string to filter
 * @return A string containing no sequence equal (" .");
 */
string TNameFormat::toShortName(string& str)
{
	DECL_TRACER("TNameFormat::toShortName(string& str)");
	string ret;
	bool ignore = false;

	for (size_t i = 0; i < str.length(); i++)
	{
		char c = str.at(i);

		if (c == ' ')
			ignore = true;

		if (ignore && c == '.')
			ignore = false;

		if (!ignore)
			ret.push_back(c);
	}

	return ret;
}

/**
 * @brief TNameFormat::transFontName Replaces blanks and percent to unterline.
 *
 * This method replaces every blank (' ') and percent (%) with an underline (_).
 * Then the seuqence **.ttf** is replace by **.woff**.
 *
 * @param str   The string to evaluate
 * @return A filtered string.
 */
string TNameFormat::transFontName(string& str)
{
	DECL_TRACER("TNameFormat::transFontName(string& str)");
	string ret;

	for (size_t i = 0; i < str.length(); i++)
	{
		char c = str.at(i);

		if (c == ' ' || c == '%')
			c = '_';

		ret.push_back(c);
	}

	replace(ret, ".ttf", ".woff");
	return ret;
}

/**
 * @brief TNameFormat::toURL converts a URL into a safe URL.
 *
 * This method filters a URL in a way, that all possible dangerous characters
 * are replaced by a percent (%) and the byte value.
 *
 * @param str   An URL to evaluate
 * @return A safe URL.
 */
string TNameFormat::toURL(string& str)
{
	DECL_TRACER("TNameFormat::toURL(string& str)");
	string ret;

	for (size_t i = 0; i < str.length(); i++)
	{
		char c = str.at(i);

		if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'z') &&
			!(c >= 'A' && c <= 'Z') && c != '_' && c != '.' &&
			c != '-' && c != '/')
		{
			ret.append("%");
			std::stringstream stream;
			stream << std::setfill ('0') << std::setw(2) << std::hex << ((int)c & 0x000000ff);
			ret.append(stream.str());
		}
		else
			ret.push_back(c);
	}

	return ret;
}

/**
 * @brief TNameFormat::EncodeTo convert into another character set.
 *
 * This method takes a string and converts it into another character set.
 *
 * @param buf   A pointer to a buffer large enough to hold the result.
 * @param len   The length of the buffer \p buf.
 * @param str   A pointer to a string. The content will be converted.
 * @param from  The name of the character set in \p str.
 * @param to    The name of the target charcter set.
 * @return The pointer \p buf. This buffer is filled up to the length of \p str
 * or the zero byte. Whichever comes first.
 */
char *TNameFormat::EncodeTo(char* buf, size_t *len, const string& str, const string& from, const string& to)
{
	DECL_TRACER("TNameFormat::EncodeTo(char* buf, size_t *len, const string& str, const string& from, const string& to)");

	if (!buf || str.empty())
		return 0;

    iconv_t cd = iconv_open(from.c_str(), to.c_str());

	if (cd == (iconv_t) -1)
	{
		MSG_ERROR("iconv_open failed!");
		TError::setError();
		return 0;
	}

	char *in_buf = (char *)str.c_str();
	size_t in_left = str.length() - 1;

	char *out_buf = buf;
	size_t out_left = *len - 1;
	size_t new_len;

	do
	{
		if ((new_len = iconv(cd, &in_buf, &in_left, &out_buf, &out_left)) == (size_t) -1)
		{
			MSG_ERROR("iconv failed: " << strerror(errno));
			TError::setError();
			return 0;
		}
	}
	while (in_left > 0 && out_left > 0);

	*out_buf = 0;
	iconv_close(cd);
	*len = out_buf - buf;
	return buf;
}

string TNameFormat::textToWeb(const string& txt)
{
	DECL_TRACER("TNameFormat::textToWeb(const string& txt)");
	string out;

	if (txt.empty())
		return out;

	for (size_t i = 0; i < txt.length(); i++)
	{
		unsigned char c = txt.at(i);

		if (c == '\r')
			continue;

		if (c == '\n')
			out += "<br>";
		else if (c == '&')
			out += "&amp;";
		else if (c == ';')
			out += "&semi;";
		else if (c == '<')
			out += "&lt;";
		else if (c == '>')
			out += "&gt;";
		else if (c == '\t')
			out += "&tab;";
		else if (c == '!')
			out += "&excl;";
		else if (c == '"')
			out += "&quot;";
		else if (c == '#')
			out += "&num;";
		else if (c == '\'')
			out += "&apos;";
		else if (c == '=')
			out += "&equals;";
		else if (c == '-')
			out += "&dash;";
		else if (c == '~')
			out += "&tilde;";
		else if (c == ' ')
			out += "&nbsp;";
		else
			out.push_back(c);
	}

	return out;
}

string TNameFormat::toHex(int num, int width)
{
	DECL_TRACER("TNameFormat::toHex(int num, int width)");

	string ret;
	std::stringstream stream;
	stream << std::setfill ('0') << std::setw(width) << std::hex << num;
	ret = stream.str();
	return ret;
}

string TNameFormat::strToHex(string str, int width, bool format, int indent)
{
	DECL_TRACER("TNameFormat::strToHex(string str, int width, bool format, int indent)");

	int len = 0, pos = 0, old = 0;
	int w = (format) ? 1 : width;
	string out, left, right;
	string ind;

	if (indent > 0)
	{
		for (int j = 0; j < indent; j++)
			ind.append(" ");
	}

	for (size_t i = 0; i < str.length(); i++)
	{
		if (len >= w)
		{
			left.append(" ");
			len = 0;
		}

		if (format && i > 0 && (pos % width) == 0)
		{
			out += ind + toHex(old, 4) + ": " + left + " | " + right + "\n";
			left.clear();
			right.clear();
			old = pos;
		}

		int c = str.at(i) & 0x000000ff;
		left.append(toHex(c, 2));

		if (format)
		{
			if (std::isprint(c))
				right.push_back(c);
			else
				right.push_back('.');
		}

		len++;
		pos++;
	}

	if (!format)
		return left;
	else if (pos > 0)
	{
		if ((pos % width) != 0)
		{
			for (int i = 0; i < (width - (pos % width)); i++)
				left.append("   ");
		}

		out += ind + toHex(old, 4)+": "+left + "  | " + right;
	}

	return out;
}

string TNameFormat::strToHex(const unsigned char *str, size_t length, int width, bool format, int indent)
{
    DECL_TRACER("TNameFormat::strToHex(const unsigned char *str, int width, bool format, int indent)");

    int len = 0, pos = 0, old = 0;
    int w = (format) ? 1 : width;
    string out, left, right;
    string ind;

    if (indent > 0)
    {
        for (int j = 0; j < indent; j++)
            ind.append(" ");
    }

    for (size_t i = 0; i < length; i++)
    {
        if (len >= w)
        {
            left.append(" ");
            len = 0;
        }

        if (format && i > 0 && (pos % width) == 0)
        {
            out += ind + toHex(old, 4) + ": " + left + " | " + right + "\n";
            left.clear();
            right.clear();
            old = pos;
        }

        int c = *(str+i) & 0x000000ff;
        left.append(toHex(c, 2));

        if (format)
        {
            if (std::isprint(c))
                right.push_back(c);
            else
                right.push_back('.');
        }

        len++;
        pos++;
    }

    if (!format)
        return left;
    else if (pos > 0)
    {
        if ((pos % width) != 0)
        {
            for (int i = 0; i < (width - (pos % width)); i++)
                left.append("   ");
        }

        out += ind + toHex(old, 4)+": "+left + "  | " + right;
    }

    return out;
}

string TNameFormat::latin1ToUTF8(const string& str)
{
	DECL_TRACER("TNameFormat::latin1ToUTF8(const string& str)");

	string out;

	for (size_t i = 0; i < str.length(); i++)
	{
		uint8_t ch = str.at(i);

		if (ch < 0x80)
		{
			out.push_back(ch);
		}
		else
		{
			out.push_back(0xc0 | ch >> 6);
			out.push_back(0x80 | (ch & 0x3f));
		}
	}

	return out;
}

string TNameFormat::cp1250ToUTF8(const string& str)
{
	DECL_TRACER("TNameFormat::cp1250ToUTF8(const string& str)");

	string out;

	for (size_t j = 0; j < str.length(); j++)
	{
		int i = -1;
		unsigned char ch = str.at(j);
		short utf = -1;

		if (ch < 0x80)
		{
			do
			{
				i++;

				if (__cht[i].ch == ch)
				{
					utf = __cht[i].byte;
					break;
				}
			}
			while (__cht[i].ch != 0xff);

			if (utf < 0)
				utf = ch;
		}
		else
			utf = ch;

		if (utf > 0x00ff)
		{
			out.push_back((utf >> 8) & 0x00ff);
			out.push_back(utf & 0x00ff);
		}
		else if (ch > 0x7f)
		{
			out.push_back(0xc0 | ch >> 6);
			out.push_back(0x80 | (ch & 0x3f));
		}
		else
			out.push_back(ch);
	}

	return out;
}

string TNameFormat::UTF8ToCp1250(const string& str)
{
	DECL_TRACER("TNameFormat::UTF8ToCp1250(const string& str)");

	char dst[1024];
	size_t srclen = 0;
	char* pIn, *pInSave;

	srclen = str.length();
	memset(&dst[0], 0, sizeof(dst));

	try
	{
		pIn = new char[srclen + 1];
		memcpy(pIn, str.c_str(), srclen);
		*(pIn+srclen) = 0;
		pInSave = pIn;
	}
	catch(std::exception& e)
	{
		MSG_ERROR(e.what());
		TError::setError();
		return "";
	}

	size_t dstlen = sizeof(dst) - 1;
	char* pOut = (char *)dst;

	iconv_t conv = iconv_open("CP1250", "UTF-8");

	if (conv == (iconv_t)-1)
	{
		MSG_ERROR("Error opening iconv!");
		TError::setError();
		delete[] pInSave;
		return str;
	}

	size_t ret = iconv(conv, &pIn, &srclen, &pOut, &dstlen);
	iconv_close(conv);
	delete[] pInSave;

	if (ret == (size_t)-1)
	{
		MSG_ERROR("Error converting a string!");
		TError::setError();
		return str;
	}

	return string(dst);
}

string TNameFormat::trimXML(const string& str)
{
    DECL_TRACER("TNameFormat::trimXML(const string& str)");

    string::const_iterator iter;
    string buffer;
    bool inEname = false;
    bool isEnd = false;
    bool isEEnd = false;
    bool isHead = false;
    char byte[2];
    byte[1] = 0;

    for (iter = str.begin(); iter != str.end(); ++iter)
    {
        if (*iter == '<')
            inEname = true;

        if (inEname && !isHead && *iter == '/')
            isEnd = true;

        if (inEname && !isHead && *iter == '?')
            isHead = true;

        if (inEname && isEnd && *iter == '>')
            inEname = isEnd = false;

        if (inEname && !isEEnd && *iter == '>')
            isEEnd = true;

        if (inEname && isHead && *iter == '>')
        {
            inEname = isHead = isEEnd = isEnd = false;
            buffer.append(">\n");
            continue;
        }

        if ((!inEname || isEEnd) && (*iter == ' ' || *iter == '\t' || *iter == '\r' || *iter == '\n'))
        {
            isEEnd = false;

            if (*iter == '\r' || *iter == '\n')
                inEname = false;

            continue;
        }

        byte[0] = *iter;
        buffer.append(byte);
    }

    return buffer;
}

string& TNameFormat::replace(const string& str, const string& old, const string& neu)
{
	mStr = str;
	size_t pos = mStr.find(old);

	while (pos != string::npos)
	{
		mStr.replace(pos, old.size(), neu);
		pos = str.find(old, pos + neu.size());
	}

	return mStr;
}


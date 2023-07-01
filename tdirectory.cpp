/*
 * Copyright (C) 2019 to 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include <chrono>

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

#include "tconfig.h"
#include "terror.h"
#include "tdirectory.h"
#include "tresources.h"

using namespace std;
using namespace dir;
using namespace std::chrono_literals;

int TDirectory::readDir()
{
	DECL_TRACER("Directory::readDir()");

	if (path.empty())
		return 0;

    if (!exists(path) || !isDirectory(path))
    {
        MSG_WARNING("Ignoring invalid path " << path);
        return 0;
    }

	int count = 0;

	try
	{
		for(auto& p: fs::directory_iterator(path))
		{
			DFILES_T dr;
			string f = fs::path(p.path()).filename();

			if (f.at(0) == '.')
				continue;

			if (path.find("__system/") == string::npos && f.find("__system") != string::npos)
				continue;
#if __GNUC__ < 9
			if (path.find("scripts") != string::npos && fs::is_directory(p.path()))
#else
			if (path.find("scripts") != string::npos && p.is_directory())
#endif
				continue;

			count++;
			dr.count = count;
#if __GNUC__ < 9
			time_t ti = fs::last_write_time(p.path()).time_since_epoch().count();
#else
			time_t ti = p.last_write_time().time_since_epoch().count();
#endif
			dr.date = (ti / 1000000000) + 6437664000;

#if __GNUC__ < 9
			if (fs::is_directory(p.path()))
#else
			if (p.is_directory())
#endif
				dr.size = 0;
			else
#if __GNUC__ < 9
				dr.size = fs::file_size(p.path());
#else
				dr.size = p.file_size();
#endif

			if (strip)
				dr.name = f;
			else
				dr.name = p.path();

			dr.attr = 0;

#if __GNUC__ < 9
			if (fs::is_directory(p.path()))
#else
			if (p.is_directory())
#endif
				dr.attr = dr.attr | ATTR_DIRECTORY;
#if __GNUC__ < 9
			else if (fs::is_regular_file(p.path()))
#else
			else if (p.is_regular_file())
#endif
			{
				if (dr.name.find(".png") != string::npos || dr.name.find(".PNG") != string::npos ||
						dr.name.find(".jpg") != string::npos || dr.name.find(".JPG") != string::npos)
					dr.attr = dr.attr | ATTR_GRAPHIC;
				else if (dr.name.find(".wav") != string::npos || dr.name.find(".WAV") != string::npos ||
						dr.name.find(".mp3") != string::npos || dr.name.find(".MP3") != string::npos)
					dr.attr = dr.attr | ATTR_SOUND;
				else
					dr.attr = dr.attr | ATTR_TEXT;
			}

#if __GNUC__ < 9
			if (fs::is_symlink(p.path()))
#else
			if (p.is_symlink())
#endif
				dr.attr |= ATTR_LINK;

			entries.push_back(dr);

            if (TStreamError::checkFilter(HLOG_DEBUG))
			{
				char buf[4096];
				char d, g, l;

				d = l = '_';
				g = ' ';

				if (dr.attr & ATTR_DIRECTORY)
					d = 'D';

				if (dr.attr & ATTR_GRAPHIC)
					g = 'g';
				else if (dr.attr & ATTR_SOUND)
					g = 's';
				else if (dr.attr & ATTR_TEXT)
					g = 't';

				if (dr.attr & ATTR_LINK)
					l = 'L';

				struct tm *t = localtime(&dr.date);

				if (t == nullptr)
					snprintf(buf, sizeof(buf), "%c%c%c %8zu 0000-00-00 00:00:00 %s", d, g, l, dr.size, dr.name.c_str());
				else
					snprintf(buf, sizeof(buf), "%c%c%c %8zu %4d-%02d-%02d %02d:%02d:%02d %s", d, g, l, dr.size, t->tm_year + 1900, t->tm_mon+1, t->tm_mday,
                         t->tm_hour, t->tm_min, t->tm_sec, dr.name.c_str());

				MSG_TRACE("Buffer: " << buf);
			}
		}

		done = true;
		MSG_TRACE("Read " << count << " entries.");
	}
	catch(exception& e)
	{
		MSG_ERROR("Error: " << e.what());
		entries.clear();
		return 0;
	}

	return count;
}

int TDirectory::readDir (const string &p)
{
	DECL_TRACER("Directory::readDir (const string &p)");

	path.assign(p);

	if (done)
		entries.clear();

	done = false;
	return readDir();
}

int TDirectory::scanFiles(const string &filter)
{
    DECL_TRACER("TDirectory::scanFiles(const string &filter)");

    if (path.empty())
        return 0;

    int count = 0;
    entries.clear();

    try
    {
        for(auto& p: fs::directory_iterator(path))
        {
            DFILES_T dr;
            string f = fs::path(p.path()).filename();

            if (checkDot(f))
                continue;

            if (!filter.empty() && f.find(filter) == string::npos)
                continue;

            count++;
            dr.count = count;
#if __GNUC__ < 9
            time_t ti = fs::last_write_time(p.path()).time_since_epoch().count();
#else
            time_t ti = p.last_write_time().time_since_epoch().count();
#endif
            dr.date = (ti / 1000000000) + 6437664000;

#if __GNUC__ < 9
            if (fs::is_directory(p.path()))
#else
            if (p.is_directory())
#endif
                dr.size = 0;
            else
#if __GNUC__ < 9
                dr.size = fs::file_size(p.path());
#else
                dr.size = p.file_size();
#endif

            if (strip)
                dr.name = f;
            else
                dr.name = p.path();

            dr.attr = 0;

#if __GNUC__ < 9
            if (fs::is_directory(p.path()))
#else
            if (p.is_directory())
#endif
                dr.attr = dr.attr | ATTR_DIRECTORY;
#if __GNUC__ < 9
            else if (fs::is_regular_file(p.path()))
#else
            else if (p.is_regular_file())
#endif
            {
                if (dr.name.find(".png") != string::npos || dr.name.find(".PNG") != string::npos ||
                    dr.name.find(".jpg") != string::npos || dr.name.find(".JPG") != string::npos)
                    dr.attr = dr.attr | ATTR_GRAPHIC;
                else if (dr.name.find(".wav") != string::npos || dr.name.find(".WAV") != string::npos ||
                    dr.name.find(".mp3") != string::npos || dr.name.find(".MP3") != string::npos)
                    dr.attr = dr.attr | ATTR_SOUND;
                else
                    dr.attr = dr.attr | ATTR_TEXT;
            }

#if __GNUC__ < 9
            if (fs::is_symlink(p.path()))
#else
            if (p.is_symlink())
#endif
                dr.attr |= ATTR_LINK;

            entries.push_back(dr);
        }

        done = true;
        MSG_DEBUG("Read " << count << " entries.");

        if (TStreamError::checkFilter(HLOG_DEBUG))
        {
            vector<DFILES_T>::iterator iter;

            for (iter = entries.begin(); iter != entries.end(); ++iter)
                MSG_DEBUG("Entry: " << iter->name);
        }
    }
    catch(exception& e)
    {
        MSG_ERROR("Error: " << e.what());
        entries.clear();
        return 0;
    }

    return count;
}

size_t TDirectory::getNumEntries()
{
	DECL_TRACER("Directory::getNumEntries()");

	if (done)
		return entries.size();

	return 0;
}

DFILES_T TDirectory::getEntry (size_t pos)
{
	DECL_TRACER("Directory::getEntry (size_t pos)");

	if (!done || pos >= entries.size())
	{
		DFILES_T d;
		d.attr = 0;
		d.count = 0;
		d.date = 0;
		d.size = 0;
		return d;
	}

	return entries.at(pos);
}

string TDirectory::stripPath (const string &p, size_t idx)
{
	DECL_TRACER("Directory::stripPath (const string &p, size_t idx)");

	if (!done || idx > entries.size())
		return "";

	size_t pos;
	DFILES_T dr = getEntry(idx);

	if ((pos = dr.name.find(p)) == string::npos)
		return "";

	return dr.name.substr(pos + p.length());
}

string TDirectory::stripPath (const string &p, const string &s)
{
	DECL_TRACER("Directory::stripPath (const string &p, const string &s)");

	size_t pos;

	if ((pos = s.find(p)) == string::npos)
		return "";

	return s.substr(pos + p.length());
}

bool TDirectory::createAllPath(string& path, bool cut)
{
    DECL_TRACER("TDirectory::createAllPath(string& path, bool cut)");

    string pth = path;

    if (cut)
    {
        size_t pos = path.find_last_of("/");

        if (pos != string::npos)
            pth = path.substr(0, pos);
        else
            return false;
    }

    MSG_INFO("Creating path: " << pth);
    return fs::create_directories(pth);
}

bool TDirectory::drop(const string &path)
{
    DECL_TRACER("TDirectory::drop(const string &path)");

    try
    {
        int n = (int)fs::remove_all(path);
        MSG_TRACE("Deleted " << n << " objects.");
        return true;
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error deleting file/directory: " << path);
        MSG_ERROR(e.what());
    }

    return false;
}

/**
 * @brief TDirectory::dropDir deletes all files in a directory
 * This methos deletes only the files in a given directory and leaves all
 * subdirectories along with the files in them alone.
 *
 * @param path  A valid path. The files in this path will be deleted.
 * @return On success TRUE is returned.
 */
bool TDirectory::dropDir(const string& path)
{
    DECL_TRACER("TDirectory::dropDir(const string& path)");

    if (path.empty())
        return 0;

    if (!fs::exists(path))
    {
        MSG_WARNING("Directory \"" << path << "\" does not exist!");
        return 0;
    }

    MSG_DEBUG("Dropping directory: " << path);
    int count = 0;

    try
    {
        for(auto& p: fs::directory_iterator(path))
        {
            string f = fs::path(p.path()).filename();

            if (checkDot(f))
                continue;
#if __GNUC__ < 9
            if (fs::is_directory(p.path()))
#else
            if (p.is_directory())
#endif
                continue;

            if (!fs::remove(p.path()))
            {
                MSG_ERROR("Error deleting file:" << p.path());
            }

            count++;
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error dir drop: " << e.what());
        return false;
    }

    MSG_DEBUG("Deleted " << count << " files.");
    return true;
}

bool TDirectory::dropFile(const string& fname)
{
    DECL_TRACER("TDirectory::dropFile(const string& fname)");

    try
    {
        fs::remove(fname);
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error removing file " << fname << "!");
        return false;
    }

    return true;
}

string TDirectory::getEntryWithEnd(const string &end)
{
    DECL_TRACER("TDirectory::getEntryWithEnd(const string &end)");

    if (entries.size() == 0)
        return string();

    vector<DFILES_T>::iterator iter;
    size_t pos;

    for (iter = entries.begin(); iter != entries.end(); ++iter)
    {
        if ((pos = iter->name.find_last_of(end)) != string::npos && (pos + end.length()) == iter->name.length())
            return iter->name;
    }

    return string();
}

string TDirectory::getEntryWithPart(const string &part, bool precice)
{
    DECL_TRACER("TDirectory::getEntryWithPart(const string &part, bool precice)");

    if (entries.size() == 0)
        return string();

    vector<DFILES_T>::iterator iter;
    size_t pos;

    for (iter = entries.begin(); iter != entries.end(); ++iter)
    {
        if ((pos = iter->name.find(part)) != string::npos)
        {
            char next = iter->name.at(pos + part.length());

            if (next == '.')
                return iter->name;

            if (precice && next == '_')
                continue;

            if ((next >= 'A' && next <= 'Z') || (next >= 'a' && next <= 'z'))
                continue;

            return iter->name;
        }
    }

    return string();
}

size_t TDirectory::getFileSize (const string &f)
{
	DECL_TRACER("Directory::getFileSize (const string &f)");
	size_t s = 0;

	try
	{
		if (!fs::path(f).has_filename())
			return s;

		s = fs::file_size(f);
    }
	catch(exception& e)
	{
		MSG_ERROR("Error: " << e.what());
		s = 0;
	}

	return s;
}

bool TDirectory::isFile (const string &f)
{
	DECL_TRACER("Directory::isFile (const string &f)");

	try
	{
		return fs::is_regular_file(f);
	}
	catch(exception& e)
	{
		MSG_ERROR("Error: " << e.what());
	}

	return false;
}

bool TDirectory::isDirectory (const string &f)
{
	DECL_TRACER("Directory::isDirectory (const string &f)");

	try
	{
		return fs::is_directory(f);
	}
	catch(exception& e)
	{
		MSG_ERROR("Error: " << e.what());
	}

	return false;
}

bool TDirectory::exists (const string &f)
{
	DECL_TRACER("Directory::exists (const string &f)");

	try
	{
		return fs::exists(f);
	}
	catch(exception& e)
	{
		MSG_ERROR("Error: " << e.what());
	}

	return false;
}

bool TDirectory::checkDot (const string &s)
{
//	DECL_TRACER("Directory::checkDot (const string &s)");

	size_t pos = s.find_last_of("/");
	string f = s;

	if (pos != string::npos)
		f = s.substr(pos + 1);

	if (f.at(0) == '.')
		return true;

	return false;
}

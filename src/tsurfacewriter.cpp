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
#include <filesystem>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>

#include "tsurfacewriter.h"
#include "tresources.h"
#include "terror.h"

namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::exception;

TSurfaceWriter::TSurfaceWriter(const string& tmpPath, const string& target)
{
    DECL_TRACER("TSurfaceWriter::TSurfaceWriter(const string& tmpPath, const string& target)");

    if (!fs::is_directory(tmpPath))
    {
        MSG_ERROR("Invalid source directory " << tmpPath);
        mHaveError = true;
        return;
    }

    if (target.empty())
    {
        MSG_ERROR("No target file given!");
        mHaveError = true;
        return;
    }

    fs::path oldPath(fs::current_path());
    fs::current_path(tmpPath);
    // Here we iterate through the directory structure
    vector<string> list;

    try
	{
		for(auto& it: fs::directory_iterator("."))
		{
            string entry = fs::path(it.path()).filename();

            if (endsWith(entry, ".") || endsWith(entry, ".."))
                continue;

            list.push_back(entry);
        }
    }
	catch(exception& e)
	{
		MSG_ERROR("Error: " << e.what());
		return;
	}

    archiveFile(list, target);
    fs::current_path(oldPath);
}

void TSurfaceWriter::archiveFile(const vector<string>& files, const string& target)
{
    DECL_TRACER("TSurfaceWriter::archiveFile(const vector<string>& files, const string& target)");

    struct archive *a;
    struct archive_entry *entry;
    struct stat st;
    char buff[8192];
    int len;
    int fd;

    a = archive_write_new();
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);
    archive_write_open_filename(a, target.c_str());

    vector<string>::const_iterator iter;

    for (iter = files.cbegin(); iter != files.cend(); ++iter)
    {
        stat(iter->c_str(), &st);
        entry = archive_entry_new();
        archive_entry_set_pathname(entry, iter->c_str());
        archive_entry_copy_stat(entry, &st);

        if (fs::is_directory(*iter))
        {
            archive_entry_set_filetype(entry, AE_IFDIR);
            archive_entry_set_perm(entry, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        }
        else
        {
            archive_entry_set_filetype(entry, AE_IFREG);
            archive_entry_set_perm(entry, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }

        archive_write_header(a, entry);

        if ((fd = open(iter->c_str(), O_RDONLY)) < 0)
        {
            MSG_ERROR("Error reading file \"" << *iter << "\": " << strerror(errno));
            mHaveError = true;
            archive_entry_free(entry);
            continue;
        }

        len = read(fd, buff, sizeof(buff));

        while (len > 0)
        {
            archive_write_data(a, buff, len);
            len = read(fd, buff, sizeof(buff));
        }

        close(fd);
        archive_entry_free(entry);
    }

    archive_write_close(a);
    archive_write_free(a);
}

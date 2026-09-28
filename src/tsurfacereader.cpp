/*
 * Copyright (C) 2025, 2026 by Andreas Theofilu <andreas@theosys.at>
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
#include <cstdio>
#include <stdexcept>
#include <archive.h>
#include <archive_entry.h>

#include "tsurfacereader.h"
#include "terror.h"

namespace fs = std::filesystem;
using std::string;

TSurfaceReader::TSurfaceReader(const string& file, const string& temp)
    : mFile(file),
      mTarget(temp)
{
    DECL_TRACER("TSurfaceReader::TSurfaceReader(const QString& file, const QString& temp)");

    mState = dismantleFile();
}

TSurfaceReader::~TSurfaceReader()
{
    DECL_TRACER("TSurfaceReader::~TSurfaceReader()");
}

bool TSurfaceReader::dismantleFile()
{
    DECL_TRACER("TSurfaceReader::dismantleFile()");

    MSG_INFO("Reading file " << mFile << "...");

    if (mFile.empty() || !fs::is_regular_file(mFile))
    {
        MSG_ERROR("File " << mFile << " doesn't exist or is not readable!");
        return false;
    }

    fs::path p(mTarget);
    fs::path oldPath(fs::current_path());

    try
    {
        if (!fs::is_directory(p))
            fs::create_directories(p);

        fs::current_path(p);
        extract(mFile);
        MSG_PROTOCOL("File was extracted to " << mTarget);
        fs::current_path(oldPath);
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error extracting file " << mFile << ": " << e.what());

        if (fs::exists(p))
            fs::remove_all(p);

        return false;
    }

    return true;
}

/**
 * @brief TSurfaceReader::extract - Extract files from a tar.gz file.
 * This method takes the name of a file in the format tar or tar.gz, where
 * the extension doesn't matter. It creates a temporary directory and
 * extracts all files there. Directories are created as needed.
 *
 * In case of an error the method throws an exception.
 *
 * @param target_file_name  The name of a file archive.
 */
void TSurfaceReader::extract(const string& target_file_name)
{
    DECL_TRACER("TSurfaceReader::extract(const string& target_file_name)");

    struct archive *a = nullptr;
    struct archive *ext = nullptr;
    struct archive_entry *entry = nullptr;
    int flags, r;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_UNLINK;
    flags |= ARCHIVE_EXTRACT_SECURE_NODOTDOT;

    a = archive_read_new();

    if (!a)
    {
        throw std::runtime_error("Cannot archive_read_new");
    }

    archive_read_support_format_tar(a);
    archive_read_support_filter_gzip(a);

    ext = archive_write_disk_new();

    if (!ext)
    {
        throw std::runtime_error("Cannot archive_write_disk_new");
    }

    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    if (archive_read_open_filename(a, target_file_name.c_str(), 10240))
    {
        throw std::runtime_error("Cannot archive_read_open_filename");
    }

    for (;;)
    {
        r = archive_read_next_header(a, &entry);

        if (r == ARCHIVE_EOF)
            break;

        handle_errors(a, r, "Encountered error while reading header.");
        r = archive_write_header(ext, entry);

        if (r < ARCHIVE_OK)
        {
            MSG_ERROR(archive_error_string(ext));
        }
        else if (archive_entry_size(entry) > 0)
        {
            r = copy_data(a, ext);
            handle_errors(ext, r, "Encountered error while copy data.");
        }

        r = archive_write_finish_entry(ext);
        handle_errors(ext, r, "Encountered error while finishing entry.");
    }
}

int TSurfaceReader::copy_data(struct archive *ar, struct archive *aw)
{
//    DECL_TRACER("TSurfaceReader::copy_data(struct archive * ar, struct archive * aw)");

    int r;
    size_t size;
    const void *buff;
    int64_t offset;

    for (;;)
    {
        r = archive_read_data_block(ar, &buff, &size, &offset);

        if (r == ARCHIVE_EOF)
            return (ARCHIVE_OK);
        else if (r < ARCHIVE_OK)
            return (r);

        r = archive_write_data_block(aw, buff, size, offset);

        if (r < ARCHIVE_OK)
        {
            MSG_ERROR(archive_error_string(aw));
            return (r);
        }
    }

    return 0;   // Should never come here!
}

void TSurfaceReader::handle_errors(archive *a, int r, const string& msg)
{
//    DECL_TRACER("TSurfaceReader::handle_errors(archive *a, int r, const string& msg)");

    if (r < ARCHIVE_OK)
    {
        MSG_ERROR(archive_error_string(a));
    }

    if (r < ARCHIVE_WARN)
    {
        throw std::runtime_error(msg);
    }
}

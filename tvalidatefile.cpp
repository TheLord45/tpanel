/*
 * Copyright (C) 2020, 2021 by Andreas Theofilu <andreas@theosys.at>
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
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

#include "tvalidatefile.h"
#include "terror.h"
#include "tresources.h"

using std::string;
using std::vector;

bool TValidateFile::isValidFile(const string& file)
{
    DECL_TRACER("TValidateFile::isValidFile(const string& file)");

    struct stat buffer;

    if (stat (file.c_str(), &buffer) != 0)
    {
        MSG_WARNING("File access error: " << strerror(errno));
        return false;
    }

    if ((buffer.st_mode & S_IFREG) > 0)
        return true;

    return false;
}

bool TValidateFile::isValidDir(const string& path)
{
    DECL_TRACER("TValidateFile::isValidDir(const string& path)");

    struct stat buffer;

    if (stat (path.c_str(), &buffer) != 0)
        return false;

    if ((buffer.st_mode & S_IFDIR) > 0)
        return true;

    return false;
}

std::string &TValidateFile::makeFileName(const std::string& path, const std::string& name)
{
    DECL_TRACER("TValidateFile::makeFileName(const std::string& path, const std::string& name)");

    if (name.empty())
    {
        MSG_DEBUG("No file name given!");
        mFile.clear();
        return mFile;
    }

    if (path.empty())
        mFile = "./";
    else
        mFile = path + "/";

    mFile.append(name);
    return mFile;
}

bool TValidateFile::createPath(const string &path)
{
    DECL_TRACER("TValidateFile::createPath(const string &path)");

    if (path.length() == 0 || path == ".")
        return true;

    vector<string> parts = StrSplit(path, "/");
    vector<string>::iterator iter;
    bool absolut = false;
    string pPart;

    if (path[0] == '/')
        absolut = true;

    for (iter = parts.begin(); iter != parts.end(); ++iter)
    {
        if ((pPart.empty() && absolut) || !pPart.empty())
            pPart += "/" + *iter;
        else
            pPart += *iter;

        // Test for existence of the path part and whether it is a file or a directory.
        struct stat buffer;

        if (stat (pPart.data(), &buffer) == 0)      // If there exists something test what it is.
        {
            if (!S_ISDIR(buffer.st_mode) && !S_ISLNK(buffer.st_mode))
            {
                MSG_WARNING(pPart << " is not a directory!");
                return false;
            }
        }
        else    // We try to create it
        {
            if (mkdir(pPart.c_str(), S_IRWXU | S_IRWXG | S_IXOTH | S_IROTH) == -1)
            {
                MSG_ERROR("Error creating directory " << pPart << ": " << strerror(errno));
                return false;
            }
        }
    }

    return true;

}

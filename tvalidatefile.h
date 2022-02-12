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

#ifndef __TVALIDATEFILE_H__
#define __TVALIDATEFILE_H__

#include <string>

class TValidateFile
{
    public:
        TValidateFile() {}
        TValidateFile(const std::string& file) { mFile = file; }

        bool isValidFile() { return isValidFile(mFile); }
        bool isValidFile(const std::string& file);
        bool isValidDir() { return isValidDir(mFile); }
        bool isValidDir(const std::string& path);
        std::string& makeFileName(const std::string& path, const std::string& name);
        std::string& getFileName() { return mFile; }
        static bool createPath(const std::string& path);

    private:
        std::string mFile;
};

#endif

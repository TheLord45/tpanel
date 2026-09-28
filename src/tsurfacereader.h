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

#ifndef TSURFACEREADER_H
#define TSURFACEREADER_H

#include <string>
#include <archive.h>

/**
 * @todo write docs
 */
class TSurfaceReader
{
    public:
        /**
        * Default constructor
        */
        TSurfaceReader(const std::string& file, const std::string& temp);

        /**
        * Destructor
        */
        ~TSurfaceReader();

        std::string& getTarget() { return mTarget; }
        bool lastState() { return mState; }

    protected:
        bool dismantleFile();
        void extract(const std::string& target_file_name);
        void handle_errors(archive *a, int r, const std::string& msg);
        int copy_data(struct archive *ar, struct archive *aw);

    private:
        std::string mFile;
        std::string mTarget;
        bool mState{false};
};

#endif // TSURFACEREADER_H

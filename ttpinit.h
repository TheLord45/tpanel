/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef TTPINIT_H
#define TTPINIT_H

#include <string>

class TTPInit
{
    public:
        TTPInit(const std::string& path);
        TTPInit();

        void setPath(const std::string& p) { mPath = p; }
        bool createDirectoryStructure();

    private:
        bool createPanelConfigs();
        bool _makeDir(const std::string& dir);
#ifdef __ANDROID__
        bool askPermissions();
#endif
        std::string mPath;
};

#endif // TTPINIT_H

/*
 * Copyright (C) 2025 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef TLAUNCHER_H
#define TLAUNCHER_H

#include <string>
#include <vector>

typedef struct APPDEF_t
{
    std::string name;
    std::vector<std::string> executeables;
}APPDEF_t;

class TLauncher
{
    public:
        static bool launch(const std::string& name);

    protected:
        static void initialize();

    private:
        TLauncher() {};     // Should never be called directly!
        static bool mInitialized;
        static std::vector<APPDEF_t> APPS;
};

#endif // TLAUNCHER_H

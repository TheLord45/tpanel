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

#ifndef __TPALETTE_H__
#define __TPALETTE_H__

#include <string>
#include <map>
#include <vector>
#include "tvalidatefile.h"

typedef struct PDATA_T
{
    int index{0};
    std::string name;
    unsigned long color{0};

    void clear()
    {
        index = 0;
        name.clear();
        color = 0;
    }
}PDATA_T;

class TPalette : public TValidateFile
{
    public:
        TPalette();
        TPalette(const std::string& file);
        ~TPalette();

        void initialize(const std::string& file);
        PDATA_T findColor(const std::string& name);
        PDATA_T findColor(int pID);
        void reset();

    private:
        bool havePalette(const std::string& name);
        void addSystemColors();

        std::map<std::string, PDATA_T> mColors;
        std::vector<std::string> mPaletteNames;
};

#endif

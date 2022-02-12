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

#ifndef __TICON_H__
#define __TICON_H__

#include <string>
#include <map>
#include "tvalidatefile.h"

class TIcons : public TValidateFile
{
    public:
        TIcons();
        ~TIcons();

        void initialize();
        std::string getFile(int number);
        int getNumber(const std::string& file);

    private:
        std::map<int, std::string> mIcons;
        int mEntries{0};
};

#endif

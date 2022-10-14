/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TMAP_H__
#define __TMAP_H__

#include <string>
#include <vector>

#include "tvalidatefile.h"

// Pages with buttons
class TMap : public TValidateFile
{
    public:
        TMap(const std::string &file);

        typedef enum MAP_TYPE
        {
            TYPE_CM = 1,    // ON / OFF
            TYPE_AM,        // TXT, ...
            TYPE_LM        // Bargraphs
        }MAP_TYPE;

        typedef struct MAP_T
        {
            int p{0};           // port number
            int c{0};           // channel number
            int ax{0};          // ??
            int pg{0};          // page number
            int bt{0};          // button number
            std::string pn;     // page name
            std::string bn;     // button name
        }MAP_T;

        // Images
        typedef struct MAP_BM_T
        {
            std::string i;      // image file name
            int id{0};
            int rt{0};
            int pg{0};          // page number
            int bt{0};          // button number
            int st{0};          // button instance
            int sl{0};
            std::string pn;     // page name
            std::string bn;     // button name
        }MAP_BM_T;

        typedef struct MAP_PM_T
        {
            int a{0};
            std::string t;      // Group name
            int pg{0};          // page number
            int bt{0};          // button number
            std::string pn;     // page name
            std::string bn;     // button name
        }MAP_PM_T;

        typedef struct MAPS_T
        {
            std::vector<MAP_T> map_cm;
            std::vector<MAP_T> map_am;
            std::vector<MAP_T> map_lm;
            std::vector<MAP_BM_T> map_bm;       // Images
            std::vector<std::string> map_sm;    // sound file names
            std::vector<MAP_T> map_strm;        // System resources
            std::vector<MAP_PM_T> map_pm;       // Button -> text
        }MAPS_T;

        std::vector<MAP_T> findButtons(int port, std::vector<int>& channels, MAP_TYPE mt = TYPE_AM);
        std::vector<MAP_T> findButtonByName(const std::string& name);
        std::string findImage(int bt, int page, int instance=0);
        std::string findImage(const std::string& name);
        std::vector<MAP_T> findBargraphs(int port, std::vector<int>& channels);
        std::vector<std::string> findSounds();
        bool soundExist(const std::string& sname);

        bool haveError() { return mError; }

    protected:
        bool readMap();

    private:
        std::string mFile;
        MAPS_T mMap;
        bool mError{false};
};

#endif

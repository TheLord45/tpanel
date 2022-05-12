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
#ifndef TEXTERNAL_H
#define TEXTERNAL_H

#include <string>
#include <vector>

#include "tvalidatefile.h"

typedef enum extButtons_t
{
    EXT_NOBUTTON,
    EXT_CURSOR_LEFT,
    EXT_CURSOR_RIGHT,
    EXT_CURSOR_UP,
    EXT_CURSOR_DOWN,
    EXT_CURSOR_SELECT,
    EXT_CURSOR_ROTATE_LEFT,
    EXT_CURSOR_ROTATE_RIGHT,
    EXT_GESTURE_LEFT,
    EXT_GESTURE_RIGHT,
    EXT_GESTURE_UP,
    EXT_GESTURE_DOWN,
    EXT_GESTURE_ROTATE_LEFT,
    EXT_GESTURE_ROTATE_RIGHT,
    EXT_GESTURE_DOUBLE_PRESS,
    EXT_GENERAL
}extButtons_t;

typedef struct EXTBUTTON_t
{
    int bi{0};              //!< Button index
    extButtons_t type{EXT_NOBUTTON};  //!< The internal distinct button type
    std::string bc;         //!< Technical name of button
    std::string na;         //!< Name of button
    int da{0};
    std::string pp;
    int ap{1};              //!< Address port (default: 1)
    int ad{0};              //!< Address channel
    int cp{1};              //!< Channel port
    int ch{0};              //!< Channel number
    std::string rt;         //!< ? (default byte)
    std::string vt;
    int lp{1};              //!< Level port
    int lv{0};              //!< Level code
    int va{0};
    int rv{0};
    int rl{0};              //!< Level range low
    int rh{255};            //!< Level range high
    int lu{2};              //!< Level time up
    int ld{2};              //!< Level time down
    int so{1};
    int co{1};              //!< Port number for command list
    std::string ac;
    std::vector<std::string>cm; //!< Commands to self feed or to send to the NetLinx
    int ac_de{0};
    int at{0};
}EXBUTTON_t;

typedef struct EXTPAGE_t
{
    int pageID;         //!< The ID of the page
    std::string name;   //!< The name of the page (empty by default)
    std::vector<EXTBUTTON_t> buttons;
}EXTPAGE_t;

class TExternal : public TValidateFile
{
    public:
        TExternal();

        void setFileName(const std::string& fn);
        EXTBUTTON_t getButton(extButtons_t bt);
        EXTBUTTON_t getButton(int pageId, extButtons_t bt);
        void setStrict(bool s) { mStrict = s; }
        bool getStrict() { return mStrict; }

    private:
        void initialize();
        extButtons_t findCompatibel(extButtons_t bt);

        std::string mFileName;          //!< The file name of the configuration file
        std::vector<EXTPAGE_t> mPages;  //!< External pages and buttons
        bool mStrict{true};             //!< TRUE = exact button comparison; FALSE = compatible button compare
};

#endif // TEXTERNAL_H

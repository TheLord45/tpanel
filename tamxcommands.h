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

#ifndef __TAMXCOMMANDS_H__
#define __TAMXCOMMANDS_H__

#include <string>
#include <vector>
#include <functional>

#include "tbutton.h"
#include "tvalidatefile.h"

class TPage;
class TSubPage;

typedef struct PCHAIN_T
{
    TPage *page{nullptr};
    PCHAIN_T *next{nullptr};
}PCHAIN_T;

typedef struct SPCHAIN_T
{
    TSubPage *page{nullptr};
    SPCHAIN_T *next{nullptr};
}SPCHAIN_T;

// Pages with buttons
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
    int st{0};
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

class TAmxCommands : public TValidateFile
{
    public:
        typedef struct CMD_TABLE
        {
            std::string cmd;                // The command (^TXT, ^BMP, ...)
            std::vector<int> channels;      // Channels the command affects (200&210, ...)
            std::vector<std::string> pars;  // Rest of parameters of the command
            std::function<void (int port, std::vector<int>& channels, std::vector<std::string>& pars)> command{nullptr};
        }CMD_TABLE;

        typedef enum MAP_TYPE
        {
            TYPE_CM = 1,    // ON / OFF
            TYPE_AM,        // TXT, ...
            TYPE_LM         // Bargraphs
        }MAP_TYPE;

        TAmxCommands();
        ~TAmxCommands();

        void setPChain(PCHAIN_T *ch) { mPChain = ch; }
        void setSPChain(SPCHAIN_T *ch) { mSPChain = ch; }
        bool parseCommand(int device, int port, const std::string& cmd);
        std::vector<MAP_T> findButtons(int port, std::vector<int>& channels, MAP_TYPE mt = TYPE_AM);
        std::vector<MAP_T> findButtonByName(const std::string& name);
        std::vector<MAP_T> findBargraphs(int port, std::vector<int>& channels);
        std::vector<std::string> findSounds();
        bool soundExist(const std::string sname);
        void registerCommand(std::function<void (int port, std::vector<int>& channels, std::vector<std::string>& pars)> command, const std::string& name);

    protected:
        bool readMap();

    private:
        bool extractChannels(const std::string& schan, std::vector<int> *ch);
        std::vector<std::string> getFields(std::string& msg, char sep);

        std::vector<CMD_TABLE> mCmdTable;
        PCHAIN_T *mPChain{nullptr};
        SPCHAIN_T *mSPChain{nullptr};
        MAPS_T mMap;
};

#endif

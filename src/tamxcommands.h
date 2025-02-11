/*
 * Copyright (C) 2020 to 2025 by Andreas Theofilu <andreas@theosys.at>
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
#include "tmap.h"

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

        TAmxCommands();
        ~TAmxCommands();

        void setPChain(PCHAIN_T *ch) { mPChain = ch; }
        void setSPChain(SPCHAIN_T *ch) { mSPChain = ch; }
        bool parseCommand(int device, int port, const std::string& cmd);
        std::vector<TMap::MAP_T> findButtons(int port, std::vector<int>& channels, TMap::MAP_TYPE mt = TMap::TYPE_AM);
        std::string findImage(int bt, int page, int instance=0);
        std::string findImage(const std::string& name);
        std::vector<TMap::MAP_T> findButtonByName(const std::string& name);
        std::vector<TMap::MAP_T> findBargraphs(int port, std::vector<int>& channels);
        std::vector<std::string> findSounds();
        bool soundExist(const std::string& sname);
        void registerCommand(std::function<void (int port, std::vector<int>& channels, std::vector<std::string>& pars)> command, const std::string& name);

    protected:
        bool readMap(bool tp5=false);

    private:
        bool extractChannels(const std::string& schan, std::vector<int> *ch);
        std::vector<std::string> getFields(std::string& msg, char sep);

        std::vector<CMD_TABLE> mCmdTable;
        PCHAIN_T *mPChain{nullptr};
        SPCHAIN_T *mSPChain{nullptr};
        TMap *mMap{nullptr};                // Map data of panel file
        TMap *mSystemMap{nullptr};          // Map data of system settings
};

#endif

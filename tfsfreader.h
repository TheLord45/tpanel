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

#ifndef __TFSFREADER_H__
#define __TFSFREADER_H__

#include <string>
#include <vector>
#include <functional>

#include "ftplib/ftplib.h"

#define _BUF_SIZE   1024

class TFsfReader
{
    public:
        typedef struct FTP_CMDS_t
        {
            std::string cmd;
            int success;
        }FTP_CMDS_t;

        TFsfReader();
        ~TFsfReader();

        bool copyOverFTP(const std::string& fname, const std::string& target);
        bool unpack(const std::string&fname, const std::string& path);
        static void callbackLog(char *str, void* arg, bool out);
        static void callbackError(char *msg, void *arg, int err);
        static int callbackXfer(off64_t xfered, void *arg);

        static void regCallbackProgress(std::function<int (off64_t xfered)> progress) { _progress = progress; }

    private:
        ftplib *mFtpLib{nullptr};
        static std::function<int (off64_t xfered)> _progress;
};

#endif

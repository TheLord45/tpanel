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

#include <fstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>

#include "tfsfreader.h"
#include "tsocket.h"
#include "tconfig.h"
#include "terror.h"
#include "tresources.h"
#include "readtp4.h"

#define FTP_PORT    21

#define FTP_CMD_USER    0
#define FTP_CMD_PASS    1
#define FTP_CMD_PASV    2
#define FTP_CMD_TYPE    3
#define FTP_CMD_RETR    4
#define FTP_CMD_QUIT    5

using std::string;
using std::vector;

TFsfReader::TFsfReader()
{
    DECL_TRACER("TFsfReader::TFsfReader()");

    mFtpCmds = {
        { "USER", 331 },    // 0
        { "PASS", 230 },    // 1
        { "PASV", 227 },    // 2
        { "TYPE", 200 },    // 3
        { "RETR", 150 },    // 4
        { "QUIT", 221 }     // 5
    };
}

TFsfReader::~TFsfReader()
{
    DECL_TRACER("TFsfReader::~TFsfReader()");

    if (mFtp)
        delete mFtp;

    if (mFtpData)
        delete mFtpData;
}

bool TFsfReader::sendCommand(int cmd, const string& param)
{
    DECL_TRACER("TFsfReader::sendCommand(int cmd, const string& param)");

    if (cmd < 0 || cmd > (int)mFtpCmds.size() || !mFtp)
        return false;

    string buf;

    if (!param.empty())
        buf = mFtpCmds[cmd].cmd + "\r\n";
    else
        buf = mFtpCmds[cmd].cmd + " " + param + "\r\n";

    if (mFtp->send((char *)buf.c_str(), buf.length()) != TSocket::npos)
        return false;

    char buffer[128];
    size_t size;
    memset(buffer, 0, sizeof(buffer));

    if ((size = mFtp->receive(buffer, sizeof(buffer))) == TSocket::npos)
        return false;

    mLastCmdBuffer = buffer;
    int code = 0;

    if (mFtpCmds[cmd].success && !isdigit(buffer[0]))
    {
        MSG_ERROR("Unexpected answer: " << buffer);
        return false;
    }
    else if (mFtpCmds[cmd].success)
        code = atoi(buffer);

    if (code != mFtpCmds[cmd].success)
    {
        MSG_ERROR("FTP error: " << buffer);
        return false;
    }

    MSG_DEBUG("FTP command " << mFtpCmds[cmd].cmd << " returned: " << buffer);
    return true;
}

bool TFsfReader::copyOverFTP(const string& fname, const string& target)
{
    DECL_TRACER("TFsfReader::copyOverFTP(const string& fname, const string& target, const string& user, const string& pw)");

    if (mFtp)
        delete mFtp;

    mFtp = new TSocket(TConfig::getController(), FTP_PORT);

    string buf, filename;
    char *f, buffer[128];
    size_t size;

    if (!mFtp->connect())
        return false;

    // Server should answer with: 220 <message>
    memset(buffer, 0, sizeof(buffer));

    if ((size = mFtp->receive(buffer, sizeof(buffer))) == TSocket::npos)
    {
        mFtp->close();
        return false;
    }

    if (!startsWith(buffer, "220 "))
    {
        MSG_ERROR("Unexpected answer: " << buffer);
        mFtp->close();
        return false;
    }

    // We send the authentication
    if (!sendCommand(FTP_CMD_USER, TConfig::getFtpUser()))
    {
        mFtp->close();
        return false;
    }

    // Here we send the password
    if (!sendCommand(FTP_CMD_PASS, TConfig::getFtpPassword()))
    {
        mFtp->close();
        return false;
    }

    // Switching to passive mode
    if (!sendCommand(FTP_CMD_PASV, ""))
    {
        mFtp->close();
        return false;
    }

    size_t pos = mLastCmdBuffer.find_last_of("(");

    if (pos != string::npos)
    {
        size_t end = mLastCmdBuffer.find_last_of(")");

        if (end != string::npos)
        {
            string info = mLastCmdBuffer.substr(pos + 1, end-pos-2);
            MSG_DEBUG("PASV " << info);
            vector<string> parts = StrSplit(info, ",");

            if (parts.size() < 6)
            {
                MSG_ERROR("PASV command failed!");
            }
            else
            {
                mDataPort = atoi(parts[4].c_str()) * 256 + atoi(parts[5].c_str());
                mPassive = true;
            }
        }
    }
    else
    {
        MSG_ERROR("Unexpected response from FTP server!");
        mFtp->close();
        return false;
    }

    // We connect to the data port
    if (mFtpData)
        delete mFtpData;

    mFtpData = new TSocket(TConfig::getController(), mDataPort);

    if (!mFtpData->connect())
    {
        mFtp->close();
        return false;
    }

    // Switch to image mode
    if (!sendCommand(FTP_CMD_TYPE, "I"))
    {
        mFtp->close();
        return false;
    }

    // GET a file
    if (!sendCommand(FTP_CMD_RETR, fname))
    {
        mFtp->close();
        return false;
    }

    size = 0;
    pos = mLastCmdBuffer.find_last_of("(");

    if (pos != string::npos)
    {
        string info = mLastCmdBuffer.substr(pos + 1);
        MSG_DEBUG("RETR size: " << info);
        size = atoi(info.c_str());
    }
    else
    {
        MSG_ERROR("Unexpected response: " << mLastCmdBuffer);
        mFtp->close();
        return false;
    }

    // FIXME: Add code to receive file
    f = new char[size+1];

    if (mFtpData->readAbsolut(f, size) == TSocket::npos)
    {
        MSG_ERROR("Error receiving file " << fname << " from FTP server!");
        mFtpData->close();
        mFtp->close();
        delete[] f;
        return false;
    }

    // Write the file into place
    try
    {
        std::ofstream file;

        file.open(target, std::ios_base::binary);
        file.write(f, size);
        file.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error on file " << target << ": " << e.what());
        mFtpData->close();
        mFtp->close();
        delete[] f;
        return false;
    }

    // QUIT from FTP-server
    if (!sendCommand(FTP_CMD_QUIT, ""))
    {
        mFtp->close();
        mFtpData->close();
        return false;
    }

    mFtp->close();
    mFtpData->close();
    return true;
}

bool TFsfReader::unpack(const string& fname, const string& path)
{
    DECL_TRACER("TFsfReader::unpack(const string& fname, const string& path)");

    if (fname.empty() || path.empty())
    {
        MSG_ERROR("Invalid parameters!");
        return false;
    }

    reader::ReadTP4 readtp4(fname, path);
    return readtp4.doRead();
}

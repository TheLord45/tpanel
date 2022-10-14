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
#ifndef __TERROR_H__
#define __TERROR_H__

#include <iostream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

#define LPATH_FILE          1   //!< Creates a log file and protocolls there
#define LPATH_SYSLOG        2   //!< Writes to the syslog.
#define LOGPATH             LPATH_FILE

#define HLOG_NONE           0x0000
#define HLOG_INFO           0x0001
#define HLOG_WARNING        0x0002
#define HLOG_ERROR          0x0004
#define HLOG_TRACE          0x0008
#define HLOG_DEBUG          0x0010
#define HLOG_PROTOCOL       (HLOG_INFO | HLOG_WARNING | HLOG_ERROR)
#define HLOG_ALL            (HLOG_INFO | HLOG_WARNING | HLOG_ERROR | HLOG_TRACE | HLOG_DEBUG)

#define SLOG_NONE           "NONE"
#define SLOG_INFO           "INFO"
#define SLOG_WARNING        "WARNING"
#define SLOG_ERROR          "ERROR"
#define SLOG_TRACE          "TRACE"
#define SLOG_DEBUG          "DEBUG"
#define SLOG_PROTOCOL       "PROTOCOL"
#define SLOG_ALL            "ALL"

typedef enum terrtype_t
{
    TERRNONE,
    TERRINFO,
    TERRWARNING,
    TERRERROR,
    TERRTRACE,
    TERRDEBUG
}terrtype_t;

std::ostream& indent(std::ostream& os);

class TStreamError
{
    public:
        TStreamError(const std::string& logFile, const std::string& logLevel);
        ~TStreamError();

        static void setLogFile(const std::string& lf);
        static void setLogFileOnly(const std::string& lf) { mLogfile = lf; }
        static std::string& getLogFile() { return mLogfile; }
        static void setLogLevel(const std::string& slv);
        static void setLogLevel(unsigned int ll) { mLogLevel = ll; }
        static unsigned int getLogLevel() { return mLogLevel; }
        static void logMsg(std::ostream& str);
        static bool checkFilter(terrtype_t err);
        static bool checkFilter(int lv);
        friend std::ostream& indent(std::ostream& os);
        static void incIndent() { mIndent++; }
        static void decIndent();
        static int getIndent() { return mIndent; }
        static std::ostream *getStream() { return mStream; }
        static std::string getTime();
        static std::ostream *resetFlags(std::ostream *os);

    private:
        static unsigned int _getLevel(const std::string& slv);
        static void _init();

        const TStreamError& operator=(const TStreamError& ref);

        static bool mInitialized;
        static std::string mLogfile;
        static unsigned int mLogLevel;
        static int mIndent;
        static std::ostream *mStream;
};

class TTracer
{
    public:
        TTracer(const std::string& msg, int line, char *file);
        ~TTracer();

    private:
        std::string mHeadMsg;
        int mLine{0};
        std::string mFile;
        std::chrono::steady_clock::time_point mTimePoint;
};

class TError : public std::ostream
{
    public:
        static void setErrorMsg(const std::string& msg);
        static void setErrorMsg(terrtype_t t, const std::string& msg);
        static void setError() { mHaveError = true; }
        static std::string& getErrorMsg() { return msError; }
        static bool isError() { return mHaveError; }
        static bool haveErrorMsg() { return !msError.empty(); }
        static terrtype_t getErrorType() { return mErrType; }
        static void setErrorType(terrtype_t et) { mErrType = et; }
        static std::ostream& append(int lv, std::ostream& os);
        static TStreamError* Current();
        static void clear() { mHaveError = false; msError.clear(); mErrType = TERRNONE; }
        static void logHex(char *str, size_t size);
        const TError& operator=(const TError& ref);
        static void lock();
        static void unlock();
#if defined(__linux__) || defined(Q_OS_ANDROID)
        static void displayMessage(const std::string& msg);
#endif

    protected:
        static std::string strToHex(const char *str, size_t size, int width, bool format, int indent);

    private:
        static std::string toHex(int num, int width);
        TError() {};
        ~TError();

        static std::string msError;
        static bool mHaveError;
        static terrtype_t mErrType;
        static TStreamError *mCurrent;
        std::string mHeadMsg;
};

#define MSG_INFO(msg)       { TError::lock(); if (TStreamError::checkFilter(HLOG_INFO)) { TError::Current()->logMsg(TError::append(HLOG_INFO, *TStreamError::getStream()) << msg << std::endl); } TError::unlock(); }
#define MSG_WARNING(msg)    { TError::lock(); if (TStreamError::checkFilter(HLOG_WARNING)) { TError::Current()->logMsg(TError::append(HLOG_WARNING, *TStreamError::getStream()) << msg << std::endl); } TError::unlock(); }
#define MSG_ERROR(msg)      { TError::lock(); if (TStreamError::checkFilter(HLOG_ERROR)) { TError::Current()->logMsg(TError::append(HLOG_ERROR, *TStreamError::getStream()) << msg << std::endl); } TError::unlock(); }
#define MSG_TRACE(msg)      { TError::lock(); if (TStreamError::checkFilter(HLOG_TRACE)) { TError::Current()->logMsg(TError::append(HLOG_TRACE, *TStreamError::getStream()) << msg << std::endl); } TError::unlock(); }
#define MSG_DEBUG(msg)      { TError::lock(); if (TStreamError::checkFilter(HLOG_DEBUG)) { TError::Current()->logMsg(TError::append(HLOG_DEBUG, *TStreamError::getStream()) << msg << std::endl); } TError::unlock(); }

#define MSG_PROTOCOL(msg)   { TError::lock(); if (TStreamError::checkFilter(HLOG_PROTOCOL)) { TError::Current()->logMsg(TError::append(HLOG_PROTOCOL, *TStreamError::getStream()) << msg << std::endl); } TError::unlock(); }

#define DECL_TRACER(msg)    TTracer _hidden_tracer(msg, __LINE__, (char *)__FILE__);

#define IS_LOG_INFO()       TStreamError::checkFilter(HLOG_INFO)
#define IS_LOG_WARNING()    TStreamError::checkFilter(HLOG_WARNING)
#define IS_LOG_ERROR()      TStreamError::checkFilter(HLOG_ERROR)
#define IS_LOG_TRACE()      TStreamError::checkFilter(HLOG_TRACE)
#define IS_LOG_DEBUG()      TStreamError::checkFilter(HLOG_DEBUG)
#define IS_LOG_PROTOCOL()   TStreamError::checkFilter(HLOG_PROTOCOL)
#define IS_LOG_ALL()        TStreamError::checkFilter(HLOG_ALL)

#endif

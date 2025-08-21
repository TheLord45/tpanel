/*
 * Copyright (C) 2020 to 2024 by Andreas Theofilu <andreas@theosys.at>
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
#include <iostream>
#include <fstream>
#include <sstream>
#include <ios>
#include <time.h>
#include <mutex>
#include <thread>
//#ifdef __APPLE__
//#include <unistd.h>
//#include <sys/syscall.h>
//#endif

#include <QMessageBox>
#include <QTimer>

#include "terror.h"
#include "tconfig.h"

#if __cplusplus < 201402L
#   error "This module requires at least C++ 14 standard!"
#else
#   if __cplusplus < 201703L
#       include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#       warning "Support for C++14 and experimental filesystem will be removed in a future version!"
#   else
#       include <filesystem>
#       ifdef __ANDROID__
namespace fs = std::__fs::filesystem;
#       else
namespace fs = std::filesystem;
#       endif
#   endif
#endif

#if LOGPATH == LPATH_SYSLOG || defined(__ANDROID__)
#   ifdef __ANDROID__
#       include <android/log.h>
#   else
#       include <syslog.h>
#       ifdef __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
#           include "ios/QASettings.h"
#       endif
#   endif
#endif

#define LOGBUFFER_SIZE      4096

using std::string;
using std::mutex;
using std::stringstream;

std::mutex message_mutex;
std::mutex _macro_mutex;

bool TError::mHaveError{false};
terrtype_t TError::mErrType{TERRNONE};
TStreamError *TError::mCurrent{nullptr};
string TError::msError;
int TError::mLastLine{0};
string TError::mLastFile;
#ifdef __ANDROID__
threadID_t TError::mThreadID;
#else
threadID_t TError::mThreadID{0};
#endif

int TStreamError::mIndent{1};
std::ostream *TStreamError::mStream{nullptr};
std::filebuf TStreamError::mOfStream;
char *TStreamError::mBuffer{nullptr};
std::string TStreamError::mLogfile;
bool TStreamError::mInitialized{false};
unsigned int TStreamError::mLogLevel{HLOG_PROTOCOL};
unsigned int TStreamError::mLogLevelOld{HLOG_NONE};
bool TStreamError::haveTemporaryLogLevel{false};
#ifdef __ANDROID__
bool TStreamError::mLogFileEnabled{false};
#else
bool TStreamError::mLogFileEnabled{true};
#endif

string _threadIDtoStr(threadID_t tid)
{
    std::stringstream s;
    s << std::hex << std::setw(8) << std::setfill('0') << tid;
    return s.str();
}

threadID_t _getThreadID()
{
#ifdef __APPLE__
//    threadID_t tid;
//    return pthread_threadid_np(NULL, &tid);
    return pthread_mach_thread_np(pthread_self());
//    return syscall(SYS_thread_selfid);
#else
    return std::this_thread::get_id();
#endif
}

void _lock()
{
    _macro_mutex.lock();
}

void _unlock()
{
    _macro_mutex.unlock();
}

#if LOGPATH == LPATH_SYSLOG || defined(__ANDROID__)
class androidbuf : public std::streambuf
{
    public:
        enum { bufsize = 1024 };
        androidbuf() { this->setp(buffer, buffer + bufsize - 1); }

    private:
        int overflow(int c)
        {
            if (c == traits_type::eof())
            {
                *this->pptr() = traits_type::to_char_type(c);
                this->sbumpc();
            }

            return this->sync()? traits_type::eof(): traits_type::not_eof(c);
        }

        int sync()
        {
            int rc = 0;

            if (this->pbase() != this->pptr())
            {
                char writebuf[bufsize+1];
                memcpy(writebuf, this->pbase(), this->pptr() - this->pbase());
                writebuf[this->pptr() - this->pbase()] = '\0';
                int eType;
#ifdef __ANDROID__
                switch(TError::getErrorType())
                {
                    case TERRINFO:      eType = ANDROID_LOG_INFO; break;
                    case TERRWARNING:   eType = ANDROID_LOG_WARN; break;
                    case TERRERROR:     eType = ANDROID_LOG_ERROR; break;
                    case TERRTRACE:     eType = ANDROID_LOG_VERBOSE; break;
                    case TERRDEBUG:     eType = ANDROID_LOG_DEBUG; break;
                    case TERRNONE:      eType = ANDROID_LOG_INFO; break;
                }

                rc = __android_log_print(eType, "tpanel", "%s", writebuf) > 0;
#else
#ifdef Q_OS_IOS
                QASettings::writeLog(TError::getErrorType(), writebuf);
#else
                switch(TError::getErrorType())
                {
                    case TERRINFO:      eType = LOG_INFO; break;
                    case TERRWARNING:   eType = LOG_WARNING; break;
                    case TERRERROR:     eType = LOG_ERR; break;
                    case TERRTRACE:     eType = LOG_INFO; break;
                    case TERRDEBUG:     eType = LOG_DEBUG; break;
                    case TERRNONE:      eType = LOG_INFO; break;
                }

                syslog(eType, "(tpanel) %s", writebuf);
                rc = 1;
#endif  // Q_OS_IOS
#endif  // __ANDROID__
                this->setp(buffer, buffer + bufsize - 1);
            }

            return rc;
        }

        char buffer[bufsize];
};
#endif

TStreamError::TStreamError(const string& logFile, const std::string& logLevel)
{
    if (!TConfig::isInitialized())
        return;

    if (!logFile.empty())
        mLogfile = logFile;
    else if (!TConfig::getLogFile().empty())
        mLogfile = TConfig::getLogFile();

    if (!logLevel.empty())
        setLogLevel(logLevel);
    else if (!TConfig::getLogLevel().empty())
        setLogLevel(TConfig::getLogLevel());

    _init();
}

TStreamError::~TStreamError()
{
    if (mOfStream.is_open())
        mOfStream.close();

    if (mStream)
    {
        delete mStream;
        mStream = nullptr;
    }

    if (mBuffer)
        delete mBuffer;

    mInitialized = false;
}

void TStreamError::setLogFile(const std::string &lf)
{
#ifdef Q_OS_IOS
    if (!lf.empty())
    {
        if ((!mLogfile.empty() && mLogfile != lf) || mLogfile.empty())
            mLogfile = lf;
    }

    if (!mInitialized)
        _init();
#else
#ifndef __ANDROID__
    if (mInitialized && mLogfile.compare(lf) == 0)
        return;
#endif
    mLogfile = lf;
    mInitialized = false;
    _init();
#endif
}

void TStreamError::setLogLevel(const std::string& slv)
{
    size_t pos = slv.find("|");
    size_t start = 0;
    string lv;
    mLogLevel = 0;

    while (pos != string::npos)
    {
        lv = slv.substr(start, pos - start);
        start = pos + 1;
#ifndef NDEBUG
        mLogLevel |= _getLevel(lv);
#else
        unsigned int llv = _getLevel(lv);

        if (llv != HLOG_DEBUG && llv != HLOG_TRACE)
            mLogLevel |= llv;
#endif
        pos = slv.find("|", start);
    }

#ifdef NDEBUG
    unsigned int llv = _getLevel(lv);

    if (llv != HLOG_DEBUG && llv != HLOG_TRACE)
        mLogLevel |= llv;
#else
    mLogLevel |= _getLevel(slv.substr(start));
#endif
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "tpanel", "TStreamError::setLogLevel: New loglevel: %s", slv.c_str());
#else
#ifdef TARGET_OS_IOS
    std::cout << TError::append(HLOG_INFO, __LINE__, __FILE__) << "New loglevel: " << slv << std::endl;
#else
    if (mInitialized && mStream)
        *mStream << TError::append(HLOG_INFO, 0, "") << "New loglevel: " << slv << std::endl;
    else
        std::cout << TError::append(HLOG_INFO, 0, "") << "New loglevel: " << slv << std::endl;
#endif
#endif
}

bool TStreamError::checkFilter(terrtype_t err)
{
    if (!TConfig::isInitialized())
        return false;

    if (err == TERRINFO && (mLogLevel & HLOG_INFO) != 0)
        return true;
    else if (err == TERRWARNING && (mLogLevel & HLOG_WARNING) != 0)
        return true;
    else if (err == TERRERROR && (mLogLevel & HLOG_ERROR) != 0)
        return true;
#ifndef NDEBUG
    else if (err == TERRTRACE && (mLogLevel & HLOG_TRACE) != 0)
        return true;
    else if (err == TERRDEBUG && (mLogLevel & HLOG_DEBUG) != 0)
        return true;
#endif
    return false;
}

bool TStreamError::checkFilter(unsigned int lv)
{
    if (!TConfig::isInitialized())
        return false;

    if ((mLogLevel & HLOG_INFO) != 0 &&
        (mLogLevel & HLOG_WARNING) != 0 &&
        (mLogLevel & HLOG_ERROR) != 0 &&
        lv == HLOG_PROTOCOL)
        return true;

    if ((mLogLevel & lv) != 0 && lv != HLOG_PROTOCOL)
    {
#ifdef NDEBUG
        if (lv == HLOG_DEBUG || lv == HLOG_TRACE)
            return false;
#endif
        return true;
    }

    return false;
}

unsigned int TStreamError::_getLevel(const std::string& slv)
{
    if (slv.compare(SLOG_NONE) == 0)
        return HLOG_NONE;

    if (slv.compare(SLOG_INFO) == 0)
        return HLOG_INFO;

    if (slv.compare(SLOG_WARNING) == 0)
        return HLOG_WARNING;

    if (slv.compare(SLOG_ERROR) == 0)
        return HLOG_ERROR;

    if (slv.compare(SLOG_PROTOCOL) == 0)
        return HLOG_PROTOCOL;
#ifndef NDEBUG
    if (slv.compare(SLOG_TRACE) == 0)
        return HLOG_TRACE;

    if (slv.compare(SLOG_DEBUG) == 0)
        return HLOG_DEBUG;

    if (slv.compare(SLOG_ALL) == 0)
        return HLOG_ALL;
#endif

    return HLOG_NONE;
}

void TStreamError::_init(bool reinit)
{
    if (!TConfig::isInitialized() || mInitialized)
        return;

    mInitialized = true;

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, "tpanel", "TStreamError::_init: Logfile is %s", (mLogFileEnabled ? "ENABLED" : "DISABLED"));
#endif

#if LOGPATH == LPATH_FILE
    if (mLogFileEnabled && !mLogfile.empty())
    {
        try
        {
#ifndef __ANDROID__
            if (mOfStream.is_open())
                mOfStream.close();

            if (mStream && mStream != &std::cout)
                delete mStream;

            if (!mBuffer)
            {
                mBuffer = new char[LOGBUFFER_SIZE];
                mOfStream.pubsetbuf(mBuffer, LOGBUFFER_SIZE);
            }
#if __cplusplus < 201402L
            mOfStream.open(mLogfile.c_str(), std::ios::out | std::ios::ate);
#else   // __cplusplus < 201402L
            mOfStream.open(mLogfile, std::ios::out | std::ios::ate);
#endif  //__cplusplus < 201402L
            mStream = new std::ostream(&mOfStream);

            if (!isStreamValid())
            {
                if (mOfStream.is_open())
                    mOfStream.close();

                delete mStream;
                mStream = &std::cout;
            }
#else   //__ANDROID__
            char *HOME = getenv("HOME");
            bool bigLog = false;
            uint logLevel = _getLevel(TConfig::getLogLevel());

            if ((logLevel & HLOG_TRACE) || (logLevel & HLOG_DEBUG))
                bigLog = true;

            if (HOME && !bigLog && mLogfile.find(HOME) == string::npos)
            {
                if (mOfStream.is_open())
                    mOfStream.close();

                if (mStream && mStream != &std::cout)
                    delete mStream;

                __android_log_print(ANDROID_LOG_DEBUG, "tpanel", "TStreamError::_init: Opening logfile: \"%s\"", mLogfile.c_str());

                if (!mOfStream.open(mLogfile, std::ios::out | std::ios::trunc))
                {
                    __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::_init: Could not open logfile!");
                    std::cout.rdbuf(new androidbuf);
                    mStream = &std::cout;
                    mLogFileEnabled = false;
                }
                else
                {
                    mStream = new std::ostream(&mOfStream);

                    if (!isStreamValid())
                    {
                        delete mStream;

                        if (mOfStream.is_open())
                            mOfStream.close();

                        std::cout.rdbuf(new androidbuf);
                        mStream = &std::cout;
                        mLogFileEnabled = false;
                    }
                }
            }
            else
            {
                std::cout.rdbuf(new androidbuf);
                mStream = &std::cout;
            }
#endif  // __ANDROID__
        }
        catch (std::exception& e)
        {
#ifdef __ANDROID__
            __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::_init: %s", e.what());
#else   // __ANDROID__
            std::cerr << "ERROR: " << e.what() << std::endl;
#endif  // __ANDROID__
            mStream = &std::cout;
        }
    }
    else if (!isStreamValid())
    {
#ifdef __ANDROID__
        std::cout.rdbuf(new androidbuf);
#endif  // __ANDROID__
        mStream = &std::cout;
#if defined(QT_DEBUG) || defined(DEBUG)
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_DEBUG, "tpanel", "TStreamError::_init: Stream wurde auf std::cout gesetzt.");
#else
        std::cout << "DEBUG: Stream wurde auf std::cout gesetzt." << std::endl;
#endif  // __ANDROID__
#endif  // defined(QT_DEBUG) || defined(DEBUG)
    }
#else  // LOGPATH == LPATH_FILE
    if (!mStream)
    {
#if defined(__ANDROID__) || defined(__IOS_AVAILABLE)
        std::cout.rdbuf(new androidbuf);
#endif
        mStream = &std::cout;
    }
#endif  // LOGPATH == LPATH_FILE

    if (reinit)
        return;

    if (mLogLevel > 0)
        *mStream << "Logfile started at " << getTime() << std::endl;

    *mStream << TConfig::getProgName() << " version " << VERSION_STRING() << std::endl;
    *mStream << "(C) Copyright by Andreas Theofilu <andreas@theosys.at>\n" << std::endl;

    if (mLogLevel > 0)
    {
        if (TConfig::isLongFormat())
            *mStream << "Timestamp          , Type LNr., File name           , ThreadID, Message" << std::endl;
        else
            *mStream << "Type LNr., ThreadID, Message" << std::endl;

        *mStream << "-----------------------------------------------------------------" << std::endl << std::flush;
    }
    else
        *mStream << std::flush;
}

std::ostream *TStreamError::resetFlags(std::ostream *os)
{
    if (!isStreamValid(*os))
        return os;

    *os << std::resetiosflags(std::ios::boolalpha) <<
           std::resetiosflags(std::ios::showbase) <<
           std::resetiosflags(std::ios::showpoint) <<
           std::resetiosflags(std::ios::showpos) <<
           std::resetiosflags(std::ios::skipws) <<
           std::resetiosflags(std::ios::unitbuf) <<
           std::resetiosflags(std::ios::uppercase) <<
           std::resetiosflags(std::ios::dec) <<
           std::resetiosflags(std::ios::hex) <<
           std::resetiosflags(std::ios::oct) <<
           std::resetiosflags(std::ios::fixed) <<
           std::resetiosflags(std::ios::scientific) <<
           std::resetiosflags(std::ios::internal) <<
           std::resetiosflags(std::ios::left) <<
           std::resetiosflags(std::ios::right) <<
           std::setfill(' ');
    return os;
}

void TStreamError::resetFlags()
{
    std::lock_guard<mutex> guard(message_mutex);
    resetFlags(TError::Current()->getStream());
}

void TStreamError::decIndent()
{
    if (mIndent > 0)
        mIndent--;
}

string TStreamError::getTime()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    rawtime = time(nullptr);
    timeinfo = localtime(&rawtime);

    if (!timeinfo)
    {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::getTime: Couldn't get the local time!");
#else
        std::cerr << "ERROR: Couldn't get the local time!" << std::endl;
#endif
        return string();
    }

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    string str(buffer);
    return str;
}

std::ostream *TStreamError::getStream()
{
    try
    {
        if (!isStreamValid())
        {
#ifdef __ANDROID__
            __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::getStream: Internal stream is invalid!");
#else
            std::cerr << "ERROR: Internal stream is invalid!" << std::endl;
#endif
            mInitialized = false;
            _init();
#ifdef __ANDROID__
            __android_log_print(ANDROID_LOG_INFO, "tpanel", "TStreamError::getStream: Reinitialized stream.");
#else
            std::cerr << "INFO: Reinitialized stream." << std::endl;
#endif

            if (!isStreamValid())
            {
#ifdef __ANDROID__
                __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::getStream: Reinitializing of stream failed!");
#else
                std::cerr << "ERROR: Reinitializing of stream failed! Using \"std::cout\" to write log messages." << std::endl;
#endif
                return &std::cout;
            }
        }

        return mStream;
    }
    catch (std::exception& e)
    {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::getStream: Error retrieving the current stream!");
#else
        std::cerr << "ERROR: Error retrieving the current stream! Using \"std::cout\" instead." << std::endl;
#endif
    }

    return &std::cout;
}

std::ostream& indent(std::ostream& os)
{
    if (!TStreamError::isStreamValid(os))
        return os;

    if (TStreamError::getIndent() > 0)
        os << std::setw(TStreamError::getIndent()) << " ";

    return os;
}

bool TStreamError::isStreamValid()
{
    if (!mStream)
    {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::isStreamValid: Stream is nullptr!");
#else
        std::cerr << "ERROR: TStreamError::isStreamValid: Stream is nullptr!" << std::endl;
#endif
        return false;
    }

    if (mStream->rdstate() & std::ostream::failbit)
    {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::isStreamValid: Stream has failbit set!");
#else
        std::cerr << "ERROR: TStreamError::isStreamValid: Stream has failbit set!" << std::endl;
#endif
        return false;
    }

    if (mStream->rdstate() & std::ostream::badbit)
    {
#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "TStreamError::isStreamValid: Stream has badbit set!");
#else
        std::cerr << "ERROR: TStreamError::isStreamValid: Stream has badbit set!" << std::endl;
#endif
        return false;
    }

    return true;
}

bool TStreamError::isStreamValid(std::ostream& os)
{
    if (os.rdstate() & std::ostream::failbit)
        return false;

    if (os.rdstate() & std::ostream::badbit)
        return false;

    return true;
}

void TStreamError::startTemporaryLogLevel(unsigned int l)
{
    if (haveTemporaryLogLevel)
        return;

    mLogLevelOld = mLogLevel;
    mLogLevel |= l;
    haveTemporaryLogLevel = true;
}

void TStreamError::endTemporaryLogLevel()
{
    if (!haveTemporaryLogLevel)
        return;

    mLogLevel = mLogLevelOld;
    haveTemporaryLogLevel = false;
}

/********************************************************************/
#ifndef NDEBUG
std::mutex tracer_mutex;

TTracer::TTracer(const std::string& msg, int line, const char *file, threadID_t tid)
    : mThreadID(tid)
{
    if (!TConfig::isInitialized() || !TStreamError::checkFilter(HLOG_TRACE))
        return;

    std::lock_guard<mutex> guard(tracer_mutex);

    mFile = file;
    size_t pos = mFile.find_last_of("/");

    if (pos != string::npos)
        mFile = mFile.substr(pos + 1);

    TError::setErrorType(TERRTRACE);
    std::lock_guard<mutex> guardm(message_mutex);

    if (!TConfig::isLongFormat())
        *TError::Current()->getStream() << "TRC, " << std::setw(5) << std::right << line << ", " << _threadIDtoStr(mThreadID) << ", " << indent << "{entry " << msg << std::endl;
    else
        *TError::Current()->getStream() << TStreamError::getTime() <<  ", TRC, " << std::setw(5) << std::right << line << ", " << std::setw(20) << std::left << mFile << ", " << _threadIDtoStr(mThreadID) << ", " << indent << "{entry " << msg << std::endl;

    TError::Current()->incIndent();
    mHeadMsg = msg;
    mLine = line;

    if (TConfig::getProfiling())
        mTimePoint = std::chrono::steady_clock::now();
}

TTracer::~TTracer()
{
    if (!TConfig::isInitialized() || !TStreamError::checkFilter(HLOG_TRACE))
        return;

    std::lock_guard<mutex> guard(tracer_mutex);
    TError::setErrorType(TERRTRACE);
    TError::Current()->decIndent();
    string nanosecs;

    if (TConfig::getProfiling())
    {
        std::chrono::steady_clock::time_point endPoint = std::chrono::steady_clock::now();
        std::chrono::nanoseconds difftime = endPoint - mTimePoint;
        std::chrono::seconds secs = std::chrono::duration_cast<std::chrono::seconds>(difftime);
        std::chrono::milliseconds msecs = std::chrono::duration_cast<std::chrono::milliseconds>(difftime) - std::chrono::duration_cast<std::chrono::seconds>(secs);
        std::stringstream s;
        s << std::chrono::duration_cast<std::chrono::nanoseconds> (difftime).count() << "[ns]" << " --> " << std::chrono::duration_cast<std::chrono::seconds>(secs).count() << "s " << std::chrono::duration_cast<std::chrono::milliseconds>(msecs).count() << "ms";
        nanosecs = s.str();
    }

    std::lock_guard<mutex> guardm(message_mutex);

    if (TConfig::getProfiling())
    {
        if (!TConfig::isLongFormat())
            *TError::Current()->getStream() << "TRC,      , " << _threadIDtoStr(mThreadID) << ", " << indent << "}exit " << mHeadMsg << " Elapsed time: " << nanosecs << std::endl;
        else
            *TError::Current()->getStream() << TStreamError::getTime() << ", TRC,      , " << std::setw(20) << std::left << mFile << ", " << _threadIDtoStr(mThreadID) << ", " << indent << "}exit " << mHeadMsg << " Elapsed time: " << nanosecs << std::endl;
    }
    else
    {
        if (!TConfig::isLongFormat())
            *TError::Current()->getStream() << "TRC,      , " << _threadIDtoStr(mThreadID) << ", " << indent << "}exit " << mHeadMsg << std::endl;
        else
            *TError::Current()->getStream() << TStreamError::getTime() << ", TRC,      , " << std::setw(20) << std::left << mFile << ", " << _threadIDtoStr(mThreadID) << ", " << indent << "}exit " << mHeadMsg << std::endl;
    }

    mHeadMsg.clear();
}
#endif

/********************************************************************/

TError::~TError()
{
    if (mCurrent)
    {
        delete mCurrent;
        mCurrent = nullptr;
    }
}

TStreamError* TError::Current()
{
    if (!mCurrent)
        mCurrent = new TStreamError(TConfig::getLogFile(), TConfig::getLogLevel());

    return mCurrent;
}

TStreamError *TError::Current(threadID_t tid)
{
    mThreadID = tid;
    return Current();
}

void TError::logHex(const char* str, size_t size)
{
    if (!str || !size)
        return;

    if (!Current())
        return;

    // Print out the message
    std::ostream *stream = mCurrent->getStream();
    *stream << strToHex(str, size, 16, true, 12) << std::endl;
    *stream << mCurrent->resetFlags(stream);
}

string TError::toHex(int num, int width)
{
    string ret;
    std::stringstream stream;
    stream << std::setfill ('0') << std::setw(width) << std::hex << num;
    ret = stream.str();
    return ret;
}

string TError::strToHex(const char *str, size_t size, int width, bool format, int indent)
{
    int len = 0, pos = 0, old = 0;
    int w = (format) ? 1 : width;
    string out, left, right;
    string ind;

    if (indent > 0)
    {
        for (int j = 0; j < indent; j++)
            ind.append(" ");
    }

    for (size_t i = 0; i < size; i++)
    {
        if (len >= w)
        {
            left.append(" ");
            len = 0;
        }

        if (format && i > 0 && (pos % width) == 0)
        {
            out += ind + toHex(old, 4) + ": " + left + " | " + right + "\n";
            left.clear();
            right.clear();
            old = pos;
        }

        int c = *(str+i) & 0x000000ff;
        left.append(toHex(c, 2));

        if (format)
        {
            if (std::isprint(c))
                right.push_back(c);
            else
                right.push_back('.');
        }

        len++;
        pos++;
    }

    if (!format)
        return left;
    else if (pos > 0)
    {
        if ((pos % width) != 0)
        {
            for (int i = 0; i < (width - (pos % width)); i++)
                left.append("   ");
        }

        out += ind + toHex(old, 4)+": "+left + "  | " + right;
    }

    return out;
}

void TError::setErrorMsg(const std::string& msg, int line, const string& file)
{
    if (msg.empty())
        return;

    msError = msg;
    mHaveError = true;
    mErrType = TERRERROR;
    mLastLine = line;
    string f = file;
    size_t pos = f.find_last_of("/");

    if (pos != string::npos)
        f = f.substr(pos + 1);

    mLastFile = f;
}

void TError::_setErrorMsg(const std::string& msg, int line, const std::string& file)
{
    mLastLine = line;
    mLastFile = file;
    setErrorMsg(msg);
}

void TError::setErrorMsg(terrtype_t t, const std::string& msg)
{
    if (msg.empty())
        return;

    msError = msg;
    mHaveError = true;
    mErrType = t;
    mLastLine = line;
    string f = file;
    size_t pos = f.find_last_of("/");

    if (pos != string::npos)
        f = f.substr(pos + 1);

    mLastFile = f;
}

std::ostream & TError::append(int lv, int line, const std::string& file, std::ostream& os)
{
    Current();

    if (!TConfig::isInitialized() && (lv == HLOG_ERROR || lv == HLOG_WARNING))
    {
        std::cerr << append(lv, line, file);
        return std::cerr;
    }

    return os << append(lv, line, file);
}

std::string TError::append(int lv, int line, const std::string& file)
{
    std::string prefix;

    switch (lv)
    {
        case HLOG_PROTOCOL: prefix = "PRT, "; mErrType = TERRINFO; break;
        case HLOG_INFO:     prefix = "INF, "; mErrType = TERRINFO; break;
        case HLOG_WARNING:  prefix = "WRN, "; mErrType = TERRWARNING; break;
        case HLOG_ERROR:    prefix = "ERR, "; mErrType = TERRERROR; break;
        case HLOG_TRACE:    prefix = "TRC, "; mErrType = TERRTRACE; break;
        case HLOG_DEBUG:    prefix = "DBG, "; mErrType = TERRDEBUG; break;

        default:
            prefix = "     ";
            mErrType = TERRNONE;
    }

    stringstream s;
    string f = file;
    size_t pos = f.find_last_of("/");

    if (pos != string::npos)
        f = f.substr(pos + 1);

    if (!TConfig::isLongFormat())
        s << prefix << std::setw(5) << std::right << line << ", " << _threadIDtoStr(mThreadID) << ", ";
    else
        s << TStreamError::getTime() << ", " << prefix << std::setw(5) << std::right << line << ", " << std::setw(20) << std::left << f << ", " << _threadIDtoStr(mThreadID) << ", ";

    return s.str();
}

void TError::displayMessage(const std::string& msg)
{
    QMessageBox m;
    m.setText(msg.c_str());

    int cnt = 10;

    QTimer cntDown;
    QObject::connect(&cntDown, &QTimer::timeout, [&m, &cnt, &cntDown]()->void
        {
            if (--cnt < 0)
            {
                cntDown.stop();
                m.close();
            }
        });

    cntDown.start(1000);
    m.exec();
}

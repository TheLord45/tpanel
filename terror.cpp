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
#include <iostream>
#include <fstream>
#include <sstream>
#include <ios>
#include <time.h>
#include <mutex>

#include "terror.h"
#include "tconfig.h"

#if defined(__linux__) || defined(Q_OS_ANDROID)
#include <QMessageBox>
#include <QTimer>
#endif
#if LOGPATH == LPATH_SYSLOG || defined(__ANDROID__)
#   ifdef __ANDROID__
#       include <android/log.h>
#   else
#       include <syslog.h>
#   endif
#endif

using std::string;
std::mutex message_mutex;

bool TError::mHaveError = false;
terrtype_t TError::mErrType = TERRNONE;
TStreamError *TError::mCurrent = nullptr;
std::string TError::msError;

int TStreamError::mIndent = 1;
std::ostream *TStreamError::mStream = nullptr;
std::string TStreamError::mLogfile;
bool TStreamError::mInitialized = false;
unsigned int TStreamError::mLogLevel = HLOG_PROTOCOL;

#if LOGPATH == LPATH_SYSLOG || defined(__ANDROID__)
class androidbuf : public std::streambuf
{
    public:
        enum { bufsize = 512 };
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
                switch(TError::getErrorType())
                {
                    case TERRINFO:      eType = LOG_INFO; break;
                    case TERRWARNING:   eType = LOG_WARN; break;
                    case TERRERROR:     eType = LOG_ERROR; break;
                    case TERRTRACE:     eType = LOG_INFO; break;
                    case TERRDEBUG:     eType = LOG_DEBUG; break;
                    case TERRNONE:      eType = LOG_INFO; break;
                }

                syslog(eType, writebuf);
                rc = 1;
#endif
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
        setLogLevel(TConfig::getLogFile());

    _init();
}

TStreamError::~TStreamError()
{
    if (mStream && mStream != &std::cout)
    {
        delete mStream;
        mStream = nullptr;
        mInitialized = false;
    }
}

void TStreamError::setLogFile(const std::string &lf)
{
    if (mInitialized && mLogfile.compare(lf) == 0)
        return;

    mLogfile = lf;
    mInitialized = false;
    _init();
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
        mLogLevel |= _getLevel(lv);
        pos = slv.find("|", start);
    }

    mLogLevel |= _getLevel(slv.substr(start));
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
    else if (err == TERRTRACE && (mLogLevel & HLOG_TRACE) != 0)
        return true;
    else if (err == TERRDEBUG && (mLogLevel & HLOG_DEBUG) != 0)
        return true;

    return false;
}

bool TStreamError::checkFilter(int lv)
{
    if (!TConfig::isInitialized())
        return false;

    if ((mLogLevel & lv) != 0)
        return true;

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

    if (slv.compare(SLOG_TRACE) == 0)
        return HLOG_TRACE;

    if (slv.compare(SLOG_DEBUG) == 0)
        return HLOG_DEBUG;

    if (slv.compare(SLOG_PROTOCOL) == 0)
        return HLOG_PROTOCOL;

    if (slv.compare(SLOG_ALL) == 0)
        return HLOG_ALL;

    return HLOG_NONE;
}

void TStreamError::_init()
{
    if (!TConfig::isInitialized() || mInitialized)
        return;

    mInitialized = true;

#if LOGPATH == LPATH_FILE
    if (!mLogfile.empty())
    {
        try
        {
#ifndef __ANDROID__
            if (mStream && mStream != &std::cout)
                delete mStream;
#if __cplusplus < 201402L
            mStream = new std::ofstream(mLogfile.c_str(), std::ios::out | std::ios::ate);
#else
            mStream = new std::ofstream(mLogfile, std::ios::out | std::ios::ate);
#endif
            if (!mStream || mStream->fail())
                mStream = &std::cout;
#else
            char *HOME = getenv("HOME");
            bool bigLog = false;
            uint logLevel = _getLevel(TConfig::getLogLevel());

            if ((logLevel & HLOG_TRACE) || (logLevel & HLOG_DEBUG))
                bigLog = true;

            if (HOME && !bigLog && mLogfile.find(HOME) == string::npos)
            {
                if (mStream && mStream != &std::cout)
                    delete mStream;

                mStream = new std::ofstream(mLogfile.c_str(), std::ios::out | std::ios::ate);

                if (!mStream || mStream->fail())
                {
                    std::cout.rdbuf(new androidbuf);
                    mStream = &std::cout;
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
            __android_log_print(ANDROID_LOG_ERROR, "tpanel", "ERROR: %s", e.what());
#else
            std::cerr << "ERROR: " << e.what() << std::endl;
#endif  // __ANDROID__
            mStream = &std::cout;
        }
    }
    else if (!mStream)
    {
#ifdef __ANDROID__
        std::cout.rdbuf(new androidbuf);
#endif
        mStream = &std::cout;
    }
#else  // LOGPATH == LPATH_FILE
    if (!mStream)
    {
#ifdef __ANDROID__
        std::cout.rdbuf(new androidbuf);
#endif
        mStream = &std::cout;
    }
#endif  // LOGPATH == LPATH_FILE

    if (!TConfig::isLongFormat())
        *mStream << "Logfile started at " << getTime() << std::endl;

    *mStream << TConfig::getProgName() << " version " << V_MAJOR << "." << V_MINOR << "." << V_PATCH << std::endl;
    *mStream << "(C) Copyright by Andreas Theofilu <andreas@theosys.at>" << std::endl << " " << std::endl;

    if (TConfig::isLongFormat())
        *mStream << "Timestamp           Type LNr., File name           , Message" << std::endl;
    else
        *mStream << "Type LNr., Message" << std::endl;

    *mStream << "-----------------------------------------------------------------" << std::endl << std::flush;
}

void TStreamError::logMsg(std::ostream& str)
{
    if (!TConfig::isInitialized())
        return;

    _init();

    if (!mStream || str.fail())
        return;

    // Print out the message
    std::stringstream s;
    s << str.rdbuf() << std::ends;
    *mStream << s.str() << std::flush;
    resetFlags(mStream);
}

std::ostream *TStreamError::resetFlags(std::ostream *os)
{
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

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    string str(buffer);
    return str;
}

std::ostream& indent(std::ostream& os)
{
    if (os.fail())
        return os;

    if (TStreamError::getIndent() > 0)
        os << std::setw(TStreamError::getIndent()) << " ";

    return os;
}

/********************************************************************/

std::mutex tracer_mutex;

TTracer::TTracer(const std::string& msg, int line, char *file)
{
    if (!TConfig::isInitialized() || !TStreamError::checkFilter(HLOG_TRACE))
        return;

    tracer_mutex.lock();

    mFile = file;
    size_t pos = mFile.find_last_of("/");

    if (pos != string::npos)
        mFile = mFile.substr(pos + 1);

    TError::setErrorType(TERRTRACE);
#if LOGPATH == LPATH_FILE
    if (!TConfig::isLongFormat())
        TError::Current()->logMsg(*TStreamError::getStream() << "TRC " << std::setw(5) << std::right << line << ", " << indent << "{entry " << msg << std::endl);
    else
        TError::Current()->logMsg(*TStreamError::getStream() << TStreamError::getTime() <<  " TRC " << std::setw(5) << std::right << line << ", " << std::setw(20) << std::left << mFile << ", " << indent << "{entry " << msg << std::endl);
#else
    std::stringstream s;

    if (!TConfig::isLongFormat())
        s  << "TRC " << std::setw(5) << std::right << line << ", " << &indents << "{entry " << msg << std::endl;
    else
        s << TStreamError::getTime() <<  " TRC " << std::setw(5) << std::right << line << ", " << std::setw(20) << std::left << mFile << ", " << &indents << "{entry " << msg << std::endl;

    TError::Current()->logMsg(s);
#endif
    TError::Current()->incIndent();
    mHeadMsg = msg;
    mLine = line;

    if (TConfig::getProfiling())
        mTimePoint = std::chrono::steady_clock::now();

    tracer_mutex.unlock();
}

TTracer::~TTracer()
{
    if (!TConfig::isInitialized() || !TStreamError::checkFilter(HLOG_TRACE))
        return;

    tracer_mutex.lock();
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

#if LOGPATH == LPATH_FILE
    if (TConfig::getProfiling())
    {
        if (!TConfig::isLongFormat())
            TError::Current()->logMsg(*TStreamError::getStream() << "TRC      , " << indent << "}exit " << mHeadMsg << " Elapsed time: " << nanosecs << std::endl);
        else
            TError::Current()->logMsg(*TStreamError::getStream() << TStreamError::getTime() << " TRC      , " << std::setw(20) << std::left << mFile << ", " << indent << "}exit " << mHeadMsg << " Elapsed time: " << nanosecs << std::endl);
    }
    else
    {
        if (!TConfig::isLongFormat())
            TError::Current()->logMsg(*TStreamError::getStream() << "TRC      , " << indent << "}exit " << mHeadMsg << std::endl);
        else
            TError::Current()->logMsg(*TStreamError::getStream() << TStreamError::getTime() << " TRC      , " << std::setw(20) << std::left << mFile << ", " << indent << "}exit " << mHeadMsg << std::endl);
    }
#else
    std::stringstream s;

    if (!TConfig::isLongFormat())
        s << "TRC      , " << &indents << "}exit " << mHeadMsg;
    else
        s << TStreamError::getTime() << " TRC      , " << std::setw(20) << std::left << mFile << ", " << &indents << "}exit " << mHeadMsg;

    if (TConfig::getProfiling())
        s  << " Elapsed time: " << nanosecs << std::endl;
    else
        s << std::endl;

    TError::Current()->logMsg(s);
#endif
    mHeadMsg.clear();
    tracer_mutex.unlock();
}

/********************************************************************/

TError::~TError()
{
    if (mCurrent)
    {
        delete mCurrent;
        mCurrent = nullptr;
    }
}

void TError::lock()
{
    message_mutex.lock();
}

void TError::unlock()
{
    message_mutex.unlock();
}

TStreamError* TError::Current()
{
    if (!mCurrent)
        mCurrent = new TStreamError(TConfig::getLogFile(), TConfig::getLogLevel());

    return mCurrent;
}

void TError::logHex(char* str, size_t size)
{
    if (!str || !size)
        return;

    message_mutex.lock();

    if (!Current())
    {
        message_mutex.unlock();
        return;
    }
    // Print out the message
    std::ostream *stream = mCurrent->getStream();
    *stream << strToHex(str, size, 16, true, 12) << std::endl;
    *stream << mCurrent->resetFlags(stream);
    message_mutex.unlock();
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

void TError::setErrorMsg(const std::string& msg)
{
    if (msg.empty())
        return;

    msError = msg;
    mHaveError = true;
    mErrType = TERRERROR;
}

void TError::setErrorMsg(terrtype_t t, const std::string& msg)
{
    if (msg.empty())
        return;

    msError = msg;
    mHaveError = true;
    mErrType = t;
}

std::ostream & TError::append(int lv, std::ostream& os)
{
    std::string prefix;
    Current();

    switch (lv)
    {
        case HLOG_PROTOCOL: prefix = "PRT    ++, "; mErrType = TERRINFO; break;
        case HLOG_INFO:     prefix = "INF    >>, "; mErrType = TERRINFO; break;
        case HLOG_WARNING:  prefix = "WRN    !!, "; mErrType = TERRWARNING; break;
        case HLOG_ERROR:    prefix = "ERR *****, "; mErrType = TERRERROR; break;
        case HLOG_TRACE:    prefix = "TRC      , "; mErrType = TERRTRACE; break;
        case HLOG_DEBUG:    prefix = "DBG    --, "; mErrType = TERRDEBUG; break;

        default:
            prefix = "           ";
            mErrType = TERRNONE;
    }

    if (!TConfig::isInitialized() && (lv == HLOG_ERROR || lv == HLOG_WARNING))
    {
        if (!TConfig::isLongFormat())
            std::cerr << prefix;
        else
            std::cerr << TStreamError::getTime() << " " << prefix << std::setw(20) << " " << ", ";
    }

    if (!TConfig::isLongFormat())
        return os << prefix;
    else
        return os << TStreamError::getTime() << " " << prefix << std::setw(20) << " " << ", ";
}

#if defined(__linux__) || defined(Q_OS_ANDROID)
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
#endif

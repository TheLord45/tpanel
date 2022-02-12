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

#ifndef __TEXCEPT_H__
#define __TEXCEPT_H__

#include <string>

class TXceptBase
{
    public:
        TXceptBase (const std::string & message,
                    const std::string & categorie,
                    const std::string & file,
                    const int         & iLine,
                    bool bFatal = false);

        virtual ~TXceptBase ();

        const std::string & what()         const { return mMessage; }
        virtual std::string  getMessage()  const { return mMessage; }
        const std::string & getCategorie() const { return mCategory; }
        const std::string & getFile()      const { return mFile; }
        int getLine()                      const { return mLine; }
        bool isFatal()                     const { return mbFatal; }
        void setFatal(bool bFatal) { mbFatal = bFatal; }
        virtual void logIt (void) ;

    private:
        std::string mMessage;
        std::string mCategory;
        std::string mFile;
        int         mLine{0};
        bool        mbFatal{false};          // true: schwerer Fehler + sofortiger Abbruch, false(default): allg. Fehler
};

class TXceptNetwork : public TXceptBase
{
    public:
        // ctr mit Uebergabe msg
        TXceptNetwork (const std::string& message, const std::string& file, int iLine, bool bFatal = false);
        // ctr ohne Uebergabe msg
        TXceptNetwork (const std::string& file, int iLine, bool bFatal = false);
};

class TXceptComm : public TXceptBase
{
public:
    // ctr mit Uebergabe msg
    TXceptComm (const std::string& message, const std::string& file, int iLine, bool bFatal = false);
    // ctr ohne Uebergabe msg
    TXceptComm (const std::string& file, int iLine, bool bFatal = false);
};

class TXceptFatal : public TXceptBase
{
public:
    // ctr mit Uebergabe msg
    TXceptFatal (const std::string& message, const std::string& file, int iLine, bool bFatal = true);
    // ctr ohne Uebergabe msg
    TXceptFatal (const std::string& file, int iLine, bool bFatal = true);
};

#define XCEPTNETWORK(msg)   throw TXceptNetwork (msg, __FILE__, __LINE__, false)
#define EXCEPTNETWORK()     throw TXceptNetwork (__FILE__, __LINE__, false)
#define EXCEPTNETFATAL()    throw TXceptNetwork (__FILE__, __LINE__, true)

#define XCEPTCOMM(msg)      throw TXceptComm    (msg, __FILE__, __LINE__, false)
#define EXCEPTCOMM()        throw TXceptComm    (__FILE__, __LINE__, false)
#define EXCEPTCOMMFATAL()   throw TXceptComm    (__FILE__, __LINE__, true)

#define EXCEPTFATALMSG(msg) throw TXceptFatal   (msg, __FILE__, __LINE__, true)
#define EXCEPTFATAL()       throw TXceptFatal   (__FILE__, __LINE__, true)

#endif

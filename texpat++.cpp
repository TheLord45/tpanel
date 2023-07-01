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

#include <string.h>

#include "texpat++.h"
#include "terror.h"

using namespace Expat;
using std::string;
using std::vector;
using std::ifstream;

int TExpat::mDepth = 0;
string TExpat::mContent;
string TExpat::mLastName;
vector<_ATTRIBUTE_t> TExpat::mAttributes;

TExpat::TExpat()
{
    DECL_TRACER("TExpat::TExpat()");
}

TExpat::TExpat(const std::string &file)
    : mFile(file)
{
    DECL_TRACER("TExpat::TExpat(const std::string &file)");

    mLastIter = mElements.end();
}

TExpat::~TExpat()
{
    DECL_TRACER("TExpat::~TExpat()");
}

void TExpat::setEncoding(TENCODING_t enc)
{
    DECL_TRACER("TExpat::setEncoding(TENCODING_t enc)");

    mSetEncoding = enc;

    switch(enc)
    {
        case ENC_ISO_8859_1:    mEncoding = "ISO-8859-1"; break;
        case ENC_US_ASCII:      mEncoding = "US-ASCII"; break;
        case ENC_UTF16:         mEncoding = "UTF-16"; break;
        case ENC_CP1250:        mEncoding = "CP1250"; break;

        default:
            mEncoding = "UTF-8";
            mSetEncoding = ENC_UTF8;
    }
}

bool TExpat::parse(bool debug)
{
    DECL_TRACER("TExpat::parse()");

    if (!isValidFile(mFile))
    {
        MSG_ERROR("Invalid file: " << mFile);
        TError::setError();
        return false;
    }

    string buf;
    size_t size = 0;

    // First we read the whole XML file into a buffer
    try
    {
        ifstream stream(mFile, std::ios::in);

        if (!stream || !stream.is_open())
            return false;

        stream.seekg(0, stream.end);    // Find the end of the file
        size = stream.tellg();          // Get the position and save it
        stream.seekg(0, stream.beg);    // rewind to the beginning of the file

        buf.resize(size, '\0');         // Initialize the buffer with zeros
        char *begin = &*buf.begin();    // Assign the plain data buffer
        stream.read(begin, size);       // Read the whole file
        stream.close();                 // Close the file
    }
    catch (std::exception& e)
    {
        MSG_ERROR("File error: " << e.what());
        TError::setError();
        return false;
    }

    // Now we parse the file and write the relevant contents into our internal
    // variables.
    // First we initialialize the parser.
    int done = 1;   // 1 = Buffer is complete
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetElementHandler(parser, &TExpat::startElement, &TExpat::endElement);
    XML_SetCharacterDataHandler(parser, &TExpat::CharacterDataHandler);
    XML_SetEncoding(parser, mEncoding.c_str());
    XML_SetUserData(parser, &mElements);

    if (mSetEncoding == ENC_CP1250)
        XML_SetUnknownEncodingHandler(parser, &TExpat::cp1250_encoding_handler, NULL);

    MSG_TRACE("Parsing XML file " << mFile);

    if (XML_Parse(parser, buf.data(), size, done) == XML_STATUS_ERROR)
    {
        MSG_ERROR(XML_ErrorString(XML_GetErrorCode(parser)) << " at line " << XML_GetCurrentLineNumber(parser));
        XML_ParserFree(parser);
        TError::setError();
        return false;
    }

    XML_ParserFree(parser);

    if (TStreamError::checkFilter(HLOG_DEBUG) && debug && mElements.size() > 0)
    {
        // Print out the whole XML file formatted
        vector<_ELEMENT_t>::iterator iter;
        size_t cnt = 0;

        for (iter = mElements.begin(); iter != mElements.end(); ++iter)
        {
            string sIndent;

            for (int i = 0; i < iter->depth; i++)
                sIndent += "   ";

            string attrs;

            if ((iter->eType == _ET_START || iter->eType == _ET_ATOMIC) && iter->attrs.size() > 0)
            {
                vector<ATTRIBUTE_t>::iterator atiter;

                for (atiter = iter->attrs.begin(); atiter != iter->attrs.end(); ++atiter)
                {
                    if (!attrs.empty())
                        attrs += " ";

                    attrs += atiter->name + " = \"" + atiter->content + "\"";
                }
            }

            if (iter->eType == _ET_START)
            {
                if (iter->attrs.size() > 0)
                {
                    MSG_DEBUG(std::setw(3) << std::setfill(' ') << "[" << cnt << "] (" << std::setw(0) << iter->depth << ") " << sIndent << "<" << iter->name << " " << attrs << ">");
                }
                else
                {
                    MSG_DEBUG(std::setw(3) << std::setfill(' ') << "[" << cnt << "] (" << std::setw(0) << iter->depth << ") " << sIndent << "<" << iter->name << ">");
                }
            }
            else if (iter->eType == _ET_ATOMIC)
            {
                if (iter->attrs.size() > 0)
                {
                    MSG_DEBUG(std::setw(3) << std::setfill(' ') << "[" << cnt << "] (" << std::setw(0) << iter->depth << ") " << sIndent << "<" << iter->name << " " << attrs << ">" << iter->content << "</" << iter->name << ">");
                }
                else
                {
                    MSG_DEBUG(std::setw(3) << std::setfill(' ') << "[" << cnt << "] (" << std::setw(0) << iter->depth << ") " << sIndent << "<" << iter->name << ">" << iter->content << "</" << iter->name << ">");
                }
            }
            else
            {
                MSG_DEBUG(std::setw(3) << std::setfill(' ') << "[" << cnt << "] (" << std::setw(0) << iter->depth << ") " << sIndent << "</" << iter->name << ">");
            }

            cnt++;
        }
    }

    return true;
}

string TExpat::getElement(const string &name, int depth, bool *valid)
{
    DECL_TRACER("TExpat::getElement(const string &name, int depth)");

    vector<_ELEMENT_t>::iterator iter, startElement;
    bool start = false;

    if (!mElements.empty() && mLastIter != mElements.end())
        startElement = mLastIter;
    else if (!mElements.empty())
        startElement = mElements.begin();
    else
    {
        MSG_DEBUG("Have no elements in queue!");

        if (valid)
            *valid = false;

        return string();
    }

    for (iter = startElement; iter != mElements.end(); ++iter)
    {
        if (!start && iter->depth == depth && iter->eType != _ET_END)
            start = true;

        if (start && iter->eType != _ET_END && iter->name.compare(name)  == 0 && iter->depth == depth)
        {
            if (valid)
                *valid = true;

            return iter->content;
        }
        else if (start && (iter->eType == _ET_END || iter->depth != depth))
            break;
    }

    if (valid)
        *valid = false;

    return string();
}

int TExpat::getElementInt(const string &name, int depth, bool *valid)
{
    DECL_TRACER("TExpat::getElementInt(const string &name, int depth, bool *valid)");

    bool val;
    string erg = getElement(name, depth, &val);

    if (valid)
        *valid = val;

    if (!val)
        return 0;

    return atoi(erg.c_str());
}

long TExpat::getElementLong(const string &name, int depth, bool *valid)
{
    DECL_TRACER("TExpat::getElementLong(const string &name, int depth, bool *valid)");

    bool val;
    string erg = getElement(name, depth, &val);

    if (valid)
        *valid = val;

    if (!val)
        return 0;

    return atol(erg.c_str());
}

float TExpat::getElementFloat(const string &name, int depth, bool *valid)
{
    DECL_TRACER("TExpat::getElementFloat(const string &name, int depth, bool *valid)");

    bool val;
    string erg = getElement(name, depth, &val);

    if (valid)
        *valid = val;

    if (!val)
        return 0;

    return (float)atof(erg.c_str());
}

double TExpat::getElementDouble(const string &name, int depth, bool *valid)
{
    DECL_TRACER("TExpat::getElementDouble(const string &name, int depth, bool *valid)");

    bool val = false;
    string erg = getElement(name, depth, &val);

    if (valid)
        *valid = val;

    if (!val)
        return 0;

    return atof(erg.c_str());
}

size_t TExpat::getElementIndex(const string& name, int depth)
{
    DECL_TRACER("TExpat::getElementIndex(const string& name, int depth)");

    vector<_ELEMENT_t>::iterator iter;
    size_t idx = 0;

    if (mElements.size() == 0)
    {
        mLastIter = mElements.end();
        return npos;
    }

    for (iter = mElements.begin(); iter != mElements.end(); ++iter)
    {
        if (iter->eType != _ET_END && iter->name.compare(name) == 0 && iter->depth == depth)
        {
            mLastIter = iter;
            return idx;
        }

        idx++;
    }

    mLastIter = mElements.end();
    return npos;
}

size_t TExpat::getElementIndex(const string& name, int* depth)
{
    DECL_TRACER("TExpat::getElementIndex(const string& name, int* depth)");

    if (mElements.size() == 0)
    {
        mLastIter = mElements.end();
        return npos;
    }

    vector<_ELEMENT_t>::iterator iter;
    size_t idx = 0;

    for (iter = mElements.begin(); iter != mElements.end(); ++iter)
    {
        if (iter->eType != _ET_END && iter->name.compare(name) == 0)
        {
            if (depth)
                *depth = iter->depth;

            mLastIter = iter;
            return idx;
        }

        idx++;
    }

    if (depth)
        *depth = -1;

    mLastIter = mElements.end();
    return npos;
}

size_t TExpat::getElementFromIndex(size_t index, string* name, string* content, vector<ATTRIBUTE_t> *attrs)
{
    DECL_TRACER("TExpat::getElementFromIndex(size_t index, string* name, string* content, vector<ATTRIBUTE_t> *attrs)");

    if (index == npos || index >= mElements.size() || mElements.at(index).eType == _ET_END)
        return npos;

    if (name)
        name->assign(mElements.at(index).name);

    if (content)
        content->assign(mElements.at(index).content);

    if (attrs)
        *attrs = mElements.at(index).attrs;

    return index;
}

size_t TExpat::getNextElementFromIndex(size_t index, string* name, string* content, vector<ATTRIBUTE_t>* attrs)
{
    DECL_TRACER("TExpat::getNextElementFromIndex(size_t index, int depth, string* name, string* content, vector<ATTRIBUTE_t>* attrs)");

    if (index == npos)
        return npos;

    size_t idx = index + 1;

    if (idx >= mElements.size() || mElements.at(idx).eType == _ET_END)
        return npos;

    if (name)
        name->assign(mElements.at(idx).name);

    if (content)
        content->assign(mElements.at(idx).content);

    if (attrs)
        *attrs = mElements.at(idx).attrs;

    return idx;
}

string TExpat::getFirstElement(const string &name, int *depth)
{
    DECL_TRACER("TExpat::getFirstElement(const string &name, int *depth)");

    vector<_ELEMENT_t>::iterator iter;

    for (iter = mElements.begin(); iter != mElements.end(); ++iter)
    {
        if (iter->eType != _ET_END && iter->name.compare(name) == 0)
        {
            if (depth)
                *depth = iter->depth;

            mLastIter = iter;
            return iter->content;
        }
    }

    if (depth)
        *depth = -1;

    mLastIter = mElements.end();
    return string();
}

string TExpat::getNextElement(const string &name, int depth, bool *valid)
{
    DECL_TRACER("TExpat::getNextElement(const string &name, int depth)");

    if (valid)
        *valid = false;

    if (mLastIter == mElements.end())
        return string();

    mLastIter++;

    while (mLastIter != mElements.end())
    {
        if (mLastIter == mElements.end() || mLastIter->eType == _ET_END)
            return string();

        if (mLastIter->name.compare(name) == 0 && mLastIter->depth == depth)
        {
            if (valid)
                *valid = true;

            return mLastIter->content;
        }

        mLastIter++;
    }

    return string();
}

size_t TExpat::getNextElementIndex(const string& name, int depth)
{
    DECL_TRACER("TExpat::getNextElementIndex(const string& name, int depth)");

    if (mLastIter == mElements.end())
        return npos;

    mLastIter++;

    while (mLastIter != mElements.end())
    {
        if (mLastIter->eType == _ET_END && mLastIter->depth < depth)
            break;

        if (mLastIter->name.compare(name)  == 0 && mLastIter->depth == depth && mLastIter->eType != _ET_END)
        {
            size_t idx = 0;
            vector<_ELEMENT_t>::iterator iter;

            for (iter = mElements.begin(); iter != mElements.end(); ++iter)
            {
                if (iter == mLastIter)
                    return idx;

                idx++;
            }
        }

        mLastIter++;
    }

    return npos;
}

string TExpat::getAttribute(const string& name, vector<ATTRIBUTE_t>& attrs)
{
    DECL_TRACER("TExpat::getAttribute(const string& name, vector<ATTRIBUTE_t>& attrs)");

    vector<ATTRIBUTE_t>::iterator iter;

    if (attrs.size() == 0)
        return string();

    for (iter = attrs.begin(); iter != attrs.end(); ++iter)
    {
        if (iter->name.compare(name) == 0)
            return iter->content;
    }

    return string();
}

int TExpat::getAttributeInt(const string& name, vector<ATTRIBUTE_t>& attrs)
{
    DECL_TRACER("TExpat::getAttributeInt(const string& name, vector<ATTRIBUTE_t>& attrs)");

    return atoi(getAttribute(name, attrs).c_str());
}

long Expat::TExpat::getAttributeLong(const std::string& name, std::vector<ATTRIBUTE_t>& attrs)
{
    DECL_TRACER("TExpat::getAttributeLong(const string& name, vector<ATTRIBUTE_t>& attrs)");

    return atol(getAttribute(name, attrs).c_str());
}

float Expat::TExpat::getAttributeFloat(const std::string& name, std::vector<ATTRIBUTE_t>& attrs)
{
    DECL_TRACER("TExpat::getAttributeFloat(const string& name, vector<ATTRIBUTE_t>& attrs)");

    return (float)atof(getAttribute(name, attrs).c_str());
}

double Expat::TExpat::getAttributeDouble(const std::string& name, std::vector<ATTRIBUTE_t>& attrs)
{
    DECL_TRACER("TExpat::getAttributeDouble(const string& name, vector<ATTRIBUTE_t>& attrs)");

    return atof(getAttribute(name, attrs).c_str());
}

int TExpat::convertElementToInt(const string& content)
{
    DECL_TRACER("TExpat::convertElementToInt(const string& content)");

    if (content.empty())
    {
        TError::setErrorMsg("Empty content!");
        return 0;
    }

    return atoi(content.c_str());
}

long TExpat::convertElementToLong(const string& content)
{
    DECL_TRACER("TExpat::convertElementToLong(const string& content)");

    if (content.empty())
    {
        TError::setErrorMsg("Empty content!");
        return 0;
    }

    return atol(content.c_str());
}

float TExpat::convertElementToFloat(const string& content)
{
    DECL_TRACER("TExpat::convertElementToFloat(const string& content)");

    if (content.empty())
    {
        TError::setErrorMsg("Empty content!");
        return 0;
    }

    return (float)atof(content.c_str());
}

double TExpat::convertElementToDouble(const string& content)
{
    DECL_TRACER("TExpat::convertElementToDouble(const string& content)");

    if (content.empty())
    {
        TError::setErrorMsg("Empty content!");
        return 0;
    }

    return atof(content.c_str());
}

bool TExpat::setIndex(size_t index)
{
    DECL_TRACER("TExpat::setIndex(size_t index)");

    if (index >= mElements.size())
    {
        TError::setErrorMsg("Invalid index " + std::to_string(index) + "!");
        mLastIter = mElements.end();
        return false;
    }

    vector<_ELEMENT_t>::iterator iter;
    size_t idx = 0;

    for (iter = mElements.begin(); iter != mElements.end(); ++iter)
    {
        if (idx == index)
        {
            mLastIter = iter;
            return true;
        }

        idx++;
    }

    mLastIter = mElements.end();
    return false;   // Should not happen and is just for satisfy the compiler.
}

vector<ATTRIBUTE_t> TExpat::getAttributes()
{
    DECL_TRACER("TExpat::getAttributes()");

    if (mLastIter == mElements.end())
        return vector<ATTRIBUTE_t>();

    return mLastIter->attrs;
}

vector<ATTRIBUTE_t> TExpat::getAttributes(size_t index)
{
    DECL_TRACER("TExpat::getAttributes(size_t index)");

    if (index >= mElements.size())
        return vector<ATTRIBUTE_t>();

    return mElements.at(index).attrs;
}

string TExpat::getElementName(bool *valid)
{
    DECL_TRACER("TExpat::getElementName()");

    if (mLastIter != mElements.end())
    {
        if (valid)
            *valid = true;

        return mLastIter->name;
    }

    if (valid)
        *valid = false;

    return string();
}

string TExpat::getElementContent(bool *valid)
{
    DECL_TRACER("TExpat::getElementContent()");

    if (mLastIter != mElements.end())
    {
        if (valid)
            *valid = true;

        return mLastIter->content;
    }

    if (valid)
        *valid = false;

    return string();
}

int TExpat::getElementContentInt(bool *valid)
{
    DECL_TRACER("TExpat::getElementContentInt(bool *valid)");

    if (mLastIter != mElements.end())
    {
        if (valid)
            *valid = true;

        return atoi(mLastIter->content.c_str());
    }

    if (valid)
        *valid = false;

    return 0;
}

long TExpat::getElementContentLong(bool *valid)
{
    DECL_TRACER("TExpat::getElementContentLong(bool *valid)");

    if (mLastIter != mElements.end())
    {
        if (valid)
            *valid = true;

        return atol(mLastIter->content.c_str());
    }

    if (valid)
        *valid = false;

    return 0;
}

float TExpat::getElementContentFloat(bool *valid)
{
    DECL_TRACER("TExpat::getElementContentFloat(bool *valid)");

    if (mLastIter != mElements.end())
    {
        if (valid)
            *valid = true;

        return (float)atof(mLastIter->content.c_str());
    }

    if (valid)
        *valid = false;

    return 0.0;
}

double TExpat::getElementContentDouble(bool *valid)
{
    DECL_TRACER("TExpat::getElementContentDouble(bool *valid)");

    if (mLastIter != mElements.end())
    {
        if (valid)
            *valid = true;

        return atof(mLastIter->content.c_str());
    }

    if (valid)
        *valid = false;

    return 0.0;
}

/******************************************************************************
 *                        Static methods starting here
 ******************************************************************************/

void TExpat::createCP1250Encoding(XML_Encoding* enc)
{
    if (!enc)
        return;

    for (int i = 0; i < 0x0100; i++)
    {
        if (i < 0x007f)
            enc->map[i] = i;
        else
        {
            switch(i)
            {
                case 0x080: enc->map[i] = 0x20AC; break;
                case 0x082: enc->map[i] = 0x201A; break;
                case 0x083: enc->map[i] = 0x0192; break;
                case 0x084: enc->map[i] = 0x201E; break;
                case 0x085: enc->map[i] = 0x2026; break;
                case 0x086: enc->map[i] = 0x2020; break;
                case 0x087: enc->map[i] = 0x2021; break;
                case 0x088: enc->map[i] = 0x02C6; break;
                case 0x089: enc->map[i] = 0x2030; break;
                case 0x08A: enc->map[i] = 0x0160; break;
                case 0x08B: enc->map[i] = 0x2039; break;
                case 0x08C: enc->map[i] = 0x0152; break;
                case 0x08D: enc->map[i] = 0x00A4; break;
                case 0x08E: enc->map[i] = 0x017D; break;
                case 0x08F: enc->map[i] = 0x00B9; break;
                case 0x091: enc->map[i] = 0x2018; break;
                case 0x092: enc->map[i] = 0x2019; break;
                case 0x093: enc->map[i] = 0x201C; break;
                case 0x094: enc->map[i] = 0x201D; break;
                case 0x095: enc->map[i] = 0x2022; break;
                case 0x096: enc->map[i] = 0x2013; break;
                case 0x097: enc->map[i] = 0x2014; break;
                case 0x098: enc->map[i] = 0x02DC; break;
                case 0x099: enc->map[i] = 0x2122; break;
                case 0x09A: enc->map[i] = 0x0161; break;
                case 0x09B: enc->map[i] = 0x203A; break;
                case 0x09C: enc->map[i] = 0x0153; break;
                case 0x09D: enc->map[i] = 0x00A5; break;
                case 0x09E: enc->map[i] = 0x017E; break;
                case 0x09F: enc->map[i] = 0x0178; break;
                case 0x0A0: enc->map[i] = 0x02A0; break;
                case 0x0A1: enc->map[i] = 0x02C7; break;
                case 0x0A2: enc->map[i] = 0x02D8; break;
                case 0x0A3: enc->map[i] = 0x0141; break;
                case 0x0A4: enc->map[i] = 0x00A4; break;
                case 0x0A5: enc->map[i] = 0x0104; break;
                case 0x0A6: enc->map[i] = 0x00A6; break;
                case 0x0A7: enc->map[i] = 0x00A7; break;
                case 0x0A8: enc->map[i] = 0x00A8; break;
                case 0x0A9: enc->map[i] = 0x00A9; break;
                case 0x0AA: enc->map[i] = 0x015E; break;
                case 0x0AB: enc->map[i] = 0x00AB; break;
                case 0x0AC: enc->map[i] = 0x00AC; break;
                case 0x0AD: enc->map[i] = 0x00AD; break;
                case 0x0AE: enc->map[i] = 0x00AE; break;
                case 0x0AF: enc->map[i] = 0x017B; break;
                case 0x0B0: enc->map[i] = 0x00B0; break;
                case 0x0B1: enc->map[i] = 0x00B1; break;
                case 0x0B2: enc->map[i] = 0x02DB; break;
                case 0x0B3: enc->map[i] = 0x0142; break;
                case 0x0B4: enc->map[i] = 0x00B4; break;
                case 0x0B5: enc->map[i] = 0x00B5; break;
                case 0x0B6: enc->map[i] = 0x00B6; break;
                case 0x0B7: enc->map[i] = 0x00B7; break;
                case 0x0B8: enc->map[i] = 0x00B8; break;
                case 0x0B9: enc->map[i] = 0x0105; break;
                case 0x0BA: enc->map[i] = 0x015F; break;
                case 0x0BB: enc->map[i] = 0x00BB; break;
                case 0x0BC: enc->map[i] = 0x013D; break;
                case 0x0BD: enc->map[i] = 0x02DD; break;
                case 0x0BE: enc->map[i] = 0x013E; break;
                case 0x0BF: enc->map[i] = 0x017C; break;
                case 0x0C0: enc->map[i] = 0x0154; break;
                case 0x0C1: enc->map[i] = 0x00C1; break;
                case 0x0C2: enc->map[i] = 0x00C2; break;
                case 0x0C3: enc->map[i] = 0x0102; break;
                case 0x0C4: enc->map[i] = 0x00C4; break;
                case 0x0C5: enc->map[i] = 0x0139; break;
                case 0x0C6: enc->map[i] = 0x0106; break;
                case 0x0C7: enc->map[i] = 0x00C7; break;
                case 0x0C8: enc->map[i] = 0x010C; break;
                case 0x0C9: enc->map[i] = 0x00C9; break;
                case 0x0CA: enc->map[i] = 0x0118; break;
                case 0x0CB: enc->map[i] = 0x00CB; break;
                case 0x0CC: enc->map[i] = 0x00CC; break;    //0x011A
                case 0x0CD: enc->map[i] = 0x00CD; break;
                case 0x0CE: enc->map[i] = 0x00CE; break;
                case 0x0CF: enc->map[i] = 0x00CF; break;
                case 0x0D0: enc->map[i] = 0x0110; break;
                case 0x0D1: enc->map[i] = 0x0143; break;
                case 0x0D2: enc->map[i] = 0x0147; break;
                case 0x0D3: enc->map[i] = 0x00D3; break;
                case 0x0D4: enc->map[i] = 0x00D4; break;
                case 0x0D5: enc->map[i] = 0x0150; break;
                case 0x0D6: enc->map[i] = 0x00D6; break;
                case 0x0D7: enc->map[i] = 0x00D7; break;
                case 0x0D8: enc->map[i] = 0x0158; break;
                case 0x0D9: enc->map[i] = 0x016E; break;
                case 0x0DA: enc->map[i] = 0x00DA; break;
                case 0x0DB: enc->map[i] = 0x0170; break;
                case 0x0DC: enc->map[i] = 0x00DC; break;
                case 0x0DD: enc->map[i] = 0x00DD; break;
                case 0x0DE: enc->map[i] = 0x0162; break;
                case 0x0DF: enc->map[i] = 0x00DF; break;
                case 0x0E0: enc->map[i] = 0x0155; break;
                case 0x0E1: enc->map[i] = 0x00E1; break;
                case 0x0E2: enc->map[i] = 0x00E2; break;
                case 0x0E3: enc->map[i] = 0x0103; break;
                case 0x0E4: enc->map[i] = 0x00E4; break;
                case 0x0E5: enc->map[i] = 0x013A; break;
                case 0x0E6: enc->map[i] = 0x0107; break;
                case 0x0E7: enc->map[i] = 0x00E7; break;
                case 0x0E8: enc->map[i] = 0x010D; break;
                case 0x0E9: enc->map[i] = 0x00E9; break;
                case 0x0EA: enc->map[i] = 0x0119; break;
                case 0x0EB: enc->map[i] = 0x00EB; break;
                case 0x0EC: enc->map[i] = 0x011B; break;
                case 0x0ED: enc->map[i] = 0x00ED; break;
                case 0x0EE: enc->map[i] = 0x00EE; break;
                case 0x0EF: enc->map[i] = 0x010F; break;
                case 0x0F0: enc->map[i] = 0x0111; break;
                case 0x0F1: enc->map[i] = 0x0144; break;
                case 0x0F2: enc->map[i] = 0x0148; break;
                case 0x0F3: enc->map[i] = 0x00F3; break;
                case 0x0F4: enc->map[i] = 0x00F4; break;
                case 0x0F5: enc->map[i] = 0x0151; break;
                case 0x0F6: enc->map[i] = 0x00F6; break;
                case 0x0F7: enc->map[i] = 0x00F7; break;
                case 0x0F8: enc->map[i] = 0x0159; break;
                case 0x0F9: enc->map[i] = 0x016F; break;
                case 0x0FA: enc->map[i] = 0x00FA; break;
                case 0x0FB: enc->map[i] = 0x0171; break;
                case 0x0FC: enc->map[i] = 0x00FC; break;
                case 0x0FD: enc->map[i] = 0x00FD; break;
                case 0x0FE: enc->map[i] = 0x0163; break;
                case 0x0FF: enc->map[i] = 0x02D9; break;

                default:
                    enc->map[i] = -1;
            }
        }
    }
}

void TExpat::startElement(void *userData, const XML_Char *name, const XML_Char **attrs)
{
    if (!name)
        return;

    std::vector<_ELEMENT_t> *ud = (std::vector<_ELEMENT_t> *)userData;
    _ELEMENT_t el;

    mLastName.assign(name);
    mDepth++;
    mAttributes.clear();
    mContent.clear();

    el.depth = mDepth;
    el.name = mLastName;
    el.eType = _ET_START;

    if (attrs && *attrs)
    {
        for (int i = 0; attrs[i]; i += 2)
        {
            _ATTRIBUTE_t at;
            at.name.assign(attrs[i]);
            at.content.assign(attrs[i + 1]);
            mAttributes.push_back(at);
        }

        el.attrs = mAttributes;
    }

    ud->push_back(el);
}

void TExpat::endElement(void *userData, const XML_Char *name)
{
    if (!userData || !name)
        return;

    std::vector<_ELEMENT_t> *ud = (std::vector<_ELEMENT_t> *)userData;

    if (mLastName.compare(name) == 0)
    {
        if (ud->back().depth == mDepth)
        {
            ud->back().eType = _ET_ATOMIC;
            ud->back().content = mContent;

            mDepth--;
            mLastName.clear();
            mContent.clear();
            return;
        }
    }

    _ELEMENT_t el;
    el.depth = mDepth;
    el.name.assign(name);
    el.eType = _ET_END;
    ud->push_back(el);
    mDepth--;

    mLastName.clear();
    mContent.clear();
}

void TExpat::CharacterDataHandler(void *, const XML_Char *s, int len)
{
    if (!s || len <= 0 || mLastName.empty())
        return;

    mContent.append(s, len);
}

int XMLCALL TExpat::cp1250_encoding_handler(void *, const XML_Char *encoding, XML_Encoding *info)
{
    if (!info)
        return XML_STATUS_ERROR;

    if (encoding && strcmp(encoding, "CP1250") != 0)
    {
        MSG_ERROR("Invalid encoding handler (" << encoding << ")");
        TError::setError();
        return XML_STATUS_ERROR;
    }

    memset(info, 0, sizeof(XML_Encoding));
    createCP1250Encoding(info);
    return XML_STATUS_OK;
}

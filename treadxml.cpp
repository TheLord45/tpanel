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
#include <errno.h>
#include <iostream>
#include <fstream>
#include <exception>
#include "tnameformat.h"
#include "treadxml.h"

using std::ifstream;
using std::ofstream;
using std::string;
using std::exception;

TReadXML::TReadXML(const std::string& fname, bool trim)
	: mFName(fname)
{
	DECL_TRACER("TReadXML::TReadXML(const std::string& fname)");
	openFile(trim);
}

TReadXML::~TReadXML()
{
	DECL_TRACER("TReadXML::~TReadXML()");

	if (mTree)
		mxmlDelete(mTree);
}

bool TReadXML::openFile(bool trim)
{
	DECL_TRACER("TReadXML::openFile()");

	TError::clear();

	if (mFName.empty())
	{
		TError::setErrorMsg("No XML file to read!");
		MSG_ERROR(TError::getErrorMsg());
		return false;
	}

	MSG_TRACE("Opening XML file " << mFName << " for reading ...");

	try
	{
		ifstream xml(mFName.c_str());

		if (!xml)
		{
			TError::setErrorMsg("Error opening the file " + mFName);
			MSG_ERROR(TError::getErrorMsg());
			return false;
		}

		string buffer;

		try
		{
			xml.seekg(0, xml.end);
			size_t length = xml.tellg();
			xml.seekg(0, xml.beg);
			buffer.resize(length, ' ');
			char *begin = &*buffer.begin();
			xml.read(begin, length);
		}
		catch (exception& e)
		{
			TError::setErrorMsg("Error reading a file: " + string(e.what()));
			MSG_ERROR(TError::getErrorMsg());
			xml.close();
			return false;
		}

		xml.close();
		string cbuf = TNameFormat::cp1250ToUTF8(buffer);

        // The used XML parser has a bug. If it is a formatted XML file, then
        // it crashes. Therefore the following call removes all formatting
        // characters to ensure the XML parser succeeds. To avoid this (maybe)
        // time consuming trimming, there is a parameter to control whether
        // the trimming should be made or not.
        if (trim)
            cbuf = TNameFormat::trimXML(cbuf);

		mTree = mxmlLoadString(NULL, cbuf.c_str(), MXML_OPAQUE_CALLBACK);
		buffer.clear();

		if (mTree == NULL)
		{
			TError::setErrorMsg("Error reading XML file " + mFName);
			MSG_ERROR(TError::getErrorMsg());
			return false;
		}
	}
	catch (exception& e)
	{
		TError::setErrorMsg("Fatal error: " + string(e.what()));
		MSG_ERROR(TError::getErrorMsg());
		return false;
	}

	return true;
}

std::string & TReadXML::findElement(const std::string& name)
{
	DECL_TRACER("TReadXML::findElement(const std::string& name)");
	const string none;
	return findElement(name, none);
}

std::string & TReadXML::findElement(const std::string& name, const std::string& attr)
{
	DECL_TRACER("TReadXML::findElement(const std::string& name, const std::string& attr)");

	TError::clear();

	if (!mTree)
	{
		TError::setErrorMsg("No valid XML available!");
		MSG_ERROR(TError::getErrorMsg());
		mValue.clear();
        mValid = false;
		return mValue;
	}

	try
	{
        if (!attr.empty())
            mNode = mxmlFindElement(mTree, mTree, name.c_str(), attr.c_str(), NULL, MXML_DESCEND);
        else
            mNode = mxmlFindElement(mTree, mTree, name.c_str(), NULL, NULL, MXML_DESCEND);

        if (!mNode)
        {
            if (attr.empty())
            {
                MSG_WARNING("Element " << name << " not found!");
            }
            else
            {
                MSG_WARNING("Element " << name << " with attribute " << attr << " not found!");
            }

            mValue.clear();
            mValid = false;
            return mValue;
        }

        mLastNode = mNode;
        extractValue(mNode);

        if (!attr.empty())
        {
            const char *attribute = mxmlElementGetAttr(mNode, attr.c_str());

            if (attribute)
                mAttribute.assign(attribute);
            else
                mAttribute.clear();

    //        MSG_DEBUG("Found attribute " << mAttribute);
        }
    }
    catch (exception& e)
    {
        TError::setErrorMsg("Fatal error: " + string(e.what()));
        MSG_ERROR(TError::getErrorMsg());
        mValid = false;
        mValue.clear();
        return mValue;
    }

    mValid = true;
    return mValue;
}

std::string & TReadXML::findElement(mxml_node_t* node, const std::string& name)
{
	DECL_TRACER("TReadXML::findElement(mxml_node_t* node, const std::string& name)");
	const string none;
	return findElement(node, name, none);
}

std::string & TReadXML::findElement(mxml_node_t* node, const std::string& name, const std::string& attr)
{
	DECL_TRACER("TReadXML::findElement(mxml_node_t* node, const std::string& name, const std::string& attr)");

	TError::clear();

	if (!mTree || !node)
	{
		TError::setErrorMsg("No valid XML available!");
		MSG_ERROR(TError::getErrorMsg());
		mValue.clear();
        mValid = false;
		return mValue;
	}

	if (!attr.empty())
		mNode = mxmlFindElement(node, mTree, name.c_str(), attr.c_str(), NULL, MXML_DESCEND);
	else
		mNode = mxmlFindElement(node, mTree, name.c_str(), NULL, NULL, MXML_DESCEND);

	if (!mNode)
	{
		if (attr.empty())
		{
			MSG_WARNING("Element " << name << " not found!");
		}
		else
		{
			MSG_WARNING("Element " << name << " with attribute " << attr << " not found!");
		}

		mValue.clear();
        mValid = false;
		return mValue;
	}

	mLastNode = mNode;
    extractValue(mNode);

    if (!attr.empty())
    {
        const char *attribute = mxmlElementGetAttr(mNode, attr.c_str());

        if (attribute)
            mAttribute.assign(attribute);
        else
            mAttribute.clear();

//        MSG_DEBUG("Found attribute " << mAttribute);
    }

    mValid = true;
    return mValue;
}

std::string & TReadXML::findNextElement(const std::string& name)
{
	DECL_TRACER("TReadXML::findNextElement(const std::string& name)");
	return findElement(mLastNode, name);
}

std::string & TReadXML::findNextElement(const std::string& name, const std::string& attr)
{
	DECL_TRACER("TReadXML::findNextElement(const std::string& name, const std::string& attr)");
	return findElement(mLastNode, name, attr);
}

std::string & TReadXML::getAttribute(const string& name)
{
    DECL_TRACER("TReadXML::getAttribute(const string& name)");

    const char *value = mxmlElementGetAttr(mNode, name.c_str());

    if (!value)
        mAttribute.clear();
    else
        mAttribute.assign(value);

//    MSG_DEBUG("Found attribute " << mAttribute << " from name " << name);
    return mAttribute;
}

std::string & TReadXML::getAttribute(const string& name, int index)
{
    DECL_TRACER("TReadXML::getAttribute(const string& name)");

    const char *pname = name.c_str();
    const char *value = mxmlElementGetAttrByIndex(mNode, index, &pname);

    if (!value)
        mAttribute.clear();
    else
        mAttribute.assign(value);

//    MSG_DEBUG("Found attribute " << mAttribute << " from name " << name << " at index " << index);
    return mAttribute;
}

mxml_node_t *TReadXML::getFirstChild()
{
    DECL_TRACER("*TReadXML::getFirstChild()");

    mxml_node_t *n = mxmlGetFirstChild(mLastNode);

    if (n)
    {
        inode = n;
        mElement = mxmlGetElement(n);
    }

//    if (inode)
//        MSG_DEBUG("Found element " << mElement);

    return n;
}

mxml_node_t *TReadXML::getNextChild()
{
    DECL_TRACER("*TReadXML::getNextChild()");

    if (!inode)
        return nullptr;

    mxml_node_t *n = mxmlGetNextSibling(inode);

    if (n)
    {
        inode = n;
        mElement = mxmlGetElement(n);
    }

//    if (inode)
//        MSG_DEBUG("Found element " << mElement);

    return n;
}

mxml_node_t *TReadXML::getLastChild()
{
    DECL_TRACER("*TReadXML::getLastChild()");

    mxml_node_t *n = mxmlGetLastChild(mLastNode);

    if (n)
    {
        inode = n;
        mElement = mxmlGetElement(n);
    }

//    if (inode)
//        MSG_DEBUG("Found element " << mElement);

    return n;
}

mxml_node_t *TReadXML::getFirstChild(mxml_node_t* node)
{
    DECL_TRACER("TReadXML::getFirstChild(mxml_node_t* node)");

    if (!node)
        return nullptr;

    mxml_node_t *n = mxmlGetFirstChild(node);

//    if (n)
//        MSG_DEBUG("Found element " << mxmlGetElement(n));

    return n;
}

mxml_node_t *TReadXML::getNextChild(mxml_node_t* node)
{
    DECL_TRACER("TReadXML::getNextChild(mxml_node_t* node)");

    if (!node)
        return nullptr;

    mxml_node_t *n = mxmlGetNextSibling(node);

//    if (n)
//        MSG_DEBUG("Found element " << mxmlGetElement(n));

    return n;
}

mxml_node_t *TReadXML::getLastChild(mxml_node_t* node)
{
    DECL_TRACER("TReadXML::getLastChild(mxml_node_t* node)");

    mxml_node_t *n = mxmlGetLastChild(node);

    if (!node)
        return nullptr;

//    if (n)
//        MSG_DEBUG("Found element " << mxmlGetElement(n));

    return n;
}

std::string& TReadXML::getTextFromNode(mxml_node_t* n)
{
    DECL_TRACER("TReadXML::getTextFromNode(mxml_node_t* n)");

    mValue.clear();

    if (!n)
        return mValue;

    extractValue(n);
//    MSG_DEBUG("Value = " << mValue << ", Element = " << mxmlGetElement(n));
    return mValue;
}

int TReadXML::getIntFromNode(mxml_node_t* n)
{
    DECL_TRACER("TReadXML::getIntFromNode(mxml_node_t* n)");

    if (!n)
        return 0;

    string val = getTextFromNode(n);

    if (val.empty())
        return 0;

    return atoi(val.c_str());
}

double TReadXML::getDoubleFromNode(mxml_node_t* n)
{
    DECL_TRACER("TReadXML::getDoubleFromNode(mxml_node_t* n)");

    if (!n)
        return 0.0;

    string val = getTextFromNode(n);

    if (val.empty())
        return 0.0;

    return atof(val.c_str());
}

std::string TReadXML::getAttributeFromNode(mxml_node_t* n, const std::string& attr)
{
    DECL_TRACER("TReadXML::getAttributeFromNode(mxml_node_t* n, const std::string& attr)");

    const char *a = mxmlElementGetAttr(n, attr.c_str());

    if (a)
        mAttribute = a;
    else
        mAttribute.clear();

    return mAttribute;
}

std::string& TReadXML::getElementName()
{
    DECL_TRACER("TReadXML::getElementName()");

    mElement.clear();

    if (!mLastNode)
        return mElement;

    const char *name = mxmlGetElement(mLastNode);

    if (name)
        mElement = name;

    return mElement;
}

std::string& TReadXML::getElementName(mxml_node_t* node)
{
    DECL_TRACER("TReadXML::getElementName(mxml_node_t* node)");

    mElement.clear();

    if (!node)
        return mElement;

    const char *name = mxmlGetElement(node);

    if (name)
        mElement = name;

    return mElement;
}

std::string TReadXML::_typeName(mxml_type_t t)
{
    string ret;

    switch (t)
    {
        case MXML_CUSTOM:   ret = "MXML_CUSTOM"; break;
        case MXML_ELEMENT:  ret = "MXML_ELEMENT"; break;
        case MXML_INTEGER:  ret = "MXML_INTEGER"; break;
        case MXML_OPAQUE:   ret = "MXML_OPAQUE"; break;
        case MXML_REAL:     ret = "MXML_REAL"; break;
        case MXML_TEXT:     ret = "MXML_TEXT"; break;
        case MXML_IGNORE:   ret = "MXML_IGNORE"; break;
    }

    return ret;
}

void TReadXML::extractValue(mxml_node_t *n)
{
    DECL_TRACER("TReadXML::extractValue(mxml_node_t *n)");

    const char *ename = nullptr, *txt = nullptr;
    int val = 0;
    double fval = 0.0;
    mxml_type_t t = mxmlGetType(n);

    if (!n)
        return;

    mValue.clear();
    ename = mxmlGetElement(n);

    if (ename)
        mElement = ename;
    else
        mElement.clear();

    switch(t)
    {
        case MXML_CUSTOM:   txt = (const char *)mxmlGetCustom(n); break;
        case MXML_ELEMENT:  txt = mxmlGetOpaque(n); break;

        case MXML_INTEGER:
            val = mxmlGetInteger(n);
            mValue = std::to_string(val);
            break;

        case MXML_OPAQUE:   txt = mxmlGetOpaque(n); break;
        case MXML_REAL:
            fval = mxmlGetReal(n);
            mValue = std::to_string(fval);
            break;

        case MXML_TEXT:     txt = mxmlGetText(n, 0); break;

        case MXML_IGNORE:   mValue.clear(); break;
    }

    if (txt)
        mValue = txt;

//    MSG_DEBUG("Element: " << mElement << ", type: " << _typeName(t) << ", content: " << mValue);
}

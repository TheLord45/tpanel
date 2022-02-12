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
#ifndef __TRADXML_H__
#define __TRADXML_H__

#include <stdio.h>
#include <string>
#include <iterator>
#include "terror.h"
#include "mxml.h"

class TReadXML
{
    public:
        TReadXML(const std::string& fname, bool trim=false);
        ~TReadXML();

        std::string& findElement(const std::string& name);
        std::string& findElement(const std::string& name, const std::string& attr);
        std::string& findElement(mxml_node_t *node, const std::string& name);
        std::string& findElement(mxml_node_t *node, const std::string& name, const std::string& attr);
        std::string& findNextElement(const std::string& name);
        std::string& findNextElement(const std::string& name, const std::string& attr);
        bool success() { return mValid; }
        std::string& getAttribute() { return mAttribute; }
        std::string& getAttribute(const std::string& name);
        std::string& getAttribute(const std::string& name, int index);

        std::string getText() { return mxmlGetText(mNode, 0); }
        int getInt() { return mxmlGetInteger(mNode); }
        double getDouble() { return mxmlGetReal(mNode); }

        mxml_node_t *getFirstChild();
        mxml_node_t *getNextChild();
        mxml_node_t *getLastChild();

        mxml_node_t *getFirstChild(mxml_node_t *node);
        mxml_node_t *getNextChild(mxml_node_t *node);
        mxml_node_t *getLastChild(mxml_node_t *node);

        std::string& getTextFromNode(mxml_node_t *n);
        int getIntFromNode(mxml_node_t *n);
        double getDoubleFromNode(mxml_node_t *n);
        std::string getAttributeFromNode(mxml_node_t *n, const std::string& attr);
        std::string& getElementName();
        std::string& getElementName(mxml_node_t *node);
        std::string& GetLastElementName() { return mElement; }

        mxml_node_t *getRoot() { return mTree; }
        mxml_node_t *getNode() { return mNode; }
        mxml_node_t *getLastNode() { return mLastNode; }

	protected:
        bool openFile(bool trim=false);
        void extractValue(mxml_node_t *n);

	private:
        TReadXML() {}
        std::string _typeName(mxml_type_t t);   // For internal debugging only!

        std::string mFName;             // The path and file name of the XML to parse
        mxml_node_t *mTree{0};          // The root of the tree
        mxml_node_t *mNode{0};          // The actual node
        mxml_node_t *mLastNode{0};      // The previous node, if any
        mxml_node_t *inode{0};          // Used internal for "iterating"
        std::string mValue;             // The content of last search
        std::string mAttribute;         // The last attribute found
        std::string mElement;           // The name of the current element
        bool mValid{false};             // TRUE = Last search was successfull
};

#endif

/*
 * Copyright (C) 2022 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TEXPATPP_H__
#define __TEXPATPP_H__

#include <string>
#include <vector>

#include <expat.h>
#include "tvalidatefile.h"

namespace Expat
{
    /**
     * @enum _ETYPE_t
     * @brief Internal used type identifier for elements.
     */
    typedef enum _ETYPE_t
    {
        _ET_START,      //!< Element is a start element. Other elements with a bigger depth follow.
        _ET_END,        //!< Element is an end element. Other elements with a bigger depth are before.
        _ET_ATOMIC      //!< Element is a single one which starts and stop in one line.
    }_ETYPE_t;

    typedef struct _ATTRIBUTE_t
    {
        std::string name;
        std::string content;
    }_ATTRIBUTE_t;

    typedef _ATTRIBUTE_t ATTRIBUTE_t;

    typedef struct _ELEMENT_t
    {
        std::string name;
        int depth;
        std::string content;
        std::vector<_ATTRIBUTE_t> attrs;
        _ETYPE_t eType;
    }_ELEMENT_t;

    /**
     * @enum TENCODING_t
     * @brief Defines the possible integrated encodings.
     * If the encoding is not ENC_UTF8 it must be set before the parser is
     * started. The encoding defines the character set used for the XML file.
     * If the XML file is encoded in a different character set than defined,
     * it may lead into parsing errors.
     */
    typedef enum TENCODING_t
    {
        ENC_UNKNOWN,        //!< Unknown encoding. This is equal with ENC_UTF8
        ENC_UTF8,           //!< Default encoding if not defined elsewhere
        ENC_UTF16,          //!< UTF16 encoding.
        ENC_ISO_8859_1,     //!< ISO-8859-1 encoding.
        ENC_CP1250,         //!< CP1250 (Windows) character set.
        ENC_US_ASCII        //!< US-ASCII encoding.
    }TENCODING_t;

    /**
    * @class TExpat
    * @brief C++ wrapper class for the XML parser expat.
    *
    * This class is a small C++ wrapper for the XML parser expat. Expat is part of
    * *NIX system and also in the NDK of Android. Therefor it is used in this
    * project.
    */
    class TExpat : public TValidateFile
    {
        public:
            /**
            * Constructor.
            */
            TExpat();
            /**
            * Constructor.
            *
            * @param file  The file name of a file containing the XML code to parse.
            */
            TExpat(const std::string& file);
            /**
            * Destructor.
            */
            ~TExpat();

            /**
             * @brief Sets the file to parse.
             * This method does not check the file. If it is invalid, the parser
             * will fail.
             *
             * @param file  The file to parse.
             */
            void setFile(const std::string& file) { mFile = file; }
            /**
             * @brief Set the encoding of the XML file.
             * By default UTF8 encoding is assumed if not explicitely defined
             * in the XML file. If there is an encoding defination in the XML
             * file like:
             *
             *  <?xml version="1.0" encoding="ISO-8859-2"?>
             *
             * for example, than this encoding is used by default.
             *
             * This method sets the encoding to one of the supported encodings.
             * If the XML file is encoded in another character set as defined,
             * than it may come to errors during parsing!
             *
             * @param enc   Encoding to be used for parsing the XML file.
             */
            void setEncoding(TENCODING_t enc);
            /**
             * @brief Starts the XML parser.
             * This method invoke the XML parser. If any error occure and error
             * message is logged and the error handler is set.
             *
             * @return On success it returns TRUE and FALSE otherwise.
             */
            bool parse(bool debug=false);

            /**
             * Retrieves the first element with the name \p name at the depth
             * \p depth.
             *
             * @param name  The name of the wanted element. This is case
             * sensitive!
             * @param depth The depth where the element is.
             *
             * @return On success the content of the element is returned.
             * Otherwise an empty string is returned.
             */
            std::string getElement(const std::string& name, int depth, bool *valid=nullptr);
            /**
             * Retrieves the first element with the name \p name at the depth
             * \p depth.
             *
             * @param name  The name of the wanted element. This is case
             * sensitive!
             * @param depth The depth where the element is.
             * @param valid An optional pointer retrieving the state of the
             * search. On success it returns TRUE, otherwise FALSE.
             * @return On success the content of the element is returned as an
             * integer. Otherwise 0 is returned and \p valid is set to FALSE.
             */
            int getElementInt(const std::string& name, int depth, bool *valid=nullptr);
            /**
             * Retrieves the first element with the name \p name at the depth
             * \p depth.
             *
             * @param name  The name of the wanted element. This is case
             * sensitive!
             * @param depth The depth where the element is.
             * @param valid An optional pointer retrieving the state of the
             * search. On success it returns TRUE, otherwise FALSE.
             * @return On success the content of the element is returned as a
             * long integer. Otherwise 0 is returned and \p valid is set to FALSE.
             */
            long getElementLong(const std::string& name, int depth, bool *valid=nullptr);
            /**
             * Retrieves the first element with the name \p name at the depth
             * \p depth.
             *
             * @param name  The name of the wanted element. This is case
             * sensitive!
             * @param depth The depth where the element is.
             * @param valid An optional pointer retrieving the state of the
             * search. On success it returns TRUE, otherwise FALSE.
             * @return On success the content of the element is returned as a
             * floating value. Otherwise 0 is returned and \p valid is set to FALSE.
             */
            float getElementFloat(const std::string& name, int depth, bool *valid=nullptr);
            /**
             * Retrieves the first element with the name \p name at the depth
             * \p depth.
             *
             * @param name  The name of the wanted element. This is case
             * sensitive!
             * @param depth The depth where the element is.
             * @param valid An optional pointer retrieving the state of the
             * search. On success it returns TRUE, otherwise FALSE.
             * @return On success the content of the element is returned as a
             * double value. Otherwise 0 is returned and \p valid is set to FALSE.
             */
            double getElementDouble(const std::string& name, int depth, bool *valid=nullptr);
            /**
             * Retrieves the first element with the name \p name at the depth
             * \p depth.
             *
             * @param name  The name of the wanted element. This is case
             * sensitive!
             * @param depth The depth where the element is.
             *
             * @return On success returns the index of the element found.
             * Otherwise TExpat::npos is returned.
             */
            size_t getElementIndex(const std::string& name, int depth);
            /**
             * @brief Retrieves the first element with the name \p name.
             * This method saves the found position internally. If no matching
             * entity was found, the internal position is set to an invalid
             * position.
             *
             * @param name  The name of the wanted element. This is case
             * sensitive!
             * @param depth This parameter is optional and can be NULL. If it is
             * present, the method returns the depth of the found element with
             * this parameter. If the element was not found, this parameter
             * will contain -1.
             *
             * @return On success returns the index of the element found.
             * Otherwise TExpat::npos is returned.
             */
            size_t getElementIndex(const std::string& name, int *depth);
            /**
             * Retrieves the \p name, the \p content and the attributes of the
             * element. If the element has no attributes, an empty attribute
             * list is returned.
             *
             * This method does not set the internal position. This means that
             * a method depending on the internal position like getNextElement()
             * will not succeed or return the content of another entity!
             *
             * @param index The index of the element. This value may be
             * retrieved by calling previously the method getElementIndex().
             * @param name  This parameter can be NULL. If it is present, the
             * name of the element is returned.
             * @param content   This parameter can be NULL. If it is present,
             * the content of the element is returned. If there was no content,
             * an empty string is returned.
             * @param attrs This parameter can be NULL. If it is present and if
             * the element contains at least 1 attribute, the attributes are
             * returned in the vector.
             *
             * @return If a valid element was found, the index is returned.
             * Otherwise TExpat::npos will be returned.
             */
            size_t getElementFromIndex(size_t index, std::string *name, std::string *content, std::vector<ATTRIBUTE_t> *attrs);
            /**
             * @brief Retrieves the next element from the given index \p index.
             * The method succeeds if the next element is not an end tag or if
             * \p index is less than the number of total elements. The method
             * increases the index by 1 and returns this entity.
             *
             * This method does not set the internal position. This means that
             * a method depending on the internal position like getNextElement()
             * will not succeed or return the content of another entity!
             *
             * @param index The index of the element. This value may be
             * retrieved by calling previously the method getElementIndex().
             * @param name  This parameter can be NULL. If it is present, the
             * name of the element is returned.
             * @param content   This parameter can be NULL. If it is present,
             * the content of the element is returned. If there was no content,
             * an empty string is returned.
             * @param attrs This parameter can be NULL. If it is present and if
             * the element contains at least 1 attribute, the attributes are
             * returned in the vector.
             *
             * @return If a valid element was found, the index is returned.
             * Otherwise TExpat::npos will be returned.
             */
            size_t getNextElementFromIndex(size_t index, std::string *name, std::string *content, std::vector<ATTRIBUTE_t> *attrs);
            /**
             * @brief Retrieves the first element with the name \p name.
             *
             * This method saves the found position internally. If no matching
             * entity was found, the internal position is set to an invalid
             * position.
             *
             * @param name  The name of the wanted element.
             * @param depth A pointer to an integer. The method returns the
             * depth of the found element in this parameter.
             *
             * @return On success the content of the element is returned, if
             * any. Otherwise an error is logged and an empty string is returned.
             * The parameter \p depth is set to -1 on error.
             */
            std::string getFirstElement(const std::string& name, int *depth);
            /**
             * Retrieves the next element in the list. This method depends on
             * the method getFirstElement(), which must be called previous.
             *
             * This method saves the found position internally. If no matching
             * entity was found, the internal position is set to an invalid
             * position.
             *
             * @param name  The name of the element to search for. This
             * parameter is optional and may be NULL.
             * @param depth The depth of the element to return. If there are
             * no more elements with the name \p name and the depth \p depth
             * than an empty string is returned.
             *
             * @return On success the content of the element is returned. If
             * an error occurs an error message is logged and the error is set.
             */
            std::string getNextElement(const std::string& name, int depth, bool *valid=nullptr);
            /**
             * Retrieves the next element in the list. This method depends on
             * the method getFirstElement(), which must be called previous.
             *
             * This method saves the found position internally. If no matching
             * entity was found, the internal position is set to an invalid
             * position.
             *
             * @param name  The name of the element to search for. This
             * parameter is optional and may be NULL.
             * @param depth The depth of the element to return. If there are
             * no more elements with the name \p name and the depth \p depth
             * than an empty string is returned.
             *
             * @return On success the index of the element is returned. If
             * an error occurs an error message is logged and the error is set.
             */
            size_t getNextElementIndex(const std::string& name, int depth);
            /**
             * Searches in the attribute list of an attribute called \p name
             * and returns the content.
             *
             * @param name  The name of the wanted attribute.
             * @param attrs A vector list of attributes.
             *
             * @return On success returns the content of the attribute \p name.
             * Otherwise an empty string is returned.
             */
            std::string getAttribute(const std::string& name, std::vector<ATTRIBUTE_t>& attrs);
            /**
             * Searches in the attribute list of an attribute called \p name
             * and returns the content.
             *
             * @param name  The name of the wanted attribute.
             * @param attrs A vector list of attributes.
             *
             * @return On success returns TRUE or FALSE depending on the
             * content.
             */
            bool getAttributeBool(const std::string& name, std::vector<ATTRIBUTE_t>& attrs);
            /**
             * Searches in the attribute list of an attribute called \p name
             * and returns the content.
             *
             * @param name  The name of the wanted attribute.
             * @param attrs A vector list of attributes.
             *
             * @return On success returns the content of the attribute \p name
             * as an integer value.
             * Otherwise an error text is set (TError) and 0 is returned.
             */
            int getAttributeInt(const std::string& name, std::vector<ATTRIBUTE_t>& attrs);
            /**
             * Searches in the attribute list of an attribute called \p name
             * and returns the content.
             *
             * @param name  The name of the wanted attribute.
             * @param attrs A vector list of attributes.
             *
             * @return On success returns the content of the attribute \p name
             * as a long value.
             * Otherwise an error text is set (TError) and 0 is returned.
             */
            long getAttributeLong(const std::string& name, std::vector<ATTRIBUTE_t>& attrs);
            /**
             * Searches in the attribute list of an attribute called \p name
             * and returns the content.
             *
             * @param name  The name of the wanted attribute.
             * @param attrs A vector list of attributes.
             *
             * @return On success returns the content of the attribute \p name
             * as a floating value.
             * Otherwise an error text is set (TError) and 0 is returned.
             */
            float getAttributeFloat(const std::string& name, std::vector<ATTRIBUTE_t>& attrs);
            /**
             * Searches in the attribute list of an attribute called \p name
             * and returns the content.
             *
             * @param name  The name of the wanted attribute.
             * @param attrs A vector list of attributes.
             *
             * @return On success returns the content of the attribute \p name
             * as a double precission floating value.
             * Otherwise an error text is set (TError) and 0 is returned.
             */
            double getAttributeDouble(const std::string& name, std::vector<ATTRIBUTE_t>& attrs);
            /**
             * Converts a string into a boolean.
             *
             * @param content   A string containing numbers.
             * @return Returns the boolean value.
             */
            bool convertElementToBool(const std::string& content);
            /**
             * Converts a string into an integer value.
             *
             * @param content   A string containing numbers.
             * @return On success returns the value representation of the
             * \p content. If not convertable, TError is set and 0 is returned.
             */
            int convertElementToInt(const std::string& content);
            /**
             * Converts a string into a long integer value.
             *
             * @param content   A string containing numbers.
             * @return On success returns the value representation of the
             * \p content. If not convertable, TError is set and 0 is returned.
             */
            long convertElementToLong(const std::string& content);
            /**
             * Converts a string into a floating value.
             *
             * @param content   A string containing numbers.
             * @return On success returns the value representation of the
             * \p content. If not convertable, TError is set and 0 is returned.
             */
            float convertElementToFloat(const std::string& content);
            /**
             * Converts a string into a double floating value.
             *
             * @param content   A string containing numbers.
             * @return On success returns the value representation of the
             * \p content. If not convertable, TError is set and 0 is returned.
             */
            double convertElementToDouble(const std::string& content);
            /**
             * Sets the internal pointer to the \p index. If this points to an
             * invalid index, the method sets an error and returns FALSE.
             *
             * @param index The index into the internal XML list.
             * @return On success it returns TRUE.
             */
            bool setIndex(size_t index);
            /**
             * Retrieves the attribute list from the current position.
             *
             * @return On success the attributes of the current position are
             * returned. If the current position is at the end or doesn't
             * has attributes, an empty list is returned.
             */
            std::vector<ATTRIBUTE_t> getAttributes();
            /**
             * Retrieves the attribute list from the \p index.
             *
             * @return On success the attributes of the position \p index are
             * returned. If the position \p index is at the end or doesn't
             * has attributes, an empty list is returned.
             */
            std::vector<ATTRIBUTE_t> getAttributes(size_t index);

            /**
             * Checks whether the internal pointer points to a valid entry or
             * not. If the internal pointer is valid it returns the name of the
             * entity the pointer points to.
             *
             * @param valid This is an optional parameter. If this is set, then
             * it is set to TRUE when the method succeeds and to FALSE
             * otherwise.
             *
             * @return On success it returns the name of the entity and set the
             * parameter \p valid to TRUE. On error it returns an empty string
             * and sets the parameter \p valid to FALSE.
             */
            std::string getElementName(bool *valid=nullptr);
            /**
             * @brief getElementName
             * Retrievs the name of the element \b index is pointing to. If
             * \b index is pointing to an invalid element, an empty string is
             * returned and \b valid is set to false.
             *
             * @param index     The index number of the element.
             * @param valid     The state of the element \b index is pointing to.
             *                  FALSE = invalid element.
             * @return The name of the element or an empty string if the element
             * is invalid.
             */
            std::string getElementName(size_t index, bool *valid=nullptr);
            /**
             * Checks whether the internal pointer points to a valid entry or
             * not. If the internal pointer is valid it returns the content of
             * the entity the pointer points to.
             *
             * @param valid This is an optional parameter. If this is set, then
             * it is set to TRUE when the method succeeds and to FALSE
             * otherwise.
             *
             * @return On success it returns the content of the entity and set
             * the parameter \p valid to TRUE. On error it returns an empty
             * string and sets the parameter \p valid to FALSE.
             */
            std::string getElementContent(bool *valid=nullptr);
            /**
             * Checks whether the internal pointer points to a valid entry or
             * not. If the internal pointer is valid it returns the content of
             * the entity converted to an integer.
             *
             * @param valid This is an optional parameter. If this is set, then
             * it is set to TRUE when the method succeeds and to FALSE
             * otherwise.
             *
             * @return On success it returns the content of the entity converted
             * to an integer and set  the parameter \p valid to TRUE. On error
             * it returns an empty string and sets the parameter \p valid to
             * FALSE.
             */
            int getElementContentInt(bool *valid=nullptr);
            /**
             * Checks whether the internal pointer points to a valid entry or
             * not. If the internal pointer is valid it returns the content of
             * the entity converted to a long.
             *
             * @param valid This is an optional parameter. If this is set, then
             * it is set to TRUE when the method succeeds and to FALSE
             * otherwise.
             *
             * @return On success it returns the content of the entity converted
             * to a long and set  the parameter \p valid to TRUE. On error
             * it returns an empty string and sets the parameter \p valid to
             * FALSE.
             */
            long getElementContentLong(bool *valid=nullptr);
            /**
             * Checks whether the internal pointer points to a valid entry or
             * not. If the internal pointer is valid it returns the content of
             * the entity converted to a floating value.
             *
             * @param valid This is an optional parameter. If this is set, then
             * it is set to TRUE when the method succeeds and to FALSE
             * otherwise.
             *
             * @return On success it returns the content of the entity converted
             * to a float and set  the parameter \p valid to TRUE. On error
             * it returns an empty string and sets the parameter \p valid to
             * FALSE.
             */
            float getElementContentFloat(bool *valid=nullptr);
            /**
             * Checks whether the internal pointer points to a valid entry or
             * not. If the internal pointer is valid it returns the content of
             * the entity converted to a double.
             *
             * @param valid This is an optional parameter. If this is set, then
             * it is set to TRUE when the method succeeds and to FALSE
             * otherwise.
             *
             * @return On success it returns the content of the entity converted
             * to a double and set  the parameter \p valid to TRUE. On error
             * it returns an empty string and sets the parameter \p valid to
             * FALSE.
             */
            double getElementContentDouble(bool *valid=nullptr);
            /**
             * @brief getDepth
             * Returns the actual depth of the the current index.
             *
             * @return Return the actual depth.
             */
            int getDepth() { return mDepth; }
            /**
             * @brief getDepth
             * Gets the depth from the element \b index is pointing to. If
             * \b index is NPOS or pointing to an end element, -1 is returned.
             *
             * @param index     The index number of the element.
             * @return The depth of the element or -1 if it is an invalid element.
             */
            int getDepth(size_t index);

            static const size_t npos = static_cast<size_t>(-1); //!< Marks an invalid index

        protected:
            static void startElement(void* userData, const XML_Char* name, const XML_Char** attrs);
            static void XMLCALL endElement(void *userData, const XML_Char *name);
            static void XMLCALL CharacterDataHandler(void *, const XML_Char *s, int len);
            static int cp1250_encoding_handler(void*, const XML_Char* encoding, XML_Encoding* info);

        private:
            static void createCP1250Encoding(XML_Encoding *enc);    //!< Internal handler to handle CP1250 encoded files.

            std::string mFile;                                      //!< The name of a file containing the XML code to parse
            std::vector<_ELEMENT_t> mElements;                      //!< The list of elemets in the order they appeared
            std::vector<_ELEMENT_t>::iterator mLastIter{mElements.end()};   //!< The pointer to the last iterator
            std::string mEncoding{"UTF-8"};                         //!< Encoding of the XML file. UTF-8 is default encoding.
            TENCODING_t mSetEncoding{ENC_UTF8};                     //!< Encoding of the XML file. UTF-8 is default encoding.
//            int mLastDepth{0};                                      //!< The depth of the last found element.
            // Variables used for the static methods
            static int mDepth;
            static std::string mContent;
            static std::string mLastName;
            static std::vector<_ATTRIBUTE_t> mAttributes;
    };
}

#endif

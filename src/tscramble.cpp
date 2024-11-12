/*
 * Copyright (C) 2024 by Andreas Theofilu <andreas@theosys.at>
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
#include <cstring>

#include <openssl/err.h>
#include <openssl/evp.h>

#include "tscramble.h"
#include "terror.h"

using std::string;
using std::ifstream;
using std::ios_base;
using std::ios;
using std::exception;
using std::min;
using std::cout;
using std::cerr;
using std::endl;

#define CHUNK_SIZE          1024
#define OSSL_SUCCESS        1
#define OSSL_ERROR          0
static unsigned long ssl_errno;

/*
 * Internally used helper functions
 */
void _print_ssl_error(unsigned long eno)
{
    char msg[1024];

    ERR_error_string_n(eno, msg, sizeof(msg));
    MSG_ERROR(msg);
}

string _get_ssl_error()
{
    char msg[1024];

    ERR_error_string_n(ssl_errno, msg, sizeof(msg));
    return string(msg);
}

/* -------------------------------------------------------------------------
 * Implementaion of methods from class TScramble
 * -------------------------------------------------------------------------*/
TScramble::TScramble()
{
    DECL_TRACER("TScramble::TScramble()");

    mCtx = EVP_CIPHER_CTX_new();

    if (!mCtx)
        MSG_ERROR("Error getting new context!");
}

TScramble::~TScramble()
{
    DECL_TRACER("TScramble::~TScramble()");

    if (mCtx)
    {
        EVP_CIPHER_CTX_cleanup(mCtx);
        EVP_CIPHER_CTX_free(mCtx);
    }
}

bool TScramble::aesInit(const string& key, const string& salt)
{
    DECL_TRACER("TScramble::aesInit(const string& key, const string& salt)");

    if (mAesInitialized)
        return true;

    if (!mCtx)
    {
        MSG_ERROR("ERROR: No context available! Initialisation failed!");
        return false;
    }

    int keySize;
    int count = 5;      // Number iterations

    const EVP_MD *md = EVP_sha1();
    const EVP_CIPHER *pCipher = EVP_aes_128_cbc();

    if (!md)
    {
        MSG_ERROR("Error getting SHA1 hash function!");
        return false;
    }

    if (!pCipher)
    {
        MSG_ERROR("Error getting the AES128-CBC cipher algorithm!");
        return false;
    }

    // If the given salt is less then AES128_SALT_SIZE bytes, we first initialize
    // the buffer with 0 bytes. Then the salt is copied into the buffer. This
    // guaranties that we have a padding.
    memset(mAesSalt, 0, AES128_SALT_SIZE);
    memcpy(mAesSalt, salt.c_str(), min((size_t)AES128_SALT_SIZE, salt.length()));
    // Initialize the key and IV with 0
    memset(mAesKey, 0, AES128_KEY_SIZE);
    memset(mAesIV, 0, AES128_KEY_SIZE);

    keySize = EVP_BytesToKey(pCipher, md, mAesSalt, (unsigned char *)key.c_str(), static_cast<int>(key.length()), count, mAesKey, mAesIV);

    if (keySize == AES128_KEY_SIZE)
    {
        EVP_CIPHER_CTX_init(mCtx);

        if ((ssl_errno = EVP_DecryptInit_ex(mCtx, pCipher, nullptr, mAesKey, mAesIV)) != OSSL_SUCCESS)
        {
            MSG_ERROR("Error initializing: " << _get_ssl_error());
            return false;
        }

        EVP_CIPHER_CTX_set_key_length(mCtx, AES128_KEY_SIZE);
    }
    else
    {
        MSG_ERROR("Key size is " << (keySize * 8) << " bits - should be 128 bits");
        return false;
    }

    mAesInitialized = true;
    return mAesInitialized;
}

bool TScramble::aesDecodeFile(const string& fname)
{
    DECL_TRACER("TScramble::aesDecodeFile(const string& fname)");

    if (fname.empty())
    {
        MSG_ERROR("Got no file name to open a file!");
        return false;
    }

    ifstream ifile;

    try
    {
        ifile.open(fname, ios::binary);
        bool state = aesDecodeFile(ifile);
        ifile.close();
        return state;
    }
    catch(exception& e)
    {
        MSG_ERROR("Error opening file \"" << fname << "\": " << e.what());

        if (ifile.is_open())
            ifile.close();
    }

    return false;
}

bool TScramble::aesDecodeFile(ifstream& is)
{
    DECL_TRACER("TScramble::aesDecodeFile(ifstream& is)");

    if (!is.is_open())
    {
        MSG_ERROR("No open file!");
        return false;
    }

    if (!mAesInitialized || !mCtx)
    {
        MSG_ERROR("Class was not initialized!");
        return false;
    }

    mDecrypted.clear();
    // Find file size
    is.seekg(0, ios_base::end);         // Seek to end of file
    size_t fileSize = is.tellg();       // Get current position (file size)
    is.seekg(0);                        // Seek to first byte of file
    size_t pos = 0;                     // Position (offset) in input and output buffer
    // Allocate space for input and output buffers
    unsigned char *buffer = new unsigned char[fileSize];
    unsigned char *outBuffer = new unsigned char[fileSize];
    unsigned char decBuffer[CHUNK_SIZE];    // Buffer for decoding a chunk.
    unsigned char encBuffer[CHUNK_SIZE];    // Buffer for decoding a chunk.
    int len = 0;
    // Not really necessary, but initialize output buffer with zeros
    memset(outBuffer, 0, fileSize);
    // Read whole file
    is.read(reinterpret_cast<char *>(buffer), fileSize);
    // decode
    if (fileSize > CHUNK_SIZE)     // Is the file size greater then a chunk?
    {                               // Yes, then start reading the file in chunks
        size_t numBlocks = fileSize / CHUNK_SIZE;

        for (size_t i = 0; i < numBlocks; ++i)
        {
            memcpy(encBuffer, buffer + i * CHUNK_SIZE, CHUNK_SIZE);

            if ((ssl_errno = EVP_DecryptUpdate(mCtx, decBuffer, &len, encBuffer, CHUNK_SIZE)) != OSSL_SUCCESS)
            {
                MSG_ERROR("Loop " << i << ": Error updating");
                _print_ssl_error(ssl_errno);
                delete[] buffer;
                delete[] outBuffer;
                return false;
            }

            memcpy(outBuffer+pos, decBuffer, len);
            pos += len;
        }

        size_t size2 = fileSize - (numBlocks * CHUNK_SIZE);

        if (size2 > 0)      // Is there something left of the file less then CHUNK_SIZE?
        {                   // Yes, then decrypt it
            memcpy(encBuffer, buffer + numBlocks * CHUNK_SIZE, size2);

            if ((ssl_errno = EVP_DecryptUpdate(mCtx, decBuffer, &len, encBuffer, static_cast<int>(size2))) != OSSL_SUCCESS)
            {
                MSG_ERROR("Error updating");
                _print_ssl_error(ssl_errno);
                delete[] buffer;
                delete[] outBuffer;
                return false;
            }

            memcpy(outBuffer+pos, decBuffer, len);
            pos += len;
        }
    }
    else
    {
        if ((ssl_errno = EVP_DecryptUpdate(mCtx, outBuffer, &len, buffer, static_cast<int>(fileSize))) != OSSL_SUCCESS)
        {
            _print_ssl_error(ssl_errno);
            delete[] buffer;
            delete[] outBuffer;
            return false;
        }

        pos = len;
    }

    if ((ssl_errno = EVP_DecryptFinal_ex(mCtx, outBuffer + pos, &len)) != OSSL_SUCCESS)
    {
        MSG_ERROR("Error finalizing: " << _get_ssl_error());
        delete[] buffer;
        delete[] outBuffer;
        return false;
    }

    pos += len;
    mDecrypted.assign(reinterpret_cast<char *>(outBuffer), pos);
    delete[] buffer;
    delete[] outBuffer;

    return true;
}

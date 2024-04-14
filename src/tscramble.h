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

#ifndef TSCRAMBLE_H
#define TSCRAMBLE_H

#include <string>
#include <openssl/core.h>

#define AES128_KEY_SIZE     16
#define AES128_SALT_SIZE    8

class TScramble
{
    public:
        TScramble();
        ~TScramble();

        bool aesInit(const std::string& key, const std::string& salt);
        bool aesDecodeFile(std::ifstream& is);
        bool aesDecodeFile(const std::string& fname);
        unsigned char *getAesKey(size_t& len) { len = AES128_KEY_SIZE; return mAesKey; }
        unsigned char *getAesSalt(size_t& len) { len = AES128_SALT_SIZE; return mAesSalt; }
        unsigned char *getAesIV(size_t& len) { len = AES128_KEY_SIZE; return mAesIV; }
        std::string& getDecrypted() { return mDecrypted; }
        void aesReset() { mAesInitialized = false; }

    private:
        unsigned char mAesKey[AES128_KEY_SIZE];     // Key used for decryption/encryption
        unsigned char mAesSalt[AES128_SALT_SIZE];   // Salt for decryption/encryption
        unsigned char mAesIV[AES128_KEY_SIZE];      // Initialization Vector
        EVP_CIPHER_CTX *mCtx{nullptr};              // Context
        std::string mDecrypted;                     // Buffer for clear (decrypted) text
        bool mAesInitialized{false};                // TRUE if initialized
};

#endif // TSCRAMBLE_H

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

#ifndef __TSOCKET_H__
#define __TSOCKET_H__

#include <string>

#include <netdb.h>
#include <openssl/ssl.h>

class TSocket
{
    public:
        TSocket();
        TSocket(const std::string& host, int port);
        ~TSocket();

        bool connect(bool encrypt=false);
        bool connect(const std::string& host, int port, bool encrypt=false);
        size_t receive(char *buffer, size_t size, bool doPoll=true);
        size_t readAbsolut(char *buffer, size_t size);
        ssize_t send(char *buffer, size_t size);
        bool close();
        bool isConnected() { return mConnected; }
        std::string& getMyIP() { return mMyIP; }
        std::string& getMyHostName() { return mMyHostName; }
        std::string& getMyNetmask() { return mMyNetmask; }
        int retrieveSSLerror(int rcode) { return SSL_get_error(mSsl, rcode); }
        int getSocket() { return mSockfd; }

        static const ssize_t npos = static_cast<ssize_t>(-1);

    protected:
        struct addrinfo *lookup_host (const std::string& host, int port);
        void initSSL();
        SSL_CTX *initCTX();
        void log_ssl_error();
        bool isSockValid();
        bool getHost();
        bool determineNetmask(int socket);

    private:
        std::string mHost;
        int mPort{0};
        int mSockfd{-1};
        bool mConnected{false};
        bool mEncrypted{false};
        bool mSSLInitialized{false};
        SSL_CTX *mCtx{nullptr};
        SSL *mSsl{nullptr};
        std::string mUser;
        std::string mPassword;
        std::string mMyIP;
        std::string mMyHostName;
        std::string mMyNetmask;
};

#endif

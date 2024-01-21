/*
 * Copyright (C) 2021, 2022 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __THTTPCLIENT_H__
#define __THTTPCLIENT_H__

#include <string>
#include <vector>

#include <netdb.h>
#include <openssl/ssl.h>

#include "tsocket.h"

typedef enum METHOD_t
{
    UNSUPPORTED,
    GET,
    PUT,
    POST,
    HEAD
}METHOD_t;

typedef struct URL_t
{
    std::string scheme;
    std::string host;
    int port{0};
    std::string path;
    std::string user;
    std::string pw;

    void clear()
    {
        scheme.clear();
        host.clear();
        port = 0;
        path.clear();
        user.clear();
        pw.clear();
    }
}URL_t;

typedef struct HTTPHEAD_t
{
    std::string name;
    std::string content;
}HTTPHEAD_t;

typedef struct HTTPBODY_t
{
    char *body{nullptr};
    size_t len{0};

    void clear()
    {
        if (body != nullptr)
            delete[] body;

        body = nullptr;
        len = 0;
    }
}HTTPBODY_t;

typedef struct HTTPREQUEST_t
{
    METHOD_t method{UNSUPPORTED};
    int direction{0};
    int code{0};            // on receive
    char *status{nullptr};  // on receive
    char *path{nullptr};
    char *version{nullptr};

    void clear()
    {
        direction = 0;
        code = 0;
        method = UNSUPPORTED;

        if (status != nullptr)
            delete[] status;

        status = nullptr;

        if (path != nullptr)
            delete[] path;

        path = nullptr;

        if (version != nullptr)
            delete[] version;

        version = nullptr;
    }
}HTTPREQUEST_t;

class THTTPClient : public TSocket
{
    public:
        THTTPClient();
        ~THTTPClient();

        char *tcall(size_t *size, const std::string& URL, const std::string& user, const std::string& pw);
        std::string makeURL(const std::string& scheme, const std::string& host, int port, const std::string& path);
        static std::string makeURLs(const std::string& scheme, const std::string& host, int port, const std::string& path);
//        char *getContent(const char *buffer);
        char *getContent() { return mBody.body; }
        size_t getContentSize() { return mBody.len; }

    protected:
        URL_t& parseURL(const std::string& URL);
        int sockWrite(int fd, char *buffer, size_t len);
        int sockRead(int fd, char *buffer, size_t len);
        int parseHeader(const char *buffer, size_t len);
        std::string getHeadParameter(const std::string& name);
        std::string makeRequest(const std::string& url);

    private:
        URL_t mURL;
        std::string mUser;
        std::string mPassword;
        std::vector<HTTPHEAD_t> mHeader;
        HTTPBODY_t mBody;
        HTTPREQUEST_t mRequest;
};

#endif

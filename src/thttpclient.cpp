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

#include <vector>
#include <thread>

#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <poll.h>

#include "thttpclient.h"
#include "base64.h"
#include "terror.h"
#include "tresources.h"
#include "tconfig.h"
#include "texcept.h"

#define HTTP_DIRECTION_RECEIVE  1
#define HTTP_DIRECTION_SEND     2

using std::string;
using std::vector;
using std::min;

THTTPClient::THTTPClient()
{
    DECL_TRACER("THTTPClient::THTTPClient()");
    mBody.body = nullptr;
    mBody.len = 0;
    mRequest.code = 0;
    mRequest.direction = 0;
    mRequest.method = UNSUPPORTED;
    mRequest.path = nullptr;
    mRequest.status = nullptr;
    mRequest.version = nullptr;
}

THTTPClient::~THTTPClient()
{
    DECL_TRACER("THTTPClient::~THTTPClient()");
    mBody.clear();
    mRequest.clear();
}

#define MAX_BUFFER  65535
#define MAX_BLOCK   32767

char *THTTPClient::tcall(size_t *size, const string& URL, const string& user, const string& pw)
{
    DECL_TRACER("THTTPClient::tcall(size_t size, const string& URL, const string& user, const string& pw)");

    char *buffer = nullptr;
    size_t bufsize = 0;
    mUser = user;
    mPassword = pw;
    bool encrypt = false;

    try
    {
        buffer = new char[MAX_BUFFER];
        bufsize = MAX_BUFFER;
    }
    catch (std::exception& e)
    {
        MSG_ERROR(e.what());
        return nullptr;
    }

    string request = makeRequest(URL);

    if (TError::isError() || request.empty())
    {
        delete[] buffer;
        return nullptr;
    }

    if (mURL.scheme == "https")
        encrypt = true;

    if (!connect(mURL.host, mURL.port, encrypt))
    {
        delete[] buffer;
        return nullptr;
    }

    size_t ret = 0;
    bool repeat = false;

    try
    {
        do
        {
            if ((ret = send((char *)request.c_str(), request.length())) < 0)
            {
                if (errno)
                {
                    MSG_ERROR("[" << mURL.host << "] Write error: " << strerror(errno));
                }
                else if (encrypt)
                {
                    int err = retrieveSSLerror(static_cast<int>(ret));
                    repeat = false;

                    switch (err)
                    {
                        case SSL_ERROR_ZERO_RETURN:     MSG_ERROR("The TLS/SSL peer has closed the connection for writing by sending the close_notify alert."); break;

                        case SSL_ERROR_WANT_READ:
                        case SSL_ERROR_WANT_WRITE:      MSG_TRACE("The operation did not complete and can be retried later."); repeat = true; break;
                        case SSL_ERROR_WANT_CONNECT:
                        case SSL_ERROR_WANT_ACCEPT:     MSG_TRACE("The operation did not complete; the same TLS/SSL I/O function should be called again later."); repeat = true; break;
                        case SSL_ERROR_WANT_X509_LOOKUP:MSG_TRACE("The operation did not complete because an application callback set by SSL_CTX_set_client_cert_cb() has asked to be called again."); repeat = true; break;
                        case SSL_ERROR_WANT_ASYNC:      MSG_TRACE("The operation did not complete because an asynchronous engine is still processing data."); repeat = true; break;
                        case SSL_ERROR_WANT_ASYNC_JOB:  MSG_TRACE("The asynchronous job could not be started because there were no async jobs available in the pool."); repeat = true; break;
                        case SSL_ERROR_WANT_CLIENT_HELLO_CB: MSG_TRACE("The operation did not complete because an application callback set by SSL_CTX_set_client_hello_cb() has asked to be called again."); repeat = true; break;

                        case SSL_ERROR_SYSCALL:         MSG_ERROR("Some non-recoverable, fatal I/O error occurred."); break;
                        case SSL_ERROR_SSL:             MSG_ERROR("A non-recoverable, fatal error in the SSL library occurred, usually a protocol error."); break;

                        default:
                            MSG_ERROR("Unknown error " << err << " occured!");
                    }
                }
                else
                {
                    MSG_ERROR("[" << mURL.host << "] Write error!");
                }

                if (!repeat)
                {
                    close();

                    if (buffer)
                        delete[] buffer;

                    return nullptr;
                }
            }

            if (repeat)
                std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }
        while (repeat);
    }
    catch (TXceptNetwork& e)
    {
        MSG_ERROR("Error writing to " << mURL.host << ":" << mURL.port);

        if (buffer)
            delete[] buffer;

        return nullptr;
    }

    char buf[8194];
    memset(buf, 0, sizeof(buf));
    size_t pos = 0, length = 0;
    size_t rlen, totalLen = 0;
    int loop = 0;

    try
    {
        std::chrono::steady_clock::time_point timePoint = std::chrono::steady_clock::now();
        totalLen = 0;

        while ((rlen = receive(buf, sizeof(buf))) > 0 && rlen != TSocket::npos)
        {
            size_t len = rlen;

            if (totalLen == 0 && loop < 2)
            {
                // Let's see if we have already the content length
                char *cLenPos = nullptr;

                if ((cLenPos = strnstr(buf, "Content-Length:", rlen)) != nullptr)
                {
                    cLenPos += 16;  // Start of content length information
                    size_t cLen = atol(cLenPos);

                    char *cStart = strstr(buf, "\r\n\r\n"); // Find start of content

                    if (cStart)
                        totalLen = cLen + ((cStart + 4) - buf);

                    MSG_DEBUG("Total length: " << totalLen << ", content length: " << cLen);
                }
            }

            if ((pos + len) >= bufsize)
            {
                renew(&buffer, bufsize, bufsize + MAX_BLOCK);

                if (!buffer)
                {
                    MSG_ERROR("[" << mURL.host << "] Memory error: Allocating of " << (bufsize + MAX_BLOCK) << " failed!");
                    return nullptr;
                }

                bufsize += MAX_BLOCK;
            }

            if (len > 0)
            {
                memcpy(buffer+pos, buf, len);
                pos += len;
                length += len;
            }

            if (length && totalLen && length >= totalLen)
                break;

            memset(buf, 0, sizeof(buf));
            loop++;
        }

        if (TStreamError::checkFilter(HLOG_DEBUG))
        {
            std::chrono::steady_clock::time_point endPoint = std::chrono::steady_clock::now();
            std::chrono::nanoseconds difftime = endPoint - timePoint;
            std::chrono::seconds secs = std::chrono::duration_cast<std::chrono::seconds>(difftime);
            std::chrono::milliseconds msecs = std::chrono::duration_cast<std::chrono::milliseconds>(difftime) - std::chrono::duration_cast<std::chrono::seconds>(secs);
            std::stringstream s;
            s << std::chrono::duration_cast<std::chrono::nanoseconds> (difftime).count() << "[ns]" << " --> " << std::chrono::duration_cast<std::chrono::seconds>(secs).count() << "s " << std::chrono::duration_cast<std::chrono::milliseconds>(msecs).count() << "ms";
            MSG_DEBUG("[" << mURL.host << "] Elapsed time for receive: " << s.str());
        }
    }
    catch(TXceptNetwork& e)
    {
        MSG_ERROR("Error reading from " << mURL.host << ": " << e.what());

        if (buffer)
            delete[] buffer;

        return nullptr;
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error reading from " << mURL.host << ": " << e.what());
        close();

        if (buffer)
            delete[] buffer;

        return nullptr;
    }

    int myerrno = errno;
    close();

    if (pos == 0)
    {
        if (myerrno)
        {
            MSG_ERROR("Internal read error [" << mURL.host << "]: " << strerror(myerrno));
        }
        else
        {
            MSG_ERROR("Internal read error: Received no data from " << mURL.host);
        }

        if (buffer)
            delete[] buffer;

        return nullptr;
    }

    MSG_DEBUG("[" << mURL.host << "] Read " << length << " bytes.");

    if (parseHeader(buffer, length) != 0)
    {
        if (buffer)
            delete[] buffer;

        return nullptr;
    }

    if (mRequest.code >= 300)
    {
        if (mRequest.status && *mRequest.status)
        {
            MSG_ERROR("[" << mURL.host << "] " << mRequest.code << ": " << mRequest.status);
        }
        else
        {
            MSG_ERROR("[" << mURL.host << "] " << mRequest.code << ": UNKNOWN");
        }

        if (buffer)
            delete[] buffer;

        return nullptr;
    }

    if (buffer)
        delete[] buffer;

    *size = mBody.len;
    return mBody.body;
}

string THTTPClient::makeURL(const string& scheme, const string& host, int port, const string& path)
{
    DECL_TRACER("THTTPClient::makeURL(const string& scheme, const string& host, int port, const string& path)");

    string url = scheme + "://" + host;

    if (port > 0)
        url += std::to_string(port);

    url += "/" + path;
    MSG_DEBUG("URL: " << url);
    return url;
}

string THTTPClient::makeURLs(const string& scheme, const string& host, int port, const string& path)
{
    DECL_TRACER("THTTPClient::makeURLs(const string& scheme, const string& host, int port, const string& path)");

    THTTPClient c;
    return c.makeURL(scheme, host, port, path);
}

URL_t& THTTPClient::parseURL(const string& URL)
{
    DECL_TRACER("THTTPClient::parseURL(const string& URL, const string& user, const string& pw)");

    if (URL.empty())
    {
        MSG_ERROR("Invalid empty URL!");
        TError::setError();
        mURL.clear();
        return mURL;
    }

    size_t pos = URL.find("://");

    if (pos == string::npos)
    {
        MSG_ERROR("Invalid URL: " << URL);
        TError::setError();
        mURL.clear();
        return mURL;
    }

    mURL.scheme = URL.substr(0, pos);
    pos += 3;
    string part = URL.substr(pos);
    pos = part.find("/");
    string host;

    if (pos == string::npos)
    {
        host = part;
        part.clear();
    }
    else
    {
        host = part.substr(0, pos);
        part = part.substr(pos + 1);
    }

    pos = host.find(":");

    if (pos != string::npos)
    {
        string sport = host.substr(pos + 1);
        mURL.host = host.substr(0, pos);
        mURL.port = atoi(sport.c_str());
    }
    else
        mURL.host = host;

    if (!part.empty())
        mURL.path = part;

    pos = host.find("@");

    if (pos != string::npos)
    {
        mURL.user = host.substr(0, pos);
        mURL.host = host.substr(pos + 1);
    }

    if (mURL.port == 0)
    {
        if (mURL.scheme.compare("https") == 0)
            mURL.port = 443;
        else
            mURL.port = 80;
    }

    MSG_DEBUG("URL components: Scheme: " << mURL.scheme << ", Host: " << mURL.host << ", Port: " << mURL.port << ", Path: " << mURL.path << ", User: " << mURL.user << ", Password: " << ((mURL.pw.empty()) ? "" : "****"));
    return mURL;
}

int THTTPClient::parseHeader(const char *buffer, size_t len)
{
    DECL_TRACER("THTTPClient::parseHeader(const char *buffer, size_t len)");

    if (buffer == nullptr || len == 0)
    {
        MSG_ERROR("[" << mURL.host << "] Empty receive buffer!");
        return -1;
    }

    int blen = 0, receive = 0, code = 0;
    char *raw = (char *)buffer;
    char *path, *version, *status;
    int direction = HTTP_DIRECTION_SEND;
    METHOD_t method;
    mHeader.clear();
    mBody.clear();
    mRequest.clear();

    // Method
    size_t meth_len = strcspn(raw, " ");

    if (meth_len >= len)
    {
        MSG_ERROR("[" << mURL.host << "] Buffer contains no valid HTTP response!");
        return -1;
    }

    if (memcmp(raw, "GET", 3) == 0)
        method = GET;
    else if (memcmp(raw, "PUT", 3) == 0)
        method = PUT;
    else if (memcmp(raw, "POST", 4) == 0)
        method = POST;
    else if (memcmp(raw, "HEAD", 4) == 0)
        method = HEAD;
    else
    {
        method = UNSUPPORTED;
        direction = HTTP_DIRECTION_RECEIVE;
        receive = 1;
        MSG_DEBUG("[" << mURL.host << "] Detected a receive buffer");
    }

    mRequest.method = method;
    mRequest.direction = direction;
    raw += meth_len + 1; // move past <SP>
    status = path = version = nullptr;

    if (!receive)
    {
        // Request-URI
        size_t path_len = strcspn(raw, " ");

        try
        {
            path = new char[path_len+1];
            memcpy(path, raw, path_len);
            path[path_len] = '\0';
            raw += path_len + 1; // move past <SP>

            // HTTP-Version
            size_t ver_len = strcspn(raw, "\r\n");
            version = new char[ver_len + 1];
            memcpy(version, raw, ver_len);
            version[ver_len] = '\0';
            raw += ver_len + 2; // move past <CR><LF>
            mRequest.path = path;
            mRequest.version = version;
        }
        catch (std::exception& e)
        {
            MSG_ERROR("[" << mURL.host << "] Error allocating memory: " << e.what());

            if (path)
                delete[] path;

            if (version)
                delete[] version;

            mRequest.path = nullptr;
            mRequest.version = nullptr;
            return -1;
        }
    }
    else
    {
        char scode[16];
        memset(scode, 0, sizeof(scode));
        size_t code_len = strcspn(raw, " ");
        strncpy(scode, raw, min(code_len, sizeof(scode)));
        scode[sizeof(scode)-1] = 0;
        code = atoi(scode);

        MSG_DEBUG("[" << mURL.host << "] Received code " << code);

        if (strstr(buffer, "\r\n\r\n") == NULL)
        {
            MSG_ERROR("[" << mURL.host << "] Received no content!");
            return -1;
        }

        if (code_len >= len)
        {
            MSG_ERROR("[" << mURL.host << "] Buffer contains no valid HTTP response!");
            return -1;
        }

        raw += code_len + 1;
        size_t stat_len = strcspn(raw, "\r\n");

        try
        {
            status = new char[stat_len + 1];
            memcpy(status, raw, stat_len);
            status[stat_len] = '\0';
            raw += stat_len + 2;
            mRequest.status = status;
        }
        catch (std::exception& e)
        {
            MSG_ERROR("[" << mURL.host << "] Error allocating memory: " << e.what());
            return -1;
        }
    }

    char *end = strstr(raw, "\r\n\r\n");

    while (raw < end)
    {
        HTTPHEAD_t head;
        char hv0[1024];
        // name
        size_t name_len = strcspn(raw, ":");

        size_t mylen = min(name_len, sizeof(hv0));
        memcpy(hv0, raw, mylen);
        hv0[mylen] = '\0';
        head.name = hv0;
        raw += name_len + 1; // move past :

        while (*raw == ' ')
            raw++;

        // value
        size_t value_len = strcspn(raw, "\r\n");
        mylen = min(value_len, sizeof(hv0));
        memcpy(hv0, raw, mylen);
        hv0[mylen] = '\0';
        head.content = hv0;
        MSG_DEBUG("[" << mURL.host << "] Found header: " << head.name << ": " << head.content);
        raw += value_len + 2; // move past <CR><LF>

        if (head.name.compare("Content-Length") == 0)
            blen = atoi(head.content.c_str());

        // next
        mHeader.push_back(head);
    }

    if (blen == 0)
    {
        size_t head_len = strcspn(buffer, "\r\n\r\n");

        if (head_len < len)
            blen = static_cast<int>(len - head_len + 4);
    }

    MSG_DEBUG("[" << mURL.host << "] Content length: " << blen);

    if (blen > 0)
    {
        raw = end + 4;

        try
        {
            mBody.body = new char[blen +1];
            memcpy(mBody.body, raw, blen);
            mBody.body[blen] = '\0';
            mBody.len = blen;
        }
        catch(std::exception& e)
        {
            MSG_ERROR(e.what());

            if (status)
                delete[] status;

            if (path)
                delete[] path;

            if (version)
                delete[] version;

            mRequest.status = nullptr;
            mRequest.path = nullptr;
            mRequest.version = nullptr;
            return -1;
        }
    }

    return 0;
}

string THTTPClient::getHeadParameter(const string& name)
{
    DECL_TRACER("THTTPClient::getHeadParameter(const string& name)");

    if (mHeader.empty())
        return string();

    vector<HTTPHEAD_t>::iterator iter;

    for (iter = mHeader.begin(); iter != mHeader.end(); ++iter)
    {
        if (iter->name.compare(name) == 0)
            return iter->content;
    }

    return string();
}
/*
char *THTTPClient::getContent(const char* buffer)
{
    DECL_TRACER("THTTPClient::getContent(const char* buffer)");

    if (!buffer)
        return nullptr;

    char *ctnt = strstr((char *)buffer, "\r\n\r\n");
    return ctnt;
}
*/
string THTTPClient::makeRequest(const string& url)
{
    DECL_TRACER("THTTPClient::makeRequest(const string& url)");

    URL_t uparts = parseURL(url);

    if (uparts.host == "0.0.0.0" || uparts.host == "8.8.8.8")
    {
        MSG_WARNING("Refusing to connect to host " << uparts.host << "!");
        return string();
    }

    string request = "GET " + uparts.path + " HTTP/1.1\r\n";
    request += "Host: " + uparts.host;

    if (uparts.port > 0 && uparts.port != 80 && uparts.port != 443)
        request += ":" + std::to_string(uparts.port);

    request += "\r\n";

    if (!mUser.empty())
    {
        string clearname = mUser + ":" + mPassword;
        string enc = Base64::encode((BYTE *)clearname.c_str(), static_cast<uint>(clearname.size()));
        request += "Authorization: Basic " + enc + "\r\n";
    }

    request += "User-Agent: tpanel/" + std::to_string(V_MAJOR) + "." + std::to_string(V_MINOR) + "." + std::to_string(V_PATCH) + "\r\n";
    request += "Accept: image/*\r\n";
    request += "\r\n";
    MSG_DEBUG("Requesting: " << std::endl << request << "------------------------------------------");
    return request;
}

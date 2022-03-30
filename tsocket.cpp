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

#include <chrono>
#include <thread>

#include "tsocket.h"
#include "terror.h"
#include "tconfig.h"
#include "texcept.h"

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

using std::string;

int _cert_callback(int preverify_ok, X509_STORE_CTX *ctx);

TSocket::TSocket()
{
    DECL_TRACER("TSocket::TSocket()")
}

TSocket::TSocket(const string& host, int port)
{
    DECL_TRACER("TSocket::TSocket(const std::string& host, int port)");

    mHost = host;
    mPort = port;
}

TSocket::~TSocket()
{
    DECL_TRACER("TSocket::~TSocket()");

    close();
}


bool TSocket::connect(bool encrypt)
{
    DECL_TRACER("TSocket::connect(bool encrypt)");

    struct addrinfo *ainfo = nullptr;
    int sock = -1;
    int on = 1;
    bool retry = true;

    MSG_DEBUG("Trying to connect to host " << mHost << " at port " << mPort);

    if ((ainfo = lookup_host(mHost, mPort)) == nullptr)
        return false;

    while (ainfo && retry)
    {
        sock = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);

        if (sock == -1)
        {
            MSG_ERROR("[" << mHost << "] Error opening socket: " << strerror(errno));
            ainfo = ainfo->ai_next;
            continue;
        }

        MSG_DEBUG("[" << mHost << "] Socket successfully created.");
        struct in_addr  *addr;

        if (ainfo->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv = (struct sockaddr_in *)ainfo->ai_addr;
            addr = &(ipv->sin_addr);
        }
        else
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ainfo->ai_addr;
            addr = (struct in_addr *) &(ipv6->sin6_addr);
        }

        char buffer[100];
        // FIXME: This is the address where we connected to, but we need the
        //        address of the local network interface where the connection
        //        was initiated from.
        inet_ntop(ainfo->ai_family, addr, buffer, sizeof(buffer));
        mMyIP.assign(buffer);
        MSG_DEBUG("Client IP: " << mMyIP);

        struct timeval tv;

        // FIXME: Make the timeouts configurable!
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(int)) == -1)
        {
            MSG_ERROR("[" << mHost << "] Error setting socket options for address reuse: " << strerror(errno));
            ::close(sock);
            return false;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval)) == -1)
        {
            MSG_ERROR("[" << mHost << "] Error setting socket options for receive: " << strerror(errno));
            ::close(sock);
            return false;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(struct timeval)) == -1)
        {
            MSG_ERROR("[" << mHost << "] Error setting socket options for send: " << strerror(errno));
            ::close(sock);
            return false;
        }

        if (::connect(sock, ainfo->ai_addr, ainfo->ai_addrlen) == -1)
        {
            if (errno != EINPROGRESS)
            {
                MSG_ERROR("[" << mHost << "] Connect error: " << strerror(errno));
                ::close(sock);
                mConnected = false;
                return false;
            }
            else
            {
                MSG_DEBUG("[" << mHost << "] Connection is in progress ...");
                mConnected = true;
                break;
            }
        }
        else
        {
            MSG_DEBUG("[" << mHost << "] Successfully connected.");
            mConnected = true;
            break;
        }
    }

    if (ainfo == nullptr)
    {
        MSG_ERROR("[" << mHost << "] No network interface to connect to target was found!");
        mConnected = false;
        return false;
    }

    if (encrypt)
    {
        int ret;

        MSG_DEBUG("[" << mHost << "] Initializing SSL connection ...");
        mCtx = initCTX();

        if (mCtx == NULL)
        {
            MSG_ERROR("[" << mHost << "] Error initializing CTX.");
            ::close(sock);
            mConnected = false;
            return false;
        }

        mSsl = SSL_new(mCtx);      /* create new SSL connection state */

        if (mSsl == NULL)
        {
            log_ssl_error();
            SSL_CTX_free(mCtx);
            ::close(sock);
            mConnected = false;
            return false;
        }

        SSL_set_fd(mSsl, sock);    /* attach the socket descriptor */

        if (TConfig::certCheck())
        {
            SSL_set_verify(mSsl, SSL_VERIFY_PEER, _cert_callback);
            MSG_TRACE("[" << mHost << "] Verify on peer certificate was set.");
        }

        while ((ret = SSL_connect(mSsl)) < 0)
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock, &fds);

            switch (SSL_get_error(mSsl, ret))
            {
                case SSL_ERROR_WANT_READ:
                    select(sock + 1, &fds, NULL, NULL, NULL);
                break;

                case SSL_ERROR_WANT_WRITE:
                    select(sock + 1, NULL, &fds, NULL, NULL);
                break;

                default:
                    MSG_ERROR("[" << mHost << "] Error getting a new SSL handle.");
                    SSL_CTX_free(mCtx);
                    ::close(sock);
                    SSL_free(mSsl);
                    mConnected = false;
                    return false;
            }
        }

        if (TConfig::certCheck())
        {
            if (SSL_get_peer_certificate(mSsl))
            {
                MSG_DEBUG("[" << mHost << "] Result of peer certificate verification is checked ...");

                if (SSL_get_verify_result(mSsl) != X509_V_OK)
                {
                    MSG_ERROR("[" << mHost << "] Error verifiying peer.");
                    SSL_CTX_free(mCtx);
                    ::close(sock);
                    SSL_free(mSsl);
                    mConnected = false;
                    return false;
                }

                MSG_TRACE("[" << mHost << "] Certificate was valid.");
            }
            else
                MSG_WARNING("[" << mHost << "] Peer offered no or invalid certificate!");
        }

        mEncrypted = true;
    }

    mSockfd = sock;
    return true;
}

bool TSocket::connect(const string& host, int port, bool encrypt)
{
    DECL_TRACER("TSocket::connect(const string&, int port, bool encrypt)");

    if (host.empty() || port < 1)
    {
        MSG_ERROR("CONNECT: Invalid credentials! (HOST: " << (host.empty() ? "<none>" : host) << ", PORT: " << port << ")");
        return false;
    }

    if (mConnected && host == mHost && port == mPort)
    {
        MSG_DEBUG("[" << mHost << "] Already connected.");
        return true;
    }

    if (mConnected)
        close();

    mHost = host;
    mPort = port;
    return connect(encrypt);
}

bool TSocket::isSockValid()
{
    DECL_TRACER("TSocket::isSockValid()");

    if (!mConnected || mSockfd <= 0)
        return false;

    int optval;
    socklen_t optlen = sizeof(optval);

    int res = getsockopt(mSockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen);

    if (optval == 0 && res == 0)
        return true;

    if (res != 0)
    {
        MSG_ERROR("[" << mHost << "] Network error: " << strerror(errno));
    }

    return false;
}

size_t TSocket::receive(char* buffer, size_t size, bool doPoll)
{
    DECL_TRACER("TSocket::receive(char* buffer, size_t size)");

    int proto = 0;
    bool retry = false;
    std::chrono::system_clock::time_point end = std::chrono::system_clock::now() + std::chrono::seconds(10);

    if (!mConnected || buffer == nullptr || (mEncrypted && mSsl == nullptr))
        return npos;

    if (!mEncrypted)
        proto = 0;
    else
        proto = 1;

    struct pollfd pfd;
    int nfds = 1;       // Only one entry in structure

    pfd.fd = mSockfd;
    pfd.events = POLLIN;

    int s = 0;

    do
    {
        if (doPoll)
            s = poll(&pfd, nfds, 10000);    // FIXME: Make the timeout configurable.
        else
            s = 1;

        if (s < 0)
        {
            close();
            XCEPTNETWORK("[" + mHost + "] Poll error on read: " + strerror(errno));
        }

        if (s == 0)
        {
            if (std::chrono::system_clock::now() < end)
            {
                retry = true;
                MSG_DEBUG("looping ...");
            }
            else
            {
                errno = ETIMEDOUT;
                retry = false;
            }
        }
        else
        {
            switch (proto)
            {
                case 0: return read(mSockfd, buffer, size);
                case 1: return SSL_read(mSsl, buffer, size);
            }
        }
    }
    while (retry);

    return npos;
}

size_t TSocket::readAbsolut(char *buffer, size_t size)
{
    DECL_TRACER("TSocket::readAbsolut(char *buffer, size_t size)");

    if (!mConnected || buffer == nullptr || !size)
        return npos;

    size_t rest = size;
    char *buf = new char[size + 1];
    char *p = buffer;
    std::chrono::system_clock::time_point end = std::chrono::system_clock::now() + std::chrono::seconds(10);

    while (rest && mConnected)
    {
        size_t rec = receive(buf, rest);

        if (rec != npos && rec > 0)
        {
            rest -= rec;
            memmove(p, buf, rec);
            p += rec;
            end = std::chrono::system_clock::now() + std::chrono::seconds(10);
        }
        else if (std::chrono::system_clock::now() >= end)
        {
            string message = "[" + mHost + "] Read: ";

            if (!mEncrypted)
            {
                if (errno)
                    message.append(strerror(errno));
                else
                    message.append("Timeout on reading");
            }
            else
            {
                log_ssl_error();
                message.append("Read error!");
            }

            close();
            delete[] buf;
            buf = nullptr;
#ifdef __ANDROID__
            MSG_ERROR(message);
            return -1;
#else
            XCEPTNETWORK(message);
#endif
        }

        if (rest)
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }

    if (buf)
        delete[] buf;

    return (size - rest);
}

size_t TSocket::send(char* buffer, size_t size)
{
    DECL_TRACER("TSocket::send(char* buffer, size_t size)");

    int proto = 0;
    bool retry = false;
    std::chrono::system_clock::time_point end = std::chrono::system_clock::now() + std::chrono::seconds(10);

    if (!mConnected || buffer == nullptr || (mEncrypted && mSsl == nullptr))
        return npos;

    if (!mEncrypted)
        proto = 0;
    else
        proto = 1;

    struct pollfd pfd;
    int nfds = 1;       // Only one entry in structure

    pfd.fd = mSockfd;
    pfd.events = POLLOUT;

    int s = 0;

    do
    {
        s = poll(&pfd, nfds, 10000);    // FIXME: Make the timeout configurable.

        if (s < 0)
        {
            close();
            XCEPTNETWORK("[" + mHost + "] Poll error on write: " + strerror(errno));
        }

        if (s == 0)
        {
            if (std::chrono::system_clock::now() < end)
                retry = true;
            else
            {
                retry = false;
                errno = ETIMEDOUT;
            }
        }
        else
        {
            switch (proto)
            {
                case 0: return write(mSockfd, buffer, size);
                case 1: return SSL_write(mSsl, buffer, size);
            }
        }
    }
    while (retry);

    return npos;
}

bool TSocket::close()
{
    DECL_TRACER("TSocket::close()");

    bool status = true;

    if (!mConnected)
        return true;

    if (mEncrypted && mSsl)
    {
        SSL_free(mSsl);
        mSsl = nullptr;
    }

    if (!isSockValid())
    {
        mConnected = false;

        if (mEncrypted && mCtx)
        {
            SSL_CTX_free(mCtx);
            mCtx = nullptr;
        }

        mSockfd = -1;
        mEncrypted = false;
        return false;
    }

    if (shutdown(mSockfd, SHUT_RDWR) != 0)
    {
        MSG_ERROR("[" << mHost << "] Error shutting down connection: " << strerror(errno));
        status = false;
    }

    mConnected = false;

    if (::close(mSockfd) != 0)
    {
        MSG_ERROR("[" << mHost << "] Error closing a socket: " << strerror(errno));
        status = false;
    }

    mSockfd = -1;

    if (mEncrypted && mCtx)
    {
        SSL_CTX_free(mCtx);
        mCtx = nullptr;
    }

    mEncrypted = false;
    return status;
}

struct addrinfo *TSocket::lookup_host (const string& host, int port)
{
    DECL_TRACER("TSocket::lookup_host (const string& host, int port)");

    struct addrinfo *res;
    struct addrinfo hints;
    char sport[16];

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    snprintf(sport, sizeof(sport), "%d", port);
    int ret = 0;

    if ((ret = getaddrinfo (host.c_str(), sport, &hints, &res)) != 0)
    {
        MSG_ERROR("[" << mHost << "] Getaddrinfo: " << gai_strerror(ret));
        return nullptr;
    }

    return res;
}

void TSocket::initSSL()
{
    DECL_TRACER("TSocket::initSSL()");

    if (mSSLInitialized)
        return;

    SSL_library_init();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    mSSLInitialized = true;
}

SSL_CTX *TSocket::initCTX()
{
    DECL_TRACER("TSocket::initCTX()");

    SSL_CTX *ctx;
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    const SSL_METHOD *method = TLS_client_method();
#else
    const SSL_METHOD *method = TLSv1_2_client_method();
#endif
    ctx = SSL_CTX_new(method);   /* Create new context */

    if ( ctx == NULL )
    {
        log_ssl_error();
        return NULL;
    }

    char *cert_check = getenv("CERT_CHECK");

    if (cert_check && strcmp(cert_check, "ON") == 0)
    {
        char *cert_path = getenv("CERT_PATH");
        char *cert_chain = getenv("CERT_CHAIN");
        char *cert_file = getenv("CERT_FILE");
        char *cert_type = getenv("CERT_TYPE");

        if (cert_path == NULL)
        {
            MSG_WARNING("Missing environment variable \"CERT_PATH\" defining the path to the cerificates.");
            return ctx;
        }

        if (cert_chain == NULL)
        {
            MSG_WARNING("Certificate check is enabled but no certificate chain file is set! No certificate verification was made.");
            return ctx;
        }

        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, _cert_callback);

        if (cert_type == NULL)
            cert_type = (char *)"PEM";

        if (SSL_CTX_load_verify_locations(ctx, cert_chain, cert_path) != 1)
        {
            MSG_ERROR("Error with certificate " << cert_path << "/" << cert_chain);
            log_ssl_error();
            SSL_CTX_free(ctx);
            return NULL;
        }

        int type = SSL_FILETYPE_PEM;

        if (strcmp(cert_type, "ASN1") == 0)
            type = SSL_FILETYPE_ASN1;

        if (cert_file && SSL_CTX_use_certificate_file(ctx, cert_file, type) != 1)
        {
            MSG_ERROR("Error with certificate " << cert_file);
            log_ssl_error();
            SSL_CTX_free(ctx);
            return NULL;
        }
    }

    return ctx;
}

void TSocket::log_ssl_error()
{
    DECL_TRACER("TSocket::log_ssl_error()");
    unsigned long int err;
    char errstr[512];

    while ((err = ERR_get_error()) != 0)
    {
        ERR_error_string_n(err, &errstr[0], sizeof(errstr));
        MSG_ERROR(errstr);
    }
}

/*
 * Callback fuction for SSL connections.
 */
int _cert_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    DECL_TRACER("_cert_callback(int preverify_ok, X509_STORE_CTX *ctx)");

    char    buf[256];
    X509   *err_cert;
    int     err, depth;

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    /*
     * Retrieve the pointer to the SSL of the connection currently treated
     * and the application specific data stored into the SSL object.
     */
    X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

    if (!preverify_ok)
    {
        MSG_WARNING("verify error:num=" << err << ":" << X509_verify_cert_error_string(err) << ":depth=" << depth << ":" << buf);
    }

    /*
     * At this point, err contains the last verification error. We can use
     * it for something special
     */
    if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT))
    {
        X509_NAME_oneline(X509_get_issuer_name(err_cert), buf, 256);
        MSG_WARNING("issuer= " << buf);
    }

    return preverify_ok;
}

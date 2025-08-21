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
#include <cstring>
#include <sstream>
#include <iomanip>

#include "tsocket.h"
#include "terror.h"
#include "tconfig.h"
#include "texcept.h"

#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>

using std::string;
using std::stringstream;

int _cert_callback(int preverify_ok, X509_STORE_CTX *ctx);

TSocket::TSocket()
{
    DECL_TRACER("TSocket::TSocket()")

    getHost();
}

TSocket::TSocket(const string& host, int port)
{
    DECL_TRACER("TSocket::TSocket(const std::string& host, int port)");

    mHost = host;
    mPort = port;
    getHost();
}

TSocket::~TSocket()
{
    DECL_TRACER("TSocket::~TSocket()");

    close();
}

bool TSocket::getHost()
{
    DECL_TRACER("TSocket::getHost()");

    char host[4096];

    if (gethostname(host, sizeof(host)))
    {
        MSG_ERROR("Error getting host name!");
        return false;
    }

    mMyHostName.assign(host);
    return true;
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


    if (ainfo && ainfo->ai_family == AF_INET)
    {
        char str[INET_ADDRSTRLEN];
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        getsockname(sock, (struct sockaddr *)&addr, &len);
        mMyIP.assign(inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN));
        determineNetmask(sock);
    }
    else
    {
        char str[INET6_ADDRSTRLEN];
        struct sockaddr_in6 addr;
        socklen_t len = sizeof(addr);
        getsockname(sock, (struct sockaddr *)&addr, &len);
        mMyIP.assign(inet_ntop(AF_INET6, &(addr.sin6_addr), str, INET6_ADDRSTRLEN));
        determineNetmask(sock);
    }

    MSG_DEBUG("Client IP: " << mMyIP);

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
                case 1: return SSL_read(mSsl, buffer, static_cast<int>(size));
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
        ssize_t rec = receive(buf, rest);

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

ssize_t TSocket::send(char* buffer, size_t size)
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
                case 1: return SSL_write(mSsl, buffer, static_cast<int>(size));
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
#ifdef __POSIX__
    hints.ai_flags = AI_CANONNAME | AI_CANONIDN;
#else
    hints.ai_flags = AI_CANONNAME;
#endif
    snprintf(sport, sizeof(sport), "%d", port);
    int ret = 0;

    if ((ret = getaddrinfo (host.c_str(), sport, &hints, &res)) != 0)
    {
        MSG_ERROR("[" << mHost << "] Getaddrinfo: " << gai_strerror(ret));
        return nullptr;
    }

    if (res->ai_canonname)
    {
        MSG_DEBUG("Canonical name: " << res->ai_canonname);
    }

    return res;
}

bool TSocket::determineNetmask(int socket)
{
    DECL_TRACER("TSocket::determineNetmask(int socket)");

    struct ifaddrs *ifaddr;
    char str[INET6_ADDRSTRLEN];
    int s, family;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        MSG_ERROR("Error getting devices: " << strerror(errno));
        return false;
    }

    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

        family = ifa->ifa_addr->sa_family;
        size_t addrSize = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(sockaddr_in6);

        if (family != AF_INET && family != AF_INET6)
            continue;

        s = getnameinfo(ifa->ifa_addr, static_cast<socklen_t>(addrSize), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);

        if (s != 0)
        {
            MSG_ERROR("Nameinfo failed: " << gai_strerror(s));
            freeifaddrs(ifaddr);
            return false;
        }

        MSG_DEBUG("Comparing \"" << host << "\" with \"" << mMyIP << "\"");

        if (mMyIP.compare(host) == 0)
        {
            struct ifreq ninfo;

            MSG_DEBUG("Device: " << ifa->ifa_name);
            mIfaceName = ifa->ifa_name;
            getMac(socket, mIfaceName, &mMacAddress);
            MSG_DEBUG("Mac address: " << mMacAddress);

            memset(&ninfo, 0, sizeof(ninfo));
#ifdef __POSIX__
            strncpy(ninfo.ifr_ifrn.ifrn_name, ifa->ifa_name, IFNAMSIZ);
            ninfo.ifr_ifrn.ifrn_name[IFNAMSIZ-1] = 0;
#else
            strncpy(ninfo.ifr_name, ifa->ifa_name, IFNAMSIZ);
            ninfo.ifr_name[IFNAMSIZ-1] = 0;
#endif
            if (ioctl(socket, SIOCGIFNETMASK, &ninfo))
            {
                MSG_ERROR("Error getting netmask: " << strerror(errno));
                freeifaddrs(ifaddr);
                return false;
            }
            else
            {
                if (family == AF_INET)
                {
                    stringstream str;
                    int mask = ((struct sockaddr_in *)&ninfo.ifr_addr)->sin_addr.s_addr;

                    str << (mask & 0xff) << "." << ((mask >> 8) & 0xff) << "." << ((mask >> 16) & 0xff) << "." << ((mask >> 24) & 0xff);
                    mMyNetmask = str.str();
                }
                else
                {
                    const char *had = nullptr;

                    had = inet_ntop(AF_INET6, ((struct sockaddr_in6 *)&ninfo.ifr_addr)->sin6_addr.s6_addr, str, INET6_ADDRSTRLEN);
                    mMyNetmask.assign(had);
                }

                MSG_DEBUG("Netmask: " << mMyNetmask);
                freeifaddrs(ifaddr);
                return true;
            }
        }
    }

    freeifaddrs(ifaddr);
    return false;
}

bool TSocket::getMac(int socket, const string& iface, string *mac)
{
    DECL_TRACER("TSocket::getMac(int socket, const string& iface, string *mac)");

    if (!mac || iface.empty() || socket < 0)
    {
        mac->clear();
        return false;
    }

    struct ifreq ifr{};
    size_t len = std::min(static_cast<size_t>(IFNAMSIZ-1), iface.length());
    strncpy(ifr.ifr_name, iface.c_str(), len);
    ifr.ifr_name[len] = 0;
    ioctl(socket, SIOCGIFHWADDR, &ifr);

    stringstream ss;

    for (size_t i = 0; i < 6; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ifr.ifr_hwaddr.sa_data[i]);

        if ((i + 1) != 6)
            ss << ':';
    }

    *mac = ss.str();
    return true;
}

void TSocket::initSSL()
{
    DECL_TRACER("TSocket::initSSL()");

    if (mSSLInitialized)
        return;

#if OPENSSL_API_COMPAT < 0x010100000
    SSL_library_init();
#endif
#if OPENSSL_SHLIB_VERSION < 3
    ERR_load_BIO_strings();
#endif
#if OPENSSL_API_COMPAT < 0x010100000
    ERR_load_crypto_strings();
    SSL_load_error_strings();
#endif
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

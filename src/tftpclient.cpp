/*
 * Copyright (C) 2026 by Andreas Theofilu <andreas@theosys.at>
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
#include <QNetworkProxy>
#include <QFile>
#include <QElapsedTimer>
#include <QNetworkInterface>

#include "tftpclient.h"
#include "terror.h"

#define WAITTIME        5000
#define WAITREADY       100

#define USEROKNOPASS    331

using std::string;

QRegularExpression TFtpClient::mRegExpr{R"(^([\-ld])[rwxstST\-]{9}\s+\d+\s+\S+\s+\S+\s+\d+\s+\w{3}\s+\d{1,2}\s+[\d:]{4,5}\s+(.*)$)"};

TFtpClient::TFtpClient(QObject *parent)
        : QObject(parent),
          mPort(21),
          mMode(Mode::Passive)
{
    DECL_TRACER("TFtpClient::TFtpClient(QObject* parent=nullptr)");

    mCtrl.setProxy(QNetworkProxy::NoProxy);
}

TFtpClient::~TFtpClient()
{
    DECL_TRACER("TFtpClient::~TFtpClient()");

    if (mCtrl.isOpen())
        mCtrl.close();
}

bool TFtpClient::connectAndLogin(const QString& host, quint16 port, const QString& user, const QString& pass, int timeoutMs)
{
    DECL_TRACER("TFtpClient::connectAndLogin(const QString& host, quint16 port, const QString& user, const QString& pass, int timeoutMs=15000)");

    mLastError.clear();
    mHost = host;
    mPort = port;
    mCtrl.abort();
    mCtrl.connectToHost(host, port);

    if (!mCtrl.waitForConnected(timeoutMs))
    {
        mLastError = "Control connection failed: " + mCtrl.errorString();
        MSG_ERROR(mLastError.toStdString());
        return false;
    }

    int code = 0;
    QStringList lines;

    if (!readReply(code, lines, timeoutMs) || (code / 100) != 2)
    {
        mLastError = "No 220 greeting from server.";
        MSG_ERROR(mLastError.toStdString());
        return false;
    }

    if (!sendCommandExpect("USER " + user, 2, code, lines, timeoutMs))
    {
        if (code == USEROKNOPASS)
        {
            MSG_WARNING("A password is needed!");
        }
        else if ((code / 100) != 2)
        {
            mLastError = "USER failed.";
            MSG_ERROR(mLastError.toStdString());
            return false;
        }
    }

    if (code == USEROKNOPASS)
    {
        if (!sendCommandExpect("PASS " + pass, 2, code, lines, timeoutMs))
        {
            mLastError = "PASS failed.";
            MSG_ERROR(mLastError.toStdString());
            return false;
        }
    }
    // Set binary
    sendCommandExpect("TYPE I", 2, code, lines, timeoutMs);
    return true;
}

QList<TFtpClient::FTPINDEX_t> TFtpClient::listTop(const QString &filter, int timeoutMs)
{
    DECL_TRACER("TFtpClient::listTop(const QString &filter, int timeoutMs)");

    mLastError.clear();
    QList<FTPINDEX_t> index;
    // Try MLSD first
    QByteArray data;

    if (!transferCommandWithData("MLSD", data, timeoutMs))
    {
        // Fallback to LIST
        data.clear();

        if (!transferCommandWithData("LIST", data, timeoutMs))
            return index;
    }

    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split("\n", Qt::SkipEmptyParts);
    bool mlsd = text.contains("type="); // rough heuristic

    if (mlsd)
    {
        for (QString line : lines)
        {
            line = line.trimmed();

            if (line.isEmpty())
                continue;
            // format: "type=file;size=...;modify=...; <space>name"
            const QStringList parts = line.split(";", Qt::SkipEmptyParts);
            FTPINDEX_t idx;

            for (const QString s : parts)
            {
                qsizetype pos = 0;

                if (s.startsWith(" "))
                    idx.name = s.trimmed();
                else if ((pos = s.indexOf("=")) >= 0)
                {
                    QString left = s.left(pos);
                    QString right = s.mid(pos+1);

                    if (left.contains("type", Qt::CaseInsensitive))
                        idx.type = right;
                    else if (left.contains("size", Qt::CaseInsensitive))
                        idx.size = right.toLongLong();
                }
            }

            if (!filter.isEmpty())
            {
                if (idx.name.endsWith(filter, Qt::CaseInsensitive))
                    index.append(idx);
            }
            else
                index.append(idx);
        }
    }
    else
    {
        for (QString line : lines)
        {
            line = line.trimmed();
            FTPINDEX_t idx;

            if (line.isEmpty())
                continue;

            QRegularExpressionMatch m = mRegExpr.match(line);

            if (m.hasMatch())
            {
                idx = parseLine(line);
                const QString typeChar = m.captured(1);
                const QString name = m.captured(2);

                if (!filter.isEmpty() && !name.endsWith(filter, Qt::CaseInsensitive))
                    continue;

                if (typeChar == "-" && !name.isEmpty())
                {
                    idx.type = "file";
                    idx.name = name;
                    index.append(idx);
                }
            }
        }
    }

    return index;
}

bool TFtpClient::downloadFile(const QString& remoteName, const QString& localPath, int timeoutMs, std::function<void(qint64,qint64)> progressCb)
{
    DECL_TRACER("TFtpClient::downloadFile(const QString& remoteName, const QString& localPath, int timeoutMs, std::function<void(qint64,qint64)> progressCb)");

    mLastError.clear();
    // Ensure binary mode
    int code;
    QStringList lines;
    sendCommandExpect("TYPE I", 2, code, lines, timeoutMs);

    QScopedPointer<QFile> out(new QFile(localPath));

    if (!out->open(QIODevice::WriteOnly))
    {
        mLastError = "Cannot open local file for writing.";
        MSG_ERROR(mLastError.toStdString());
        return false;
    }

    std::function<bool(QTcpSocket*)> consumer = [&](QTcpSocket *dataSock) -> bool
    {
        qint64 total = 0;

        while (true)
        {
            if (!dataSock->waitForReadyRead(WAITTIME))
            {
                if (dataSock->state() == QAbstractSocket::ConnectedState)
                    continue;
            }

            const QByteArray chunk = dataSock->read(64 * 1024);

            if (chunk.isEmpty())
            {
                if (dataSock->state() != QAbstractSocket::ConnectedState && dataSock->bytesAvailable() == 0)
                    break;
            }
            else
            {
                if (out->write(chunk) != chunk.size())
                {
                    mLastError = "Write error.";
                    MSG_ERROR(mLastError.toStdString());
                    return false;
                }

                total += chunk.size();

                if (progressCb)
                    progressCb(total, -1);
            }
        }

        out->flush();
        return true;
    };

    const QString cmd = "RETR " + remoteName;

    if (!transferDataFlow(cmd, consumer, timeoutMs))
        return false;

    return true;
}

bool TFtpClient::sendLine(const QString& line)
{
    DECL_TRACER("TFtpClient::sendLine(const QString& line)");

    QByteArray b = line.toUtf8();

    if (!b.endsWith("\r\n"))
        b += "\r\n";

    if (mCtrl.write(b) != b.size())
        return false;

    return mCtrl.waitForBytesWritten(WAITTIME);
}

bool TFtpClient::readReply(int& codeOut, QStringList& linesOut, int timeoutMs)
{
    DECL_TRACER("TFtpClient::readReply(int& codeOut, QStringList& linesOut, int timeoutMs)");

    linesOut.clear();
    QString firstLine;
    QString codeStr;
    bool multiline = false;
    int code = 0;

    QElapsedTimer timer; timer.start();

    while (true)
    {
        if (!mCtrl.bytesAvailable())
        {
            if (!mCtrl.waitForReadyRead(WAITREADY))
            {
                if (timer.elapsed() > timeoutMs)
                    break;

                continue;
            }
        }

        while (mCtrl.canReadLine())
        {
            QByteArray raw = mCtrl.readLine();
            QString line = QString::fromUtf8(raw).trimmed();

            if (line.isEmpty())
                continue;

            linesOut << line;

            if (linesOut.size() == 1)
            {
                if (line.size() >= 3 && line[0].isDigit() && line[1].isDigit() && line[2].isDigit())
                {
                    codeStr = line.left(3);
                    code = codeStr.toInt();
                    multiline = (line.size() >= 4 && line[3] == '-');

                    if (!multiline)
                    {
                        codeOut = code;
                        return true;
                    }
                }
            }
            else if (multiline)
            {
                if (line.startsWith(codeStr + " "))
                {
                    codeOut = code;
                    return true;
                }
            }
        }

        if (timer.elapsed() > timeoutMs) break;
    }

    return false;
}

bool TFtpClient::sendCommandExpect(const QString& cmd, int expectClass, int& codeOut, QStringList& linesOut, int timeoutMs)
{
    DECL_TRACER("TFtpClient::sendCommandExpect(const QString& cmd, int expectClass, int& codeOut, QStringList& linesOut, int timeoutMs)");

    if (!sendLine(cmd))
        return false;

    if (!readReply(codeOut, linesOut, timeoutMs))
        return false;

    return ((codeOut / 100) == expectClass);
}

bool TFtpClient::parsePASV(const QStringList& lines, QHostAddress& addr, quint16& port)
{
    DECL_TRACER("TFtpClient::parsePASV(const QStringList& lines, QHostAddress& addr, quint16& port)");
    // Look for 227 ... (h1,h2,h3,h4,p1,p2)
    for (const QString& l : lines)
    {
        int lp = l.indexOf('(');
        int rp = l.indexOf(')', lp + 1);

        if (lp >= 0 && rp > lp)
        {
            const QStringList nums = l.mid(lp + 1, rp - lp - 1).split(',', Qt::SkipEmptyParts);

            if (nums.size() == 6)
            {
                QString ip = QString("%1.%2.%3.%4").arg(nums[0], nums[1], nums[2], nums[3]);
                bool ok1 = false, ok2 = false;
                int p1 = nums[4].toInt(&ok1);
                int p2 = nums[5].toInt(&ok2);

                if (ok1 && ok2)
                {
                    addr = QHostAddress(ip);
                    port = quint16(p1 * 256 + p2);
                    return true;
                }
            }
        }
    }

    return false;
}

QHostAddress TFtpClient::pickIPv4(const QHostAddress& preferred)
{
    DECL_TRACER("TFtpClient::pickIPv4(const QHostAddress& preferred)");

    if (preferred.protocol() == QAbstractSocket::IPv4Protocol &&
            !preferred.isNull() && !preferred.isLoopback())
        return preferred;

    const QList<QNetworkInterface> ifs = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &ni : ifs)
    {
        if ((!(ni.flags() & QNetworkInterface::IsUp)) ||
            (ni.flags() & QNetworkInterface::IsLoopBack))
            continue;

        for (const QNetworkAddressEntry &e : ni.addressEntries())
        {
            if (e.ip().protocol() == QAbstractSocket::IPv4Protocol)
                return e.ip();
        }
    }

    return QHostAddress::LocalHost;
}

bool TFtpClient::openPassiveData(QTcpSocket*& dataSock, int timeoutMs)
{
    DECL_TRACER("TFtpClient::openPassiveData(QTcpSocket*& dataSock, int timeoutMs)");

    int code; QStringList lines;

    if (!sendCommandExpect("PASV", 2, code, lines, timeoutMs) || code != 227)
    {
        mLastError = "PASV failed.";
        MSG_ERROR(mLastError.toStdString());
        return false;
    }
    QHostAddress addr; quint16 port;

    if (!parsePASV(lines, addr, port))
    {
        mLastError = "Failed to parse PASV address.";
        MSG_ERROR(mLastError.toStdString());
        return false;
    }

    dataSock = new QTcpSocket();
    dataSock->connectToHost(addr, port);

    if (!dataSock->waitForConnected(timeoutMs))
    {
        mLastError = "Data connection (PASV) failed: " + dataSock->errorString();
        MSG_ERROR(mLastError.toStdString());
        dataSock->deleteLater();
        dataSock = nullptr;
        return false;
    }

    return true;
}

bool TFtpClient::openActivePrepare(quint16& listeningPort, QHostAddress& sentAddr, int timeoutMs)
{
    DECL_TRACER("TFtpClient::openActivePrepare(quint16& listeningPort, QHostAddress& sentAddr, int timeoutMs)");

    mActiveServer.close();

    if (!mActiveServer.listen(QHostAddress::AnyIPv4, 0))
    {
        mLastError = "Active listen failed.";
        MSG_ERROR(mLastError.toStdString());
        return false;
    }

    listeningPort = mActiveServer.serverPort();
    QHostAddress local = pickIPv4(mCtrl.localAddress());
    sentAddr = local;
    quint16 p = listeningPort;

    const QString arg = QString("%1,%2,%3,%4,%5,%6")
            .arg(QString::number((uint8_t)local.toIPv4Address() >> 24))
            .arg(QString::number((uint8_t)(local.toIPv4Address() >> 16)))
            .arg(QString::number((uint8_t)(local.toIPv4Address() >> 8)))
            .arg(QString::number((uint8_t)(local.toIPv4Address())))
            .arg(QString::number((p >> 8) & 0xff))
            .arg(QString::number(p & 0xff));

    int code; QStringList lines;

    if (!sendCommandExpect("PORT " + arg, 2, code, lines, timeoutMs) || (code / 100) != 2)
    {
        mLastError = "PORT failed.";
        MSG_ERROR(mLastError.toStdString());
        mActiveServer.close();
        return false;
    }

    return true;
}

bool TFtpClient::transferDataFlow(const QString& ftpCmd, const std::function<bool(QTcpSocket*)>& consumer, int timeoutMs)
{
    DECL_TRACER("TFtpClient::transferDataFlow(const QString& ftpCmd, const std::function<bool(QTcpSocket*)>& consumer, int timeoutMs)");

    int code; QStringList lines;
    std::unique_ptr<QTcpSocket> dataSock;

    if (mMode == Mode::Passive)
    {
        QTcpSocket* raw = nullptr;

        if (!openPassiveData(raw, timeoutMs))
            return false;

        dataSock.reset(raw);

        if (!sendLine(ftpCmd))
        {
            mLastError = "Failed to send command.";
            MSG_ERROR(mLastError.toStdString());
            return false;
        }

        if (!readReply(code, lines, timeoutMs) || (code / 100) != 1)
        {
            mLastError = "Server did not start data transfer.";
            MSG_ERROR(mLastError.toStdString());
            return false;
        }

        if (!consumer(dataSock.get()))
            return false;
        // Expect 226
        if (!readReply(code, lines, timeoutMs) || (code / 100) != 2)
        {
            mLastError = "Transfer did not complete properly.";
            MSG_ERROR(mLastError.toStdString());
            return false;
        }

        return true;
    }
    else
    {
        // Active
        quint16 lp;
        QHostAddress la;

        if (!openActivePrepare(lp, la, timeoutMs))
            return false;

        if (!sendLine(ftpCmd))
        {
            mLastError = "Failed to send command.";
            MSG_ERROR(mLastError.toStdString());
            mActiveServer.close();
            return false;
        }

        if (!readReply(code, lines, timeoutMs) || (code / 100) != 1)
        {
            mLastError = "Server did not start data transfer.";
            MSG_ERROR(mLastError.toStdString());
            mActiveServer.close();
            return false;
        }

        if (!mActiveServer.waitForNewConnection(timeoutMs))
        {
            mLastError = "Server did not connect back for data.";
            MSG_ERROR(mLastError.toStdString());
            mActiveServer.close();
            return false;
        }

        QTcpSocket* raw = mActiveServer.nextPendingConnection();
        dataSock.reset(raw);
        mActiveServer.close();

        if (!consumer(dataSock.get()))
            return false;

        if (!readReply(code, lines, timeoutMs) || (code / 100) != 2)
        {
            mLastError = "Transfer did not complete properly.";
            MSG_ERROR(mLastError.toStdString());
            return false;
        }

        return true;
    }
}

bool TFtpClient::transferCommandWithData(const QString& ftpCmd, QByteArray& out, int timeoutMs)
{
    DECL_TRACER("TFtpClient::transferCommandWithData(const QString& ftpCmd, QByteArray& out, int timeoutMs)");

    out.clear();

    auto consumer = [&](QTcpSocket* dataSock)->bool
    {
        while (true)
        {
            if (!dataSock->waitForReadyRead(WAITTIME))
            {
                if (dataSock->state() == QAbstractSocket::ConnectedState)
                    continue;
            }

            const QByteArray chunk = dataSock->read(64*1024);

            if (chunk.isEmpty())
            {
                if (dataSock->state() != QAbstractSocket::ConnectedState && dataSock->bytesAvailable() == 0)
                    break;
            }
            else
            {
                out += chunk;
            }
        }

        return true;
    };

    return transferDataFlow(ftpCmd, consumer, timeoutMs);
}

TFtpClient::FTPINDEX_t TFtpClient::parseLine(const QString& line)
{
    DECL_TRACER("TFtpClient::parseLine(const QString& line)");

    string buf = line.toStdString();
    string fname;
    size_t size = 0;
    bool oldNetLinx = false;
    // We must detect whether we have a new NetLinx or an old one.
    if (buf.at(42) != ' ')
        oldNetLinx = true;

    // Filter out the filename and it's size
    if (oldNetLinx)
    {
        size = atoll(buf.substr(27, 12).c_str());
        fname = buf.substr(53);
    }
    else
    {
        size = atoll(buf.substr(30, 12).c_str());
        fname = buf.substr(56);
    }

    FTPINDEX_t fl;
    fl.size = size;
    fl.name = QString::fromStdString(fname);

    if (line.at(0) == '-')
        fl.type = "file";
    else if (line.at(0) == 'd')
        fl.type = "dir";
    else if (line.at(0) == 'l')
        fl.type = "link";
    else
        fl.type = "file";

    return fl;
}

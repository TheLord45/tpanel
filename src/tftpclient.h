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
#ifndef TFTPCLIENT_H
#define TFTPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QRegularExpression>

class TFtpClient : public QObject
{
    Q_OBJECT

    public:
        enum class Mode { Passive, Active };

        typedef struct FTPINDEX_t
        {
            QString type;
            qint64 size{0};
            QString name;
        }FTPINDEX_t;

        explicit TFtpClient(QObject* parent=nullptr);
        ~TFtpClient();

        bool connectAndLogin(const QString& host, quint16 port, const QString& user, const QString& pass, int timeoutMs=15000);
        QList<FTPINDEX_t> listTop(const QString &filter="", int timeoutMs=20000);
        bool downloadFile(const QString& remoteName, const QString& localPath, int timeoutMs=60000, std::function<void(qint64,qint64)> progressCb = {});

        void setMode(Mode m) { mMode = m; }
        QString lastError() const { return mLastError; }

    private:
        bool sendLine(const QString& line);
        bool readReply(int& codeOut, QStringList& linesOut, int timeoutMs);
        bool sendCommandExpect(const QString& cmd, int expectClass, int& codeOut, QStringList& linesOut, int timeoutMs);
        static bool parsePASV(const QStringList& lines, QHostAddress& addr, quint16& port);
        static QHostAddress pickIPv4(const QHostAddress& preferred = QHostAddress());
        bool openPassiveData(QTcpSocket*& dataSock, int timeoutMs);
        bool openActivePrepare(quint16& listeningPort, QHostAddress& sentAddr, int timeoutMs);
        bool transferDataFlow(const QString& ftpCmd, const std::function<bool(QTcpSocket*)>& consumer, int timeoutMs);
        bool transferCommandWithData(const QString& ftpCmd, QByteArray& out, int timeoutMs);
        FTPINDEX_t parseLine(const QString& line);

        QTcpSocket mCtrl;
        QTcpServer mActiveServer;
        QString mHost;
        quint16 mPort;
        Mode mMode;
        QString mLastError;
        static QRegularExpression mRegExpr;
};

#endif // TFTPCLIENT_H

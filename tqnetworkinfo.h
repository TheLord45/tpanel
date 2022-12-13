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
#ifndef TQNETWORKINFO_H
#define TQNETWORKINFO_H

#include <QtGlobal>

#define _SUPPORTED_MAJOR    6
#define _SUPPORTED_MINOR    3
#define _SUPPORTED_PATCH    0

#if QT_VERSION >= QT_VERSION_CHECK(_SUPPORTED_MAJOR, _SUPPORTED_MINOR, _SUPPORTED_PATCH)
#include <QNetworkInformation>
#else
#include <QObject>
#warning "This module requires at least Qt version 6.3.x or newer to work!"
#endif

QT_BEGIN_NAMESPACE
class QWidget;
class QObject;
QT_END_NAMESPACE

class TQNetworkInfo : public QObject
{
    public:
        TQNetworkInfo();
        ~TQNetworkInfo();

        typedef enum
        {
            NET_UNKNOWN,
            NET_ETHERNET,
            NET_CELLULAR,
            NET_WIFI,
            NET_BLUETOOTH
        }NET_TRANSPORT;

#if QT_VERSION >= QT_VERSION_CHECK(_SUPPORTED_MAJOR, _SUPPORTED_MINOR, _SUPPORTED_PATCH)
        QNetworkInformation::Reachability getReachability() { return mReachability; }
#else
        int getReachability() { return 0; }
#endif
        int getConnectionStrength() { return mLevel; };
        bool isConnected() { return mConnection; };
        NET_TRANSPORT getTransport() { return mType; }

    private slots:
#if QT_VERSION >= QT_VERSION_CHECK(_SUPPORTED_MAJOR, _SUPPORTED_MINOR, _SUPPORTED_PATCH)
        void onReachabilityChanged(QNetworkInformation::Reachability reachability);
        void onTransportMediumChanged(QNetworkInformation::TransportMedium current);
#endif
    private:
#if QT_VERSION >= QT_VERSION_CHECK(_SUPPORTED_MAJOR, _SUPPORTED_MINOR, _SUPPORTED_PATCH)
        NET_TRANSPORT toNetTransport(QNetworkInformation::TransportMedium trans);

        QNetworkInformation::Reachability mReachability{QNetworkInformation::Reachability::Unknown};
#endif
        bool mConnection{false};
        int mLevel{0};
        NET_TRANSPORT mType{NET_UNKNOWN};
        bool mInitialized{false};
};

#endif // TQNETWORKINFO_H

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

#include <QObject>

#include "tqnetworkinfo.h"
#include "tpagemanager.h"
#include "terror.h"

TQNetworkInfo::TQNetworkInfo()
{
    DECL_TRACER("TQNetworkInfo::TQNetworkInfo()");

#if QT_VERSION >= QT_VERSION_CHECK(_SUPPORTED_MAJOR, _SUPPORTED_MINOR, _SUPPORTED_PATCH)
    if (!QNetworkInformation::instance())
    {
        if (!QNetworkInformation::loadDefaultBackend())
        {
            MSG_WARNING("No network backend available!");
            mInitialized = false;
            return;
        }
    }

    QNetworkInformation *inst = QNetworkInformation::instance();
    connect(inst, &QNetworkInformation::reachabilityChanged, this, &TQNetworkInfo::onReachabilityChanged);
    connect(inst, &QNetworkInformation::transportMediumChanged, this, &TQNetworkInfo::onTransportMediumChanged);
    QNetworkInformation::TransportMedium trans = inst->transportMedium();
    mType = toNetTransport(trans);
    mReachability = inst->reachability();

    if (mReachability == QNetworkInformation::Reachability::Disconnected)
    {
        mConnection = false;

        if (gPageManager)
            gPageManager->informTPanelNetwork(false, 0, mType);
    }
    else if (gPageManager)
    {
        gPageManager->informTPanelNetwork(true, mLevel, mType);
        mConnection = true;
    }
#else
    mConnection = true;
#endif
    mLevel = 6;     // Range is 0 - 6
    mInitialized = true;
}

TQNetworkInfo::~TQNetworkInfo()
{
    DECL_TRACER("TQNetworkInfo::~TQNetworkInfo()");
}


#if QT_VERSION >= QT_VERSION_CHECK(_SUPPORTED_MAJOR, _SUPPORTED_MINOR, _SUPPORTED_PATCH)
void TQNetworkInfo::onReachabilityChanged(QNetworkInformation::Reachability reachability)
{
    DECL_TRACER("TQNetworkInfo::onReachabilityChanged(QNetworkInformation::Reachability reachability)");

    mReachability = reachability;
    MSG_DEBUG("Rachability changed to " << (int)mReachability);
    if (reachability == QNetworkInformation::Reachability::Disconnected)
    {
        mConnection = false;

        if (gPageManager)
            gPageManager->informTPanelNetwork(false, 0, mType);
    }
    else if (gPageManager)
    {
        gPageManager->informTPanelNetwork(true, mLevel, mType);
        mConnection = true;
    }
}

void TQNetworkInfo::onTransportMediumChanged(QNetworkInformation::TransportMedium current)
{
    DECL_TRACER("TQNetworkInfo::onTransportMediumChanged(QNetworkInformation::TransportMedium current)");

    mType = toNetTransport(current);
    MSG_DEBUG("Transport changed to: " << mType);
    if (mReachability == QNetworkInformation::Reachability::Disconnected)
    {
        if (gPageManager)
            gPageManager->informTPanelNetwork(false, 0, mType);
    }
    else if (gPageManager)
        gPageManager->informTPanelNetwork(true, mLevel, mType);
}

TQNetworkInfo::NET_TRANSPORT TQNetworkInfo::toNetTransport(QNetworkInformation::TransportMedium trans)
{
    DECL_TRACER("TQNetworkInfo::toNetTransport(QNetworkInformation::TransportMedium trans)");

    NET_TRANSPORT net = NET_UNKNOWN;

    switch(trans)
    {
        case QNetworkInformation::TransportMedium::Ethernet:    net = NET_ETHERNET;  break;
        case QNetworkInformation::TransportMedium::Cellular:    net = NET_CELLULAR;  break;
        case QNetworkInformation::TransportMedium::WiFi:        net = NET_WIFI;      break;
        case QNetworkInformation::TransportMedium::Bluetooth:   net = NET_BLUETOOTH; break;
        case QNetworkInformation::TransportMedium::Unknown:     net = NET_UNKNOWN;   break;
    }

    return net;
}
#endif

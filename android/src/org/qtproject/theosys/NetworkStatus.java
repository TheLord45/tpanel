/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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
package org.qtproject.theosys;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoNr;
import android.telephony.CellInfoTdscdma;
import android.telephony.CellInfoWcdma;
import android.telephony.CellSignalStrength;
import android.telephony.CellSignalStrengthCdma;
import android.telephony.CellSignalStrengthGsm;
import android.telephony.CellSignalStrengthLte;
import android.telephony.CellSignalStrengthNr;
import android.telephony.CellSignalStrengthTdscdma;
import android.telephony.CellSignalStrengthWcdma;
import android.telephony.PhoneStateListener;
import android.telephony.SignalStrength;
import android.telephony.TelephonyManager;
import android.util.Log;

//import org.qtproject.qt.android.QtNative;    // Enable for Qt 6.x
import org.qtproject.qt5.android.QtNative;      // Enable for Qt 5.x
import java.lang.String;
import java.util.List;

import org.qtproject.theosys.Logger;

public class NetworkStatus extends Logger
{
    static private Activity m_ActivityInstance = null;
    static private IntentFilter NetworkLevelFilter = null;
    static private BroadcastReceiver NetworkLevelReceiver = null;
    static private boolean mInitialized = false;
    static private Boolean connected;
    static private int level = 0;
    static private int sigStrength;
    static private int netType = 0;     // 1 = Wifi, 2 = mobile, 0 = undefined

    static public boolean isConnected() { return connected; }
    static public int getLevel() { return level; }

    static public void Init(Activity act)
    {
        m_ActivityInstance = act;
    }

    static public void destroyNetworkListener()
    {
        if (!mInitialized)
            return;

        m_ActivityInstance.unregisterReceiver(NetworkLevelReceiver);
        mInitialized = false;
    }

    static public int dbmToLevel(int dbm)
    {
        if (dbm < (-120) || dbm > (-24))
        {
            log(HLOG_WARNING, "NetworkStatus: Invalid Dbm value " + String.valueOf(dbm));
            return 0;
        }

        int l = dbm + 120;
        l = (int)(6.0 / 96.0 * (double)l);

        if (l < 0)
            l = 0;
        else if (l > 6)
            l = 6;

        return l;
    }

    static public void InstallNetworkListener()
    {
        if (mInitialized)
            return;

        mInitialized = true;
        log(HLOG_DEBUG, "NetworkStatus: Initializing the network listener ...");

        if (NetworkLevelFilter == null)
        {
            NetworkLevelFilter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
            NetworkLevelFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
            NetworkLevelFilter.addAction(WifiManager.RSSI_CHANGED_ACTION);
        }

        if (NetworkLevelReceiver != null)
        {
            m_ActivityInstance.registerReceiver(NetworkLevelReceiver, NetworkLevelFilter);
            informTPanelNetwork(connected, level, netType);
            return;
        }

        NetworkLevelReceiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                if (intent == null || intent.getExtras() == null)
                    return;

                // Retrieve a ConnectivityManager for handling management of network connections
                ConnectivityManager manager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
                // Details about the currently active default data network. When connected, this network is the default route for outgoing connections
                NetworkInfo networkInfo = manager.getActiveNetworkInfo();

                /**
                 * NOTE: getActiveNetworkInfo() may return null when there is no default network e.g. Airplane Mode
                 */
                if(networkInfo != null && networkInfo.getState() == NetworkInfo.State.CONNECTED)
                {
                    connected = true;
                }
                else if (intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false))    // Boolean that indicates whether there is a complete lack of connectivity
                {
                    connected = false;
                }

                if (connected && networkInfo.getType() == ConnectivityManager.TYPE_WIFI)
                {
                    try
                    {
                        WifiManager wifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
                        netType = 1;

                        level = WifiManager.calculateSignalLevel(wifiManager.getConnectionInfo().getRssi(), 6);

                        if (level > 6)
                            level = 6;
                        else if (level < 0)
                            level = 0;

                        log(HLOG_DEBUG, "NetworkStatus: Wifi level: " + String.valueOf(level));
                    }
                    catch (Exception e)
                    {
                        log(HLOG_ERROR, "NetworkStatus: Wifi exception: " + e);
                    }
                }
                else if (connected)     // mobile
                {
                    TelephonyManager telephonyManager = (TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE);

                    try
                    {
                        List<CellInfo> cellInfos = telephonyManager.getAllCellInfo();
                        level = 0;
                        netType = 2;
                        log(HLOG_INFO, "NetworkStatus: Entered mobile section ...");

                        if (cellInfos != null)
                        {
                            log(HLOG_INFO, "NetworkStatus: Have cellInfos");

                            if (cellInfos.size() == 0)
                            {
                                log(HLOG_INFO, "NetworkStatus: No cell infos found.");
                                SignalStrength signalStrength = telephonyManager.getSignalStrength();
                                level = signalStrength.getLevel();

                                if (level < 0)
                                    level = 0;
                                else if (level > 6)
                                    level = 6;
                            }
                            else
                            {
                                for (int i = 0; i < cellInfos.size(); i++)
                                {
                                    log(HLOG_INFO, "NetworkStatus: Testing for cell info " + String.valueOf(i) + ", " + cellInfos.get(i).toString());
                                    if (cellInfos.get(i).isRegistered())
                                    {
                                        log(HLOG_INFO, "NetworkStatus: Found registered index " + String.valueOf(i));
                                        if (cellInfos.get(i) instanceof CellInfoWcdma)
                                        {
                                            CellInfoWcdma cellInfoWcdma = (CellInfoWcdma) cellInfos.get(i);
                                            CellSignalStrengthWcdma cellSignalStrengthWcdma = cellInfoWcdma.getCellSignalStrength();
                                            level = dbmToLevel(cellSignalStrengthWcdma.getDbm());
                                        }
                                        else if (cellInfos.get(i) instanceof CellInfoGsm)
                                        {
                                            CellInfoGsm cellInfoGsm = (CellInfoGsm) cellInfos.get(i);
                                            CellSignalStrengthGsm cellSignalStrengthGsm = cellInfoGsm.getCellSignalStrength();
                                            level = dbmToLevel(cellSignalStrengthGsm.getDbm());
                                        }
                                        else if (cellInfos.get(i) instanceof CellInfoLte)
                                        {
                                            CellInfoLte cellInfoLte = (CellInfoLte) cellInfos.get(i);
                                            CellSignalStrengthLte cellSignalStrengthLte = cellInfoLte.getCellSignalStrength();
                                            level = dbmToLevel(cellSignalStrengthLte.getDbm());
                                        }
                                        else if (cellInfos.get(i) instanceof CellInfoCdma)
                                        {
                                            CellInfoCdma cellInfoCdma = (CellInfoCdma) cellInfos.get(i);
                                            CellSignalStrengthCdma cellSignalStrengthCdma = cellInfoCdma.getCellSignalStrength();
                                            level = dbmToLevel(cellSignalStrengthCdma.getDbm());
                                        }
                                        else if (cellInfos.get(i) instanceof CellInfoNr)
                                        {
                                            CellInfoNr cellInfoNr = (CellInfoNr) cellInfos.get(i);
                                            CellSignalStrength cellSignalStrength = cellInfoNr.getCellSignalStrength();
                                            level = dbmToLevel(cellSignalStrength.getDbm());
                                        }
                                        else if (cellInfos.get(i) instanceof CellInfoTdscdma)
                                        {
                                            CellInfoTdscdma cellInfoTdscdma = (CellInfoTdscdma) cellInfos.get(i);
                                            CellSignalStrengthTdscdma cellSignalStrengthTdscdma = cellInfoTdscdma.getCellSignalStrength();
                                            level = dbmToLevel(cellSignalStrengthTdscdma.getDbm());
                                        }

                                        log(HLOG_INFO, "NetworkStatus: Signal level: " + String.valueOf(level));
                                    }
                                }
                            }
                        }
                    }
                    catch (SecurityException e)
                    {
                        log(HLOG_ERROR, "NetworkStatus: Security exception: " + e);
                        level = 0;
                        netType = 0;
                    }
                    catch (Exception e)
                    {
                        log(HLOG_ERROR, "NetworkStatus: Exception: " + e);
                        level = 0;
                        netType = 0;
                    }
                }
                else
                {
                    level = 0;
                    netType = 0;
                }

                informTPanelNetwork(connected, level, netType);
            }
        };

        m_ActivityInstance.registerReceiver(NetworkLevelReceiver, NetworkLevelFilter);
        log(HLOG_INFO, "NetworkStatus: Network listener initialized.");
    }

    private static native void informTPanelNetwork(boolean conn, int level, int netType);
}

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

import android.os.BatteryManager;
import android.os.Bundle;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import org.qtproject.theosys.Logger;

public class BatteryState extends Logger
{
    static private Activity m_ActivityInstance = null;
    static private BroadcastReceiver BatteryLevelReceiver = null;
    static private IntentFilter BatteryLevelFilter = null;
    static private int mProgressStatus = 0;
    static private boolean mCharging = false;
    static private int mChargeType = 0;
    static private boolean mInitialized = false;

    static public int getLevel() { return mProgressStatus; }
    static public boolean isCharging() { return mCharging; }
    static public int getChargeType() { return mChargeType; }

    static protected void setLevel(int level) { mProgressStatus = level; }
    static protected void setCharging(boolean state) { mCharging = state; }
    static protected void setChargeType(int type) { mChargeType = type; }

    static public void Init(Activity act)
    {
        m_ActivityInstance = act;
    }

    static public void destroyBatteryListener()
    {
        if (!mInitialized)
            return;

        m_ActivityInstance.unregisterReceiver(BatteryLevelReceiver);
        mInitialized = false;
    }

    static public void InstallBatteryListener()
    {
        if (mInitialized)
            return;

        mInitialized = true;
        log(HLOG_INFO, "BatteryState: Initializing the battery listener ...");

        if (BatteryLevelFilter == null)
            BatteryLevelFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);

        if (BatteryLevelReceiver != null)
        {
            m_ActivityInstance.registerReceiver(BatteryLevelReceiver, BatteryLevelFilter);
            informBatteryStatus(mProgressStatus, mCharging, mChargeType);
            return;
        }

        BatteryLevelReceiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                try
                {
                    int scale  = intent.getIntExtra(BatteryManager.EXTRA_SCALE,-1);
                    int level  = intent.getIntExtra(BatteryManager.EXTRA_LEVEL,-1);
                    int status = intent.getIntExtra(BatteryManager.EXTRA_STATUS, -1);

                    double percentage = ((double)level / (double)scale) * 100.0;
                    setLevel((int)percentage);
                    boolean ch = ((status == BatteryManager.BATTERY_STATUS_CHARGING) ||
                                  (status == BatteryManager.BATTERY_STATUS_FULL));
                    setCharging(ch);

                    // How are we charging?
                    int chargePlug = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
                    boolean usbCharge = chargePlug == BatteryManager.BATTERY_PLUGGED_USB;
                    boolean acCharge = chargePlug == BatteryManager.BATTERY_PLUGGED_AC;
//                    boolean wlCharge = chargePlug == BatteryManage.BATTERY_PLUGGED_WIRELESS;

                    if (usbCharge)
                        setChargeType(1);    // Charging over USB
                    else if (acCharge)
                        setChargeType(2);    // Charging over plug
//                    else if (wlCharge)
//                        setChargeType(3);    // Wireless charging
                    else
                        setChargeType(0);    // No charging

                    informBatteryStatus(level, ch, mChargeType);
                }
                catch (Exception e)
                {
                    log(HLOG_ERROR, "BatteryState: Exception: " + e);
                }
            }
        };

        try
        {
            m_ActivityInstance.registerReceiver(BatteryLevelReceiver, BatteryLevelFilter);
            log(HLOG_INFO, "BatteryState: Battery listener initialized.");
        }
        catch (Exception e)
        {
            log(HLOG_ERROR, "BatteryState: Register exception: " + e);
        }
    }

    private static native void informBatteryStatus(int level, boolean charging, int chargeType);
}

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
import android.os.Bundle;
import android.telephony.TelephonyManager;

import org.qtproject.theosys.Logger;

public class PhoneCallState extends Logger
{
    static private Activity m_ActivityInstance = null;
    static private boolean mInitialized = false;
    static private boolean callInProgress = false;
    static private String phoneNumber = null;
    static private BroadcastReceiver phoneStateListener = null;
    static private IntentFilter PhoneCallFilter = null;

    static private final int MY_PERMISSION_REQUEST_CODE_PHONE_STATE = 1;

    static public boolean isCallInProgess() { return callInProgress; }
    static public String getPhoneNumber() { return phoneNumber; }

    static public void Init(Activity act)
    {
        m_ActivityInstance = act;
    }

    static public void destroyPhoneListener()
    {
        if (!mInitialized)
            return;

        m_ActivityInstance.unregisterReceiver(phoneStateListener);
        mInitialized = false;
    }

    static public void InstallPhoneListener()
    {
        if (mInitialized)
            return;

        mInitialized = true;
        log(HLOG_DEBUG, "PhoneCallState: Initializing the phone call listener ...");

        if (PhoneCallFilter == null)
            PhoneCallFilter = new IntentFilter(TelephonyManager.ACTION_PHONE_STATE_CHANGED);

        if (phoneStateListener != null)
        {
            m_ActivityInstance.registerReceiver(phoneStateListener, PhoneCallFilter);
            informPhoneState(callInProgress, phoneNumber);
            log(HLOG_INFO, "PhoneCallState: Phone state listener initialized.");
            return;
        }

        phoneStateListener = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                Bundle bundle = intent.getExtras();

                if (bundle == null)
                {
                    log(HLOG_ERROR, "PhoneCallState: bundle was NULL!");
                    return;
                }

                phoneNumber = null;

                // Incoming call
                String state = bundle.getString(TelephonyManager.EXTRA_STATE);

                if (state != null)
                {
                    if (state.equalsIgnoreCase(TelephonyManager.EXTRA_STATE_RINGING))
                    {
                        phoneNumber = bundle.getString(TelephonyManager.EXTRA_INCOMING_NUMBER);
                        callInProgress = true;
                        log(HLOG_INFO, "PhoneCallState: RINGING --> " + phoneNumber);
                    }
                    else if (state.equalsIgnoreCase(TelephonyManager.EXTRA_STATE_OFFHOOK))
                    {
                        phoneNumber = null;
                        callInProgress = true;
                        log(HLOG_INFO, "PhoneCallState: OFFHOOK");
                    }
                    else
                    {
                        phoneNumber = null;
                        callInProgress = false;
                        log(HLOG_INFO, "PhoneCallState: IDLE");
                    }
                }
                else // outgoing call
                {
                    phoneNumber = bundle.getString(Intent.EXTRA_PHONE_NUMBER);
                    callInProgress = true;
                    log(HLOG_INFO, "PhoneCallState: OUTGOING CALL: " + phoneNumber);
                }

                informPhoneState(callInProgress, phoneNumber);
            }
        };

        m_ActivityInstance.registerReceiver(phoneStateListener, PhoneCallFilter);
        log(HLOG_INFO, "PhoneCallState: Phone state listener initialized.");
    }

    private static native void informPhoneState(boolean call, String pnumber);
}

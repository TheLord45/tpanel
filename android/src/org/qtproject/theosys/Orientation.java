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
package org.qtproject.theosys;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.view.OrientationEventListener;

import org.qtproject.theosys.Logger;

public class Orientation extends Logger
{
    static private Activity m_ActivityInstance = null;
    static private OrientationEventListener mOrientationListener = null;
    static private IntentFilter OrientationFilter = null;
    static private BroadcastReceiver OrientationReceiver = null;
    static private SensorManager mSensorManager = null;
    static private Sensor mAccelerometer, mMagnetSensor;
    static private boolean mInitialized = false;

    static public void Init(Activity act)
    {
        m_ActivityInstance = act;
    }

    static public void destroyOrientationListener()
    {
        if (!mInitialized)
            return;

        m_ActivityInstance.unregisterReceiver(OrientationReceiver);
        mInitialized = false;
    }

    static public void InstallOrientationListener()
    {
        if (mInitialized)
            return;

        mInitialized = true;
        log(HLOG_DEBUG, "Orientation: Initializing the orientation listener ...");


        if (OrientationFilter == null)
            OrientationFilter = new IntentFilter(Intent.ACTION_CONFIGURATION_CHANGED);

        if (OrientationReceiver != null)
        {
            m_ActivityInstance.registerReceiver(OrientationReceiver, OrientationFilter);
            return;
        }

        SensorEventListener listener = new SensorEventListener()
        {
            float[] accelerometerValues = new float[3];
            float[] magneticValues = new float[3];

            @Override
            public void onSensorChanged(SensorEvent event)
            {
                // Determine whether it is an acceleration sensor or a geomagnetic sensor
                if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
                {
                    // Pay attention to call the clone() method when assigning
                    accelerometerValues = event.values.clone();
                }
                else if (event.sensor.getType() == Sensor.TYPE_MAGNETIC_FIELD)
                {
                    magneticValues = event.values.clone();
                }

                float[] R = new float[9];
                float[] values = new float[3];

                SensorManager.getRotationMatrix(R, null, accelerometerValues, magneticValues);
                SensorManager.getOrientation(R, values);
                int orientation = (int)Math.toDegrees(values[2]);

                int ori = -1;

                log(HLOG_INFO, "Orientation: " + String.valueOf(orientation));

                if ((orientation >= 0 && orientation <= 45) || orientation >= 315)
                    ori = 1;    // Portrait
                else if (orientation > 45 && orientation < 136)
                    ori = 8;    // Inverse landscape
                else if (orientation > 135 && orientation < 226)
                    ori = 9;    // Inverse portrait
                else
                    ori = 0;    // Landscape

                informTPanelOrientation(ori);
            }

            @Override
            public void onAccuracyChanged(Sensor sensor, int accuracy)
            {
            }
        };

        OrientationReceiver = new BroadcastReceiver()
        {
            public void onReceive(Context context, Intent intent)
            {
                if (intent == null || intent.getExtras() == null)
                    return;

                mSensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);
                mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
                mMagnetSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
                mSensorManager.registerListener(listener, mMagnetSensor, SensorManager.SENSOR_DELAY_NORMAL);
                mSensorManager.registerListener(listener, mAccelerometer, SensorManager.SENSOR_DELAY_NORMAL);
            }
        };

        m_ActivityInstance.registerReceiver(OrientationReceiver, OrientationFilter);
        log(HLOG_INFO, "Orientation: Orientation listener initialized.");
    }

    private static native void informTPanelOrientation(int orientation);
}

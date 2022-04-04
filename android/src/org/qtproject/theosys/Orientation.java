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
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.view.OrientationEventListener;

import org.qtproject.theosys.Logger;

public class Orientation extends Logger
{
    public static final int ORIENTATION_PORTRAIT = 1;
    public static final int ORIENTATION_LANDSCAPE_REVERSE = 8;
    public static final int ORIENTATION_LANDSCAPE = 0;
    public static final int ORIENTATION_PORTRAIT_REVERSE = 9;
    public static final int ORIENTATION_FACE_UP = 15;
    public static final int ORIENTATION_FACE_DOWN = 16;

    static private Activity m_ActivityInstance = null;
    static private SensorManager mSensorManager = null;
    static private Sensor mAccelerometer, mMagnetSensor;
    static private SensorEventListener mListener = null;
    static private boolean mInitialized = false;
    static private int mOrientation = ORIENTATION_PORTRAIT;
    static private int mOldOrientation = 9999;
    static private float mAveragePitch = 0;
    static private float mAverageRoll = 0;
    static private float[] mPitches;
    static private float[] mRolls;
    static private int mSmoothness = 5;     // Number of values to calculate average
    static private boolean mPaused = false;

    static public void Init(Activity act, int ori)
    {
        m_ActivityInstance = act;
        mPitches = new float[mSmoothness];
        mRolls = new float[mSmoothness];

        if (ori == 0)   // Landscape
            mOrientation = ORIENTATION_LANDSCAPE;
        else
            mOrientation = ORIENTATION_PORTRAIT;

        if ((mSensorManager = (SensorManager)act.getSystemService(Context.SENSOR_SERVICE)) == null)
        {
            log(HLOG_ERROR, "Orientation: Can't get SENSOR_SERVICE!");
            return;
        }

        mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        mMagnetSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    }

    static public void destroyOrientationListener()
    {
        if (!mInitialized)
            return;

        if (mSensorManager != null && mListener != null)
            mSensorManager.unregisterListener(mListener);

        mInitialized = false;
    }

    static public void pauseOrientationListener()
    {
        if (mPaused || !mInitialized)
            return;

        if (mInitialized && mSensorManager != null && mListener != null)
        {
            mSensorManager.unregisterListener(mListener);
            mPaused = true;
            log(HLOG_INFO,"Orientation: Listener paused.");
        }
    }

    static public void resumeOrientationListener()
    {
        if (!mPaused || !mInitialized)
            return;

        if (mSensorManager != null && mMagnetSensor != null && mAccelerometer != null)
        {
            mSensorManager.registerListener(mListener, mMagnetSensor, SensorManager.SENSOR_DELAY_UI);
            mSensorManager.registerListener(mListener, mAccelerometer, SensorManager.SENSOR_DELAY_UI);
            mPaused = false;
            log(HLOG_INFO,"Orientation: Listener resumed.");
        }
    }

    static public void InstallOrientationListener()
    {
        if (mInitialized)
            return;

        mInitialized = true;
        log(HLOG_DEBUG, "Orientation: Initializing the orientation listener ...");

        if (mListener != null)
            return;

        mListener = new SensorEventListener()
        {
            float[] accelerometerValues = new float[3];
            float[] magneticValues = new float[3];

            @Override
            public void onSensorChanged(SensorEvent event)
            {
                // Determine whether it is an acceleration sensor or a geomagnetic sensor
                if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
                    accelerometerValues = event.values.clone();
                else if (event.sensor.getType() == Sensor.TYPE_MAGNETIC_FIELD)
                    magneticValues = event.values.clone();
                else
                    return;

                if (accelerometerValues == null || magneticValues == null)
                    return;

                float[] R = new float[9];
                float[] values = new float[3];

                boolean success = SensorManager.getRotationMatrix(R, null, accelerometerValues, magneticValues);

                if (!success)
                    return;

                SensorManager.getOrientation(R, values);
                mAveragePitch = addValue(values[1], mPitches);
                mAverageRoll = addValue(values[2], mRolls);
                mOrientation = calculateOrientation();

                if (mOrientation == mOldOrientation)
                    return;

                mOldOrientation = mOrientation;
                informTPanelOrientation(mOrientation);
            }

            @Override
            public void onAccuracyChanged(Sensor sensor, int accuracy)
            {
            }


            private int calculateOrientation()
            {
                if (mAverageRoll < 20 && mAverageRoll > -20)
                    return ORIENTATION_FACE_UP;

                if (mAverageRoll > 160 || mAverageRoll < -160)
                    return ORIENTATION_FACE_DOWN;

                // finding local orientation dip
                if (((mOrientation == ORIENTATION_PORTRAIT || mOrientation == ORIENTATION_PORTRAIT_REVERSE)
                        && (mAverageRoll > -30 && mAverageRoll < 30)))
                {
                    if (mAveragePitch > 0)
                        return ORIENTATION_PORTRAIT_REVERSE;
                    else
                        return ORIENTATION_PORTRAIT;
                }
                else
                {
                    // divides between all orientations
                    if (Math.abs(mAveragePitch) >= 30)
                    {
                        if (mAveragePitch > 0)
                            return ORIENTATION_PORTRAIT_REVERSE;
                        else
                            return ORIENTATION_PORTRAIT;
                    }
                    else
                    {
                        if (mAverageRoll > 0)
                            return ORIENTATION_LANDSCAPE_REVERSE;
                        else
                            return ORIENTATION_LANDSCAPE;
                    }
                }
            }

            private float addValue(float value, float[] values)
            {
                value = (float) Math.round((Math.toDegrees(value)));
                float average = 0;

                for (int i = 1; i < mSmoothness; i++)
                {
                    values[i - 1] = values[i];
                    average += values[i];
                }

                values[mSmoothness - 1] = value;
                average = (average + value) / mSmoothness;
                return average;
            }
        };

        if (mMagnetSensor != null && mAccelerometer != null)
        {
            mSensorManager.registerListener(mListener, mMagnetSensor, SensorManager.SENSOR_DELAY_UI);
            mSensorManager.registerListener(mListener, mAccelerometer, SensorManager.SENSOR_DELAY_UI);
        }

        log(HLOG_INFO, "Orientation: Orientation listener initialized.");
    }

    private static native void informTPanelOrientation(int orientation);
}

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

import java.util.*;

import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;

import androidx.appcompat.app.AppCompatActivity;

import org.qtproject.theosys.SettingsActivity;
import org.qtproject.theosys.Logger;

public class Settings extends Logger
{
    static private Activity m_ActivityInstance = null;
    static private Intent m_intent = null;
    static private List<String> mSurfaces = null;

    static public void callSettings(Activity act)
    {
        m_ActivityInstance = act;

        m_intent = new Intent(act, SettingsActivity.class);

        if (m_intent == null)
        {
            log(HLOG_ERROR, "Settings.callSettings: Couldn't initialize a new Intent!");
            return;
        }

        act.setTheme(R.style.Theme_Tpanel_settings);
        deploySurfaces();
        act.startActivity(m_intent);
    }

    static public void addSurface(String sf)
    {
        if (mSurfaces == null)
            mSurfaces = new ArrayList<String>();

        log(HLOG_DEBUG, "Settings.addSurface: String: " + sf);
        mSurfaces.add(sf);
    }

    static public void deploySurfaces()
    {
        if (mSurfaces == null || m_ActivityInstance == null || m_intent == null)
            return;

        if (m_ActivityInstance.isDestroyed() || mSurfaces.isEmpty())
            return;

        // Transfer list
        log(HLOG_DEBUG, "Settings.deploySurfaces: Deploy surfaces to instance ...");
        CharSequence[] entries = mSurfaces.toArray(new CharSequence[mSurfaces.size()]);
        m_intent.putExtra("surfaces", entries);
    }

    static public void clearSurfaces()
    {
        if (mSurfaces == null || mSurfaces.isEmpty())
            return;

        mSurfaces.clear();
    }
}

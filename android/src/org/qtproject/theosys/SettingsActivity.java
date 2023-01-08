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

import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.view.View;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.DropDownPreference;
import androidx.preference.EditTextPreference;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.SeekBarPreference;
import androidx.preference.SwitchPreference;

import org.qtproject.theosys.Logger;

public class SettingsActivity extends AppCompatActivity
{
    static private Intent m_intent = null;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Logger.log(Logger.HLOG_DEBUG, "SettingsActivity.onCreate()");
        setContentView(R.layout.settings_activity);

        try
        {
            if (savedInstanceState == null)
            {
                getSupportFragmentManager()
                        .beginTransaction()
                        .replace(R.id.settings, new SettingsFragment())
                        .commit();
            }
        }
        catch(Exception e)
        {
            Logger.log(Logger.HLOG_ERROR, "SettingsActivity.onCreate: " + e);
        }

        m_intent = getIntent();
        ActionBar actionBar = getSupportActionBar();

        if (actionBar != null)
        {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setTitle(getString(R.string.title_activity_settings));
        }

        Logger.log(Logger.HLOG_DEBUG, "SettingsActivity.onCreate: finished");
    }

    public static class SettingsFragment extends PreferenceFragmentCompat
    {
        @Override
        public void onDestroyView()
        {
            Logger.log(Logger.HLOG_DEBUG, "onDestroyView: Saving settings ...");
            saveSettings();
            super.onDestroyView();
        }

        public void onViewCreated(View view, Bundle savedInstanceState)
        {
            super.onViewCreated(view, savedInstanceState);
            Logger.log(Logger.HLOG_DEBUG, "SettingsActivity.SettingsFragment.onViewCreated()");

            if (m_intent == null)
            {
                Logger.log(Logger.HLOG_ERROR, "SettingsActivity.SettingsFragment.onViewCreated: Error getting the Intent!");
                return;
            }

            CharSequence[] values = m_intent.getCharSequenceArrayExtra("surfaces");

            if (values == null)
            {
                Logger.log(Logger.HLOG_WARNING, "SettingsActivity.SettingsFragment.onViewCreated: No extra data found!");
                return;
            }

            DropDownPreference surfaces = findPreference("netlinx_surface");

            if (surfaces == null)
            {
                Logger.log(Logger.HLOG_ERROR, "SettingsActivity.SettingsFragment.onViewCreated: The surface preference \"netlinx_surface\" was not found!");
                return;
            }

            CharSequence[] entries = new String[values.length];

            for (int i = 0; i < values.length; i++)
            {
                if (values[i] == null)
                {
                    Logger.log(Logger.HLOG_WARNING, "SettingsActivity.SettingsFragment.onViewCreated: Index " + String.valueOf(i) + " is not initialized!");
                    continue;
                }

                String sf = values[i].toString();
                int pos = sf.lastIndexOf(".");
                String name;

                if (pos >= 0)
                    name = sf.substring(0, pos);
                else
                    name = sf;

                entries[i] = name;
            }

            surfaces.setEntries(entries);
            surfaces.setEntryValues(values);
            surfaces.setDefaultValue(values[0]);
        }

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
        {
            setPreferencesFromResource(R.xml.root_preferences, rootKey);

            EditTextPreference netlinxIp = findPreference("netlinx_ip");

            if (netlinxIp != null)
            {
                netlinxIp.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_VARIATION_URI));
                netlinxIp.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx IP: " + value);
                    setNetlinxIp(value.toString());
                    return true;
                });
            }

            EditTextPreference netlinxPort = findPreference("netlinx_port");

            if (netlinxPort != null)
            {
                netlinxPort.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_CLASS_NUMBER));
                netlinxPort.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx Port: " + value);
                    setNetlinxPort(Integer.parseInt(value.toString()));
                    return true;
                });
            }

            EditTextPreference netlinxChannel = findPreference("netlinx_channel");

            if (netlinxChannel != null)
            {
                netlinxChannel.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_CLASS_NUMBER));
                netlinxChannel.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx Channel: " + value);
                    setNetlinxChannel(Integer.parseInt(value.toString()));
                    return true;
                });
            }

            EditTextPreference netlinxType = findPreference("netlinx_type");

            if (netlinxType != null)
            {
                netlinxType.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS));
                netlinxType.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx Type: " + value);
                    setNetlinxType(value.toString());
                    return true;
                });
            }

            EditTextPreference netlinxFtpUser = findPreference("netlinx_ftp_user");

            if (netlinxFtpUser != null)
            {
                netlinxFtpUser.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS));
                netlinxFtpUser.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx User: " + value);
                    setNetlinxFtpUser(value.toString());
                    return true;
                });
            }

            EditTextPreference netlinxPassword = findPreference("netlinx_ftp_password");

            if (netlinxPassword != null)
            {
                netlinxPassword.setOnBindEditTextListener(editText -> {
                    editText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
                    netlinxPassword.setSummaryProvider(preference -> setAsterisks(editText.getText().toString().length()));
                });

                netlinxPassword.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx Password: " + value);
                    setNetlinxFtpPassword(value.toString());
                    return true;
                });
            }

            DropDownPreference netlinxSurface = findPreference("netlinx_surface");

            if (netlinxSurface != null)
            {
                netlinxSurface.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx surface: " + value);
                    setNetlinxSurface(value.toString());
                    return true;
                });
            }

            SwitchPreference netlinxFtpPassive = findPreference("netlinx_ftp_passive");

            if (netlinxFtpPassive != null)
            {
                netlinxFtpPassive.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "NetLinx passive: " + value);
                    setNetlinxFtpPassive((Boolean)value);
                    return true;
                });
            }

            // View
            SwitchPreference viewScale = findPreference("view_scale");

            if (viewScale != null)
            {
                viewScale.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "View scale: " + value);
                    setViewScale((Boolean)value);
                    return true;
                });
            }

            SwitchPreference viewToolbar = findPreference("view_toolbar");

            if (viewToolbar != null)
            {
                viewToolbar.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "View toolbar: " + value);
                    setViewToolbar((Boolean)value);
                    return true;
                });
            }

            SwitchPreference viewToolbarForce = findPreference("view_toolbar_force");

            if (viewToolbarForce != null)
            {
                if (viewToolbar != null)
                {
                    if (viewToolbarForce.isChecked())
                        viewToolbar.setVisible(false);
                    else
                        viewToolbar.setVisible(true);
                }

                viewToolbarForce.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "View toolbar force: " + value);

                    if (viewToolbar != null)
                        viewToolbar.setVisible(!(Boolean)value);

                    setViewToolbarForce((Boolean)value);
                    return true;
                });
            }

            SwitchPreference viewRotation = findPreference("view_rotation");

            if (viewRotation != null)
            {
                viewRotation.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "View rotation: " + value);
                    setViewRotation((Boolean)value);
                    return true;
                });
            }
            // Sound
            ListPreference soundSystem = findPreference("sound_system");

            if (soundSystem != null)
            {
                soundSystem.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Sound system: " + value.toString());
                    setSoundSystem(value.toString());
                    return true;
                });
            }

            ListPreference soundSingle = findPreference("sound_single");

            if (soundSingle != null)
            {
                soundSingle.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Sound single: " + value.toString());
                    setSoundSingle(value.toString());
                    return true;
                });
            }

            ListPreference soundDouble = findPreference("sound_double");

            if (soundDouble != null)
            {
                soundDouble.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Sound double: " + value.toString());
                    setSoundDouble(value.toString());
                    return true;
                });
            }

            SwitchPreference soundEnable = findPreference("sound_enable");

            if (soundEnable != null)
            {
                soundEnable.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Sound enable: " + value);
                    setSoundEnable((Boolean)value);
                    return true;
                });
            }

            SeekBarPreference soundVolume = (SeekBarPreference)findPreference("sound_volume");

            if (soundVolume != null)
            {
                soundVolume.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Sound volume: " + value);
                    setSoundVolume((int)value);
                    return true;
                });
            }

            SeekBarPreference soundGain = (SeekBarPreference)findPreference("sound_gain");

            if (soundGain != null)
            {
                soundGain.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Sound gain: " + value);
                    setSoundGain((int)value);
                    return true;
                });
            }

            // SIP
            EditTextPreference sipProxy = findPreference("sip_proxy");

            if (sipProxy != null)
            {
                sipProxy.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_VARIATION_URI));
                sipProxy.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP proxy: " + value);
                    setSipProxy(value.toString());
                    return true;
                });
            }

            EditTextPreference sipPort = findPreference("sip_port");

            if (sipPort != null)
            {
                sipProxy.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_CLASS_NUMBER));
                sipProxy.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP port: " + value);
                    setSipPort(Integer.parseInt(value.toString()));
                    return true;
                });
            }

            EditTextPreference sipTlsPort = findPreference("sip_tls_port");

            if (sipTlsPort != null)
            {
                sipProxy.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_CLASS_NUMBER));
                sipProxy.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP TLS port: " + value);
                    setSipTlsPort(Integer.parseInt(value.toString()));
                    return true;
                });
            }

            EditTextPreference sipStun = findPreference("sip_stun");

            if (sipStun != null)
            {
                sipStun.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_VARIATION_URI));
                sipStun.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP STUN: " + value);
                    setSipStun(value.toString());
                    return true;
                });
            }

            EditTextPreference sipDomain = findPreference("sip_domain");

            if (sipDomain != null)
            {
                sipDomain.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_VARIATION_URI));
                sipDomain.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP somain: " + value);
                    setSipDomain(value.toString());
                    return true;
                });
            }

            EditTextPreference sipUser = findPreference("sip_user");

            if (sipUser != null)
            {
                sipUser.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS));
                sipUser.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP user: " + value);
                    setSipUser(value.toString());
                    return true;
                });
            }

            EditTextPreference sipPassword = findPreference("sip_password");

            if (sipPassword != null)
            {
                sipPassword.setOnBindEditTextListener(editText -> {
                    editText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
                    sipPassword.setSummaryProvider(preference -> setAsterisks(editText.getText().toString().length()));
                });

                sipPassword.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP password: " + value);
                    setSipPassword(value.toString());
                    return true;
                });
            }

            SwitchPreference sipIpv4 = findPreference("sip_ipv4");

            if (sipIpv4 != null)
            {
                sipIpv4.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP IPv4: " + value);
                    setSipIpv4((Boolean)value);
                    return true;
                });
            }

            SwitchPreference sipIpv6 = findPreference("sip_ipv6");

            if (sipIpv6 != null)
            {
                sipIpv6.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP IPv6: " + value);
                    setSipIpv6((Boolean)value);
                    return true;
                });
            }

            SwitchPreference sipEnabled = findPreference("sip_enabled");

            if (sipEnabled != null)
            {
                sipEnabled.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP enabled: " + value);
                    setSipEnabled((Boolean)value);
                    return true;
                });
            }

            SwitchPreference sipIphone = findPreference("sip_internal_phone");

            if (sipIphone != null)
            {
                sipIphone.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "SIP internal phone: " + value);
                    setSipIphone((Boolean)value);
                    return true;
                });
            }

            // Logging
            SwitchPreference logInfo = findPreference("logging_info");

            if (logInfo != null)
            {
                logInfo.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log info: " + value);
                    setLogInfo((Boolean)value);
                    return true;
                });
            }

            SwitchPreference logWarning = findPreference("logging_warning");

            if (logWarning != null)
            {
                logWarning.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log warning: " + value);
                    setLogWarning((Boolean)value);
                    return true;
                });
            }

            SwitchPreference logError = findPreference("logging_error");

            if (logError != null)
            {
                logError.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log error: " + value);
                    setLogError((Boolean)value);
                    return true;
                });
            }

            SwitchPreference logTrace = findPreference("logging_trace");

            if (logTrace != null)
            {
                logTrace.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log trace: " + value);
                    setLogTrace((Boolean)value);
                    return true;
                });
            }

            SwitchPreference logDebug = findPreference("logging_debug");

            if (logDebug != null)
            {
                logDebug.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log debug: " + value);
                    setLogDebug((Boolean)value);
                    return true;
                });
            }

            SwitchPreference logProfile = findPreference("logging_profile");

            if (logProfile != null)
            {
                logProfile.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log profile: " + value);
                    setLogProfile((Boolean)value);
                    return true;
                });
            }

            SwitchPreference logFormat = findPreference("logging_long_format");

            if (logFormat != null)
            {
                logFormat.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log long format: " + value);
                    setLogLongFormat((Boolean)value);
                    return true;
                });
            }

            SwitchPreference logFileEnable = findPreference("logging_logfile_enabled");

            if (logFileEnable != null)
            {
                logFileEnable.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log file enabled: " + value);
                    setLogEnableFile((Boolean)value);
                    return true;
                });
            }

            EditTextPreference logFile = findPreference("logging_logfile");

            if (logFile != null)
            {
                logFile.setOnBindEditTextListener(editText -> editText.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS));
                logFile.setOnPreferenceChangeListener((preference, value) -> {
                    Logger.log(Logger.HLOG_DEBUG, "Log file: " + value);
                    setLogFile(value.toString());
                    return true;
                });
            }
        }
        //return the password in asterisks
        private String setAsterisks(int length)
        {
            StringBuilder sb = new StringBuilder();

            for (int s = 0; s < length; s++)
                sb.append("*");

            return sb.toString();
        }
    }

    private static native void setNetlinxIp(String ip);
    private static native void setNetlinxPort(int port);
    private static native void setNetlinxChannel(int channel);
    private static native void setNetlinxType(String type);
    private static native void setNetlinxFtpUser(String user);
    private static native void setNetlinxFtpPassword(String pw);
    private static native void setNetlinxSurface(String surface);
    private static native void setNetlinxFtpPassive(boolean passive);

    private static native void setViewScale(boolean scale);
    private static native void setViewToolbar(boolean bar);
    private static native void setViewToolbarForce(boolean bar);
    private static native void setViewRotation(boolean rotate);

    private static native void setSoundSystem(String sound);
    private static native void setSoundSingle(String sound);
    private static native void setSoundDouble(String sound);
    private static native void setSoundEnable(boolean sound);
    private static native void setSoundVolume(int sound);
    private static native void setSoundGain(int sound);

    private static native void setSipProxy(String sip);
    private static native void setSipPort(int sip);
    private static native void setSipTlsPort(int sip);
    private static native void setSipStun(String sip);
    private static native void setSipDomain(String sip);
    private static native void setSipUser(String sip);
    private static native void setSipPassword(String sip);
    private static native void setSipIpv4(boolean sip);
    private static native void setSipIpv6(boolean sip);
    private static native void setSipEnabled(boolean sip);
    private static native void setSipIphone(boolean sip);

    private static native void setLogInfo(boolean log);
    private static native void setLogWarning(boolean log);
    private static native void setLogError(boolean log);
    private static native void setLogTrace(boolean log);
    private static native void setLogDebug(boolean log);
    private static native void setLogProfile(boolean log);
    private static native void setLogLongFormat(boolean log);
    private static native void setLogEnableFile(boolean log);
    private static native void setLogFile(String log);

    private static native void saveSettings();
}

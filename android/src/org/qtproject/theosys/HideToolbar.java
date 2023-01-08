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

import android.os.Bundle;
import android.os.Build;
import android.app.Activity;
import android.app.ActionBar;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import org.qtproject.theosys.Logger;

public class HideToolbar extends Logger
{
    static private boolean mBusy = false;

    static public void hide(Activity act, boolean h)
    {
        if (act == null)
            return;

        if (mBusy)
            return;

        mBusy = true;
        act.setTheme(R.style.Theme_Tpanel_settings);

        act.runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                try
                {
                    Window window = act.getWindow();

                    if (window != null)
                    {
                        if (Build.VERSION.SDK_INT < 30)
                            window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);
                        else
                        {
                            WindowInsetsController wic = window.getInsetsController();

                            if (wic != null)
                            {
                                if (h)
                                {
                                    wic.hide(WindowInsets.Type.statusBars());
                                    wic.hide(WindowInsets.Type.systemBars());
                                    log(HLOG_DEBUG, "HideToolbar.hide: Statusbars were hided.");
                                }
                                else
                                {
                                    wic.show(WindowInsets.Type.statusBars());
                                    wic.show(WindowInsets.Type.systemBars());
                                    log(HLOG_DEBUG, "HideToolbar.hide: Statusbars were shown.");
                                }
                            }
                            else
                                log(HLOG_WARNING, "HideToolbar.hide: Error retrieving window insets controller!");
                        }
                    }
                    else
                        log(HLOG_WARNING, "HideToolbar.hide: Error retrieving window!");
                }
                catch (Exception e)
                {
                    log(HLOG_ERROR, "HideToolbar.hide: " + e);
                }
            }
        });

        ActionBar ab = act.getActionBar();

        if (ab != null)
        {
            if (h)
                ab.hide();
            else
                ab.show();

            ab.setDisplayShowTitleEnabled(!h);
            log(HLOG_DEBUG, "HideToolbar.hide: Action bar was set " + (h ? "HIDDEN" : "VISIBLE"));
        }
        else
            log(HLOG_WARNING, "HideToolbar.hide: No action bar found!");

        mBusy = false;
    }
}

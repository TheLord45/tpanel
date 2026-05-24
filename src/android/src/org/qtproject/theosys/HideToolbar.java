/*
 * Copyright (C) 2022 to 2026 by Andreas Theofilu <andreas@theosys.at>
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
import android.app.ActionBar;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Build;
import android.view.DisplayCutout;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import androidx.core.view.WindowCompat;

import java.util.List;
import java.util.Locale;

import org.qtproject.theosys.Logger;

public class HideToolbar extends Logger
{
    static private boolean mBusy = false;
    static private boolean mInit = false;

    static public void hide(Activity act, boolean hide)
    {
        if (act == null)
            return;

        if (mBusy)
            return;

        mBusy = true;
        act.setTheme(R.style.TPanelAppTheme);

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
                        WindowInsetsController wic = window.getInsetsController();

                        if (wic != null)
                        {
                            if (!mInit)
                            {
                                WindowCompat.enableEdgeToEdge(window);
                                mInit = true;
                            }

                            if (hide)
                            {
                                wic.hide(WindowInsets.Type.statusBars() |
                                         WindowInsets.Type.navigationBars() |
                                         WindowInsets.Type.systemOverlays() |
                                         WindowInsets.Type.displayCutout() |
                                         WindowInsets.Type.captionBar());   // All types of bars
                                wic.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                                log(HLOG_DEBUG, "HideToolbar.hide: Statusbars were hidden.");
                            }
                            else
                            {
                                wic.show(WindowInsets.Type.statusBars() |
                                         WindowInsets.Type.navigationBars() |
                                         WindowInsets.Type.systemOverlays() |
                                         WindowInsets.Type.displayCutout() |
                                         WindowInsets.Type.captionBar());   // All types of bars
                                wic.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_DEFAULT);
                                log(HLOG_DEBUG, "HideToolbar.hide: Statusbars were shown.");
                            }
                        }
                        else
                            log(HLOG_WARNING, "HideToolbar.hide: Error retrieving WindowInsetsController!");
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

        mBusy = false;
    }

    public static void notchInfo(Activity activity)
    {
        if (activity == null)
        {
            log(HLOG_ERROR, "Notch: activity unavailable");
            return;
        }

        if (Build.VERSION.SDK_INT < 28)
        {
            log(HLOG_ERROR, "Notch: API < 28 (DisplayCutout not supported)");
            return;
        }

        final Window window = activity.getWindow();

        if (window == null)
        {
            log(HLOG_ERROR, "Notch: window unavailable");
            return;
        }

        final View decor = window.getDecorView();

        if (decor == null)
        {
            log(HLOG_ERROR, "Notch: decor view unavailable");
            return;
        }

        final WindowInsets insets = decor.getRootWindowInsets();

        if (insets == null)
        {
            log(HLOG_ERROR, "Notch: info unavailable (insets not ready)");
            return;
        }

        final DisplayCutout cutout = insets.getDisplayCutout();

        if (cutout == null)
        {
            log(HLOG_INFO, "Notch: none");
            return;
        }

        final List<Rect> rects = cutout.getBoundingRects();
        int maxW = 0, maxH = 0;

        if (rects != null)
        {
            for (Rect r : rects)
            {
                if (r != null)
                {
                    maxW = Math.max(maxW, r.width());
                    maxH = Math.max(maxH, r.height());
                }
            }
        }

        final int l = Math.min(maxH, cutout.getSafeInsetLeft());
        final int t = Math.min(maxH, cutout.getSafeInsetTop());
        final int r = Math.min(maxH, cutout.getSafeInsetRight());
        final int b = Math.min(maxH, cutout.getSafeInsetBottom());

        String debug = String.format(
                Locale.US,
                "Notch: rects=%d max=%dx%d px, safeInsets L=%d T=%d R=%d B=%d",
                rects == null ? 0 : rects.size(),
                maxW, maxH, l, t, r, b
        );

        log(HLOG_DEBUG, debug);

        if (rects != null)
            informTPanelNotch(maxW, maxH, l, t, r, b);
    }

    private static native void informTPanelNotch(int width, int height, int left, int top, int right, int bottom);
}

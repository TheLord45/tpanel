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

public class Logger
{
    static final int HLOG_NONE =    0x0000;
    static final int HLOG_INFO =    0x0001;
    static final int HLOG_WARNING = 0x0002;
    static final int HLOG_ERROR =   0x0004;
    static final int HLOG_TRACE =   0x0008;
    static final int HLOG_DEBUG =   0x0010;

    public static void log(int mode, String msg)
    {
        if (mode != HLOG_INFO && mode != HLOG_WARNING && mode != HLOG_ERROR &&
            mode != HLOG_TRACE && mode != HLOG_DEBUG)
            return;

        logger(mode, msg);
    }

    private static native void logger(int mode, String msg);
}

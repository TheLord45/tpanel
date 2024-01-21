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

#include "timagerefresh.h"
#include "terror.h"

TImageRefresh::TImageRefresh()
{
    DECL_TRACER("TImageRefresh::TImageRefresh()");
}

TImageRefresh::~TImageRefresh()
{
    DECL_TRACER("TImageRefresh::~TImageRefresh()");

    stopWait();
}

void TImageRefresh::run(const std::string& url)
{
    DECL_TRACER("TImageRefresh::run()");

    if (mRunning || mThread.joinable() || mSec == std::chrono::seconds(0))
        return;

    try
    {
        mStopped = false;
        mThread = std::thread([=] { this->_run(url); });
        mThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting the TImageRefresh thread: " << e.what());
    }
}

void TImageRefresh::stopWait()
{
    DECL_TRACER("TImageRefresh::stopWait()");

    mStopped = true;

    while(mRunning)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void TImageRefresh::_run(const std::string& url)
{
    DECL_TRACER("TImageRefresh::_run(const std::string& url)");

    if (mRunning)
        return;

    mRunning = true;

    while (!mStopped && !prg_stopped)
    {
        if (_callback)
            _callback(url);

        if (mStopped || mRunOnce)
            break;

        std::this_thread::sleep_for(mSec);
    }

    mRunning = false;
}

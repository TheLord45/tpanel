/*
 * Copyright (C) 2021 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

#include "ttimer.h"
#include "terror.h"

TTimer::TTimer()
{
    DECL_TRACER("TTimer::TTimer()");
}

TTimer::~TTimer()
{
    DECL_TRACER("TTimer::~TTimer()");

    if (mRunning)
    {
        mStopped = true;

        while (mRunning)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TTimer::run()
{
    DECL_TRACER("TTimer::run()");

    if (mRunning || mThread.joinable() || mMsec == std::chrono::milliseconds(0))
        return;

    try
    {
        mStopped = false;
        mOnce = false;
        mThread = std::thread([=] { this->_run(); });
        mThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting the TTimer thread: " << e.what());
    }
}

void TTimer::run_once()
{
    DECL_TRACER("TTimer::run_once()");

    if (mRunning || mThread.joinable() || mMsec == std::chrono::milliseconds(0))
        return;

    try
    {
        mStopped = false;
        mOnce = true;
        mThread = std::thread([=] { this->_run(); });
        mThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting the TTimer thread: " << e.what());
    }
}

void TTimer::run_once(std::chrono::milliseconds ms)
{
    DECL_TRACER("TTimer::run_once(std::chrono::milliseconds ms)");

    if (mRunning || mThread.joinable() || ms == std::chrono::milliseconds(0))
        return;

    try
    {
        mStopped = false;
        mOnce = true;
        mMsec = ms;
        mThread = std::thread([=] { this->_run(); });
        mThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting the TTimer thread: " << e.what());
    }
}

void TTimer::_run()
{
    DECL_TRACER("TTimer::_run()");

    if (mRunning)
        return;

    mRunning = true;

    while (!mStopped && !prg_stopped)
    {
        std::this_thread::sleep_for(mMsec);

        if (mStopped)
            break;

        if (_callback)
            _callback(mCounter);

        mCounter++;

        if (mOnce)
            break;
    }

    mRunning = false;
}

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

#ifndef __TTIMER_H__
#define __TTIMER_H__

#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

#ifdef __ANDROID__
typedef unsigned long int ulong;
#endif

extern bool prg_stopped;

class TTimer
{
    public:
        TTimer();
        ~TTimer();

        void run();
        void run_once();
        void run_once(std::chrono::milliseconds ms);
        void stop() { mStopped = true; }
        void setInterval(std::chrono::milliseconds ms) { mMsec = ms; }
        void registerCallback(std::function<void(ulong)> callback) { _callback = callback; }
        bool isRunning() { return mRunning; }

    protected:
        void _run();

    private:
        std::function<void(ulong)> _callback{nullptr};
        std::chrono::milliseconds mMsec{0};
        bool mOnce{false};
        std::thread mThread;
        ulong mCounter{0};
        std::atomic<bool> mStopped{false};
        std::atomic<bool> mRunning{false};
};

#endif

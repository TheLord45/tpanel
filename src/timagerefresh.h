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

#ifndef __TIMAGEREFRESH_H__
#define __TIMAGEREFRESH_H__

#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

extern bool prg_stopped;

class TImageRefresh
{
    public:
        TImageRefresh();
        ~TImageRefresh();

        void run(const std::string& url);
        void stop() { mStopped = true; }
        void stopWait();
        bool isRunning() { return mRunning; }
        void setInterval(std::chrono::seconds s) { mSec = s; }
        void setRunOnce() { mRunOnce = true; }
        void setUsername(const std::string& user) { mUser = user; }
        void setPassword(const std::string& pw) { mPassword = pw; }
        void registerCallback(std::function<void(const std::string& url)> callback) { _callback = callback; }

    protected:
        void _run(const std::string& url);
        std::function<void(const std::string& url)> _callback{nullptr};

    private:
        std::thread mThread;
        std::chrono::seconds mSec{0};
        std::atomic<bool> mStopped{false};
        std::atomic<bool> mRunning{false};
        std::atomic<bool> mRunOnce{false};
        std::string mUser;
        std::string mPassword;
};

#endif

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

#ifndef __TSIPCLIENT_H__
#define __TSIPCLIENT_H__

#include <thread>
#include <atomic>

#include <linphone/core.h>

class TSIPClient
{
    public:
        TSIPClient();
        ~TSIPClient();

        void cleanUp();
        bool connectSIPProxy();
        bool run();
        void stop() { mRunning = false; }

        bool call(const std::string& dest); //<! Start a phone call
        bool pickup(LinphoneCall *call);    //<! Lift up if the phone is ringing
        bool terminate();                   //<! Terminate a call
        bool setOnline();
        bool setOffline();

        static void LinphoneCoreCbsRegistrationStateChanged(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
        static void LinphoneCoreCbsSubscriptionStateChanged(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state);
        static void callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg);

    protected:
        void start();
        void bell();
        void bellStop() { mBellRun = false; }

    private:
        std::thread mThread;
        std::atomic<bool> mRunning{false};
        std::atomic<bool> mThreadRun{false};
        std::atomic<bool> mBellRun{false};
        std::atomic<bool> mBellThreadRun{false};

        static LinphoneCore *mCore;
        static LinphoneProxyConfig *mProxyCfg;
        static LinphoneCall *mCall;
};

#endif

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

#define SIP_MAX_LINES   2

class TSIPClient
{
    public:
        typedef enum SIP_STATE_t
        {
            SIP_NONE,
            SIP_CONNECTED,
            SIP_DISCONNECTED,
            SIP_TRYING,
            SIP_RINGING,
            SIP_HOLD
        }SIP_STATE_t;

        TSIPClient(int id);
        ~TSIPClient();

        void cleanUp();
        bool connectSIPProxy();
        bool run();
        void stop() { mRunning = false; }
        int getLineID() { return mLine; }
        LinphoneCore *getCore() { return mCore; }
        SIP_STATE_t getSIPState() { return mSIPState; }

        bool call(const std::string& dest); //<! Start a phone call
        bool pickup(LinphoneCall *call);    //<! Lift up if the phone is ringing
        bool terminate();                   //<! Terminate a call
        bool hold();                        //<! Pause a call
        bool resume();                      //<! Resume a paused call
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
        int mLine{0};
        SIP_STATE_t mSIPState{SIP_NONE};

        LinphoneCore *mCore;
        LinphoneProxyConfig *mProxyCfg;
        LinphoneCall *mCall;
};

class TSIPPhone
{
    public:
        TSIPPhone();
        ~TSIPPhone();

        void cleanUp(int id);
        bool connectSIPProxy(int id);
        bool run(int id);
        void stop(int id);
        TSIPClient::SIP_STATE_t getSIPState(int id);

        bool call(int id, const std::string& dest); //<! Start a phone call
        bool pickup(int id);    //<! Lift up if the phone is ringing
        bool terminate(int id);                   //<! Terminate a call
        bool hold(int id);
        bool resume(int id);
        bool setOnline(int id);
        bool setOffline(int id);

        static int getId(LinphoneCore *lc);
        static TSIPClient *getClient(int id);

    private:
        static TSIPClient *mClients[SIP_MAX_LINES];
};

#endif

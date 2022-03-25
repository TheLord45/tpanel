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
            SIP_NONE,               // Undefined state (only valid on startup before initialisation)
            SIP_IDLE,               // Initialized but no event
            SIP_CONNECTED,          // Call is in progress
            SIP_DISCONNECTED,       // Call has ended
            SIP_TRYING,             // Trying to call someone
            SIP_RINGING,            // Phone is ringing (incoming call)
            SIP_HOLD,               // Active call is paused
            SIP_REJECTED,           // Outgoing call was rejected
            SIP_ERROR               // An error occured
        }SIP_STATE_t;

        TSIPClient();
        ~TSIPClient();

        void cleanUp();
        bool connectSIPProxy();
        bool run();
        void stop() { mRunning = false; }
        int getLineID() { return mLine; }
        LinphoneCore *getCore() { return mCore; }
        SIP_STATE_t getSIPState(int) { return mSIPState; }

        bool call(const std::string& dest);         //<! Start a phone call
        bool pickup(LinphoneCall *call, int id);    //<! Lift up if the phone is ringing
        bool terminate(int id);                     //<! Terminate a call
        bool hold(int id);                          //<! Pause a call
        bool resume(int id);                        //<! Resume a paused call
        bool sendDTMF(std::string& dtmf);           //<! Send a DTMF string
        bool setOnline();
        bool setOffline();

        static void LinphoneCoreCbsRegistrationStateChanged(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
        static void LinphoneCoreCbsSubscriptionStateChanged(LinphoneCore *lc, LinphoneEvent *lev, LinphoneSubscriptionState state);
        static void callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg);
        static void loggingServiceCbs(LinphoneLoggingService *, const char *domain, LinphoneLogLevel level, const char *message);

    protected:
        void start();

    private:
        LinphoneCall *getCallbyID(int id);
        int getCallId(LinphoneCall *call);
        int getNumberCalls();

        std::thread mThread;
        std::atomic<bool> mRunning{false};
        std::atomic<bool> mThreadRun{false};
        int mLine{0};
        SIP_STATE_t mSIPState{SIP_NONE};

        LinphoneCore *mCore;
        LinphoneProxyConfig *mProxyCfg;

        static TSIPClient *mMyself;
};

#endif

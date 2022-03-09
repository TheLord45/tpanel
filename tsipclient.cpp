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
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>

#include <linphone/linphonecore_utils.h>
//#include <linphone/linphone_proxy_config.h>

#include "tsipclient.h"
#include "tconfig.h"
#include "terror.h"
#include "tpagemanager.h"

using std::string;
using std::vector;
using std::to_string;

LinphoneCore *TSIPClient::mCore{nullptr};
LinphoneProxyConfig *TSIPClient::mProxyCfg{nullptr};
LinphoneCall *TSIPClient::mCall{nullptr};

TSIPClient *SIPClient{nullptr};
extern TPageManager *gPageManager;

TSIPClient::TSIPClient()
{
    DECL_TRACER("TSIPClient::TSIPClient()");

    SIPClient = this;

    if (TConfig::getSIPstatus())    // Is SIP enabled?
    {                               // Yes, try to connect to SIP proxy
        if (!connectSIPProxy())
            TError::setError();
    }
}

TSIPClient::~TSIPClient()
{
    DECL_TRACER("TSIPClient::~TSIPClient()");

    SIPClient = nullptr;
    cleanUp();
}

void TSIPClient::cleanUp()
{
    DECL_TRACER("TSIPClient::cleanUp()");

    if (mThreadRun)
    {
        mRunning = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    if (mCall)
        linphone_call_unref(mCall);

    if (mProxyCfg)
        linphone_proxy_config_unref(mProxyCfg);

    if (mCore)
        linphone_core_unref(mCore);

    mCall = nullptr;
    mProxyCfg = nullptr;
    mCore = nullptr;
}

bool TSIPClient::run()
{
    DECL_TRACER("TSIPClient::run()");

    if (!mCore || mThreadRun || !TConfig::getSIPstatus())
        return false;

    try
    {
        mRunning = true;
        mThread = std::thread([=] { this->start(); });
        mThread.detach();
    }
    catch(std::exception& e)
    {
        MSG_ERROR("Thread error: " << e.what());
        mThreadRun = false;
        mRunning = false;
        return false;
    }

    return true;
}

void TSIPClient::start()
{
    DECL_TRACER("TSIPClient::start()");

    if (!mCore || !mProxyCfg || mThreadRun)
        return;

    mThreadRun = true;

    while(mRunning && linphone_proxy_config_get_state(mProxyCfg) != LinphoneRegistrationCleared)
    {
        linphone_core_iterate(mCore);   // to make sure we receive call backs before shutting down
        long ms = 0;

        while(mBellRun && ms < 50000)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ms += 100;
        }
    }

    mRunning = false;
    mThreadRun = false;
}

void TSIPClient::bell()
{
    DECL_TRACER("TSIPClient::bell()");

    if (mBellThreadRun || !gPageManager)
        return;

    mBellThreadRun = true;
    string rtone;
    vector<string> slist = gPageManager->findSounds();

    for (size_t i = 0; i < slist.size(); ++i)
    {
        if (slist[i].find("ringtone.wav"))
        {
            rtone = slist[i];
            break;
        }
    }

    if (rtone.empty())
    {
        MSG_ERROR("No ringtone found!");
        mBellThreadRun = false;
        return;
    }

    while (mBellRun)
    {
        if (gPageManager->havePlaySound())
            gPageManager->getCallPlaySound()(rtone);
        else
            break;

        long ms = 0;

        while(mBellRun && ms < 12000)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ms += 100;
        }
    }

    mBellRun = false;
    mBellThreadRun = false;
}

bool TSIPClient::connectSIPProxy()
{
    DECL_TRACER("TSIPClient::connectSIPProxy()");

    string identity;
    LinphoneAddress *from = nullptr;
    LinphoneAuthInfo *info = nullptr;
    const char* server_addr;

    identity = "sip:" + TConfig::getSIPuser() + "@" + TConfig::getSIPproxy();

    mCore = linphone_factory_create_core_3(linphone_factory_get(), NULL, NULL, NULL);

    if (!mCore)
    {
        MSG_ERROR("Error initializing the core phone library!");
        return false;
    }

    // Register the callbacks
    LinphoneCoreCbs *cbs = linphone_factory_create_core_cbs(linphone_factory_get());
    linphone_core_cbs_set_registration_state_changed(cbs, this->LinphoneCoreCbsRegistrationStateChanged);
    linphone_core_cbs_set_subscription_state_changed(cbs, this->LinphoneCoreCbsSubscriptionStateChanged);
    linphone_core_cbs_set_call_state_changed(cbs, this->callStateChanged);
    linphone_core_add_callbacks(mCore, cbs);
    // create proxy config
    mProxyCfg = linphone_core_create_proxy_config(mCore);
    // parse identity
    from = linphone_address_new(identity.c_str());

    if (!from)
    {
        MSG_ERROR(identity << " not a valid sip uri, must be like sip:toto@sip.linphone.org");
        linphone_core_unref(mCore);
        mCore = nullptr;
        return false;
    }

    if (!TConfig::getSIPuser().empty())
    {
        char *passwd = nullptr;
        char *domain = nullptr;
        char *stun = nullptr;

        if (!TConfig::getSIPpassword().empty())
            passwd = TConfig::getSIPpassword().data();

        if (!TConfig::getSIPdomain().empty())
            domain = TConfig::getSIPdomain().data();

        if (!TConfig::getSIPstun().empty())
            stun = TConfig::getSIPstun().data();

        info = linphone_auth_info_new(linphone_address_get_username(from), NULL, passwd, NULL, stun, domain); // create authentication structure from identity
        linphone_core_add_auth_info(mCore, info);                   // add authentication info to LinphoneCore
    }

    LinphoneTransports *transport = linphone_factory_create_transports(linphone_factory_get());

    if (transport)
    {
        linphone_transports_set_udp_port(transport, TConfig::getSIPport());
        linphone_transports_set_tcp_port(transport, TConfig::getSIPport());
        linphone_core_set_transports(mCore, transport);
    }
    else
    {
        MSG_WARNING("Error creating a tranport class. Will use SIP default port 5060!");
    }

    // configure proxy entries
    linphone_proxy_config_set_identity_address(mProxyCfg, from);    // set identity with user name and domain
    server_addr = linphone_address_get_domain(from);                // extract domain address from identity
    linphone_proxy_config_set_server_addr(mProxyCfg, server_addr);  // we assume domain = proxy server address
    linphone_proxy_config_enable_register(mProxyCfg, TRUE);         // activate registration for this proxy config
    linphone_address_unref(from);                                   // release resource

    linphone_core_add_proxy_config(mCore, mProxyCfg);               // add proxy config to linphone core
    linphone_core_set_default_proxy(mCore, mProxyCfg);              // set to default proxy

    return true;
}

bool TSIPClient::call(const string& dest)
{
    DECL_TRACER("TSIPClient::call(const string& dest)");

    if (dest.empty())
        return false;

    mCall = linphone_core_invite(mCore, dest.c_str());

    if (!mCall)
    {
        MSG_ERROR("Could not place call to " << dest);
        return false;
    }
    else
        MSG_INFO("Call to " << dest << " is in progress...");

    linphone_call_ref(mCall);
    return run();
}

bool TSIPClient::pickup(LinphoneCall *call)
{
    DECL_TRACER("TSIPClient::pickup()");

    linphone_call_accept(call);
    return false;
}

bool TSIPClient::terminate()
{
    DECL_TRACER("TSIPClient::terminate()");

    if (mCall && linphone_call_get_state(mCall) != LinphoneCallEnd)
    {
        // terminate the call
        MSG_DEBUG("Terminating the call...");
        linphone_call_terminate(mCall);
        // at this stage we don't need the call object
        linphone_call_unref(mCall);
        MSG_DEBUG("Call terminated.");
    }
    else
        return false;

    return true;
}

bool TSIPClient::setOnline()
{
    DECL_TRACER("TSIPClient::setOnline()");

    if (!mCore || !TConfig::getSIPstatus())
        return false;

    LinphonePresenceModel *model;

    model = linphone_presence_model_new();
    linphone_presence_model_set_basic_status(model, LinphonePresenceBasicStatusOpen);
    linphone_core_set_presence_model(mCore, model);
    linphone_presence_model_unref(model);
    return true;
}

bool TSIPClient::setOffline()
{
    DECL_TRACER("TSIPClient::setOffline()");

    if (!mCore || !TConfig::getSIPstatus())
        return false;

    LinphonePresenceModel *model;

    model = linphone_presence_model_new();
    linphone_presence_model_set_basic_status(model, LinphonePresenceBasicStatusClosed);
    linphone_core_set_presence_model(mCore, model);
    linphone_presence_model_unref(model);
    linphone_core_iterate(mCore);                       // just to make sure new status is initiate message is issued
    return true;
}

void TSIPClient::LinphoneCoreCbsRegistrationStateChanged(LinphoneCore* lc, LinphoneProxyConfig* cfg, LinphoneRegistrationState cstate, const char* message)
{
    DECL_TRACER("TSIPClient::LinphoneCoreCbsRegistrationStateChanged(LinphoneCore* lc, LinphoneProxyConfig* cfg, LinphoneRegistrationState cstate, const char* message)");

    const LinphoneAddress *addr = linphone_proxy_config_get_identity_address(cfg);

    MSG_DEBUG("New registration state " << linphone_registration_state_to_string(cstate) << " for user id [" << linphone_address_get_username(addr) << "] at proxy [" << linphone_proxy_config_get_addr(cfg) << "]");

    if (message)
        MSG_INFO(message);
}

void TSIPClient::LinphoneCoreCbsSubscriptionStateChanged(LinphoneCore* lc, LinphoneEvent* lev, LinphoneSubscriptionState state)
{
    DECL_TRACER("TSIPClient::LinphoneCoreCbsSubscriptionStateChanged(LinphoneCore* lc, LinphoneEvent* lev, LinphoneSubscriptionState state)");
}

void TSIPClient::callStateChanged(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg)
{
    if (!SIPClient || !gPageManager)
    {
        MSG_ERROR("A callback function was illegal called.");
        return;
    }

    vector<string> cmds;

    switch(cstate)
    {
        case LinphoneCallStateIncomingReceived:
            MSG_INFO("Receiving incoming call!");
            // Ring the bell
            try
            {
                if (gPageManager->getPHNautoanswer())
                {
                    LinphoneAddress const *addr = linphone_call_get_remote_address(call);
                    const char *uname = linphone_address_get_username(addr);
                    const char *dispname = linphone_address_get_display_name(addr);
                    time_t t = time(nullptr);
                    std::tm tm = *std::localtime(&t);
                    std::ostringstream oss;
                    oss << std::put_time(&tm, "%m/%d/%Y %H:%M:%S");
                    cmds = { "INCOMING", dispname, uname, "0", oss.str() };
                    gPageManager->sendPHN(cmds);
                    SIPClient->pickup(call);
                }
                else
                {
                    std::thread thread = std::thread([=] { SIPClient->bell(); });
                    thread.detach();
                    cmds = { "CALL", "RINGING", "0" };
                    gPageManager->sendPHN(cmds);
                }
            }
            catch(std::exception& e)
            {
                MSG_ERROR("Thread error: " << e.what());
            }
        break;
        case LinphoneCallOutgoingRinging:
            MSG_DEBUG("It is now ringing remotely !");

            cmds = { "CALL", "TRYING", "0" };
            gPageManager->sendPHN(cmds);
        break;
        case LinphoneCallConnected:
            MSG_INFO("We are connected!");

            cmds = { "CALL", "CONNECTED", "0" };
            SIPClient->bellStop();
            gPageManager->sendPHN(cmds);
        break;
        case LinphoneCallStatePausing:
            MSG_DEBUG("Pausing call");
        break;
        case LinphoneCallStatePaused:
            MSG_DEBUG("Call is paused.");

            cmds = { "CALL", "HOLD", "0" };
            gPageManager->sendPHN(cmds);
        break;
        case LinphoneCallStateResuming:
            MSG_DEBUG("Resuming call");
        break;

        case LinphoneCallEnd:
            MSG_DEBUG("Call is terminated.");

            cmds = { "CALL", "DISCONNECTED", "0" };
            SIPClient->bellStop();
            gPageManager->sendPHN(cmds);
        break;

        case LinphoneCallError:
            MSG_DEBUG("Call failure!");

            if (SIPClient)
                SIPClient->bellStop();
        break;

        default:
            MSG_DEBUG("Unhandled notification " << cstate);
    }

    if (msg)
        MSG_DEBUG("Message: " << msg);
}

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

#include "tsipclient.h"
#include "tconfig.h"
#include "terror.h"
#include "tpagemanager.h"

using std::string;
using std::vector;
using std::to_string;

TSIPClient *TSIPClient::mMyself{nullptr};

extern TPageManager *gPageManager;

TSIPClient::TSIPClient()
{
    DECL_TRACER("TSIPClient::TSIPClient()");

    mMyself = this;
    mLine = 0;
    // Install a log handler
    LinphoneLoggingService *lserv = linphone_logging_service_get();
    LinphoneLoggingServiceCbs *log = linphone_factory_create_logging_service_cbs(linphone_factory_get());
    linphone_logging_service_cbs_set_log_message_written(log, this->loggingServiceCbs);
    LinphoneLogLevel ll = LinphoneLogLevelFatal;

    if (TStreamError::checkFilter(HLOG_DEBUG))
        ll = LinphoneLogLevelDebug;
    else if (TStreamError::checkFilter(HLOG_TRACE))
        ll = LinphoneLogLevelTrace;
    else if (TStreamError::checkFilter(HLOG_ERROR))
        ll = LinphoneLogLevelError;
    else if (TStreamError::checkFilter(HLOG_WARNING))
        ll = LinphoneLogLevelWarning;
    else if (TStreamError::checkFilter(HLOG_INFO))
        ll = LinphoneLogLevelMessage;

    linphone_logging_service_set_log_level(lserv, ll);
    linphone_logging_service_add_callbacks(lserv, log);
    // Start the SIP client
    if (TConfig::getSIPstatus())    // Is SIP enabled?
    {                               // Yes, try to connect to SIP proxy
        if (!connectSIPProxy())
            TError::setError();
    }
}

TSIPClient::~TSIPClient()
{
    DECL_TRACER("TSIPClient::~TSIPClient()");

    cleanUp();
    mMyself = nullptr;
}

void TSIPClient::cleanUp()
{
    DECL_TRACER("TSIPClient::cleanUp()");

    if (mThreadRun)
    {
        mRunning = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    for (int i = 0; i < SIP_MAX_LINES; i++)
    {
        LinphoneCall *call = getCallbyID(i);

        if (call)
            linphone_call_unref(call);
    }

    if (mProxyCfg)
        linphone_proxy_config_unref(mProxyCfg);

    if (mCore)
        linphone_core_unref(mCore);

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
        linphone_core_iterate(mCore);   // to make sure we receive call backs
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    mRunning = false;
    mThreadRun = false;
}

bool TSIPClient::connectSIPProxy()
{
    DECL_TRACER("TSIPClient::connectSIPProxy()");

    string identity;
    LinphoneAddress *from = nullptr;
    LinphoneAuthInfo *info = nullptr;
    const char* server_addr;

    if (TConfig::getSIPproxy().empty())
    {
        MSG_ERROR("No proxy defined!");
        return false;
    }

    if (!TConfig::getSIPnetworkIPv4() && !TConfig::getSIPnetworkIPv6())
    {
        MSG_ERROR("Both network protocolls (IPv4 and IPv6) are disabled!")
        return false;
    }

    // Create the Linphone core
    identity = "sip:" + TConfig::getSIPuser() + "@" + TConfig::getSIPproxy();
    string sipConfig = TConfig::getProjectPath() + "/sipconfig.cfg";
    string sipFactory = TConfig::getProjectPath() + "/sipfactory.cfg";
    mCore = linphone_factory_create_core_3(linphone_factory_get(), sipConfig.c_str(), sipFactory.c_str(), NULL);

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

    // Define some basic values
    linphone_core_set_max_calls(mCore, SIP_MAX_LINES);
    string ringtone = TConfig::getProjectPath() + "/__system/graphics/sounds/ringtone.wav";
    linphone_core_set_ring(mCore, ringtone.c_str());
    string ringback = TConfig::getProjectPath() + "/__system/graphics/sounds/ringback.wav";
    linphone_core_set_ringback(mCore, ringback.c_str());
    string sipCallLogs = TConfig::getProjectPath() + "/sipcalls.db";
    linphone_core_set_call_logs_database_path(mCore, sipCallLogs.c_str());

    // STUN server (if any)
    if (!TConfig::getSIPstun().empty())
        linphone_core_set_stun_server(mCore, TConfig::getSIPstun().c_str());

    // create proxy config
    mProxyCfg = linphone_core_create_proxy_config(mCore);

    if (!mProxyCfg)
    {
        MSG_ERROR("Error creating the proxy class!");
        linphone_core_unref(mCore);
        mCore = nullptr;
        return false;
    }

    // parse identity
    from = linphone_address_new(identity.c_str());

    if (!from)
    {
        MSG_ERROR(identity << " not a valid sip uri, must be like sip:toto@sip.linphone.org");
        linphone_proxy_config_unref(mProxyCfg);
        mProxyCfg = nullptr;
        linphone_core_unref(mCore);
        mCore = nullptr;
        return false;
    }

    if (!TConfig::getSIPuser().empty())
    {
        char *passwd = nullptr;
        char *domain = nullptr;
        char *proxy = nullptr;

        if (!TConfig::getSIPpassword().empty())
            passwd = (char *)TConfig::getSIPpassword().data();

        if (!TConfig::getSIPdomain().empty())
            domain = (char *)TConfig::getSIPdomain().data();

        if (!TConfig::getSIPproxy().empty())
            proxy = (char *)TConfig::getSIPproxy().data();

        info = linphone_auth_info_new(linphone_address_get_username(from), NULL, passwd, NULL, proxy, domain); // create authentication structure from identity
        linphone_core_add_auth_info(mCore, info);                   // add authentication info to LinphoneCore
    }

    LinphoneTransports *transport = linphone_factory_create_transports(linphone_factory_get());

    if (transport)
    {
        linphone_transports_set_udp_port(transport, TConfig::getSIPport());
        linphone_transports_set_tcp_port(transport, TConfig::getSIPport());

        if (TConfig::getSIPportTLS() > 0)
            linphone_transports_set_tls_port(transport, TConfig::getSIPportTLS());

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
#ifndef __ANDROID__
    linphone_core_set_default_proxy(mCore, mProxyCfg);              // set to default proxy
#else
    linphone_core_set_default_proxy_config(mCore, mProxyCfg);       // set to default proxy
#endif
    if (!TConfig::getSIPnetworkIPv6())
        linphone_core_enable_ipv6(mCore, 0);

    LinphoneNatPolicy *nat = linphone_core_create_nat_policy(mCore);

    if (nat)
    {
        switch(TConfig::getSIPfirewall())
        {
            case TConfig::SIP_STUN:
                if (!TConfig::getSIPstun().empty())
                {
                    linphone_nat_policy_set_stun_server(nat, TConfig::getSIPstun().c_str());
                    linphone_nat_policy_set_stun_server_username(nat, TConfig::getSIPuser().c_str());
                    linphone_nat_policy_enable_stun(nat, true);
                }
            break;

            case TConfig::SIP_ICE:  linphone_nat_policy_enable_ice(nat, 1); break;
            case TConfig::SIP_UPNP: linphone_nat_policy_enable_upnp(nat, 1); break;

            default:
                if (TConfig::getSIPfirewall() != TConfig::SIP_NO_FIREWALL)
                    linphone_nat_policy_enable_turn(nat, 1);
        }

        linphone_core_set_nat_policy(mCore, nat);
    }

    int lstat = linphone_core_start(mCore);

    switch(lstat)
    {
        case -1: MSG_ERROR("Global failure starting linphone core!"); return false;
        case -2: MSG_ERROR("Error connecting to linphone database!"); return false;
    }

    if (!mRunning)
        run();

    return true;
}

bool TSIPClient::call(const string& dest)
{
    DECL_TRACER("TSIPClient::call(const string& dest, int)");

    if (dest.empty() || mRegState != LinphoneRegistrationOk)
        return false;

    int numCalls = getNumberCalls();

    if (numCalls >= 2)
    {
        MSG_ERROR("There are already " << numCalls << " active!");
        return false;
    }

    LinphoneCall *call = linphone_core_invite(mCore, dest.c_str());

    if (!call)
    {
        MSG_ERROR("Could not place call to " << dest);
        return false;
    }
    else
        MSG_INFO("Call to " << dest << " is in progress...");

    linphone_call_ref(call);
    return run();
}

bool TSIPClient::pickup(LinphoneCall *call, int id)
{
    DECL_TRACER("TSIPClient::pickup(LinphoneCall *call, int)");

    if (mRegState != LinphoneRegistrationOk)
        return false;

    if (call)
        linphone_call_accept(call);
    else
    {
        LinphoneCall *cl = getCallbyID(id);

        if (cl)
            linphone_call_accept(cl);
    }

    return false;
}

bool TSIPClient::terminate(int id)
{
    DECL_TRACER("TSIPClient::terminate(int)");

    if (mRegState != LinphoneRegistrationOk)
        return false;

    LinphoneCall *call = getCallbyID(id);

    if (call && linphone_call_get_state(call) != LinphoneCallEnd)
    {
        // terminate the call
        linphone_call_terminate(call);
        MSG_DEBUG("Call terminated.");
    }
    else
        return false;

    return true;
}

bool TSIPClient::hold(int id)
{
    DECL_TRACER("TSIPClient::hold(int id)");

    if (mRegState != LinphoneRegistrationOk)
        return false;

    LinphoneCall *call = getCallbyID(id);

    if (call)
    {
        LinphoneStatus st = linphone_call_get_state(call);

        if (st == LinphoneCallPausing || st == LinphoneCallPaused)
            return true;

        st = linphone_call_pause(call);

        if (st == LinphoneCallPausing || st == LinphoneCallPaused)
            return true;
    }

    return false;
}

bool TSIPClient::resume(int id)
{
    DECL_TRACER("TSIPClient::resume(int)");

    if (mRegState != LinphoneRegistrationOk)
        return false;

    LinphoneCall *call = getCallbyID(id);

    if (call)
    {
        LinphoneStatus st = linphone_call_get_state(call);

        if (st != LinphoneCallPausing && st != LinphoneCallPaused)
            return true;

        st = linphone_call_resume(call);

        if (st == LinphoneCallPausing || st == LinphoneCallPaused)
            return true;
    }

    return false;
}

bool TSIPClient::sendDTMF(string& dtmf)
{
    DECL_TRACER("TSIPClient::sendDTMF(string& dtmf, int id)");

    if (!mCore || mRegState != LinphoneRegistrationOk)
        return false;

    LinphoneCall *call = linphone_core_get_current_call(mCore);

    if (!call)
        return false;

    int lstate = linphone_call_send_dtmfs(call, dtmf.c_str());

    switch(lstate)
    {
        case -1: MSG_ERROR("No call or call is not ready!"); return false;
        case -2: MSG_ERROR("A previous DTMF sequence is still in progress!"); return false;
    }

    return true;
}

bool TSIPClient::sendLinestate()
{
    DECL_TRACER("TSIPClient::sendLinestate()");

    vector<string> cmds;
    cmds.push_back("LINESTATE");

    if (getNumberCalls() == 0)
    {
        cmds.push_back("0");
        cmds.push_back("IDLE");
        cmds.push_back("1");
        cmds.push_back("IDLE");
        gPageManager->sendPHN(cmds);
        return true;
    }

    for (int id = 0; id < SIP_MAX_LINES; id++)
    {
        LinphoneCall *call = getCallbyID(id);

        if (call)
        {
            LinphoneCallState cs = linphone_call_get_state(call);

            switch(cs)
            {
                case LinphoneCallStateIdle:
                case LinphoneCallStateError:
                    cmds.push_back(to_string(id));
                    cmds.push_back("IDLE");
                break;

                case LinphoneCallStateConnected:
                case LinphoneCallStateResuming:
                    cmds.push_back(to_string(id));
                    cmds.push_back("CONNECTED");
                break;

                case LinphoneCallStatePausing:
                case LinphoneCallStatePaused:
                case LinphoneCallStatePausedByRemote:
                    cmds.push_back(to_string(id));
                    cmds.push_back("HOLD");
                break;

                default:
                    cmds.push_back(to_string(id));
                    cmds.push_back("IDLE");
            }
        }
    }

    gPageManager->sendPHN(cmds);
    return true;
}

bool TSIPClient::sendPrivate(bool state)
{
    DECL_TRACER("TSIPClient::sendPrivate(bool state)");

    if (state)
        setOffline();
    else
        setOnline();

    vector<string> cmds;
    cmds.push_back("PRIVACY");

    if (state)
        cmds.push_back("1");
    else
        cmds.push_back("0");

    gPageManager->sendPHN(cmds);
    return true;
}

bool TSIPClient::redial()
{
    DECL_TRACER("TSIPClient::redial()");

    // FIXME: Enter code to dial the last number
//    return false;
    const bctbx_list_t *clist = linphone_core_get_call_logs(mCore);
    bctbx_list_t *p = (bctbx_list_t *)clist;
    LinphoneCallLog *plast = nullptr, *cl = nullptr;

    if (!clist)
    {
        MSG_INFO("There was no previous dialed number!");
        return false;
    }

    while (p)
    {
        cl = (LinphoneCallLog *)p;
        LinphoneCallStatus cs = linphone_call_log_get_status(cl);

        if (cs == LinphoneCallSuccess)
            plast = cl;

        p = p->next;
    }

    // Here we are on the last entry in the logfile, if any.
    if (!plast)
    {
        MSG_INFO("The was no successfull call found!");
        return false;
    }

    const LinphoneAddress *addr = linphone_call_log_get_to_address(plast);

    if (!addr)
    {
        MSG_ERROR("No target address in log entry found!");
        return false;
    }

    char *uri = linphone_address_as_string(addr);

    if (uri)
    {
        MSG_DEBUG("Redialing: " << uri);
        return call(uri);
    }

    return false;
}

bool TSIPClient::transfer(int id, const string& num)
{
    DECL_TRACER("TSIPClient::transfer(int id, const string& num)");

    LinphoneCall *call = getCallbyID(id);

    if (!call)
        return false;

    int state = linphone_call_transfer(call, num.c_str());

    if (state != 0)
    {
        MSG_ERROR("Transfer to " << num << " failed!");
        return false;
    }

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
    return true;
}

LinphoneCall *TSIPClient::getCallbyID(int id)
{
    DECL_TRACER("TSIPClient::getCallbyID(int id)");

    if (!mCore)
        return nullptr;

    const bctbx_list_t *clist = linphone_core_get_calls(mCore);
    bctbx_list_t *pos = (bctbx_list_t *)clist;
    int idx = 0;

    if (!clist)
        return nullptr;

    while(pos)
    {
        if (idx == id && pos->data)
        {
            LinphoneCall *call = (LinphoneCall *)pos->data;
            return call;
        }

        pos = pos->next;
        idx++;
    }

    return nullptr;
}

int TSIPClient::getCallId(LinphoneCall* call)
{
    DECL_TRACER("TSIPClient::getCallId(LinphoneCall* call)");

    int id = 0;

    if (!mCore || !call)
        return 0;

    const bctbx_list_t *clist = linphone_core_get_calls(mCore);
    bctbx_list_t *pos = (bctbx_list_t *)clist;

    if (!clist)
        return 0;

    while(pos)
    {
        if (pos->data && pos->data == call)
            return id;

        pos = pos->next;
        id++;
    }

    return 0;
}

int TSIPClient::getNumberCalls()
{
    DECL_TRACER("TSIPClient::getNumberCalls()");

    if (!mCore)
        return 0;

    const bctbx_list_t *clist = linphone_core_get_calls(mCore);
    bctbx_list_t *pos = (bctbx_list_t *)clist;
    int idx = 0;

    if (!clist)
        return 0;

    while(pos)
    {
        idx++;
        pos = pos->next;
    }

    return idx;
}

void TSIPClient::LinphoneCoreCbsRegistrationStateChanged(LinphoneCore* /*lc*/, LinphoneProxyConfig* cfg, LinphoneRegistrationState cstate, const char* message)
{
    DECL_TRACER("TSIPClient::LinphoneCoreCbsRegistrationStateChanged(LinphoneCore* lc, LinphoneProxyConfig* cfg, LinphoneRegistrationState cstate, const char* message)");

    mMyself->setRegState(cstate);
    const LinphoneAddress *addr = linphone_proxy_config_get_identity_address(cfg);

    MSG_DEBUG("New registration state " << linphone_registration_state_to_string(cstate) << " for user id [" << linphone_address_get_username(addr) << "] at proxy [" << linphone_proxy_config_get_addr(cfg) << "]");

    if (message)
        MSG_INFO(message);
}

void TSIPClient::LinphoneCoreCbsSubscriptionStateChanged(LinphoneCore* /*lc*/, LinphoneEvent* /*lev*/, LinphoneSubscriptionState /*state*/)
{
    DECL_TRACER("TSIPClient::LinphoneCoreCbsSubscriptionStateChanged(LinphoneCore* lc, LinphoneEvent* lev, LinphoneSubscriptionState state)");
}

void TSIPClient::callStateChanged(LinphoneCore * /*lc*/, LinphoneCall *call, LinphoneCallState cstate, const char *msg)
{
    if (!gPageManager)
    {
        MSG_ERROR("A callback function was illegal called.");
        return;
    }

    int id = mMyself->getCallId(call);

    if (!mMyself)
    {
        MSG_ERROR("Called for illegal ID " << id << "!");
        return;
    }

    vector<string> cmds;

    switch(cstate)
    {
        case LinphoneCallStateIdle:
            mMyself->mSIPState = SIP_NONE;
        break;

        case LinphoneCallStateIncomingReceived:
            MSG_INFO("Receiving incoming call!");
            mMyself->mSIPState = SIP_RINGING;
            // Ring the bell
            if (gPageManager->getPHNautoanswer())
            {
                LinphoneAddress const *addr = linphone_call_get_remote_address(call);
                const char *uname = linphone_address_get_username(addr);
                const char *dispname = linphone_address_get_display_name(addr);
                time_t t = time(nullptr);
                std::tm tm = *std::localtime(&t);
                std::ostringstream oss;
                oss << std::put_time(&tm, "%m/%d/%Y %H:%M:%S");
                cmds = { "INCOMING", dispname, uname, to_string(id), oss.str() };
                gPageManager->sendPHN(cmds);
                mMyself->pickup(call, id);
            }
            else
            {
                int n = mMyself->getNumberCalls();

                if (n < SIP_MAX_LINES)
                    n++;

                cmds = { "CALL", "RINGING", to_string(n) };
                gPageManager->sendPHN(cmds);
            }
        break;

        case LinphoneCallStateOutgoingInit:
        case LinphoneCallStateOutgoingProgress:
        case LinphoneCallOutgoingRinging:
            MSG_DEBUG("It is now ringing remotely !");

            mMyself->mSIPState = SIP_TRYING;
            cmds = { "CALL", "TRYING", to_string(id) };
            gPageManager->sendPHN(cmds);
        break;

        case LinphoneCallConnected:
            MSG_INFO("We are connected!");

            mMyself->mSIPState = SIP_CONNECTED;
            cmds = { "CALL", "CONNECTED", to_string(id) };
            gPageManager->sendPHN(cmds);
        break;
        case LinphoneCallStatePausing:
            MSG_DEBUG("Pausing call");
        break;
        case LinphoneCallStatePaused:
            MSG_DEBUG("Call is paused.");

            mMyself->mSIPState = SIP_HOLD;
            cmds = { "CALL", "HOLD", to_string(id) };
            gPageManager->sendPHN(cmds);
        break;
        case LinphoneCallStateResuming:
            MSG_DEBUG("Resuming call");

            mMyself->mSIPState = SIP_CONNECTED;
            cmds = { "CALL", "CONNECTED", to_string(id) };
            gPageManager->sendPHN(cmds);
        break;

        case LinphoneCallEnd:
            MSG_DEBUG("Call is terminated.");

            mMyself->mSIPState = SIP_DISCONNECTED;
            cmds = { "CALL", "DISCONNECTED", to_string(id) };
            gPageManager->sendPHN(cmds);
        break;

        case LinphoneCallError:
        {
            const LinphoneErrorInfo *einfo = linphone_call_get_error_info(call);
            const char *phrase = linphone_error_info_get_phrase(einfo);

            if (phrase)
            {
                MSG_DEBUG("Call failure! (" << phrase << ")");
            }

            mMyself->mSIPState = SIP_NONE;
            cmds = { "CALL", "IDLE", to_string(id) };
            gPageManager->sendPHN(cmds);
        }
        break;

        default:
            MSG_DEBUG("Unhandled notification " << cstate);
    }

    if (msg)
        MSG_DEBUG("Message: " << msg);
}

void TSIPClient::loggingServiceCbs(LinphoneLoggingService *, const char *domain, LinphoneLogLevel level, const char *message)
{
    //    DECL_TRACER("TSIPClient::loggingServiceCbs(LinphoneLoggingService *, const char *domain, LinphoneLogLevel level, const char *message)");

    if (level == LinphoneLogLevelDebug)
        MSG_DEBUG(domain << ": " << message);

    if (level == LinphoneLogLevelTrace)
        MSG_TRACE(domain << ": " << message);

    if (level == LinphoneLogLevelMessage)
        MSG_INFO(domain << ": " << message);

    if (level == LinphoneLogLevelWarning)
        MSG_WARNING(domain << ": " << message);

    if (level == LinphoneLogLevelError)
        MSG_ERROR(domain << ": " << message);

    if (level == LinphoneLogLevelFatal)
        MSG_ERROR(domain << ": FATAL ERROR: " << message);
}


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
#include <thread>

#ifdef __ANDROID__
#include <jni.h>
#endif
#include "tsipclient.h"
#include "tconfig.h"
#include "terror.h"
#include "tpagemanager.h"

#ifndef _NOSIP_

static pj_thread_desc _desc_;
static pj_thread_t *_thread_;

#define REGISTER_THREAD() \
if(!pj_thread_is_registered()) {\
    pj_thread_register(NULL, _desc_, &_thread_);\
}

using std::string;
using std::vector;
using std::to_string;
using std::atomic;

TSIPClient *TSIPClient::mMyself{nullptr};
TSIPClient::pjsua_app_config TSIPClient::mAppConfig;
pjsua_call_id TSIPClient::mCurrentCall{PJSUA_INVALID_ID};
atomic<bool> TSIPClient::mRefreshRun{false};

// The module instance.
pjsip_module TSIPClient::mod_default_handler =
{
    NULL, NULL,                             // prev, next.
    { (char *)"mod-default-handler", 19 },  // Name.
    -1,                                     // Id
    PJSIP_MOD_PRIORITY_APPLICATION+99,      // Priority
    NULL,                                   // load()
    NULL,                                   // start()
    NULL,                                   // stop()
    NULL,                                   // unload()
    &default_mod_on_rx_request,             // on_rx_request()
    NULL,                                   // on_rx_response()
    NULL,                                   // on_tx_request.
    NULL,                                   // on_tx_response()
    NULL,                                   // on_tsx_state()
};

extern TPageManager *gPageManager;


TSIPClient::TSIPClient()
{
    DECL_TRACER("TSIPClient::TSIPClient()");

    if (mMyself)
        return;

    mMyself = this;
    mLine = 0;

    for (int i = 0; i < SIP_MAX_LINES; i++)
        mCallIDs[i] = PJSUA_INVALID_ID;

    // Start the SIP client
    if (TConfig::getSIPstatus())    // Is SIP enabled?
    {                               // Yes, try to connect to SIP proxy
        if (!init())
            TError::setError();
    }
}

TSIPClient::~TSIPClient()
{
    DECL_TRACER("TSIPClient::~TSIPClient()");

    if (mMyself != this)
        return;

    cleanUp();
    mMyself = nullptr;
}

bool TSIPClient::init()
{
    DECL_TRACER("TSIPClient::init()");

    if (mRegistered)
        return true;

    pj_status_t status;
    string sipProxy = TConfig::getSIPproxy();
    string sipDomain = TConfig::getSIPdomain();

    if (!sipProxy.empty() && sipDomain.empty())
        sipDomain = sipProxy;
    else if (sipProxy.empty() && !sipDomain.empty())
        sipProxy = sipDomain;
    else if (sipProxy.empty() && sipDomain.empty())
    {
        MSG_ERROR("No proxy server and no domain given!");
        return false;
    }

    // Initialize the library
    if (pj_init() != PJ_SUCCESS)
    {
        MSG_ERROR("Initialisation of PJSIP failed!");
        return false;
    }

    // Define the callback for logging
    pj_log_init();
    pj_log_set_log_func(&_log_call);
    pj_log_set_decor(PJ_LOG_HAS_SENDER);
    pj_log_set_level(4);

    /* Create pjsua first! */
    if (pjsua_create() != PJ_SUCCESS)
    {
        MSG_ERROR("Error in pjsua_create()");
        return false;
    }

    mAppConfig.pool = pjsua_pool_create("TPanel", 1000, 1000);
    pj_pool_t *tmp_pool = pjsua_pool_create("tmp-TPanel", 1000, 1000);

    // Global configuration
    string idUri = "sip:" + TConfig::getSIPuser() + "@" + sipProxy;
    string regUri = "sip:" + sipProxy + ":" + to_string(TConfig::getSIPport());
    mAppConfig.cfg.outbound_proxy_cnt = 1;
    mAppConfig.cfg.outbound_proxy[0] = pj_str((char *)regUri.c_str());
    mAppConfig.cfg.max_calls = SIP_MAX_LINES;

    if (!TConfig::getSIPstun().empty())
    {
        mAppConfig.cfg.stun_srv_cnt = 1;
        mAppConfig.cfg.stun_srv[0] = pj_str((char *)TConfig::getSIPstun().c_str());

        if (TConfig::getSIPnetworkIPv6())
            mAppConfig.cfg.stun_try_ipv6 = PJ_TRUE;
    }

    string uAgent = string("TPanel v") + VERSION_STRING();
    mAppConfig.cfg.user_agent = pj_str((char *)uAgent.c_str());

    // Define main account
    mAppConfig.acc_cnt = 1;
    pjsua_acc_config_default(&mAppConfig.acc_cfg[0]);
    mAppConfig.acc_cfg[0].id = pj_str((char *)idUri.c_str());
    mAppConfig.acc_cfg[0].reg_uri = pj_str((char *)regUri.c_str());
    mAppConfig.acc_cfg[0].proxy_cnt = 1;
    mAppConfig.acc_cfg[0].proxy[0] = pj_str((char *)regUri.c_str());
    mAppConfig.acc_cfg[0].cred_count = 1;
    mAppConfig.acc_cfg[0].cred_info[0].scheme = pjsip_DIGEST_STR;
    mAppConfig.acc_cfg[0].cred_info[0].realm = pj_str((char *)"*");
    mAppConfig.acc_cfg[0].cred_info[0].username = pj_str((char *)TConfig::getSIPuser().c_str());
    mAppConfig.acc_cfg[0].cred_info[0].data_type = 0;
    mAppConfig.acc_cfg[0].cred_info[0].data = pj_str((char *)TConfig::getSIPpassword().c_str());

    /* Init pjsua */
    pjsua_logging_config log_cfg;

    pjsua_config_default(&mAppConfig.cfg);
    mAppConfig.cfg.cb.on_incoming_call = &on_incoming_call;
    mAppConfig.cfg.cb.on_call_media_state = &on_call_media_state;
    mAppConfig.cfg.cb.on_call_state = &on_call_state;
    mAppConfig.cfg.cb.on_dtmf_digit2 = &call_on_dtmf_callback2;
    mAppConfig.cfg.cb.on_call_redirected = &call_on_redirected;
    mAppConfig.cfg.cb.on_call_transfer_status = &on_call_transfer_status;
    mAppConfig.cfg.cb.on_transport_state = &on_transport_state;
    mAppConfig.cfg.cb.on_ip_change_progress = &on_ip_change_progress;

    pjsua_logging_config_default(&log_cfg);
    log_cfg.console_level = 4;
    log_cfg.level = 5;
    log_cfg.decor = (PJ_LOG_HAS_SENDER);
    log_cfg.cb = &_log_call;

    pjsua_media_config_default(&mAppConfig.media_cfg);

    if (mAppConfig.capture_lat > 0)
        mAppConfig.media_cfg.snd_rec_latency = mAppConfig.capture_lat;

    if (mAppConfig.playback_lat)
        mAppConfig.media_cfg.snd_play_latency = mAppConfig.playback_lat;

    if (TConfig::getSIPfirewall() == TConfig::SIP_ICE)
        mAppConfig.media_cfg.enable_ice = true;

    if (!TConfig::getSIPstun().empty())
        mAppConfig.media_cfg.turn_server = pj_str((char *)TConfig::getSIPstun().c_str());

    if (pjsua_init(&mAppConfig.cfg, &log_cfg, &mAppConfig.media_cfg) != PJ_SUCCESS)
    {
        MSG_ERROR("Error in pjsua_init()");
        pj_pool_release(tmp_pool);
        pjsua_destroy();
        return false;
    }

    if (pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &mod_default_handler) != PJ_SUCCESS)
    {
        MSG_ERROR("Error registrating module handler!");
        pjsua_destroy();
        return false;
    }

    // Initialize calls data
    for (size_t i = 0; i < PJ_ARRAY_SIZE(mAppConfig.call_data); ++i)
    {
        mAppConfig.call_data[i].timer.id = PJSUA_INVALID_ID;
        mAppConfig.call_data[i].timer.cb = &call_timeout_callback;
    }

    /* Optionally registers WAV file */
    for (uint_t i = 0; i < mAppConfig.wav_count; ++i)
    {
        pjsua_player_id wav_id;
        uint_t play_options = 0;

        if (mAppConfig.auto_play_hangup)
            play_options |= PJMEDIA_FILE_NO_LOOP;

        status = pjsua_player_create(&mAppConfig.wav_files[i], play_options, &wav_id);

        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Error creating a player!");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        if (mAppConfig.wav_id == PJSUA_INVALID_ID)
        {
            mAppConfig.wav_id = wav_id;
            mAppConfig.wav_port = pjsua_player_get_conf_port(mAppConfig.wav_id);

            if (mAppConfig.auto_play_hangup)
            {
                pjmedia_port *port;

                pjsua_player_get_port(mAppConfig.wav_id, &port);

                if (pjmedia_wav_player_set_eof_cb2(port, NULL, &on_playfile_done) != PJ_SUCCESS)
                {
                    MSG_ERROR("Error setting callback function for player!");
                    pj_pool_release(tmp_pool);
                    pjsua_destroy();
                    return false;
                }

                pj_timer_entry_init(&mAppConfig.auto_hangup_timer, 0, NULL, &hangup_timeout_callback);
            }
        }
    }

    /* Register tone players */
    for (uint_t i = 0; i < mAppConfig.tone_count; ++i)
    {
        pjmedia_port *tport;
        char name[80];
        pj_str_t label;
        pj_status_t status2;

        pj_ansi_snprintf(name, sizeof(name), "tone-%d,%d",
                         mAppConfig.tones[i].freq1,
                         mAppConfig.tones[i].freq2);
        label = pj_str(name);
        status2 = pjmedia_tonegen_create2(mAppConfig.pool, &label,
                                          8000, 1, 160, 16,
                                          PJMEDIA_TONEGEN_LOOP,  &tport);
        if (status2 != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to create tone generator! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        status2 = pjsua_conf_add_port(mAppConfig.pool, tport, &mAppConfig.tone_slots[i]);
        pj_assert(status2 == PJ_SUCCESS);

        status2 = pjmedia_tonegen_play(tport, 1, &mAppConfig.tones[i], 0);
        pj_assert(status2 == PJ_SUCCESS);
    }

    /* Create ringback tones */
    if (mAppConfig.no_tones == PJ_FALSE)
    {
        uint_t samples_per_frame;
        pjmedia_tone_desc tone[RING_CNT+RINGBACK_CNT];
        pj_str_t name;

        samples_per_frame = mAppConfig.media_cfg.audio_frame_ptime *
        mAppConfig.media_cfg.clock_rate *
        mAppConfig.media_cfg.channel_count / 1000;

        /* Ringback tone (call is ringing) */
        name = pj_str((char *)"ringback");
        status = pjmedia_tonegen_create2(mAppConfig.pool, &name,
                                         mAppConfig.media_cfg.clock_rate,
                                         mAppConfig.media_cfg.channel_count,
                                         samples_per_frame,
                                         16, PJMEDIA_TONEGEN_LOOP,
                                         &mAppConfig.ringback_port);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to create tone generator 2! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        pj_bzero(&tone, sizeof(tone));

        for (int i = 0; i < RINGBACK_CNT; ++i)
        {
            tone[i].freq1 = RINGBACK_FREQ1;
            tone[i].freq2 = RINGBACK_FREQ2;
            tone[i].on_msec = RINGBACK_ON;
            tone[i].off_msec = RINGBACK_OFF;
        }

        tone[RINGBACK_CNT-1].off_msec = RINGBACK_INTERVAL;

        pjmedia_tonegen_play(mAppConfig.ringback_port, RINGBACK_CNT, tone,
                             PJMEDIA_TONEGEN_LOOP);


        status = pjsua_conf_add_port(mAppConfig.pool, mAppConfig.ringback_port,
                                     &mAppConfig.ringback_slot);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to add a port to tone generator! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        /* Ring (to alert incoming call) */
        name = pj_str((char *)"ring");
        status = pjmedia_tonegen_create2(mAppConfig.pool, &name,
                                         mAppConfig.media_cfg.clock_rate,
                                         mAppConfig.media_cfg.channel_count,
                                         samples_per_frame,
                                         16, PJMEDIA_TONEGEN_LOOP,
                                         &mAppConfig.ring_port);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to create tone generator 2! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        for (int i = 0; i < RING_CNT; ++i)
        {
            tone[i].freq1 = RING_FREQ1;
            tone[i].freq2 = RING_FREQ2;
            tone[i].on_msec = RING_ON;
            tone[i].off_msec = RING_OFF;
        }

        tone[RING_CNT-1].off_msec = RING_INTERVAL;

        pjmedia_tonegen_play(mAppConfig.ring_port, RING_CNT, tone, PJMEDIA_TONEGEN_LOOP);

        status = pjsua_conf_add_port(mAppConfig.pool, mAppConfig.ring_port,
                                     &mAppConfig.ring_slot);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to add a port to tone generator! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }
    }

    /* Add UDP transport. */
    pjsua_transport_id transport_id = -1;
    pjsua_transport_config tcp_cfg;

    if (!mAppConfig.no_udp && TConfig::getSIPnetworkIPv4())
    {
        pjsua_acc_id aid;
        pjsip_transport_type_e type = PJSIP_TRANSPORT_UDP;

        status = pjsua_transport_create(type,
                                        &mAppConfig.udp_cfg,
                                        &transport_id);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to create transport! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        /* Add local account */
        pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);
        /* Adjust local account config based on pjsua app config */
        {
            pjsua_acc_config acc_cfg;
            pjsua_acc_get_config(aid, tmp_pool, &acc_cfg);

            acc_cfg.rtp_cfg = mAppConfig.rtp_cfg;
            pjsua_acc_modify(aid, &acc_cfg);
        }

        pjsua_acc_set_online_status(current_acc, PJ_TRUE);
        // Find a free port to listen on
        pjsua_transport_info ti;
        pj_sockaddr_in *a;

        pjsua_transport_get_info(transport_id, &ti);
        ti.local_addr.addr.sa_family = PJ_AF_INET;
        a = (pj_sockaddr_in*)&ti.local_addr;
        tcp_cfg.port = pj_ntohs(a->sin_port);
    }

    if (!mAppConfig.no_udp && TConfig::getSIPnetworkIPv6())
    {
        pjsua_acc_id aid;
        pjsip_transport_type_e type = PJSIP_TRANSPORT_UDP6;
        pjsua_transport_config udp_cfg;

        udp_cfg = mAppConfig.udp_cfg;

        if (udp_cfg.port == 0)
            udp_cfg.port = 5060;
        else
            udp_cfg.port += 10;

        status = pjsua_transport_create(type,
                                        &udp_cfg,
                                        &transport_id);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to create IPv6 transport! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        /* Add local account */
        pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);
        /* Adjust local account config based on pjsua app config */
        {
            pjsua_acc_config acc_cfg;
            pjsua_acc_get_config(aid, tmp_pool, &acc_cfg);

            acc_cfg.rtp_cfg = mAppConfig.rtp_cfg;
            acc_cfg.ipv6_media_use = PJSUA_IPV6_ENABLED;
            pjsua_acc_modify(aid, &acc_cfg);
        }

        //pjsua_acc_set_transport(aid, transport_id);
        pjsua_acc_set_online_status(current_acc, PJ_TRUE);

        if (mAppConfig.udp_cfg.port == 0)
        {
            pjsua_transport_info ti;

            pjsua_transport_get_info(transport_id, &ti);
            tcp_cfg.port = pj_sockaddr_get_port(&ti.local_addr);
        }
    }

    /* Add TCP transport unless it's disabled */
    if (!mAppConfig.no_tcp && TConfig::getSIPnetworkIPv4())
    {
        pjsua_acc_id aid;

        status = pjsua_transport_create(PJSIP_TRANSPORT_TCP,
                                        &tcp_cfg,
                                        &transport_id);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to create TCP transport! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        /* Add local account */
        pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);

        /* Adjust local account config based on pjsua app config */
        {
            pjsua_acc_config acc_cfg;
            pjsua_acc_get_config(aid, tmp_pool, &acc_cfg);

            acc_cfg.rtp_cfg = mAppConfig.rtp_cfg;
            pjsua_acc_modify(aid, &acc_cfg);
        }

        pjsua_acc_set_online_status(current_acc, PJ_TRUE);
    }

    /* Add TCP IPv6 transport unless it's disabled. */
    if (!mAppConfig.no_tcp && TConfig::getSIPnetworkIPv6())
    {
        pjsua_acc_id aid;
        pjsip_transport_type_e type = PJSIP_TRANSPORT_TCP6;

        tcp_cfg.port += 10;

        status = pjsua_transport_create(type,
                                        &tcp_cfg,
                                        &transport_id);
        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to create TCP IPv6 transport! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        /* Add local account */
        pjsua_acc_add_local(transport_id, PJ_TRUE, &aid);

        /* Adjust local account config based on pjsua app config */
        {
            pjsua_acc_config acc_cfg;
            pjsua_acc_get_config(aid, tmp_pool, &acc_cfg);

            acc_cfg.rtp_cfg = mAppConfig.rtp_cfg;
            acc_cfg.ipv6_media_use = PJSUA_IPV6_ENABLED;
            pjsua_acc_modify(aid, &acc_cfg);
        }

        //pjsua_acc_set_transport(aid, transport_id);
        pjsua_acc_set_online_status(current_acc, PJ_TRUE);
    }

    if (transport_id == -1)
    {
        MSG_ERROR("Transport couldn't be configured!");
        pj_pool_release(tmp_pool);
        pjsua_destroy();
        return false;
    }

    /* Add accounts */
    for (uint_t i = 0; i < mAppConfig.acc_cnt; ++i)
    {
        mAppConfig.acc_cfg[i].rtp_cfg = mAppConfig.rtp_cfg;
        mAppConfig.acc_cfg[i].reg_retry_interval = 300;
        mAppConfig.acc_cfg[i].reg_first_retry_interval = 60;

        status = pjsua_acc_add(&mAppConfig.acc_cfg[i], PJ_TRUE, NULL);

        if (status != PJ_SUCCESS)
        {
            MSG_ERROR("Unable to add an account! (" << status << ")");
            pj_pool_release(tmp_pool);
            pjsua_destroy();
            return false;
        }

        pjsua_acc_set_online_status(current_acc, PJ_TRUE);
    }

    /* Init call setting */
    pjsua_call_setting call_opt;
    pjsua_call_setting_default(&call_opt);
    call_opt.aud_cnt = mAppConfig.aud_cnt;
    call_opt.vid_cnt = 0;

#if defined(PJSIP_HAS_TLS_TRANSPORT) && PJSIP_HAS_TLS_TRANSPORT!=0
    /* Wipe out TLS key settings in transport configs */
    pjsip_tls_setting_wipe_keys(&mAppConfig.udp_cfg.tls_setting);
#endif

    pj_pool_release(tmp_pool);

    /* Initialization is done, now start pjsua */
    status = pjsua_start();

    if (status != PJ_SUCCESS)
    {
        MSG_ERROR("Error starting pjsua");
        pjsua_destroy();
        return false;
    }

    mRegistered = true;
    return true;
}

void TSIPClient::cleanUp()
{
    DECL_TRACER("TSIPClient::cleanUp()");

    pjsua_destroy();
    mAccountID = 0;

    for (int i = 0; i < SIP_MAX_LINES; i++)
        mCallIDs[i] = 0;

    mRegistered = false;
}

#ifdef __ANDROID__X
JNIEnv* env{nullptr};

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*)
{
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
#endif

#ifdef __ANDROID__X
void* TSIPClient::getJNIContext()
{
    DECL_TRACER("TSIPClient::getJNIContext()");

    if (!env)
    {
        MSG_ERROR("Environment is not initialized!");
        return nullptr;
    }

    jclass activityThreadCls = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(activityThreadCls,
                                                             "currentActivityThread",
                                                             "()Landroid/app/ActivityThread;");
    jobject activityThreadObj =
            env->CallStaticObjectMethod(activityThreadCls, currentActivityThread);

    jmethodID getApplication =
            env->GetMethodID(activityThreadCls, "getApplication", "()Landroid/app/Application;");
    jobject context = env->CallObjectMethod(activityThreadObj, getApplication);
    return context;
}
#endif

bool TSIPClient::call(const string& dest)
{
    DECL_TRACER("TSIPClient::call(const string& dest, int)");

    if (dest.empty())
    {
        MSG_ERROR("No destination defined!");
        return false;
    }

    if (!mRegistered)
    {
        MSG_ERROR("Phone core is not registered!");
        return false;
    }

    int numCalls = getNumberCalls();

    if (numCalls >= 2)
    {
        MSG_ERROR("There are already " << numCalls << " active!");
        return false;
    }

    string sUri;

    if (dest.find("sip:") == string::npos)
        sUri = "sip:";

    sUri += dest;
    pj_str_t uri = pj_str((char *)sUri.c_str());
    REGISTER_THREAD();
    pjsua_call_id cid = PJSUA_INVALID_ID;
    pjsua_call_setting call_opt;

    pjsua_call_setting_default(&call_opt);
    call_opt.vid_cnt = 0;
    mAccountID = pjsua_acc_get_default();

    if (pjsua_call_make_call(mAccountID, &uri, &call_opt, NULL, NULL, &cid) != PJ_SUCCESS)
    {
        MSG_ERROR("Error calling " << dest << "!");
        return false;
    }

    for (int i = 0; i < SIP_MAX_LINES; i++)
    {
        if (mCallIDs[i] == PJSUA_INVALID_ID)
        {
            mCallIDs[i] = cid;
            mLine = i;
            break;
        }
    }

    sendConnectionStatus(SIP_TRYING, mLine);
    return true;
}

bool TSIPClient::pickup(pjsua_call_id call)
{
    DECL_TRACER("TSIPClient::pickup(LinphoneCall *call, int)");

    pjsua_call_info ci;

    REGISTER_THREAD();
    pjsua_call_get_info(call, &ci);

    if (ci.remote_info.slen > 0)
    {
        MSG_DEBUG("Incoming call from " << ci.remote_info.ptr);
    }
    else
    {
        MSG_DEBUG("Incoming call (" << ci.id << ")");
    }

    if (pjsua_call_answer(call, 200, NULL, NULL) != PJ_SUCCESS)
    {
        MSG_ERROR("Couldn't answer with call ID " << call);
        return false;
    }

    return true;
}

bool TSIPClient::terminate(int id)
{
    DECL_TRACER("TSIPClient::terminate(int)");

    if (mLine > 0)
        mLine--;

    pjsua_call_id cid = 0;

    if (id < SIP_MAX_LINES)
        cid = mCallIDs[id];
    else
        return false;

    REGISTER_THREAD();
    // FIXME: Enter code to terminate a call
    if (pjsua_call_hangup(cid, 200, NULL, NULL) != PJ_SUCCESS)
    {
        MSG_ERROR("The call " << id << " can't be ended successfull!");
        return false;
    }

    return true;
}

bool TSIPClient::hold(int id)
{
    DECL_TRACER("TSIPClient::hold(int id)");

    pjsua_call_id cid = 0;

    if (id < SIP_MAX_LINES)
        cid = mCallIDs[id];
    else
        return false;

    REGISTER_THREAD();

    if (pjsua_call_set_hold(cid, NULL) != PJ_SUCCESS)
    {
        MSG_ERROR("Error setting line " << id << " on hold!");
        return false;
    }

    return false;
}

bool TSIPClient::resume(int id)
{
    DECL_TRACER("TSIPClient::resume(int)");

    // FIXME: Enter code to resume a call
    REGISTER_THREAD();
    return false;
}

bool TSIPClient::sendDTMF(string& dtmf)
{
    DECL_TRACER("TSIPClient::sendDTMF(string& dtmf, int id)");

    // FIXME: Enter code to sen DTMF signals
    REGISTER_THREAD();
    return false;
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
/*
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
*/
    return true;
}

bool TSIPClient::sendPrivate(bool state)
{
    DECL_TRACER("TSIPClient::sendPrivate(bool state)");

    // FIXME: Enter code to set privacy
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

    REGISTER_THREAD();
    return false;
}

bool TSIPClient::transfer(int id, const string& num)
{
    DECL_TRACER("TSIPClient::transfer(int id, const string& num)");

    REGISTER_THREAD();
    return false;
}

int TSIPClient::getNumberCalls()
{
    DECL_TRACER("TSIPClient::getNumberCalls()");

    return mLine + 1;
}

void TSIPClient::sendConnectionStatus(SIP_STATE_t state, int id)
{
    DECL_TRACER("TSIPClient::sendConnectionStatus(SIP_STATE_t state)");

    if (!gPageManager)
        return;

    vector<string> cmds;

    cmds.push_back("CALL");

    switch(state)
    {
        case SIP_CONNECTED:     cmds.push_back("CONNECTED"); break;
        case SIP_DISCONNECTED:  cmds.push_back("DISCONNECTED"); break;
        case SIP_HOLD:          cmds.push_back("HOLD"); break;
        case SIP_RINGING:       cmds.push_back("RINGING"); break;
        case SIP_TRYING:        cmds.push_back("TRYING"); break;

        default:
            return;
    }

    cmds.push_back(to_string(id));
    gPageManager->sendPHN(cmds);
}

/*******************************************************************************
 * All the following functions are static callback functions.
 * The following functions are called by the PJSUA-library for different
 * events.
 ******************************************************************************/

void TSIPClient::_log_call(int level, const char *data, int len)
{
    string msg;
    msg.assign(data, len);

    switch(level)
    {
        case 0: MSG_ERROR("FATAL:" << msg); break;
        case 1: MSG_ERROR(msg); break;
        case 2: MSG_WARNING(msg); break;
        case 3: MSG_INFO(msg); break;
        default:
            MSG_DEBUG(msg);
    }
}

void TSIPClient::ringback_start(pjsua_call_id call_id)
{
    DECL_TRACER("TSIPClient::ringback_start(pjsua_call_id call_id)");

    if (mAppConfig.call_data[call_id].ringback_on)
        return;

    mAppConfig.call_data[call_id].ringback_on = PJ_TRUE;

    if (++mAppConfig.ringback_cnt==1 && mAppConfig.ringback_slot!=PJSUA_INVALID_ID)
    {
        pjsua_conf_connect(mAppConfig.ringback_slot, 0);
    }
}

void TSIPClient::ring_stop(pjsua_call_id call_id)
{
    DECL_TRACER("TSIPClient::ring_stop(pjsua_call_id call_id)");

    if (mAppConfig.no_tones)
        return;

    if (mAppConfig.call_data[call_id].ringback_on)
    {
        mAppConfig.call_data[call_id].ringback_on = PJ_FALSE;

        pj_assert(mAppConfig.ringback_cnt > 0);

        if (--mAppConfig.ringback_cnt == 0 && mAppConfig.ringback_slot != PJSUA_INVALID_ID)
        {
            pjsua_conf_disconnect(mAppConfig.ringback_slot, 0);
            pjmedia_tonegen_rewind(mAppConfig.ringback_port);
        }
    }

    if (mAppConfig.call_data[call_id].ring_on)
    {
        mAppConfig.call_data[call_id].ring_on = PJ_FALSE;

        pj_assert(mAppConfig.ring_cnt>0);

        if (--mAppConfig.ring_cnt == 0 && mAppConfig.ring_slot != PJSUA_INVALID_ID)
        {
            pjsua_conf_disconnect(mAppConfig.ring_slot, 0);
            pjmedia_tonegen_rewind(mAppConfig.ring_port);
        }
    }
}

void TSIPClient::ring_start(pjsua_call_id call_id)
{
    DECL_TRACER("TSIPClient::ring_start(pjsua_call_id call_id)");

    if (mAppConfig.no_tones)
        return;

    if (mAppConfig.call_data[call_id].ring_on)
        return;

    mAppConfig.call_data[call_id].ring_on = PJ_TRUE;

    if (++mAppConfig.ring_cnt==1 && mAppConfig.ring_slot != PJSUA_INVALID_ID)
    {
        pjsua_conf_connect(mAppConfig.ring_slot, 0);
    }
}

pj_bool_t TSIPClient::find_next_call(void)
{
    int i, max;

    max = pjsua_call_get_max_count();

    for (i = mCurrentCall + 1; i < max; ++i)
    {
        if (pjsua_call_is_active(i))
        {
            mCurrentCall = i;
            return PJ_TRUE;
        }
    }

    for (i = 0; i < mCurrentCall; ++i)
    {
        if (pjsua_call_is_active(i))
        {
            mCurrentCall = i;
            return PJ_TRUE;
        }
    }

    mCurrentCall = PJSUA_INVALID_ID;
    return PJ_FALSE;
}

void TSIPClient::call_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)
{
    DECL_TRACER("TSIPClient::call_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)");

    pjsua_call_id call_id = entry->id;
    pjsua_msg_data msg_data_;
    pjsip_generic_string_hdr warn;
    pj_str_t hname = pj_str((char *)"Warning");
    pj_str_t hvalue = pj_str((char *)"399 pjsua \"Call duration exceeded\"");

    PJ_UNUSED_ARG(timer_heap);

    if (call_id == PJSUA_INVALID_ID)
    {
        MSG_WARNING("Invalid call ID in timer callback");
        return;
    }

    /* Add warning header */
    pjsua_msg_data_init(&msg_data_);
    pjsip_generic_string_hdr_init2(&warn, &hname, &hvalue);
    pj_list_push_back(&msg_data_.hdr_list, &warn);

    /* Call duration has been exceeded; disconnect the call */
    MSG_WARNING("Duration (" << mAppConfig.duration << " seconds) has been exceeded for call " << call_id << ", disconnecting the call,");
    entry->id = PJSUA_INVALID_ID;
    pjsua_call_hangup(call_id, 200, NULL, &msg_data_);
    mMyself->mSIPState = SIP_DISCONNECTED;
    sendConnectionStatus(SIP_DISCONNECTED, call_id);
}

void TSIPClient::on_playfile_done(pjmedia_port *port, void *usr_data)
{
    DECL_TRACER("TSIPClient::on_playfile_done(pjmedia_port *port, void *usr_data)");

    pj_time_val delay;

    PJ_UNUSED_ARG(port);
    PJ_UNUSED_ARG(usr_data);

    /* Just rewind WAV when it is played outside of call */
    if (pjsua_call_get_count() == 0) {
        pjsua_player_set_pos(mAppConfig.wav_id, 0);
    }

    /* Timer is already active */
    if (mAppConfig.auto_hangup_timer.id == 1)
        return;

    mAppConfig.auto_hangup_timer.id = 1;
    delay.sec = 0;
    delay.msec = 200; /* Give 200 ms before hangup */
    pjsip_endpt_schedule_timer(pjsua_get_pjsip_endpt(), &mAppConfig.auto_hangup_timer, &delay);
}

void TSIPClient::hangup_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)
{
    DECL_TRACER("TSIPClient::hangup_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry)");

    PJ_UNUSED_ARG(timer_heap);
    PJ_UNUSED_ARG(entry);

    mAppConfig.auto_hangup_timer.id = 0;
    pjsua_call_hangup_all();
}

/* Callback called by the library upon receiving incoming call */
void TSIPClient::on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
    DECL_TRACER("on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)");

    pjsua_call_info ci;

    PJ_UNUSED_ARG(acc_id);
    PJ_UNUSED_ARG(rdata);

    pjsua_call_get_info(call_id, &ci);

    MSG_DEBUG("Incoming call from " << ci.remote_info.ptr);

    /* Automatically answer incoming calls with 200/OK */
    if (gPageManager && gPageManager->getPHNautoanswer() && mMyself)
        mMyself->pickup(call_id);
}

/* Callback called by the library when call's state has changed */
void TSIPClient::on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
    DECL_TRACER("on_call_state(pjsua_call_id call_id, pjsip_event *e)");

    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
    {
        /* Stop all ringback for this call */
        ring_stop(call_id);

        /* Cancel duration timer, if any */
        if (mAppConfig.call_data[call_id].timer.id != PJSUA_INVALID_ID)
        {
            app_call_data *cd = &mAppConfig.call_data[call_id];
            pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();

            cd->timer.id = PJSUA_INVALID_ID;
            pjsip_endpt_cancel_timer(endpt, &cd->timer);
        }

        /* Rewind play file when hangup automatically,
         * since file is not looped
         */
        if (mAppConfig.auto_play_hangup)
            pjsua_player_set_pos(mAppConfig.wav_id, 0);

        string dbgMsg;

        if (ci.last_status_text.slen > 0)
            dbgMsg.assign(ci.last_status_text.ptr, ci.last_status_text.slen);

        MSG_DEBUG("Call " << call_id << " disconnected [reason: " << ci.last_status << " (" << dbgMsg << ")]");

        if (call_id == mCurrentCall)
        {
            find_next_call();
        }
    }
    else
    {
        if (mAppConfig.duration != PJSUA_APP_NO_LIMIT_DURATION && ci.state == PJSIP_INV_STATE_CONFIRMED)
        {
            /* Schedule timer to hangup call after the specified duration */
            app_call_data *cd = &mAppConfig.call_data[call_id];
            pjsip_endpoint *endpt = pjsua_get_pjsip_endpt();
            pj_time_val delay;

            cd->timer.id = call_id;
            delay.sec = mAppConfig.duration;
            delay.msec = 0;
            pjsip_endpt_schedule_timer(endpt, &cd->timer, &delay);
        }

        if (ci.state == PJSIP_INV_STATE_EARLY)
        {
            int code;
            pj_str_t reason;
            pjsip_msg *msg;

            /* This can only occur because of TX or RX message */
            pj_assert(e->type == PJSIP_EVENT_TSX_STATE);

            if (e->body.tsx_state.type == PJSIP_EVENT_RX_MSG) {
                msg = e->body.tsx_state.src.rdata->msg_info.msg;
            } else {
                msg = e->body.tsx_state.src.tdata->msg;
            }

            code = msg->line.status.code;
            reason = msg->line.status.reason;

            /* Start ringback for 180 for UAC unless there's SDP in 180 */
            if (ci.role == PJSIP_ROLE_UAC && code == 180 && msg->body == NULL && ci.media_status == PJSUA_CALL_MEDIA_NONE)
            {
                ringback_start(call_id);
            }

            string dbgMsg;
            string dbgCode;

            if (ci.state_text.slen > 0)
                dbgMsg.assign(ci.state_text.ptr, ci.state_text.slen);

            if (reason.slen > 0)
                dbgCode.assign(reason.ptr, reason.slen);

            MSG_DEBUG("Call " << call_id << " state changed to " << dbgMsg << " (" << code << " " << dbgCode << ")");
        }
        else
        {
            string dbgMsg;

            if (ci.state_text.slen > 0)
                dbgMsg.assign(ci.state_text.ptr, ci.state_text.slen);

            MSG_DEBUG("Call " << call_id << " state changed to " << dbgMsg);
        }

        if (mCurrentCall == PJSUA_INVALID_ID)
            mCurrentCall = call_id;
    }
}

/* Callback called by the library when call's media state has changed */
void TSIPClient::on_call_media_state(pjsua_call_id call_id)
{
    DECL_TRACER("on_call_media_state(pjsua_call_id call_id)");

    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE)
    {
        // When media is active, connect call to sound device.
        pjsua_conf_connect(ci.conf_slot, 0);
        pjsua_conf_connect(0, ci.conf_slot);
    }
}

void TSIPClient::call_on_dtmf_callback2(pjsua_call_id call_id, const pjsua_dtmf_info *info)
{
    DECL_TRACER("TSIPClient::call_on_dtmf_callback2(pjsua_call_id call_id, const pjsua_dtmf_info *info)");

    char duration[16];
    char method[16];

    duration[0] = '\0';

    switch (info->method)
    {
        case PJSUA_DTMF_METHOD_RFC2833:
            pj_ansi_snprintf(method, sizeof(method), "RFC2833");
        break;

        case PJSUA_DTMF_METHOD_SIP_INFO:
            pj_ansi_snprintf(method, sizeof(method), "SIP INFO");
            pj_ansi_snprintf(duration, sizeof(duration), ":duration(%d)", info->duration);
        break;
    }

    MSG_DEBUG("Incoming DTMF on call " << call_id << ": " << info->digit << duration << ", using " << method << " method.");
}

pjsip_redirect_op TSIPClient::call_on_redirected(pjsua_call_id call_id, const pjsip_uri *target, const pjsip_event *e)
{
    DECL_TRACER("TSIPClient::call_on_redirected(pjsua_call_id call_id, const pjsip_uri *target, const pjsip_event *e)");

    PJ_UNUSED_ARG(e);

    if (mAppConfig.redir_op == PJSIP_REDIRECT_PENDING)
    {
        char uristr[PJSIP_MAX_URL_SIZE];
        int len;

        len = pjsip_uri_print(PJSIP_URI_IN_FROMTO_HDR, target, uristr, sizeof(uristr));

        if (len < 1)
        {
            pj_ansi_strcpy(uristr, "--URI too long--");
        }

        string dbgMsg;

        if (len > 0)
            dbgMsg.assign(uristr, len);

        MSG_DEBUG("Call " << call_id << " is being redirected to " << dbgMsg << ".");
    }

    return mAppConfig.redir_op;
}

void TSIPClient::on_call_transfer_status(pjsua_call_id call_id, int status_code, const pj_str_t *status_text, pj_bool_t final, pj_bool_t *p_cont)
{
    DECL_TRACER("TSIPClient::on_call_transfer_status(pjsua_call_id call_id, int status_code, const pj_str_t *status_text, pj_bool_t final, pj_bool_t *p_cont)");

    string dbgMsg;

    if (status_text->slen > 0)
        dbgMsg.assign(status_text->ptr, status_text->slen);

    MSG_DEBUG("Call " << call_id << ": transfer status: " << status_code << " (" << dbgMsg << ") " << (final ? "[final]" : ""));

    if ((status_code / 100) == 2)
    {
        MSG_DEBUG("Call " << call_id << ": Call transferred successfully, disconnecting call.");
        pjsua_call_hangup(call_id, PJSIP_SC_GONE, NULL, NULL);
        *p_cont = PJ_FALSE;

        if (gPageManager)
        {
            vector<string> cmds;
            cmds.push_back("TRANSFERRED");
            cmds.push_back(to_string(call_id));
            gPageManager->sendPHN(cmds);
        }
    }
}

void TSIPClient::on_transport_state(pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info)
{
    DECL_TRACER("TSIPClient::on_transport_state(pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info)");

    char host_port[128];

    pj_addr_str_print(&tp->remote_name.host, tp->remote_name.port, host_port, sizeof(host_port), 1);

    switch (state)
    {
        case PJSIP_TP_STATE_CONNECTED:
            MSG_DEBUG("SIP " << tp->type_name << " transport is connected to " << host_port);
            mMyself->mSIPState = SIP_CONNECTED;
        break;

        case PJSIP_TP_STATE_DISCONNECTED:
        {
            char buf[100];
            size_t len;

            len = pj_ansi_snprintf(buf, sizeof(buf), "SIP %s transport is disconnected from %s", tp->type_name, host_port);
            PJ_CHECK_TRUNC_STR(len, buf, sizeof(buf));
            MSG_ERROR(buf << " (" << info->status << ")");
            mMyself->mSIPState = SIP_DISCONNECTED;
        }
        break;

        default:
            break;
    }
}

void TSIPClient::on_ip_change_progress(pjsua_ip_change_op op, pj_status_t status, const pjsua_ip_change_op_info *info)
{
    DECL_TRACER("TSIPClient::on_ip_change_progress(pjsua_ip_change_op op, pj_status_t status, const pjsua_ip_change_op_info *info)");

    char info_str[128];
    pjsua_acc_info acc_info;
    pjsua_transport_info tp_info;

    if (status == PJ_SUCCESS)
    {
        switch (op)
        {
            case PJSUA_IP_CHANGE_OP_RESTART_LIS:
                pjsua_transport_get_info(info->lis_restart.transport_id, &tp_info);
                pj_ansi_snprintf(info_str, sizeof(info_str),
                                 "restart transport %.*s",
                                 (int)tp_info.info.slen, tp_info.info.ptr);
            break;

            case PJSUA_IP_CHANGE_OP_ACC_SHUTDOWN_TP:
                pjsua_acc_get_info(info->acc_shutdown_tp.acc_id, &acc_info);

                pj_ansi_snprintf(info_str, sizeof(info_str),
                                 "transport shutdown for account %.*s",
                                 (int)acc_info.acc_uri.slen,
                                 acc_info.acc_uri.ptr);
            break;

            case PJSUA_IP_CHANGE_OP_ACC_UPDATE_CONTACT:
                pjsua_acc_get_info(info->acc_shutdown_tp.acc_id, &acc_info);
                if (info->acc_update_contact.code) {
                    pj_ansi_snprintf(info_str, sizeof(info_str),
                                     "update contact for account %.*s, code[%d]",
                                     (int)acc_info.acc_uri.slen,
                                     acc_info.acc_uri.ptr,
                                     info->acc_update_contact.code);
                } else {
                    pj_ansi_snprintf(info_str, sizeof(info_str),
                                     "update contact for account %.*s",
                                     (int)acc_info.acc_uri.slen,
                                     acc_info.acc_uri.ptr);
                }
            break;

            case PJSUA_IP_CHANGE_OP_ACC_HANGUP_CALLS:
                pjsua_acc_get_info(info->acc_shutdown_tp.acc_id, &acc_info);
                pj_ansi_snprintf(info_str, sizeof(info_str),
                                 "hangup call for account %.*s, call_id[%d]",
                                 (int)acc_info.acc_uri.slen, acc_info.acc_uri.ptr,
                                 info->acc_hangup_calls.call_id);
            break;

            case PJSUA_IP_CHANGE_OP_ACC_REINVITE_CALLS:
                pjsua_acc_get_info(info->acc_shutdown_tp.acc_id, &acc_info);
                pj_ansi_snprintf(info_str, sizeof(info_str),
                                 "reinvite call for account %.*s, call_id[%d]",
                                 (int)acc_info.acc_uri.slen, acc_info.acc_uri.ptr,
                                 info->acc_reinvite_calls.call_id);
            break;

            case PJSUA_IP_CHANGE_OP_COMPLETED:
                pj_ansi_snprintf(info_str, sizeof(info_str), "done");

            default:
                break;
        }

        MSG_DEBUG("IP change progress report: " << info_str);
    }
    else
    {
        MSG_ERROR("IP change progress failed (" << status << ")");
    }
}

/*
 * A simple registrar, invoked by default_mod_on_rx_request()
 */
void TSIPClient::simple_registrar(pjsip_rx_data *rdata)
{
    pjsip_tx_data *tdata;
    const pjsip_expires_hdr *exp;
    const pjsip_hdr *h;
    unsigned cnt = 0;
    pjsip_generic_string_hdr *srv;
    pj_status_t status;

    status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(), rdata, 200, NULL, &tdata);

    if (status != PJ_SUCCESS)
        return;

    exp = (pjsip_expires_hdr *)pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_EXPIRES, NULL);

    h = rdata->msg_info.msg->hdr.next;

    while (h != &rdata->msg_info.msg->hdr)
    {
        if (h->type == PJSIP_H_CONTACT)
        {
            const pjsip_contact_hdr *c = (const pjsip_contact_hdr*)h;
            unsigned e = c->expires;

            if (e != PJSIP_EXPIRES_NOT_SPECIFIED)
            {
                if (exp)
                    e = exp->ivalue;
                else
                    e = 3600;
            }

            if (e > 0)
            {
                pjsip_contact_hdr *nc = (pjsip_contact_hdr *)pjsip_hdr_clone(tdata->pool, h);
                nc->expires = e;
                pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)nc);
                ++cnt;
            }
        }

        h = h->next;
    }

    srv = pjsip_generic_string_hdr_create(tdata->pool, NULL, NULL);
    srv->name = pj_str((char *)"Server");
    srv->hvalue = pj_str((char *)"pjsua simple registrar");
    pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)srv);

    status = pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(), rdata, tdata, NULL, NULL);

    if (status != PJ_SUCCESS)
        pjsip_tx_data_dec_ref(tdata);
}

/*****************************************************************************
 * A simple module to handle otherwise unhandled request. We will register
 * this with the lowest priority.
 */

/* Notification on incoming request */
pj_bool_t TSIPClient::default_mod_on_rx_request(pjsip_rx_data *rdata)
{
    pjsip_tx_data *tdata;
    pjsip_status_code status_code;
    pj_status_t status;

    /* Don't respond to ACK! */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, &pjsip_ack_method) == 0)
        return PJ_TRUE;

    /* Simple registrar */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, &pjsip_register_method) == 0)
    {
        simple_registrar(rdata);
        return PJ_TRUE;
    }

    /* Create basic response. */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, &pjsip_notify_method) == 0)
    {
        /* Unsolicited NOTIFY's, send with Bad Request */
        status_code = PJSIP_SC_BAD_REQUEST;
    }
    else
    {
        /* Probably unknown method */
        status_code = PJSIP_SC_METHOD_NOT_ALLOWED;
    }

    status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(), rdata, status_code, NULL, &tdata);

    if (status != PJ_SUCCESS)
    {
        MSG_ERROR("Unable to create response");
        return PJ_TRUE;
    }

    /* Add Allow if we're responding with 405 */
    if (status_code == PJSIP_SC_METHOD_NOT_ALLOWED)
    {
        const pjsip_hdr *cap_hdr;
        cap_hdr = pjsip_endpt_get_capability(pjsua_get_pjsip_endpt(), PJSIP_H_ALLOW, NULL);

        if (cap_hdr)
        {
            pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr *)pjsip_hdr_clone(tdata->pool, cap_hdr));
        }
    }

    /* Add User-Agent header */
    {
        pj_str_t user_agent;
        char tmp[80];
        const pj_str_t USER_AGENT = { (char *)"User-Agent", 10};
        pjsip_hdr *h;

        pj_ansi_snprintf(tmp, sizeof(tmp), "PJSUA v%s/%s", pj_get_version(), PJ_OS_NAME);
        pj_strdup2_with_null(tdata->pool, &user_agent, tmp);

        h = (pjsip_hdr*) pjsip_generic_string_hdr_create(tdata->pool, &USER_AGENT, &user_agent);
        pjsip_msg_add_hdr(tdata->msg, h);
    }

    status = pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(), rdata, tdata, NULL, NULL);

    if (status != PJ_SUCCESS) pjsip_tx_data_dec_ref(tdata);
    return PJ_TRUE;
}

#endif  // _NOSIP_

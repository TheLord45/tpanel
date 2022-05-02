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

#include <pjsua-lib/pjsua.h>

#ifndef _NOSIP_
#define SIP_MAX_LINES       2

#define current_acc         pjsua_acc_get_default()

#define PJSUA_APP_NO_LIMIT_DURATION (int)0x7FFFFFFF
// Ringtones                US         UK
#define RINGBACK_FREQ1      440     // 400
#define RINGBACK_FREQ2      480     // 450
#define RINGBACK_ON         2000    // 400
#define RINGBACK_OFF        4000    // 200
#define RINGBACK_CNT        1       // 2
#define RINGBACK_INTERVAL   4000    // 2000

#define RING_FREQ1          800
#define RING_FREQ2          640
#define RING_ON             200
#define RING_OFF            100
#define RING_CNT            3
#define RING_INTERVAL       3000

typedef unsigned int        uint_t;

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

        bool init();

        void cleanUp();
        int getLineID() { return mLine; }
        SIP_STATE_t getSIPState(pjsua_call_id id) { if (id >= 0 && id < PJSUA_MAX_CALLS) return mSIPState[id]; else return SIP_NONE; }
        bool isRegistered() { return mRegistered; }
        static TSIPClient *getSIPClient() { return mMyself; }

        bool call(const std::string& dest);         //<! Start a phone call
        bool pickup(pjsua_call_id call);            //<! Lift up if the phone is ringing
        bool terminate(int id);                     //<! Terminate a call
        bool hold(int id);                          //<! Pause a call
        bool resume(int id);                        //<! Resume a paused call
        bool sendDTMF(std::string& dtmf);           //<! Send a DTMF string
        bool sendLinestate();                       //<! Queries the state of each of the connections used by the SIP device.
        bool sendPrivate(bool state);               //<! Enables or disables the privacy feature on the phone (do not disturb).
        bool redial();                              //<! Redial last number
        bool transfer(int id, const std::string& num);  //<! transfer the current call
        bool setDTMFduration(uint_t ms);            //<! Set the DTMF duration in ms.
        bool getPrivate() { return mDoNotDisturb; } //<! Returns the current private mode.
        bool sendIM(const std::string& target, const std::string& msg); //<! Send an IM to somebody or to the peer of the current call
        size_t getNumberMessages() { return mMessages.size(); }

    protected:
        void runRinger();                           //<! Plays a ring tone if a call is coming.

        static void _log_call(int level, const char *data, int len);
        static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);
        static void on_call_state(pjsua_call_id call_id, pjsip_event *e);
        static void on_call_media_state(pjsua_call_id call_id);
        static void call_on_dtmf_callback2(pjsua_call_id call_id, const pjsua_dtmf_info *info);
        static pjsip_redirect_op call_on_redirected(pjsua_call_id call_id, const pjsip_uri *target, const pjsip_event *e);
        static void on_call_transfer_status(pjsua_call_id call_id, int status_code, const pj_str_t *status_text, pj_bool_t final, pj_bool_t *p_cont);
        static void on_transport_state(pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info);
        static void on_ip_change_progress(pjsua_ip_change_op op, pj_status_t status, const pjsua_ip_change_op_info *info);
        static void simple_registrar(pjsip_rx_data *rdata);
        static pj_bool_t default_mod_on_rx_request(pjsip_rx_data *rdata);
        static void ringback_start(pjsua_call_id call_id);
        static void ring_start(pjsua_call_id call_id);
        static void ring_stop(pjsua_call_id call_id);
        static pj_bool_t find_next_call(void);
        static void call_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);
        static void on_playfile_done(pjmedia_port *port, void *usr_data);
        static void hangup_timeout_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);
        static void on_pager2(pjsua_call_id call_id, const pj_str_t *from,
                              const pj_str_t *to, const pj_str_t *contact,
                              const pj_str_t *mime_type, const pj_str_t *body,
                              pjsip_rx_data *rdata, pjsua_acc_id acc_id);
        static void on_buddy_state(pjsua_buddy_id buddy_id);
        static void on_buddy_evsub_state(pjsua_buddy_id buddy_id, pjsip_evsub *sub, pjsip_event *event);
        static void on_mwi_info(pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info);

    private:
        typedef struct _uri_t
        {
            std::string name;           //!< The name coming in double quotes (optional)
            std::string scheme;         //!< The scheme (e.g. sip)
            std::string user;           //!< The user name / telephone number
            std::string server;         //!< The IP address or FQDN of the server
            int port;                   //!< An optional port number
        }_uri_t;

        static pjsip_module mod_default_handler;

        int getNumberCalls();
        pjsua_call_id getActiveCall();
        void setSIPState(SIP_STATE_t s, pjsua_call_id id) { if (id >= 0 && id < PJSUA_MAX_CALLS) mSIPState[id] = s; }

        static void sendConnectionStatus(SIP_STATE_t state, int id);
        static void init_ringtone_player();
        static pj_status_t start_ring_tone();
        static pj_status_t stop_ring_tone();
        static pjsua_buddy_id addBuddy(const std::string& rsipurl);
        static _uri_t parseUri(const std::string& uri);

        int mLine{0};
        bool mRegistered{false};
        SIP_STATE_t mSIPState[PJSUA_MAX_CALLS];
        pjsua_acc_id mAccountID{0};
        std::string mLastCall;
        uint_t mDTMFduration{PJSUA_CALL_SEND_DTMF_DURATION_DEFAULT};
        bool mDoNotDisturb{false};
        std::vector<std::string>mMessages;      // Holds all received messages

        static TSIPClient *mMyself;
        static pjsua_call_id mCurrentCall;
        static std::atomic<bool> mRefreshRun;
        static bool mPhoneRingInit;
        // Here is the configuration for the PJSUA library
        typedef struct app_call_data
        {
            pj_timer_entry      timer;
            pj_bool_t           ringback_on{PJ_FALSE};
            pj_bool_t           ring_on{PJ_FALSE};
        } app_call_data;

        typedef struct ringtone_port_info_t
        {
            pj_bool_t ring_on{PJ_FALSE};
            int ring_slot{0};
            pjmedia_port *ring_port{nullptr};
            pj_pool_t *pool{nullptr};
        } ringtone_port_info_t;

        typedef struct pjsua_app_config
        {
            pjsua_config        cfg;
            pjsua_logging_config log_cfg;
            pjsua_media_config  media_cfg;
            pj_bool_t           no_refersub{PJ_FALSE};
            pj_bool_t           enable_qos{PJ_FALSE};
            pj_bool_t           no_tcp{PJ_TRUE};
            pj_bool_t           no_udp{PJ_FALSE};
            pj_bool_t           use_tls{PJ_FALSE};
            pjsua_transport_config  udp_cfg;
            pjsua_transport_config  rtp_cfg;
            pjsip_redirect_op   redir_op;
            int                 srtp_keying{0};

            uint_t              acc_cnt{0};
            pjsua_acc_config    acc_cfg[PJSUA_MAX_ACC];

            uint_t              buddy_cnt{0};
            pjsua_buddy_config  buddy_cfg[PJSUA_MAX_BUDDIES];

            app_call_data       call_data[PJSUA_MAX_CALLS];

            pj_pool_t          *pool{nullptr};
            /* Compatibility with older pjsua */

            uint_t              codec_cnt{0};
            pj_str_t            codec_arg[32];
            uint_t              codec_dis_cnt{0};
            pj_str_t            codec_dis[32];
            pj_bool_t           null_audio{PJ_FALSE};
            uint_t              wav_count{0};
            pj_str_t            wav_files[32];
            uint_t              tone_count{0};
            pjmedia_tone_desc   tones[32];
            pjsua_conf_port_id  tone_slots[32];
            pjsua_player_id     wav_id{0};
            pjsua_conf_port_id  wav_port{0};
            pj_bool_t           auto_play{PJ_FALSE};
            pj_bool_t           auto_play_hangup{PJ_FALSE};
            pj_timer_entry      auto_hangup_timer;
            pj_bool_t           auto_loop{PJ_FALSE};
            pj_bool_t           auto_conf{PJ_FALSE};
            pj_str_t            rec_file;
            pj_bool_t           auto_rec{PJ_FALSE};
            pjsua_recorder_id   rec_id{0};
            pjsua_conf_port_id  rec_port{0};
            unsigned            auto_answer{0};
            unsigned            duration{36000};
            float               mic_level{100.0};
            float               speaker_level{100.0};

            int                 capture_dev{0};
            int                 playback_dev{0};
            uint_t              capture_lat{0};
            uint_t              playback_lat{0};

            pj_bool_t           no_tones{PJ_FALSE};
            int                 ringback_slot{0};
            int                 ringback_cnt{0};
            pjmedia_port        *ringback_port{nullptr};
            int                 ring_slot{0};
            int                 ring_cnt{0};
            pjmedia_port        *ring_port{nullptr};

            uint_t              aud_cnt{0};
        }pjsua_app_config;

        static pjsua_app_config mAppConfig;
        static ringtone_port_info_t mRingtonePortInfo;
};
#endif  // _NOSIP_
#endif  // __TSIPCLIENT_H__

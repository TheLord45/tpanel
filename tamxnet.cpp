/*
 * Copyright (C) 2018 to 2023 by Andreas Theofilu <andreas@theosys.at>
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

#include <sys/utsname.h>

#include <functional>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "tamxnet.h"
#include "terror.h"
#include "tconfig.h"
#include "tdirectory.h"
#include "tresources.h"
#include "texpand.h"
#include "tsocket.h"
#include "texcept.h"
#include "tpagemanager.h"

using namespace amx;
using namespace std;
using std::mutex;

using placeholders::_1;
using placeholders::_2;

string cmdList[] =
{
    "@WLD-", "@AFP-", "@GCE-", "@APG-", "@CPG-", "@DPG-", "@PDR-", "@PHE-",
    "@PHP-", "@PHT-", "@PPA-", "@PPF-", "@PPG-", "@PPK-", "@PPM-", "@PPN-",
    "@PPT-", "@PPX", "@PSE-", "@PSP-", "@PST-", "PAGE-", "PPOF-", "PPOG-",
    "PPON-", "^ANI-", "^APF-", "^BAT-", "^BAU-", "^BCB-", "^BCF-", "^BCT-",
    "^BDO-", "^BFB-", "^BIM-", "^BLN-", "^BMC-", "^BMF-", "^BMI-", "^BML-",
    "^BMP-", "^BNC-", "^BNN-", "^BNT-", "^BOP-", "^BOR-", "^BOS-", "^BPP-",
    "^BRD-", "^BSF-", "^BSP-", "^BSM-", "^BSO-", "^BVL-", "^BVN-", "^BVP-",
    "^BVT-", "^BWW-", "^CPF-", "^DLD-", "^DPF-", "^ENA-", "^FON-", "^GDI-",
    "^GIV-", "^GLH-", "^GLL-", "^GRD-", "^GRU-", "^GSC-", "^GSN-", "^ICO-",
    "^IRM-", "^JSB-", "^JSI-", "^JST-", "^MBT-", "^MDC-", "^SHO-", "^TEC-",
    "^TEF-", "^TOP-", "^TXT-", "^UNI-", "^LPC-", "^LPR-", "^LPS-", "?BCB-",
    "?BCF-", "?BCT-", "?BMP-", "?BOP-", "?BRD-", "?BWW-", "?FON-", "?ICO-",
    "?JSB-", "?JSI-", "?JST-", "?TEC-", "?TEF-", "?TXT-", "ABEEP", "ADBEEP",
    "@AKB-", "AKEYB-", "AKEYP-", "AKEYR-", "@AKP-", "@AKR", "BEEP", "BRIT-",
    "@BRT-", "DBEEP", "@EKP-", "PKEYP-", "@PKP-", "SETUP", "SHUTDOWN", "SLEEP",
    "@SOU-", "@TKP-", "TPAGEON", "TPAGEOFF", "@VKB", "WAKE", "^CAL", "^KPS-",
    "^VKS-", "@PWD-", "^PWD-", "^BBR-", "^RAF-", "^RFR-", "^RMF-", "^RSR-",
    "^MODEL?", "^ICS-", "^ICE-", "^ICM-", "^PHN-", "?PHN-", "LEVON", "RXON",
    "BLINK", "TPCCMD", "TPCACC", "^EPR", "^SCE", "^SDR", "^SHD", "^SSH",
    "^STG", "^MSP", "^LPB",
    // G5 commands
    "^ABP-", "^ADB", "^SOU-", "^STP", "^TKP-", "^PGE-", "^PPA-", "^PPF-",
    "^PPG-", "^PPK-", "^PPM-", "^PPN-", "^PPT-", "^PPX", "^UTF-", "^LVC-",
    "^LVD-", "^LVE-", "^LVF-", "^LVL-", "^LVM-", "^LVN-", "^LVR-", "^LVS-",
    "^MUT-", "\0"
};

#define NUMBER_CMDS     144

std::atomic<bool> killed{false};
std::atomic<bool> _netRunning{false};
amx::TAmxNet *gAmxNet = nullptr;
static bool __CommValid = false;
std::map<ulong, FUNC_NETWORK_t> mFuncsNetwork;
std::map<ulong, FUNC_TIMER_t> mFuncsTimer;

//std::vector<ANET_COMMAND> TAmxNet::comStack; // commands to answer
extern TPageManager *gPageManager;

//mutex read_mutex;

TAmxNet::TAmxNet()
{
    DECL_TRACER("TAmxNet::TAmxNet()");

    init();
}

TAmxNet::TAmxNet(const string& sn)
    : serNum(sn)
{
    DECL_TRACER("TAmxNet::TAmxNet(const string& sn)");

    init();
}

TAmxNet::TAmxNet(const string& sn, const string& nm)
    : panName(nm),
      serNum(sn)
{
    DECL_TRACER("TAmxNet::TAmxNet(const string& sn)");

    size_t pos = nm.find(" (TPC)");

    if (pos != string::npos)
    {
        panName = nm.substr(0, pos) + "i";
        MSG_TRACE("Converted TP name: " << panName);
    }

    init();
}

TAmxNet::~TAmxNet()
{
    DECL_TRACER("TAmxNet::~TAmxNet()");

    callback = 0;
    write_busy = false;
    stop();

    if (mSocket)
    {
        delete mSocket;
        mSocket = nullptr;
        __CommValid = false;
    }

    gAmxNet = nullptr;
}

void TAmxNet::init()
{
    DECL_TRACER("TAmxNet::init()");

    sendCounter = 0;
    initSend = false;
    ready = false;

    callback = 0;
    stopped_ = false;
    write_busy = false;
    gAmxNet = this;

    if (!mSocket)
    {
        mSocket = new TSocket;
        __CommValid = true;
    }

    string version = "v2.01.00";        // A version > 2.0 is needed for file transfer!
    int devID = 0x0163, fwID = 0x0290;

    if (TConfig::getPanelType().length() > 0)
        panName = TConfig::getPanelType();
    else if (panName.empty())
        panName.assign("TheoSys");

    if (panName.find("MVP") != string::npos && panName.find("5200") != string::npos)
    {
        devID = 0x0149;
        fwID = 0x0310;
    }
    else if (panName.find("NX-CV7") != string::npos)
    {
        devID = 0x0123;
        fwID = 0x0135;
    }

    // Initialize the devive info structure
    DEVICE_INFO di;
    // Answer to MC = 0x0017 --> MC = 0x0097
    di.objectID = 0;
    di.parentID = 0;
    di.manufacturerID = 1;
    di.deviceID = devID;
    memset(di.serialNum, 0, sizeof(di.serialNum));

    if (!serNum.empty())
        memcpy(di.serialNum, serNum.c_str(), serNum.length());

    di.firmwareID = fwID;
    memset(di.versionInfo, 0, sizeof(di.versionInfo));
    strncpy(di.versionInfo, version.c_str(), version.length());
    memset(di.deviceInfo, 0, sizeof(di.deviceInfo));
    strncpy(di.deviceInfo, panName.c_str(), min(panName.length(), sizeof(di.deviceInfo) - 1));
    memset(di.manufacturerInfo, 0, sizeof(di.manufacturerInfo));
    strncpy(di.manufacturerInfo, "TheoSys", 7);
    di.format = 2;
    di.len = 4;
    memset(di.addr, 0, sizeof(di.addr));
    devInfo.push_back(di);
    // Kernel info
    di.objectID = 2;
    di.firmwareID = fwID + 1;
    memset(di.serialNum, 0x20, sizeof(di.serialNum));
    memcpy(di.serialNum, "N/A", 3);
    memset(di.deviceInfo, 0, sizeof(di.deviceInfo));
    strncpy(di.deviceInfo, "Kernel", 6);
    memset(di.versionInfo, 0, sizeof(di.versionInfo));
#ifdef __linux__
    struct utsname kinfo;
    uname(&kinfo);
    strncpy(di.versionInfo, kinfo.release, sizeof(di.versionInfo));
    char *ppos = strnstr(di.versionInfo, "-", sizeof(di.versionInfo));

    if (ppos && (ppos - di.versionInfo) < 16)
        *ppos = 0;
#else
    strncpy(di.versionInfo, "4.00.00", 7);
#endif
    devInfo.push_back(di);
}

void TAmxNet::registerNetworkState(function<void (int)> registerNetwork, ulong handle)
{
    DECL_TRACER("TAmxNet::registerNetworkState(function<void (int)> registerNetwork, ulong handle)");

    map<ulong, FUNC_NETWORK_t>::iterator iter = mFuncsNetwork.find(handle);

    if (mFuncsNetwork.size() == 0 || iter == mFuncsNetwork.end())
    {
        FUNC_NETWORK_t fn;
        fn.handle = handle;
        fn.func = registerNetwork;
        mFuncsNetwork.insert(pair<ulong, FUNC_NETWORK_t>(handle, fn));
    }

    registerNetwork((isRunning() ? NSTATE_ONLINE : NSTATE_OFFLINE));
}

void TAmxNet::registerTimer(function<void (const ANET_BLINK &)> registerBlink, ulong handle)
{
    DECL_TRACER("TAmxNet::registerTimer(function<void (const ANET_BLINK &)> registerBlink, ulong handle)");

    map<ulong, FUNC_TIMER_t>::iterator iter = mFuncsTimer.find(handle);

    if (mFuncsTimer.size() == 0 || iter == mFuncsTimer.end())
    {
        FUNC_TIMER_t ft;
        ft.handle = handle;
        ft.func = registerBlink;
        mFuncsTimer.insert(pair<ulong, FUNC_TIMER_t>(handle, ft));
    }
}

void TAmxNet::deregNetworkState(ulong handle)
{
    DECL_TRACER("TAmxNet::deregNetworkState(ulong handle)");

    if (mFuncsNetwork.size() == 0)
        return;

    map<ulong, FUNC_NETWORK_t>::iterator iter = mFuncsNetwork.find(handle);

    if (iter != mFuncsNetwork.end())
        mFuncsNetwork.erase(iter);
}

void TAmxNet::deregTimer(ulong handle)
{
    DECL_TRACER("TAmxNet::deregTimer(ulong handle)");

    if (mFuncsTimer.size() == 0)
        return;

    map<ulong, FUNC_TIMER_t>::iterator iter = mFuncsTimer.find(handle);

    if (iter != mFuncsTimer.end())
        mFuncsTimer.erase(iter);
}

void TAmxNet::stop(bool soft)
{
    DECL_TRACER("TAmxNet::stop: Stopping the client...");

    if (stopped_)
        return;

    if (!soft)
        stopped_ = true;

    if (mSocket)
        mSocket->close();
}

bool TAmxNet::reconnect()
{
    DECL_TRACER("TAmxNet::reconnect()");

    if (!mSocket || !__CommValid)
        return false;

    mSocket->close();
    initSend = false;
    ready = false;
    return true;
}

bool TAmxNet::isNetRun()
{
    return _netRunning;
}

void TAmxNet::Run()
{
    DECL_TRACER("TAmxNet::Run()");

    if (_netRunning || !__CommValid)
        return;

    _netRunning = true;
    stopped_ = false;
    _retry = false;

    try
    {
        mThread = std::thread([=] { this->start(); });
        mThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error connecting to " << TConfig::getController() << ":" << to_string(TConfig::getPort()) << " [" << e.what() << "]");
        _netRunning = false;
    }
}

/*
 * This function is running as a thread. It is the main method connecting to
 * a controller and it handles all the communication with the controller.
 */
void TAmxNet::start()
{
    DECL_TRACER("TAmxNet::start()");

    sendAllFuncNetwork(NSTATE_CONNECTING);

    while (__CommValid && mSocket && isRunning())
    {
        initSend = false;
        ready = false;

        if (__CommValid && TConfig::getController() == "0.0.0.0")
        {
            string controller = TConfig::getController();
            MSG_INFO("Refusing to connect to invalid controller " << controller);
            sendAllFuncNetwork(mLastOnlineState == NSTATE_OFFLINE ? NSTATE_OFFLINE1 : NSTATE_OFFLINE);
            std::this_thread::sleep_for(std::chrono::seconds(10));  // Wait 10 seconds before next try
            continue;
        }

        if (__CommValid && mSocket && !mSocket->connect(TConfig::getController(), TConfig::getPort()))
        {
            MSG_DEBUG("Connection failed. Retrying ...");
            sendAllFuncNetwork(NSTATE_OFFLINE);
            std::this_thread::sleep_for(std::chrono::seconds(mWaitTime));
            setWaitTime(WAIT_RESET);
            continue;
        }
        else if (!mSocket)
            break;

        sendAllFuncNetwork(NSTATE_ONLINE);

        if (__CommValid && isRunning() && gPageManager && gPageManager->getRepaintWindows())
            gPageManager->getRepaintWindows()();

        handle_connect();
        std::this_thread::sleep_for(std::chrono::seconds(mWaitTime));
        setWaitTime(WAIT_RESET);
        MSG_INFO("Network will be reestablished ...");
    }

    _netRunning = false;
}

void TAmxNet::handle_connect()
{
    DECL_TRACER("TAmxNet::handle_connect()");

    try
    {
        while (__CommValid && mSocket && isRunning() && mSocket->isConnected())
        {
            // Start the input actor.
            start_read();

            // Start the output actor.
            if (isRunning() && !write_busy)
                runWrite();
        }

        if (!stopped_ && (killed || prg_stopped))
            stop();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error: " << e.what());

        if (mSocket)
            mSocket->close();
    }
    catch (TXceptComm& e)
    {
        if (mSocket)
            mSocket->close();
    }

    sendAllFuncNetwork(NSTATE_CONNECTING);
    setWaitTime(WAIT_RECONNECT);
}

void TAmxNet::start_read()
{
    DECL_TRACER("TAmxNet::start_read()");

    if (!__CommValid || !mSocket || !isRunning() || !mSocket->isConnected())
        return;

    string _message = "TAmxNet::start_read(): Invalid argument received!";
    protError = false;
    comm.clear();

    try
    {
        // Read the first byte. It should be 0x02
        if (mSocket->readAbsolut(buff_, 1) == 1)
            handle_read(1, RT_ID);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_ID]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_LEN);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_LEN]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 1) == 1)
            handle_read(1, RT_SEP1);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_SEP1]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 1) == 1)
            handle_read(1, RT_TYPE);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_TYPE]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_WORD1);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_WORD1]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(30);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_DEVICE);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_DEVIVE]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_WORD2);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_WORD2]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_WORD3);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_WORD3]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_WORD4);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_WORD4]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_WORD5);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_WORD5]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 1) == 1)
            handle_read(1, RT_SEP2);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_SEP2]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_COUNT);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_COUNT]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, 2) == 2)
            handle_read(2, RT_MC);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_MC]");
            XCEPTCOMM(_message);
        }
        else
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        // Calculate the length of the data block. This is the rest of the total length
        size_t len = (comm.hlen + 3) - 0x0015;

        if (mSocket->isConnected() && len > BUF_SIZE)
        {
            _message = "Length to read is " + to_string(len) + " bytes, but the buffer is only " + to_string(BUF_SIZE) + " bytes!";
            XCEPTCOMM(_message);
        }
        else if (!mSocket->isConnected())
        {
            setWaitTime(WAIT_RECONNECT);
            return;
        }

        if (mSocket->readAbsolut(buff_, len) == len)
            handle_read(len, RT_DATA);
        else if (mSocket->isConnected())
        {
            _message.append(" [RT_DATA]");
            XCEPTCOMM(_message);
        }
    }
    catch (TXceptNetwork& e)
    {
        setWaitTime(WAIT_RECONNECT);
    }
}

void TAmxNet::handle_read(size_t n, R_TOKEN tk)
{
    DECL_TRACER("TAmxNet::handle_read(const error_code& error, size_t n, R_TOKEN tk)");

    if (stopped_ || !__CommValid || !mSocket || !mSocket->isConnected())
        return;

    if (killed || prg_stopped)
    {
        stop();
        return;
    }

    uint32_t dw;
    int val, pos;
    size_t len;
    ANET_SEND s;        // Used to answer system requests
    string cmd;

    len = (n < BUF_SIZE) ? n : BUF_SIZE - 1;
    input_buffer_.assign((char *)&buff_[0], len);

    MSG_DEBUG("Token: " << tk << ", " << len << " bytes");

    switch (tk)
    {
        case RT_ID:
            if (buff_[0] != 0x02)
                protError = true;
            else
                comm.ID = buff_[0];
        break;

        case RT_LEN:    comm.hlen = makeWord(buff_[0], buff_[1]); break;

        case RT_SEP1:
            if (buff_[0] != 0x02)
                protError = true;
            else
                comm.sep1 = buff_[0];
        break;

        case RT_TYPE:   comm.type = buff_[0]; break;
        case RT_WORD1:  comm.unk1 = makeWord(buff_[0], buff_[1]); break;
        case RT_DEVICE: comm.device1 = makeWord(buff_[0], buff_[1]); break;
        case RT_WORD2:  comm.port1 = makeWord(buff_[0], buff_[1]); break;
        case RT_WORD3:  comm.system = makeWord(buff_[0], buff_[1]); break;
        case RT_WORD4:  comm.device2 = makeWord(buff_[0], buff_[1]); break;
        case RT_WORD5:  comm.port2 = makeWord(buff_[0], buff_[1]); break;

        case RT_SEP2:
            if (buff_[0] != 0x0f)
                protError = true;
            else
                comm.unk6 = buff_[0];
        break;

        case RT_COUNT:  comm.count = makeWord(buff_[0], buff_[1]); break;
        case RT_MC:     comm.MC = makeWord(buff_[0], buff_[1]); break;

        case RT_DATA:
            if (protError || !isRunning())
                break;

            MSG_DEBUG("Received message type: 0x" << std::setw(4) << std::setfill('0') << std::hex << comm.MC);

            switch (comm.MC)
            {
                case 0x0001:    // ACK
                case 0x0002:    // NAK
                    comm.checksum = buff_[0];
                break;

                case 0x0084:    // input channel ON
                case 0x0085:    // input channel OFF
                case 0x0006:    // output channel ON
                case 0x0086:    // output channel ON status
                case 0x0007:    // output channel OFF
                case 0x0087:    // output channel OFF status
                case 0x0088:    // input/output channel ON status
                case 0x0089:    // input/output channel OFF status
                case 0x0018:    // feedback channel ON
                case 0x0019:    // feedback channel OFF
                    comm.data.chan_state.device = makeWord(buff_[0], buff_[1]);
                    comm.data.chan_state.port = makeWord(buff_[2], buff_[3]);
                    comm.data.chan_state.system = makeWord(buff_[4], buff_[5]);
                    comm.data.chan_state.channel = makeWord(buff_[6], buff_[7]);
                    comm.checksum = buff_[8];

                    s.channel = comm.data.chan_state.channel;
                    s.level = 0;
                    s.port = comm.data.chan_state.port;
                    s.value = 0;

                    switch (comm.MC)
                    {
                        case 0x0006: s.MC = 0x0086; break;
                        case 0x0007: s.MC = 0x0087; break;
                    }

                    if (comm.MC < 0x0020)
                    {
                        if (callback)
                            callback(comm);
                        else
                            MSG_WARNING("Missing callback function!");
                    }
                    else
                        sendCommand(s);
                break;

                case 0x000a:    // level value change
                case 0x008a:
                    comm.data.message_value.device = makeWord(buff_[0], buff_[1]);
                    comm.data.message_value.port = makeWord(buff_[2], buff_[3]);
                    comm.data.message_value.system = makeWord(buff_[4], buff_[5]);
                    comm.data.message_value.value = makeWord(buff_[6], buff_[7]);
                    comm.data.message_value.type = buff_[8];
                    val = (int)buff_[8];

                    switch (val)
                    {
                        case 0x010: comm.data.message_value.content.byte = buff_[9]; comm.checksum = buff_[10]; break;
                        case 0x011: comm.data.message_value.content.ch = buff_[9]; comm.checksum = buff_[10]; break;
                        case 0x020: comm.data.message_value.content.integer = makeWord(buff_[9], buff_[10]); comm.checksum = buff_[11]; break;
                        case 0x021: comm.data.message_value.content.sinteger = makeWord(buff_[9], buff_[10]); comm.checksum = buff_[11]; break;
                        case 0x040: comm.data.message_value.content.dword = makeDWord(buff_[9], buff_[10], buff_[11], buff_[12]); comm.checksum = buff_[13]; break;
                        case 0x041: comm.data.message_value.content.sdword = makeDWord(buff_[9], buff_[10], buff_[11], buff_[12]); comm.checksum = buff_[13]; break;

                        case 0x04f:
                            dw = makeDWord(buff_[9], buff_[10], buff_[11], buff_[12]);
                            memcpy(&comm.data.message_value.content.fvalue, &dw, 4);
                            comm.checksum = buff_[13];
                        break;

                        case 0x08f:
                            memcpy(&comm.data.message_value.content.dvalue, &buff_[9], 8);  // FIXME: wrong byte order on Intel CPU?
                            comm.checksum = buff_[17];
                        break;
                    }

                    if (callback)
                        callback(comm);
                    else
                        MSG_WARNING("Missing callback function!");
                break;

                case 0x000b:    // string value change
                case 0x008b:
                case 0x000c:    // command string
                case 0x008c:
                    comm.data.message_string.device = makeWord(buff_[0], buff_[1]);
                    comm.data.message_string.port = makeWord(buff_[2], buff_[3]);
                    comm.data.message_string.system = makeWord(buff_[4], buff_[5]);
                    comm.data.message_string.type = buff_[6];
                    comm.data.message_string.length = makeWord(buff_[7], buff_[8]);
                    memset(&comm.data.message_string.content[0], 0, sizeof(comm.data.message_string.content));
                    len = (buff_[6] == 0x01) ? comm.data.message_string.length : comm.data.message_string.length * 2;

                    if (len >= sizeof(comm.data.message_string.content))
                    {
                        len = sizeof(comm.data.message_string.content) - 1;
                        comm.data.message_string.length = (buff_[6] == 0x01) ? len : len / 2;
                    }

                    memcpy(&comm.data.message_string.content[0], &buff_[9], len);
                    pos = (int)(len + 10);
                    comm.checksum = buff_[pos];
                    cmd.assign((char *)&comm.data.message_string.content[0], len);
                    MSG_DEBUG("cmd=" << cmd);

                    if (isCommand(cmd))
                    {
                        MSG_DEBUG("Command found!");
                        oldCmd.assign(cmd);
                    }
                    else
                    {
                        oldCmd.append(cmd);
                        MSG_DEBUG("Concatenated cmd=" << oldCmd);
                        memset(&comm.data.message_string.content[0], 0, sizeof(comm.data.message_string.content));
                        memcpy(&comm.data.message_string.content[0], oldCmd.c_str(), sizeof(comm.data.message_string.content) - 1);
                        comm.data.message_string.length = oldCmd.length();
                        oldCmd.clear();
                    }

                    if (callback)
                        callback(comm);
                    else
                        MSG_WARNING("Missing callback function!");
                break;

                case 0x000e:    // request level value
                    comm.data.level.device = makeWord(buff_[0], buff_[1]);
                    comm.data.level.port = makeWord(buff_[2], buff_[3]);
                    comm.data.level.system = makeWord(buff_[4], buff_[5]);
                    comm.data.level.level = makeWord(buff_[6], buff_[7]);
                    comm.checksum = buff_[8];

                    if (callback)
                        callback(comm);
                    else
                        MSG_WARNING("Missing callback function!");
                break;

                case 0x000f:    // request output channel status
                    comm.data.channel.device = makeWord(buff_[0], buff_[1]);
                    comm.data.channel.port = makeWord(buff_[2], buff_[3]);
                    comm.data.channel.system = makeWord(buff_[4], buff_[5]);
                    comm.data.channel.channel = makeWord(buff_[6], buff_[7]);
                    comm.checksum = buff_[8];

                    if (callback)
                        callback(comm);
                    else
                        MSG_WARNING("Missing callback function!");
                break;

                case 0x0010:    // request port count
                case 0x0017:    // request device info
                    comm.data.reqPortCount.device = makeWord(buff_[0], buff_[1]);
                    comm.data.reqPortCount.system = makeWord(buff_[2], buff_[3]);
                    comm.checksum = buff_[4];
                    s.channel = false;
                    s.level = 0;
                    s.port = 0;
                    s.value = 0x0015;
                    s.MC = (comm.MC == 0x0010) ? 0x0090 : 0x0097;

                    if (s.MC == 0x0097)
                    {
                        comm.data.srDeviceInfo.device = comm.device2;
                        comm.data.srDeviceInfo.system = comm.system;
                        comm.data.srDeviceInfo.flag = 0x0000;
                        comm.data.srDeviceInfo.parentID = 0;
                        comm.data.srDeviceInfo.herstID = 1;
                        msg97fill(&comm);
                    }
                    else
                        sendCommand(s);
                break;

                case 0x0011:    // request output channel count
                case 0x0012:    // request level count
                case 0x0013:    // request string size
                case 0x0014:    // request command size
                    comm.data.reqOutpChannels.device = makeWord(buff_[0], buff_[1]);
                    comm.data.reqOutpChannels.port = makeWord(buff_[2], buff_[3]);
                    comm.data.reqOutpChannels.system = makeWord(buff_[4], buff_[5]);
                    comm.checksum = buff_[6];
                    s.channel = false;
                    s.level = 0;
                    s.port = comm.data.reqOutpChannels.port;
                    s.value = 0;

                    switch (comm.MC)
                    {
                        case 0x0011:
                            s.MC = 0x0091;
                            s.value = 0x0f75;   // # channels
                        break;

                        case 0x0012:
                            s.MC = 0x0092;
                            s.value = 0x000d;   // # levels
                        break;

                        case 0x0013:
                            s.MC = 0x0093;
                            s.value = 0x00c7;   // string size
                        break;

                        case 0x0014:
                            s.MC = 0x0094;
                            s.value = 0x00c7;   // command size
                        break;
                    }

                    sendCommand(s);
                break;

                case 0x0015:    // request level size
                    comm.data.reqLevels.device = makeWord(buff_[0], buff_[1]);
                    comm.data.reqLevels.port = makeWord(buff_[2], buff_[3]);
                    comm.data.reqLevels.system = makeWord(buff_[4], buff_[5]);
                    comm.data.reqLevels.level = makeWord(buff_[6], buff_[7]);
                    comm.checksum = buff_[8];
                    s.channel = false;
                    s.level = comm.data.reqLevels.level;
                    s.port = comm.data.reqLevels.port;
                    s.value = 0;
                    s.MC = 0x0095;
                    sendCommand(s);
                break;

                case 0x0016:    // request status code
                    comm.data.sendStatusCode.device = makeWord(buff_[0], buff_[1]);
                    comm.data.sendStatusCode.port = makeWord(buff_[2], buff_[3]);
                    comm.data.sendStatusCode.system = makeWord(buff_[4], buff_[5]);

                    if (callback)
                        callback(comm);
                    else
                        MSG_WARNING("Missing callback function!");
                break;

                case 0x0097:    // receive device info
                    comm.data.srDeviceInfo.device = makeWord(buff_[0], buff_[1]);
                    comm.data.srDeviceInfo.system = makeWord(buff_[2], buff_[3]);
                    comm.data.srDeviceInfo.flag = makeWord(buff_[4], buff_[5]);
                    comm.data.srDeviceInfo.objectID = buff_[6];
                    comm.data.srDeviceInfo.parentID = buff_[7];
                    comm.data.srDeviceInfo.herstID = makeWord(buff_[8], buff_[9]);
                    comm.data.srDeviceInfo.deviceID = makeWord(buff_[10], buff_[11]);
                    memcpy(comm.data.srDeviceInfo.serial, &buff_[12], 16);
                    comm.data.srDeviceInfo.fwid = makeWord(buff_[28], buff_[29]);
                    memset(comm.data.srDeviceInfo.info, 0, sizeof(comm.data.srDeviceInfo.info));
                    memcpy(comm.data.srDeviceInfo.info, &buff_[30], comm.hlen - 0x0015 - 29);
                    comm.checksum = buff_[comm.hlen + 3];
                    // Prepare answer
                    s.channel = false;
                    s.level = 0;
                    s.port = 0;
                    s.value = 0;

                    if (!initSend)
                    {
                        s.MC = 0x0097;
                        initSend = true;
                    }
                    else if (!ready)
                    {
                        // Send counts
                        s.MC = 0x0090;
                        s.value = 0x0015;   // # ports
                        sendCommand(s);
                        s.MC = 0x0091;
                        s.value = 0x0f75;   // # channels
                        sendCommand(s);
                        s.MC = 0x0092;
                        s.value = 0x000d;   // # levels
                        sendCommand(s);
                        s.MC = 0x0093;
                        s.value = 0x00c7;   // string size
                        sendCommand(s);
                        s.MC = 0x0094;
                        s.value = 0x00c7;   // command size
                        sendCommand(s);
                        s.MC = 0x0098;
                        ready = true;
                    }
                    else
                        break;

                    sendCommand(s);

                    MSG_DEBUG("S/N: " << comm.data.srDeviceInfo.serial << " | " << comm.data.srDeviceInfo.info);
                break;

                case 0x00a1:    // request status
                    reqDevStatus = makeWord(buff_[0], buff_[1]);
                    comm.checksum = buff_[2];
                break;

                case 0x0204:    // file transfer
                    s.device = comm.device2;
                    comm.data.filetransfer.ftype = makeWord(buff_[0], buff_[1]);
                    comm.data.filetransfer.function = makeWord(buff_[2], buff_[3]);
                    pos = 4;

                    if (comm.data.filetransfer.ftype == 0 && comm.data.filetransfer.function == 0x0105)         // Directory exist?
                    {
                        for (size_t i = 0; i < 0x0104; i++)
                        {
                            comm.data.filetransfer.data[i] = buff_[pos];
                            pos++;
                        }

                        comm.data.filetransfer.data[0x0103] = 0;
                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0100)    // Controller have more files
                        handleFTransfer(s, comm.data.filetransfer);
                    else if (comm.data.filetransfer.ftype == 0 && comm.data.filetransfer.function == 0x0100)    // Request directory listing
                    {
                        comm.data.filetransfer.unk = makeWord(buff_[4], buff_[5]);
                        pos = 6;

                        for (size_t i = 0; i < 0x0104; i++)
                        {
                            comm.data.filetransfer.data[i] = buff_[pos];
                            pos++;
                        }

                        comm.data.filetransfer.data[0x0103] = 0;
                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0102)    // controller will send a file
                    {
                        comm.data.filetransfer.unk = makeDWord(buff_[4], buff_[5], buff_[6], buff_[7]);
                        comm.data.filetransfer.unk1 = makeDWord(buff_[8], buff_[9], buff_[10], buff_[11]);
                        pos = 12;

                        for (size_t i = 0; i < 0x0104; i++)
                        {
                            comm.data.filetransfer.data[i] = buff_[pos];
                            pos++;
                        }

                        comm.data.filetransfer.data[0x0103] = 0;
                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0103)    // file or part of a file
                    {
                        comm.data.filetransfer.unk = makeWord(buff_[4], buff_[5]);
                        pos = 6;

                        for (size_t i = 0; i < comm.data.filetransfer.unk; i++)
                        {
                            comm.data.filetransfer.data[i] = buff_[pos];
                            pos++;
                        }

                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 0 && comm.data.filetransfer.function == 0x0104)    // Does file exist;
                    {
                        for (size_t i = 0; i < 0x0104; i++)
                        {
                            comm.data.filetransfer.data[i] = buff_[pos];
                            pos++;
                        }

                        comm.data.filetransfer.data[0x0103] = 0;
                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0104)    // request a file
                    {
                        comm.data.filetransfer.unk = makeWord(buff_[4], buff_[5]);
                        pos = 6;

                        for (size_t i = 0; i < 0x0104; i++)
                        {
                            comm.data.filetransfer.data[i] = buff_[pos];
                            pos++;
                        }

                        comm.data.filetransfer.data[0x0103] = 0;
                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0106)    // ACK for 0x0105
                    {
                        comm.data.filetransfer.unk = makeDWord(buff_[4], buff_[5], buff_[6], buff_[7]);
                        pos = 8;
                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0002)    // request next part of file
                        handleFTransfer(s, comm.data.filetransfer);
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0003)    // File content from controller
                    {
                        comm.data.filetransfer.unk = makeWord(buff_[4], buff_[5]);  // length of data block
                        pos = 6;

                        for (size_t i = 0; i < comm.data.filetransfer.unk; i++)
                        {
                            comm.data.filetransfer.data[i] = buff_[pos];
                            pos++;
                        }

                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0004)    // End of file
                        handleFTransfer(s, comm.data.filetransfer);
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0005)    // End of file ACK
                        handleFTransfer(s, comm.data.filetransfer);
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0006)    // End of directory listing ACK
                    {
                        comm.data.filetransfer.unk = makeWord(buff_[4], buff_[5]);  // length of received data block
                        pos = 6;
                        handleFTransfer(s, comm.data.filetransfer);
                    }
                    else if (comm.data.filetransfer.ftype == 4 && comm.data.filetransfer.function == 0x0007)    // End of file transfer
                        handleFTransfer(s, comm.data.filetransfer);

                break;

                case 0x0501:    // ping
                    comm.data.chan_state.device = makeWord(buff_[0], buff_[1]);
                    comm.data.chan_state.system = makeWord(buff_[2], buff_[3]);
                    s.channel = 0;
                    s.level = 0;
                    s.port = 0;
                    s.value = 0;
                    s.MC = 0x0581;
                    sendCommand(s);
                break;

                case 0x0502:    // Date and time
                    comm.data.blinkMessage.heartBeat = buff_[0];
                    comm.data.blinkMessage.LED = buff_[1];
                    comm.data.blinkMessage.month = buff_[2];
                    comm.data.blinkMessage.day = buff_[3];
                    comm.data.blinkMessage.year = makeWord(buff_[4], buff_[5]);
                    comm.data.blinkMessage.hour = buff_[6];
                    comm.data.blinkMessage.minute = buff_[7];
                    comm.data.blinkMessage.second = buff_[8];
                    comm.data.blinkMessage.weekday = buff_[9];
                    comm.data.blinkMessage.extTemp = makeWord(buff_[10], buff_[11]);
                    memset(comm.data.blinkMessage.dateTime, 0, sizeof(comm.data.blinkMessage.dateTime));
                    memcpy(comm.data.blinkMessage.dateTime, &buff_[12], comm.hlen - 0x0015 - 11);
                    comm.checksum = buff_[comm.hlen + 3];

                    sendAllFuncTimer(comm.data.blinkMessage);
                    sendAllFuncNetwork(mLastOnlineState == NSTATE_ONLINE ? NSTATE_ONLINE1 : NSTATE_ONLINE);
                break;
            }
        break;

        default:        // Every unknown or unsupported command/request should be ignored.
            ignore = true;
    }
}

bool TAmxNet::sendCommand(const ANET_SEND& s)
{
    DECL_TRACER("TAmxNet::sendCommand (const ANET_SEND& s)");

    size_t len, size;
    ANET_COMMAND com;
    com.clear();
    com.MC = s.MC;

    if (s.MC == 0x0204)     // file transfer
        com.device1 = s.device;
    else
        com.device1 = 0;

    com.device2 = panelID;
    com.port1 = 1;
    com.system = TConfig::getSystem();
    com.port2 = s.port;
    sendCounter++;
    com.count = sendCounter;

    switch (s.MC)
    {
        case 0x0084:        // push button
            com.data.channel.device = com.device2;
            com.data.channel.port = s.port;
            com.data.channel.system = com.system;
            com.data.channel.channel = s.channel;
            com.hlen = 0x0016 - 0x0003 + sizeof(ANET_CHANNEL);
            MSG_DEBUG("SEND: BUTTON PUSH-" << s.channel << ":" << s.port << ":" << com.device2);
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0085:        // release button
            com.data.channel.device = com.device2;
            com.data.channel.port = s.port;
            com.data.channel.system = com.system;
            com.data.channel.channel = s.channel;
            com.hlen = 0x0016 - 0x0003 + sizeof(ANET_CHANNEL);
            MSG_DEBUG("SEND: BUTTON RELEASE-" << s.channel << ":" << s.port << ":" << com.device2);
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0086:    // output channel on
        case 0x0088:    // feedback/input channel on
            com.data.channel.device = com.device2;
            com.data.channel.port = s.port;
            com.data.channel.system = com.system;
            com.data.channel.channel = s.channel;
            com.hlen = 0x0016 - 0x0003 + sizeof(ANET_CHANNEL);
            MSG_DEBUG("SEND: CHANNEL ON-" << s.channel << ":" << s.port << ":" << com.device2);
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0087:    // output channel off
        case 0x0089:    // feedback/input channel off
            com.data.channel.device = com.device2;
            com.data.channel.port = s.port;
            com.data.channel.system = com.system;
            com.data.channel.channel = s.channel;
            com.hlen = 0x0016 - 0x0003 + sizeof(ANET_CHANNEL);
            MSG_DEBUG("SEND: CHANNEL OFF-" << s.channel << ":" << s.port << ":" << com.device2);
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x008a:        // level value changed
            if (gPageManager && gPageManager->getLevelSendState())
            {
                com.data.message_value.device = com.device2;
                com.data.message_value.port = s.port;
                com.data.message_value.system = com.system;
                com.data.message_value.value = s.level;
                com.data.message_value.type = DTSZ_UINT;     // unsigned integer
                com.data.message_value.content.integer = s.value;
                com.hlen = 0x0016 - 0x0003 + 11;
                MSG_DEBUG("SEND: LEVEL-" << s.value << "," << s.level << ":" << s.port << ":" << com.device2);
                comStack.push_back(com);
                mSendReady = true;
            }
        break;

        case 0x008b:        // string command
        case 0x008c:        // send command string
            if (gPageManager && gPageManager->getRxSendState())
            {
                com.data.message_string.device = com.device2;
                com.data.message_string.port = s.port;
                com.data.message_string.system = com.system;
                com.data.message_string.type = DTSZ_STRING;    // char string

                if (s.msg.length() >= sizeof(com.data.message_string.content))
                    len = sizeof(com.data.message_string.content) - 1;
                else
                    len = s.msg.length();

                com.data.message_string.length = len;
                strncpy((char *)&com.data.message_string.content[0], s.msg.c_str(), len);
                com.hlen = 0x0016 - 3 + 9 + len;

                if (s.MC == 0x008b)
                {
                    MSG_DEBUG("SEND: STRING-'" << s.msg << "'," << s.port << ":" << com.device2);
                }
                else
                {
                    MSG_DEBUG("SEND: COMMAND-'" << s.msg << "'," << s.port << ":" << com.device2);
                }

                comStack.push_back(com);
                mSendReady = true;
            }
        break;

        case 0x008d:    // Custom event
            com.data.customEvent.device = com.device2;
            com.data.customEvent.port = s.port;
            com.data.customEvent.system = com.system;
            com.data.customEvent.ID = s.ID;
            com.data.customEvent.type = s.type;
            com.data.customEvent.flag = s.flag;
            com.data.customEvent.value1 = s.value1;
            com.data.customEvent.value2 = s.value2;
            com.data.customEvent.value3 = s.value3;
            com.data.customEvent.dtype = s.dtype;

            if (s.msg.length() >= sizeof(com.data.customEvent.data))
                len = sizeof(com.data.customEvent.data) - 1;
            else
                len = s.msg.length();

            com.data.customEvent.length = len;
            memset(com.data.customEvent.data, 0, sizeof(com.data.customEvent.data));

            if (len > 0)
                memcpy(&com.data.customEvent.data[0], s.msg.c_str(), len);

            com.hlen = 0x0016 - 3 + 29 + len;
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0090:        // port count
            com.data.sendPortNumber.device = com.device2;
            com.data.sendPortNumber.system = com.system;
            com.data.sendPortNumber.pcount = s.value;
            com.hlen = 0x0016 - 3 + 6;
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0091:        // output channel count
        case 0x0092:        // send level count
            com.data.sendOutpChannels.device = com.device2;
            com.data.sendOutpChannels.port = s.port;
            com.data.sendOutpChannels.system = com.system;
            com.data.sendOutpChannels.count = s.value;
            com.hlen = 0x0016 - 3 + 8;
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0093:        // string size
        case 0x0094:        // command size
            com.data.sendSize.device = com.device2;
            com.data.sendSize.port = s.port;
            com.data.sendSize.system = com.system;
            com.data.sendSize.type = DTSZ_STRING;
            com.data.sendSize.length = s.value;
            com.hlen = 0x0016 - 3 + 9;
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0095:        // suported level types
            com.data.sendLevSupport.device = com.device2;
            com.data.sendLevSupport.port = s.port;
            com.data.sendLevSupport.system = com.system;
            com.data.sendLevSupport.level = s.level;
            com.data.sendLevSupport.num = 6;
            com.data.sendLevSupport.types[0] = 0x10;
            com.data.sendLevSupport.types[1] = 0x11;
            com.data.sendLevSupport.types[2] = 0x20;
            com.data.sendLevSupport.types[3] = 0x21;
            com.data.sendLevSupport.types[4] = 0x40;
            com.data.sendLevSupport.types[5] = 0x41;
            com.hlen = 0x0016 - 0x0003 + sizeof(ANET_LEVSUPPORT);
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0096:        // Status code
            com.data.sendStatusCode.device = com.device2;
            com.data.sendStatusCode.port = s.port;
            com.data.sendStatusCode.system = com.system;
            com.data.sendStatusCode.status = 0;
            com.data.sendStatusCode.type = DTSZ_CHAR;
            com.data.sendStatusCode.length = 2;
            com.data.sendStatusCode.str[0] = 'O';
            com.data.sendStatusCode.str[1] = 'K';
            com.hlen = 0x0016 - 3 + 13;
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0097:        // device info
            com.data.srDeviceInfo.device = com.device2;
            com.data.srDeviceInfo.system = com.system;
            com.data.srDeviceInfo.flag = 0x0000;
            com.data.srDeviceInfo.objectID = 0;
            com.data.srDeviceInfo.parentID = 0;
            com.data.srDeviceInfo.herstID = 1;
            msg97fill(&com);
            mSendReady = true;
        break;

        case 0x0098:
            com.data.reqPortCount.device = com.device2;
            com.data.reqPortCount.system = com.system;
            com.hlen = 0x0016 - 3 + 4;
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0204:        // File transfer
            com.port1 = 0;
            com.port2 = 0;
            com.data.filetransfer.ftype = s.dtype;
            com.data.filetransfer.function = s.type;
            com.data.filetransfer.info1 = s.value;
            com.data.filetransfer.info2 = s.level;
            com.data.filetransfer.unk = s.value1;
            com.data.filetransfer.unk1 = s.value2;
            com.data.filetransfer.unk2 = s.value3;
            size = min(s.msg.length(), sizeof(com.data.filetransfer.data) - 1);
            memcpy(com.data.filetransfer.data, s.msg.c_str(), size);
            com.data.filetransfer.data[size] = 0;
            len = 4;

            if (s.dtype == 0)
            {
                switch (s.type)
                {
                    case 0x0001: len += 2; break;
                    case 0x0101: len += 16 + size + 1; break;
                    case 0x0102: len += 19 + size + 1; break;
                }
            }
            else
            {
                switch (s.type)
                {
                    case 0x0003: len += 2 + s.value1; break;
                    case 0x0101: len += 8; break;
                    case 0x0103: len += 6; break;
                    case 0x0105: len += 8; break;
                }
            }

            com.hlen = 0x0016 - 3 + len;
            comStack.push_back(com);
            mSendReady = true;
        break;

        case 0x0581:        // Pong
            com.data.srDeviceInfo.device = panelID; // Configuration->getAMXChannel();
            com.data.srDeviceInfo.system = TConfig::getSystem();
            com.data.srDeviceInfo.herstID = devInfo[0].manufacturerID;
            com.data.srDeviceInfo.deviceID = devInfo[0].deviceID;
            com.data.srDeviceInfo.info[0] = 2;  // Type: IPv4 address
            com.data.srDeviceInfo.info[1] = 4;  // length of following data

            {
                string addr = mSocket->getMyIP();
                vector<string> parts = StrSplit(addr, ".");

                for (size_t i = 0; i < parts.size(); i++)
                    com.data.srDeviceInfo.info[i + 2] = (unsigned char)atoi(parts[i].c_str());
            }

            com.hlen = 0x0016 - 3 + 14;
            comStack.push_back(com);
            mSendReady = true;
        break;
    }

    if (mSendReady && !write_busy)
        runWrite();

    return mSendReady;
}

void TAmxNet::handleFTransfer(ANET_SEND &s, ANET_FILETRANSFER &ft)
{
    DECL_TRACER("TAmxNet::handleFTransfer (ANET_SEND &s, ANET_FILETRANSFER &ft)");

    int len;
    ANET_COMMAND ftr;
    ftr.MC = 0x1000;
    ftr.device1 = s.device;
    ftr.device2 = s.device;
    ftr.port1 = 0;
    ftr.port2 = 0;
    ftr.count = 0;
    ftr.data.filetransfer.ftype = ft.ftype;
    ftr.data.filetransfer.function = ft.function;
    ftr.data.filetransfer.data[0] = 0;

    if (ft.ftype == 0 && ft.function == 0x0105)     // Create directory
    {
        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 0;                // ftype --> function type
        s.type = 0x0001;            // function
        s.value1 = 0;               // 1st data byte 0x00
        s.value2 = 0x10;            // 2nd data byte 0x10
        string f((char *)&ft.data);
        MSG_DEBUG("0x0000/0x0105: Directory " << f << " exist?");
        string prjPath = TConfig::getProjectPath();
        string newPath;
        dir::TDirectory dir;

        if (f.compare(0, 8, "AMXPanel") == 0)
        {
            if (f.find("/images") > 0)
            {
                newPath = prjPath + "/images";
                dir.createAllPath(newPath);
            }
            else if (f.find("/sounds") > 0)
            {
                newPath = prjPath + "/sounds";
                dir.createAllPath(newPath);
            }
            else if (f.find("/fonts") > 0)
            {
                newPath = prjPath + "/fonts";
                dir.createAllPath(newPath);
            }
        }
        else if (f.compare(0, 8, "__system") == 0)
        {
            vector<string> subDirs = { "borders", "cursors", "fonts", "images", "sliders", "sounds" };
            vector<string>::iterator iter;

            for (iter = subDirs.begin(); iter != subDirs.end(); ++iter)
            {
                newPath = prjPath + "/__system/graphics/" + *iter;
                dir.createAllPath(newPath);
            }
        }

        sendCommand(s);

        if (!receiveSetup)
        {
            receiveSetup = true;
            ftransfer.maxFiles = countFiles();

            if (callback)
                callback(ftr);
            else
                MSG_WARNING("Missing callback function!");
        }
    }
    else if (ft.ftype == 0 && ft.function == 0x0100)    // Request directory
    {
        string fname((char *)&ft.data);
        string amxpath(fname);
        string realPath;
        size_t pos = 0;
        len = 0;
        dir::TDirectory dr;

        if (fname.compare("AMXPanel/") == 0)
        {
            realPath.assign(TConfig::getProjectPath());
            amxpath.assign("/opt/amx/user/AMXPanel");
        }
        else if ((pos = fname.find("AMXPanel/")) != string::npos)
        {
            if (pos == 0)
                amxpath = "/opt/amx/user/" + fname;

            realPath = dr.stripPath("AMXPanel", fname);
            realPath = TConfig::getProjectPath() + "/" + realPath;

            if (dr.isFile(realPath))
                len = dr.getFileSize(realPath);
        }

        MSG_DEBUG("0x0000/0x0100: Request directory " << fname);
        snprintf((char *)&ftr.data.filetransfer.data[0], sizeof(ftr.data.filetransfer.data), "Syncing %d files ...", ftransfer.maxFiles);

        if (callback)
            callback(ftr);
        else
            MSG_WARNING("Missing callback function!");

        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 0x0000;
        s.type = 0x0101;
        s.value1 = len;     // File length
        s.value2 = 0x0000be42;
        s.value3 = 0x00003e75;
        s.msg = amxpath;
        sendCommand(s);
        // Read the directory tree
        dr.setStripPath(true);
        dr.readDir(realPath);
        amxpath = fname;

        if (amxpath.length() > 1 && amxpath.at(amxpath.length() - 1) == '/')
            amxpath = amxpath.substr(0, amxpath.length() - 1);

        for (pos = 0; pos < dr.getNumEntries(); pos++)
        {
            dir::DFILES_T df = dr.getEntry(pos);
            s.type = 0x0102;

            s.value = (dr.testDirectory(df.attr)) ? 1 : 0;  // Depends on type of entry
            s.level = dr.getNumEntries();       // # entries
            s.value1 = df.count;                // counter
            s.value2 = df.size;                 // Size of file
            s.value3 = df.date;                 // Last modification date (epoch)
            s.msg.assign(amxpath + "/" + df.name);
            sendCommand(s);
        }

        if (dr.getNumEntries() == 0)
        {
            s.type = 0x0102;

            s.value = 0;
            s.level = 0;       // # entries
            s.value1 = 0;                // counter
            s.value2 = 0;                 // Size of file
            s.value3 = 0;                 // Last modification date (epoch)
            s.msg.assign(amxpath + "/");
            sendCommand(s);
        }
    }
    else if (ft.ftype == 4 && ft.function == 0x0100)    // Have more files to send.
    {
        MSG_DEBUG("0x0004/0x0100: Have more files to send.");
        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 4;                // ftype --> function type
        s.type = 0x0101;            // function:
        s.value1 = 0x01bb3000;      // ?
        s.value2 = 0;               // ?
        sendCommand(s);
    }
    else if (ft.ftype == 4 && ft.function == 0x0102)    // Controller will send a file
    {
        string f((char*)&ft.data);
        size_t pos;
        rcvFileName.assign(TConfig::getProjectPath());

        if (f.find("AMXPanel") != string::npos)
        {
            pos = f.find_first_of("/");
            rcvFileName.append(f.substr(pos));
        }
        else
        {
            rcvFileName.append("/");
            rcvFileName.append((char*)&ft.data);
        }

        if (rcvFile != nullptr)
        {
            fclose(rcvFile);
            rcvFile = nullptr;
            isOpenRcv = false;
        }

        // The file name is encoded as CP1250 (Windows). Because we use UTF-8 we
        // must convert the file name to get rid of non ASCII characters.
        rcvFileName = cp1250ToUTF8(rcvFileName);
        dir::TDirectory dr;

        if (!dr.exists(rcvFileName))
            dr.createAllPath(rcvFileName, true);
        else
            dr.drop(rcvFileName);

        rcvFile = fopen(rcvFileName.c_str(), "w+");

        if (!rcvFile)
        {
            MSG_ERROR("Error creating file " << rcvFileName << " (" << strerror(errno) << ")");
            isOpenRcv = false;
        }
        else
        {
            isOpenRcv = true;

            if (!TStreamError::checkFilter(HLOG_TRACE))
            {
                MSG_INFO("Writing file: " << rcvFileName);
            }
        }

        MSG_DEBUG("0x0004/0x0102: Controller will send file " << rcvFileName);

        ftransfer.actFileNum++;
        ftransfer.lengthFile = ft.unk;

        if (ftransfer.actFileNum > ftransfer.maxFiles)
            ftransfer.maxFiles = ftransfer.actFileNum;

        ftransfer.percent = (int)(100.0 / (double)ftransfer.maxFiles * (double)ftransfer.actFileNum);
        pos = rcvFileName.find_last_of("/");
        string shfn;

        if (pos != string::npos)
            shfn = rcvFileName.substr(pos + 1);
        else
            shfn = rcvFileName;

        snprintf((char*)&ftr.data.filetransfer.data[0], sizeof(ftr.data.filetransfer.data), "[%d/%d] %s", ftransfer.actFileNum, ftransfer.maxFiles, shfn.c_str());
        ftr.count = ftransfer.percent;
        ftr.data.filetransfer.info1 = 0;

        if (callback)
            callback(ftr);
        else
            MSG_WARNING("Missing callback function!");

        posRcv = 0;
        lenRcv = ft.unk;
        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 4;                // ftype --> function type
        s.type = 0x0103;            // function: ready for receiving file
        s.value1 = MAX_CHUNK;       // Maximum length of a chunk
        s.value2 = ft.unk1;         // ID?
        sendCommand(s);
    }
    else if (ft.ftype == 0 && ft.function == 0x0104)    // Delete file <name>
    {
        dir::TDirectory dr;
        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        string f((char*)&ft.data[0]);
        size_t pos = 0;

        if ((pos = f.find("AMXPanel/")) == string::npos)
            pos = f.find("__system/");

        MSG_DEBUG("0x0000/0x0104: Delete file " << f);

        if (pos != string::npos)
            f = TConfig::getProjectPath() + "/" + f.substr(pos + 9);
        else
            f = TConfig::getProjectPath() + "/" + f;

        if (dr.exists(f))
        {
            s.dtype = 0;                // ftype --> function type
            s.type = 0x0002;            // function: yes file exists
            remove(f.c_str());
        }
        else    // Send: file was deleted although it does not exist.
        {
            MSG_ERROR("[DELETE] File " << f << " not found!");
            s.dtype = 0;                // ftype --> function type
            s.type = 0x0002;            // function: yes file exists
        }

        sendCommand(s);

        if (ftransfer.actDelFile == 0)
        {
            ftransfer.actDelFile++;
            ftransfer.percent = (int)(100.0 / (double)ftransfer.maxFiles * (double)ftransfer.actDelFile);
            ftr.count = ftransfer.percent;

            if (callback)
                callback(ftr);
            else
                MSG_WARNING("Missing callback function!");
        }
        else
        {
            ftransfer.actDelFile++;
            int prc = (int)(100.0 / (double)ftransfer.maxFiles * (double)ftransfer.actDelFile);

            if (prc != ftransfer.percent)
            {
                ftransfer.percent = prc;
                ftr.count = prc;

                if (callback)
                    callback(ftr);
                else
                    MSG_WARNING("Missing callback function!");
            }
        }
    }
    else if (ft.ftype == 4 && ft.function == 0x0104)    // request a file
    {
        string f((char*)&ft.data);
        size_t pos;
        len = 0;
        sndFileName.assign(TConfig::getProjectPath());
        MSG_DEBUG("0x0004/0x0104: Request file " << f);

        if (f.find("AMXPanel") != string::npos)
        {
            pos = f.find_first_of("/");
            sndFileName.append(f.substr(pos));
        }
        else
        {
            sndFileName.append("/");
            sndFileName.append(f);
        }

        if (!access(sndFileName.c_str(), R_OK))
        {
            struct stat s;

            if (stat(sndFileName.c_str(), &s) == 0)
                len = s.st_size;
            else
                len = 0;
        }
        else if (sndFileName.find("/version.xma") > 0)
            len = 0x0015;
        else
            len = 0;

        MSG_DEBUG("0x0004/0x0104: (" << len << ") File: " << sndFileName);

        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 4;                // ftype --> function type
        s.type = 0x0105;            // function
        s.value1 = len;             // length of file to send
        s.value2 = 0x00001388;      // ID for device when sending a file.
        sendCommand(s);
    }
    else if (ft.ftype == 4 && ft.function == 0x0106)    // Controller is ready for receiving file
    {
        MSG_DEBUG("0x0004/0x0106: Controller is ready for receiving file.");

        if (!access(sndFileName.c_str(), R_OK))
        {
            struct stat st;
            stat(sndFileName.c_str(), &st);
            len = st.st_size;
            lenSnd = len;
            posSnd = 0;
            sndFile = fopen(sndFileName.c_str(), "r");

            if (!sndFile)
            {
                MSG_ERROR("Error reading file " << sndFileName);
                len = 0;
                isOpenSnd = false;
            }
            else
                isOpenSnd = true;

            if (isOpenSnd && len <= MAX_CHUNK)
            {
                char *buf = new char[len + 1];
                fread(buf, 1, len, sndFile);
                s.msg.assign(buf, len);
                delete[] buf;
                posSnd = len;
            }
            else if (isOpenSnd)
            {
                char *buf = new char[MAX_CHUNK + 1];
                fread(buf, 1, MAX_CHUNK, sndFile);
                s.msg.assign(buf, MAX_CHUNK);
                delete[] buf;
                posSnd = MAX_CHUNK;
                len = MAX_CHUNK;
            }
        }
        else if (sndFileName.find("/version.xma") > 0)
        {
            s.msg.assign("<version>9</version>\n");
            len = s.msg.length();
            posSnd = len;
        }
        else
            len = 0;

        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 4;                // ftype --> function type
        s.type = 0x0003;            // function: Sending file with length <len>
        s.value1 = len;             // length of content to send
        sendCommand(s);
    }
    else if (ft.ftype == 4 && ft.function == 0x0002)    // request next part of file
    {
        MSG_DEBUG("0x0004/0x0002: Request next part of file.");
        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 4;                // ftype --> function type

        if (posSnd < lenSnd)
        {
            s.type = 0x0003;        // Next part of file

            if ((posSnd + MAX_CHUNK) > lenSnd)
                len = lenSnd - posSnd;
            else
                len = MAX_CHUNK;

            s.value1 = len;

            if (isOpenSnd)
            {
                char *buf = new char[len + 1];
                fread(buf, 1, len, sndFile);
                s.msg.assign(buf, len);
                delete[] buf;
                posSnd += len;
            }
            else
                s.value1 = 0;
        }
        else
            s.type = 0x0004;        // function: End of file reached

        sendCommand(s);
    }
    else if (ft.ftype == 4 && ft.function == 0x0003)    // File content
    {
        MSG_DEBUG("0x0004/0x0003: Received (part of) file.");
        len = ft.unk;

        if (isOpenRcv)
        {
            fwrite(ft.data, 1, len, rcvFile);
            posRcv += ft.unk;
        }
        else
            MSG_WARNING("No open file to write to! (" << rcvFileName << ")");

        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 4;                // ftype --> function type
        s.type = 0x0002;            // function: Request next part of file
        sendCommand(s);

        int prc = (int)(100.0 / (double)ftransfer.lengthFile * (double)posRcv);

        if (prc != ftr.data.filetransfer.info1)
        {
            ftr.data.filetransfer.info1 = (int)(100.0 / (double)ftransfer.lengthFile * (double)posRcv);
            ftr.count = ftransfer.percent;

            if (callback)
                callback(ftr);
            else
                MSG_WARNING("Missing callback function!");
        }
    }
    else if (ft.ftype == 4 && ft.function == 0x0004)    // End of file
    {
        MSG_DEBUG("0x0004/0x0004: End of file.");

        if (isOpenRcv)
        {
            unsigned char buf[8];
            fseek(rcvFile, 0, SEEK_SET);
            fread(buf, 1, sizeof(buf), rcvFile);
            fclose(rcvFile);
            isOpenRcv = false;
            rcvFile = nullptr;
            posRcv = 0;

            if (buf[0] == 0x1f && buf[1] == 0x8b)   // GNUzip compressed?
            {
                TExpand exp(rcvFileName);
                exp.unzip();
            }
        }

        ftr.count = ftransfer.percent;
        ftr.data.filetransfer.info1 = 100;

        if (callback)
            callback(ftr);
        else
            MSG_WARNING("Missing callback functiom!");

        s.channel = 0;
        s.level = 0;
        s.port = 0;
        s.value = 0;
        s.MC = 0x0204;
        s.dtype = 4;                // ftype --> function type
        s.type = 0x0005;            // function: ACK, file received. No answer expected.
        sendCommand(s);
    }
    else if (ft.ftype == 4 && ft.function == 0x0005)    // ACK, controller received file, no answer
    {
        MSG_DEBUG("0x0004/0x0005: Controller received file.");
        posSnd = 0;
        lenSnd = 0;

        if (isOpenSnd && sndFile != nullptr)
            fclose(sndFile);

        ftransfer.lengthFile = 0;
        sndFile = nullptr;
    }
    else if (ft.ftype == 4 && ft.function == 0x0006)    // End of directory transfer ACK
    {
        MSG_DEBUG("0x0004/0x0006: End of directory transfer.");
    }
    else if (ft.ftype == 4 && ft.function == 0x0007)    // End of file transfer
    {
        MSG_DEBUG("0x0004/0x0007: End of file transfer.");

        if (callback)
            callback(ftr);
        else
            MSG_WARNING("Missing callback function!");

        receiveSetup = false;
    }
}

int TAmxNet::msg97fill(ANET_COMMAND *com)
{
    DECL_TRACER("TAmxNet::msg97fill(ANET_COMMAND *com)");

    int pos = 0;
    unsigned char buf[512];

    for (size_t i = 0; i < devInfo.size(); i++)
    {
        pos = 0;

        if (i == 0)
            com->sep1 = 0x12;
        else
            com->sep1 = 0x02;

        memset(buf, 0, sizeof(buf));
        com->data.srDeviceInfo.objectID = devInfo[i].objectID;
        com->data.srDeviceInfo.parentID = devInfo[i].parentID;
        com->data.srDeviceInfo.herstID = devInfo[i].manufacturerID;
        com->data.srDeviceInfo.deviceID = devInfo[i].deviceID;
        memcpy(com->data.srDeviceInfo.serial, devInfo[i].serialNum, 16);
        com->data.srDeviceInfo.fwid = devInfo[i].firmwareID;
        memcpy(buf, devInfo[i].versionInfo, strlen(devInfo[i].versionInfo));
        pos = (int)strlen(devInfo[i].versionInfo) + 1;
        memcpy(buf + pos, devInfo[i].deviceInfo, strlen(devInfo[i].deviceInfo));
        pos += strlen(devInfo[i].deviceInfo) + 1;
        memcpy(buf + pos, devInfo[i].manufacturerInfo, strlen(devInfo[i].manufacturerInfo));
        pos += strlen(devInfo[i].manufacturerInfo) + 1;
        *(buf + pos) = 0x02; // type IP address
        pos++;
        *(buf + pos) = 0x04; // field length: 4 bytes
        // Now the IP Address
        string addr = mSocket->getMyIP();
        vector<string> parts = StrSplit(addr, ".");

        for (size_t i = 0; i < parts.size(); i++)
        {
            pos++;
            *(buf + pos) = (unsigned char)atoi(parts[i].c_str());
        }

        pos++;
        com->data.srDeviceInfo.len = pos;
        memcpy(com->data.srDeviceInfo.info, buf, pos);
        com->hlen = 0x0016 - 3 + 31 + pos - 1;
        comStack.push_back(*com);
        sendCounter++;
        com->count = sendCounter;
    }

    return pos;
}

void TAmxNet::runWrite()
{
    DECL_TRACER("TAmxNet::runWrite()");

    if (write_busy)
        return;

    try
    {
        mWriteThread = std::thread([=] { this->start_write(); });
        mWriteThread.detach();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting write thread: " << e.what());
        _netRunning = false;
    }
}

void TAmxNet::start_write()
{
    DECL_TRACER("TAmxNet::start_write()");

    if (!__CommValid || !mSocket || !isRunning() || !mSocket->isConnected())
        return;

    if (write_busy)
        return;

    write_busy = true;

    while (write_busy && !_restart_ && !killed && _netRunning)
    {
        while (comStack.size() > 0)
        {
            if (!isRunning())
            {
                comStack.clear();
                write_busy = false;
                return;
            }

            mSend = comStack.at(0);
            comStack.erase(comStack.begin());   // delete oldest element
            unsigned char *buf = makeBuffer(mSend);

            if (buf == nullptr)
            {
                MSG_ERROR("Error creating a buffer! Token number: " << mSend.MC);
                continue;
            }

            MSG_DEBUG("Wrote buffer with " << (mSend.hlen + 4) << " bytes.");
            mSocket->send((char *)buf, mSend.hlen + 4);
            delete[] buf;
        }

        mSendReady = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    write_busy = false;
}

uint16_t TAmxNet::swapWord(uint16_t w)
{
    uint16_t word = 0;
    word = ((w << 8) & 0xff00) | ((w >> 8) & 0x00ff);
    return word;
}

uint32_t TAmxNet::swapDWord(uint32_t dw)
{
    uint32_t dword = 0;
    dword = ((dw << 24) & 0xff000000) | ((dw << 8) & 0x00ff0000) | ((dw >> 8) & 0x0000ff00) | ((dw >> 24) & 0x000000ff);
    return dword;
}

unsigned char TAmxNet::calcChecksum(const unsigned char* buffer, size_t len)
{
    DECL_TRACER("TAmxNet::calcChecksum(const unsigned char* buffer, size_t len)");
    unsigned long sum = 0;

    for (size_t i = 0; i < len; i++)
        sum += (unsigned long)(*(buffer + i)) & 0x000000ff;

    sum &= 0x000000ff;
    MSG_DEBUG("Checksum=" << std::setw(2) << std::setfill('0') << std::hex << sum << ", #bytes=" << len << " bytes.");
    return (unsigned char)sum;
}

uint16_t TAmxNet::makeWord(unsigned char b1, unsigned char b2)
{
    return ((b1 << 8) & 0xff00) | b2;
}

uint32_t TAmxNet::makeDWord(unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4)
{
    return ((b1 << 24) & 0xff000000) | ((b2 << 16) & 0x00ff0000) | ((b3  << 8) & 0x0000ff00) | b4;
}

bool TAmxNet::isCommand(const string& cmd)
{
    DECL_TRACER("TAmxNet::isCommand(string& cmd)");

    int i = 0;

    while (cmdList[i][0] != 0)
    {
        if (cmd.find(cmdList[i]) == 0)
            return true;

        i++;
    }

    if (cmd.length() > 0 && (cmd[0] == '^' || cmd[0] == '@' || cmd[0] == '?'))
        return true;

    if (startsWith(cmd, "GET ") || startsWith(cmd, "SET "))
        return true;

    return false;
}

unsigned char *TAmxNet::makeBuffer(const ANET_COMMAND& s)
{
    DECL_TRACER("TAmxNet::makeBuffer (const ANET_COMMAND& s)");

    int pos = 0;
    int len;
    bool valid = false;
    unsigned char *buf;

    try
    {
        buf = new unsigned char[s.hlen + 5];
        memset(buf, 0, s.hlen + 5);
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error allocating memory: " << e.what());
        return nullptr;
    }

    *buf = s.ID;
    *(buf + 1) = s.hlen >> 8;
    *(buf + 2) = s.hlen;
    *(buf + 3) = s.sep1;
    *(buf + 4) = s.type;
    *(buf + 5) = s.unk1 >> 8;
    *(buf + 6) = s.unk1;
    *(buf + 7) = s.device1 >> 8;
    *(buf + 8) = s.device1;
    *(buf + 9) = s.port1 >> 8;
    *(buf + 10) = s.port1;
    *(buf + 11) = s.system >> 8;
    *(buf + 12) = s.system;
    *(buf + 13) = s.device2 >> 8;
    *(buf + 14) = s.device2;
    *(buf + 15) = s.port2 >> 8;
    *(buf + 16) = s.port2;
    *(buf + 17) = s.unk6;
    *(buf + 18) = s.count >> 8;
    *(buf + 19) = s.count;
    *(buf + 20) = s.MC >> 8;
    *(buf + 21) = s.MC;

    // Here the fixed block is complete. The data are following.
    switch (s.MC)
    {
        case 0x0006:
        case 0x0007:
        case 0x0018:
        case 0x0019:
        case 0x0084:
        case 0x0085:
        case 0x0086:
        case 0x0087:
        case 0x0088:
        case 0x0089:
            *(buf + 22) = s.data.chan_state.device >> 8;
            *(buf + 23) = s.data.chan_state.device;
            *(buf + 24) = s.data.chan_state.port >> 8;
            *(buf + 25) = s.data.chan_state.port;
            *(buf + 26) = s.data.chan_state.system >> 8;
            *(buf + 27) = s.data.chan_state.system;
            *(buf + 28) = s.data.chan_state.channel >> 8;
            *(buf + 29) = s.data.chan_state.channel;
            *(buf + 30) = calcChecksum(buf, 30);
            valid = true;
            break;

        case 0x000a:
        case 0x008a:
            *(buf + 22) = s.data.message_value.device >> 8;
            *(buf + 23) = s.data.message_value.device;
            *(buf + 24) = s.data.message_value.port >> 8;
            *(buf + 25) = s.data.message_value.port;
            *(buf + 26) = s.data.message_value.system >> 8;
            *(buf + 27) = s.data.message_value.system;
            *(buf + 28) = s.data.message_value.value >> 8;
            *(buf + 29) = s.data.message_value.value;
            *(buf + 30) = s.data.message_value.type;
            pos = 31;

            switch (s.data.message_value.type)
            {
                case 0x10: *(buf + pos) = s.data.message_value.content.byte; break;

                case 0x11: *(buf + pos) = s.data.message_value.content.ch; break;

                case 0x20:
                    *(buf + pos) = s.data.message_value.content.integer >> 8;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.integer;
                    break;

                case 0x21:
                    *(buf + pos) = s.data.message_value.content.sinteger >> 8;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.sinteger;
                    break;

                case 0x40:
                    *(buf + pos) = s.data.message_value.content.dword >> 24;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.dword >> 16;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.dword >> 8;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.dword;
                    break;

                case 0x41:
                    *(buf + pos) = s.data.message_value.content.sdword >> 24;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.sdword >> 16;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.sdword >> 8;
                    pos++;
                    *(buf + pos) = s.data.message_value.content.sdword;
                    break;

                case 0x4f:
                    memcpy(buf + pos, &s.data.message_value.content.fvalue, 4);
                    pos += 3;
                    break;

                case 0x8f:
                    memcpy(buf + pos, &s.data.message_value.content.fvalue, 8);
                    pos += 3;
                    break;
            }

            pos++;
            *(buf + pos) = calcChecksum(buf, pos);
            valid = true;
        break;

        case 0x000b:
        case 0x000c:
        case 0x008b:
        case 0x008c:
            *(buf + 22) = s.data.message_string.device >> 8;
            *(buf + 23) = s.data.message_string.device;
            *(buf + 24) = s.data.message_string.port >> 8;
            *(buf + 25) = s.data.message_string.port;
            *(buf + 26) = s.data.message_string.system >> 8;
            *(buf + 27) = s.data.message_string.system;
            *(buf + 28) = s.data.message_string.type;
            *(buf + 29) = s.data.message_string.length >> 8;
            *(buf + 30) = s.data.message_string.length;
            pos = 31;
            memcpy(buf + pos, s.data.message_string.content, s.data.message_string.length);
            pos += s.data.message_string.length;
            *(buf + pos) = calcChecksum(buf, pos);
            valid = true;
        break;

        case 0x008d:    // Custom event
            *(buf + 22) = s.data.customEvent.device >> 8;
            *(buf + 23) = s.data.customEvent.device;
            *(buf + 24) = s.data.customEvent.port >> 8;
            *(buf + 25) = s.data.customEvent.port;
            *(buf + 26) = s.data.customEvent.system >> 8;
            *(buf + 27) = s.data.customEvent.system;
            *(buf + 28) = s.data.customEvent.ID >> 8;
            *(buf + 29) = s.data.customEvent.ID;
            *(buf + 30) = s.data.customEvent.type >> 8;
            *(buf + 31) = s.data.customEvent.type;
            *(buf + 32) = s.data.customEvent.flag >> 8;
            *(buf + 33) = s.data.customEvent.flag;
            *(buf + 34) = s.data.customEvent.value1 >> 24;
            *(buf + 35) = s.data.customEvent.value1 >> 16;
            *(buf + 36) = s.data.customEvent.value1 >> 8;
            *(buf + 37) = s.data.customEvent.value1;
            *(buf + 38) = s.data.customEvent.value2 >> 24;
            *(buf + 39) = s.data.customEvent.value2 >> 16;
            *(buf + 40) = s.data.customEvent.value2 >> 8;
            *(buf + 41) = s.data.customEvent.value2;
            *(buf + 42) = s.data.customEvent.value3 >> 24;
            *(buf + 43) = s.data.customEvent.value3 >> 16;
            *(buf + 44) = s.data.customEvent.value3 >> 8;
            *(buf + 45) = s.data.customEvent.value3;
            *(buf + 46) = s.data.customEvent.dtype;
            *(buf + 47) = s.data.customEvent.length >> 8;
            *(buf + 48) = s.data.customEvent.length;
            pos = 49;

            if (s.data.customEvent.length > 0)
            {
                memcpy(buf + pos, s.data.customEvent.data, s.data.customEvent.length);
                pos += s.data.customEvent.length;
            }

            *(buf + pos) = 0;
            *(buf + pos + 1) = 0;
            pos += 2;
            *(buf + pos) = calcChecksum(buf, pos);
            valid = true;
        break;

        case 0x0090:
            *(buf + 22) = s.data.sendPortNumber.device >> 8;
            *(buf + 23) = s.data.sendPortNumber.device;
            *(buf + 24) = s.data.sendPortNumber.system >> 8;
            *(buf + 25) = s.data.sendPortNumber.system;
            *(buf + 26) = s.data.sendPortNumber.pcount >> 8;
            *(buf + 27) = s.data.sendPortNumber.pcount;
            *(buf + 28) = calcChecksum(buf, 28);
            valid = true;
        break;

        case 0x0091:
        case 0x0092:
            *(buf + 22) = s.data.sendOutpChannels.device >> 8;
            *(buf + 23) = s.data.sendOutpChannels.device;
            *(buf + 24) = s.data.sendOutpChannels.port >> 8;
            *(buf + 25) = s.data.sendOutpChannels.port;
            *(buf + 26) = s.data.sendOutpChannels.system >> 8;
            *(buf + 27) = s.data.sendOutpChannels.system;
            *(buf + 28) = s.data.sendOutpChannels.count >> 8;
            *(buf + 29) = s.data.sendOutpChannels.count;
            *(buf + 30) = calcChecksum(buf, 30);
            valid = true;
        break;

        case 0x0093:
        case 0x0094:
            *(buf + 22) = s.data.sendSize.device >> 8;
            *(buf + 23) = s.data.sendSize.device;
            *(buf + 24) = s.data.sendSize.port >> 8;
            *(buf + 25) = s.data.sendSize.port;
            *(buf + 26) = s.data.sendSize.system >> 8;
            *(buf + 27) = s.data.sendSize.system;
            *(buf + 28) = s.data.sendSize.type;
            *(buf + 29) = s.data.sendSize.length >> 8;
            *(buf + 30) = s.data.sendSize.length;
            *(buf + 31) = calcChecksum(buf, 31);
            valid = true;
        break;

        case 0x0095:
            *(buf + 22) = s.data.sendLevSupport.device >> 8;
            *(buf + 23) = s.data.sendLevSupport.device;
            *(buf + 24) = s.data.sendLevSupport.port >> 8;
            *(buf + 25) = s.data.sendLevSupport.port;
            *(buf + 26) = s.data.sendLevSupport.system >> 8;
            *(buf + 27) = s.data.sendLevSupport.system;
            *(buf + 28) = s.data.sendLevSupport.level >> 8;
            *(buf + 29) = s.data.sendLevSupport.level;
            *(buf + 30) = s.data.sendLevSupport.num;
            *(buf + 31) = s.data.sendLevSupport.types[0];
            *(buf + 32) = s.data.sendLevSupport.types[1];
            *(buf + 33) = s.data.sendLevSupport.types[2];
            *(buf + 34) = s.data.sendLevSupport.types[3];
            *(buf + 35) = s.data.sendLevSupport.types[4];
            *(buf + 36) = s.data.sendLevSupport.types[5];
            *(buf + 37) = calcChecksum(buf, 37);
            valid = true;
        break;

        case 0x0096:
            *(buf + 22) = s.data.sendStatusCode.device >> 8;
            *(buf + 23) = s.data.sendStatusCode.device;
            *(buf + 24) = s.data.sendStatusCode.port >> 8;
            *(buf + 25) = s.data.sendStatusCode.port;
            *(buf + 26) = s.data.sendStatusCode.system >> 8;
            *(buf + 27) = s.data.sendStatusCode.system;
            *(buf + 28) = s.data.sendStatusCode.status >> 8;
            *(buf + 29) = s.data.sendStatusCode.status;
            *(buf + 30) = s.data.sendStatusCode.type;
            *(buf + 31) = s.data.sendStatusCode.length >> 8;
            *(buf + 32) = s.data.sendStatusCode.length;
            pos = 33;
            memset((void*)&s.data.sendStatusCode.str[0], 0, sizeof(s.data.sendStatusCode.str));
            memcpy(buf + pos, s.data.sendStatusCode.str, s.data.sendStatusCode.length);
            pos += s.data.sendStatusCode.length;
            *(buf + pos) = calcChecksum(buf, pos);
            valid = true;
        break;

        case 0x0097:
            *(buf + 22) = s.data.srDeviceInfo.device >> 8;
            *(buf + 23) = s.data.srDeviceInfo.device;
            *(buf + 24) = s.data.srDeviceInfo.system >> 8;
            *(buf + 25) = s.data.srDeviceInfo.system;
            *(buf + 26) = s.data.srDeviceInfo.flag >> 8;
            *(buf + 27) = s.data.srDeviceInfo.flag;
            *(buf + 28) = s.data.srDeviceInfo.objectID;
            *(buf + 29) = s.data.srDeviceInfo.parentID;
            *(buf + 30) = s.data.srDeviceInfo.herstID >> 8;
            *(buf + 31) = s.data.srDeviceInfo.herstID;
            *(buf + 32) = s.data.srDeviceInfo.deviceID >> 8;
            *(buf + 33) = s.data.srDeviceInfo.deviceID;
            pos = 34;
            memcpy(buf + pos, s.data.srDeviceInfo.serial, 16);
            pos += 16;
            *(buf + pos) = s.data.srDeviceInfo.fwid >> 8;
            pos++;
            *(buf + pos) = s.data.srDeviceInfo.fwid;
            pos++;
            memcpy(buf + pos, s.data.srDeviceInfo.info, s.data.srDeviceInfo.len);
            pos += s.data.srDeviceInfo.len;
            *(buf + pos) = calcChecksum(buf, pos);
            valid = true;
        break;

        case 0x0098:
            *(buf + 22) = s.data.reqPortCount.device >> 8;
            *(buf + 23) = s.data.reqPortCount.device;
            *(buf + 24) = s.data.reqPortCount.system >> 8;
            *(buf + 25) = s.data.reqPortCount.system;
            *(buf + 26) = calcChecksum(buf, 26);
            valid = true;
        break;

        case 0x0204:    // file transfer
            *(buf + 22) = s.data.filetransfer.ftype >> 8;
            *(buf + 23) = s.data.filetransfer.ftype;
            *(buf + 24) = s.data.filetransfer.function >> 8;
            *(buf + 25) = s.data.filetransfer.function;
            pos = 26;

            switch (s.data.filetransfer.function)
            {
                case 0x0001:
                    *(buf + 26) = s.data.filetransfer.unk;
                    *(buf + 27) = s.data.filetransfer.unk1;
                    pos = 28;
                break;

                case 0x0003:
                    *(buf + 26) = s.data.filetransfer.unk >> 8;
                    *(buf + 27) = s.data.filetransfer.unk;
                    pos = 28;

                    for (uint32_t i = 0; i < s.data.filetransfer.unk && pos < (s.hlen + 3); i++)
                    {
                        *(buf + pos) = s.data.filetransfer.data[i];
                        pos++;
                    }
                break;

                case 0x0101:
                    if (s.data.filetransfer.ftype == 0)
                    {
                        *(buf + 26) = s.data.filetransfer.unk >> 24;
                        *(buf + 27) = s.data.filetransfer.unk >> 16;
                        *(buf + 28) = s.data.filetransfer.unk >> 8;
                        *(buf + 29) = s.data.filetransfer.unk;
                        *(buf + 30) = s.data.filetransfer.unk1 >> 24;
                        *(buf + 31) = s.data.filetransfer.unk1 >> 16;
                        *(buf + 32) = s.data.filetransfer.unk1 >> 8;
                        *(buf + 33) = s.data.filetransfer.unk1;
                        *(buf + 34) = s.data.filetransfer.unk2 >> 24;
                        *(buf + 35) = s.data.filetransfer.unk2 >> 16;
                        *(buf + 36) = s.data.filetransfer.unk2 >> 8;
                        *(buf + 37) = s.data.filetransfer.unk2;
                        *(buf + 38) = 0x00;
                        *(buf + 39) = 0x00;
                        *(buf + 40) = 0x3e;
                        *(buf + 41) = 0x75;
                        pos = 42;
                        len = 0;

                        while (s.data.filetransfer.data[len] != 0)
                        {
                            *(buf + pos) = s.data.filetransfer.data[len];
                            len++;
                            pos++;
                        }

                        *(buf + pos) = 0;
                        pos++;
                    }
                    else
                    {
                        *(buf + 26) = s.data.filetransfer.unk >> 24;
                        *(buf + 27) = s.data.filetransfer.unk >> 16;
                        *(buf + 28) = s.data.filetransfer.unk >> 8;
                        *(buf + 29) = s.data.filetransfer.unk;
                        *(buf + 30) = 0x00;
                        *(buf + 31) = 0x00;
                        *(buf + 32) = 0x00;
                        *(buf + 33) = 0x00;
                        pos = 34;
                    }

                break;

                case 0x0102:
                    *(buf + 26) = 0x00;
                    *(buf + 27) = 0x00;
                    *(buf + 28) = 0x00;
                    *(buf + 29) = s.data.filetransfer.info1;        // dir flag
                    *(buf + 30) = s.data.filetransfer.info2 >> 8;   // # entries
                    *(buf + 31) = s.data.filetransfer.info2;
                    *(buf + 32) = s.data.filetransfer.unk >> 8;     // counter
                    *(buf + 33) = s.data.filetransfer.unk;
                    *(buf + 34) = s.data.filetransfer.unk1 >> 24;   // file size
                    *(buf + 35) = s.data.filetransfer.unk1 >> 16;
                    *(buf + 36) = s.data.filetransfer.unk1 >> 8;
                    *(buf + 37) = s.data.filetransfer.unk1;
                    *(buf + 38) = (s.data.filetransfer.info1 == 1) ? 0x0c : 0x0b;
                    *(buf + 39) = (s.data.filetransfer.info1 == 1) ? 0x0e : 0x13;
                    *(buf + 40) = 0x07;
                    *(buf + 41) = s.data.filetransfer.unk2 >> 24;   // Date
                    *(buf + 42) = s.data.filetransfer.unk2 >> 16;
                    *(buf + 43) = s.data.filetransfer.unk2 >> 8;
                    *(buf + 44) = s.data.filetransfer.unk2;
                    pos = 45;
                    len = 0;

                    while (s.data.filetransfer.data[len] != 0)
                    {
                        *(buf + pos) = s.data.filetransfer.data[len];
                        pos++;
                        len++;
                    }

                    *(buf + pos) = 0;
                    pos++;
                break;

                case 0x0103:
                    *(buf + 26) = s.data.filetransfer.unk >> 8;
                    *(buf + 27) = s.data.filetransfer.unk;
                    *(buf + 28) = s.data.filetransfer.unk1 >> 24;
                    *(buf + 29) = s.data.filetransfer.unk1 >> 16;
                    *(buf + 30) = s.data.filetransfer.unk1 >> 8;
                    *(buf + 31) = s.data.filetransfer.unk1;
                    pos = 32;
                break;

                case 0x105:
                    *(buf + 26) = s.data.filetransfer.unk >> 24;
                    *(buf + 27) = s.data.filetransfer.unk >> 16;
                    *(buf + 28) = s.data.filetransfer.unk >> 8;
                    *(buf + 29) = s.data.filetransfer.unk;
                    *(buf + 30) = s.data.filetransfer.unk1 >> 24;
                    *(buf + 31) = s.data.filetransfer.unk1 >> 16;
                    *(buf + 32) = s.data.filetransfer.unk1 >> 8;
                    *(buf + 33) = s.data.filetransfer.unk1;
                    pos = 34;
                break;
            }

            *(buf + pos) = calcChecksum(buf, pos);
            valid = true;
        break;

        case 0x0581:    // Pong
            *(buf + 22) = s.data.srDeviceInfo.device >> 8;
            *(buf + 23) = s.data.srDeviceInfo.device;
            *(buf + 24) = s.data.srDeviceInfo.system >> 8;
            *(buf + 25) = s.data.srDeviceInfo.system;
            *(buf + 26) = s.data.srDeviceInfo.herstID >> 8;
            *(buf + 27) = s.data.srDeviceInfo.herstID;
            *(buf + 28) = s.data.srDeviceInfo.deviceID >> 8;
            *(buf + 29) = s.data.srDeviceInfo.deviceID;
            *(buf + 30) = s.data.srDeviceInfo.info[0];
            *(buf + 31) = s.data.srDeviceInfo.info[1];
            *(buf + 32) = s.data.srDeviceInfo.info[2];
            *(buf + 33) = s.data.srDeviceInfo.info[3];
            *(buf + 34) = s.data.srDeviceInfo.info[4];
            *(buf + 35) = s.data.srDeviceInfo.info[5];
            *(buf + 36) = calcChecksum(buf, 36);
            valid = true;
        break;
    }

    if (!valid)
    {
        delete[] buf;
        return 0;
    }

//    MSG_TRACE("Buffer:");
//    TError::logHex((char *)buf, s.hlen + 4);
    return buf;
}

void TAmxNet::setSerialNum(const string& sn)
{
    DECL_TRACER("TAmxNet::setSerialNum(const string& sn)");

    serNum = sn;
    size_t len = (sn.length() > 15) ? 15 : sn.length();

    for (size_t i = 0; i < devInfo.size(); i++)
        strncpy(devInfo[i].serialNum, sn.c_str(), len);
}

int TAmxNet::countFiles()
{
    DECL_TRACER("TAmxNet::countFiles()");

    int count = 0;
    ifstream in;

    try
    {
        in.open(TConfig::getProjectPath() + "/manifest.xma", fstream::in);

        if (!in)
            return 0;

        for (string line; getline(in, line);)
            count++;

        in.close();
    }
    catch (exception& e)
    {
        MSG_ERROR("Error: " << e.what());
        return 0;
    }

    return count;
}

void TAmxNet::sendAllFuncNetwork(int state)
{
    DECL_TRACER("TAmxNet::sendAllFuncNetwork(int state)");

    mLastOnlineState = state;

    if (mFuncsNetwork.empty())
        return;

    MSG_DEBUG("Setting network state to " << state);
    map<ulong, FUNC_NETWORK_t>::iterator iter;

    for (iter = mFuncsNetwork.begin(); iter != mFuncsNetwork.end(); ++iter)
        iter->second.func(state);
}

void TAmxNet::sendAllFuncTimer(const ANET_BLINK& blink)
{
    DECL_TRACER("TAmxNet::sendAllFuncTimer(const ANET_BLINK& blink)");

    if (mFuncsTimer.empty())
        return;

    map<ulong, FUNC_TIMER_t>::iterator iter;

    for (iter = mFuncsTimer.begin(); iter != mFuncsTimer.end(); ++iter)
        iter->second.func(blink);
}

void TAmxNet::setWaitTime(int secs)
{
    DECL_TRACER("TAmxNet::setWaitTime(int secs)");

    if (secs <= 0 || secs > 300)    // Maximal 5 minutes
        return;

    mOldWaitTime = mWaitTime;
    mWaitTime = secs;
}

int TAmxNet::swapWaitTime()
{
    DECL_TRACER("TAmxNet::restoreWaitTime()");

    int wt = mWaitTime;

    mWaitTime = mOldWaitTime;
    mOldWaitTime = wt;
    return mWaitTime;
}

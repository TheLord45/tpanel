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

#ifndef __TAMXNET_H__
#define __TAMXNET_H__

//#include "asio.hpp"

#include <vector>
#include <map>
#include <functional>
#include <cstring>
#include <cstdio>
#include <atomic>
#include <thread>

#include "tsocket.h"
#include "tvector.h"

#if defined(__ANDROID__) || defined(__MACH__)
typedef unsigned long int ulong;
#endif

extern std::atomic<bool> killed;
extern bool prg_stopped;

namespace amx
{
#define MAX_CHUNK   0x07d0  // Maximum size a part of a file can have. The file will be splitted into this size of chunks.
#define BUF_SIZE    0x1000  // 4096 bytes

#define NSTATE_OFFLINE      0
#define NSTATE_OFFLINE1     1
#define NSTATE_ONLINE       6
#define NSTATE_ONLINE1      5
#define NSTATE_CONNECTING   9

#define WAIT_RESET          3   // Number of seconds to wait in case of connecting to another controller
#define WAIT_RECONNECT      15  // Number of seconds to wait in case of reconnecting to the same controller

#define DTSZ_STRING         0x01    // ASCII string (8 bit)
#define DTSZ_WSTRING        0x02    // Wide string (16 bit)
#define DTSZ_BYTE           0x10    // Byte (8 bit unsigned)
#define DTSZ_CHAR           0x11    // Byte (8 bit signed)
#define DTSZ_UINT           0x20    // Word (16 bit unsigned)
#define DTSZ_INT            0x21    // Word (16 bit signed)
#define DTSZ_LUINT          0x40    // DWORD (32 bit unsigned)
#define DTSZ_LINTT          0x41    // DWORD (32 bit signed)
#define DTSZ_FLOAT          0x4f    // FLOAT (32 bit)
#define DTSZ_DOUBLE         0x8f    // DOUBLE (32/64 bit)

    typedef struct ANET_SEND
    {
        uint16_t device{0};     // Device ID of panel
        uint16_t MC{0};         // message command number
        uint16_t port{0};       // port number
        uint16_t level{0};      // level number (if any)
        uint16_t channel{0};    // channel status
        uint16_t value{0};      // level value
        // this is for custom events
        uint16_t ID{0};         // ID of button
        uint16_t type{0};       // Type of event
        uint16_t flag{0};       // Flag
        uint32_t value1{0};     // Value 1
        uint32_t value2{0};     // Value 2
        uint32_t value3{0};     // Value 3
        unsigned char dtype{0}; // Type of data
        std::string msg;        // message string
    } ANET_SEND;

    typedef union
    {
        unsigned char byte; // type = 0x10
        char ch;            // type = 0x11
        uint16_t integer;   // type = 0x20 (also wide char)
        int16_t sinteger;   // type = 0x21
        uint32_t dword;     // type = 0x40
        int32_t sdword;     // type = 0x41
        float fvalue;       // type = 0x4f
        double dvalue;      // type = 0x8f
    } ANET_CONTENT;

    typedef struct ANET_MSG
    {
        uint16_t device;        // device number
        uint16_t port;          // port number
        uint16_t system;        // system number
        uint16_t value;         // value of command
        unsigned char type;     // defines how to interpret the content of cmd
        ANET_CONTENT content;
    } ANET_MSG;

    typedef struct ANET_MSG_STRING
    {
        uint16_t device;        // device number
        uint16_t port;          // port number
        uint16_t system;        // system number
        unsigned char type;     // Definnes the type of content (0x01 = 8 bit chars, 0x02 = 16 bit chars --> wide chars)
        uint16_t length;        // length of following content
        unsigned char content[1500];// content string
    } ANET_MSG_STRING;

    typedef struct ANET_ASIZE
    {
        uint16_t device;        // device number
        uint16_t port;          // port number
        uint16_t system;        // system number
        unsigned char type;     // Defines the type of content (0x01 = 8 bit chars, 0x02 = 16 bit chars --> wide chars)
        uint16_t length;        // length of following content
    } ANET_ASIZE;

    typedef struct ANET_LEVSUPPORT
    {
        uint16_t device;        // device number
        uint16_t port;          // port number
        uint16_t system;        // system number
        uint16_t level;         // level number
        unsigned char num;      // number of supported types
        unsigned char types[6]; // Type codes
    } ANET_LEVSUPPORT;

    typedef struct ANET_ASTATCODE
    {
        uint16_t device;        // device number
        uint16_t port;          // port number
        uint16_t system;        // system number
        uint16_t status;        // status code
        unsigned char type;     // defines how to interpret the content of cmd
        uint16_t length;        // length of following string
        unsigned char str[512];
    } ANET_ASTATCODE;

    typedef struct ANET_LEVEL
    {
        uint16_t device;        // device number
        uint16_t port;          // port number
        uint16_t system;        // system number
        uint16_t level;         // level number
    } ANET_LEVEL;

    typedef struct ANET_CHANNEL
    {
        uint16_t device;        // device number
        uint16_t port;          // port number
        uint16_t system;        // system number
        uint16_t channel;       // level number
    } ANET_CHANNEL;

    typedef struct ANET_RPCOUNT
    {
        uint16_t device;        // device number
        uint16_t system;        // system number
    } ANET_RPCOUNT;

    typedef struct ANET_APCOUNT // Answer to request for port count
    {
        uint16_t device;        // device number
        uint16_t system;        // system number
        uint16_t pcount;        // number of supported ports
    } ANET_APCOUNT;

    typedef struct ANET_ROUTCHAN    // Request for port count
    {
        uint16_t device;        // device number
        uint16_t port;          // system number
        uint16_t system;        // number of supported ports
    } ANET_ROUTCHAN;

    typedef struct ANET_AOUTCHAN    // Answer to request for port count
    {
        uint16_t device;        // device number
        uint16_t port;          // system number
        uint16_t system;        // number of supported ports
        uint16_t count;         // number of supported channels
    } ANET_AOUTCHAN;

    typedef struct ANET_ADEVINFO // Answer to "request device info"
    {
        uint16_t device;        // device number
        uint16_t system;        // system number
        uint16_t flag;          // Bit 8 - If set, this message was generated in response to a key press, during the identification mode is active.
        unsigned char objectID; // object ID
        unsigned char parentID; // parent ID
        uint16_t herstID;       // herst ID
        uint16_t deviceID;      // device ID
        unsigned char serial[16]; // serial number
        uint16_t fwid;          // firmware ID
        unsigned char info[512];// several NULL terminated informations
        int len;                // length of field info
    } ANET_ADEVINFO;

    typedef struct ANET_ASTATUS // Answer to "master status"
    {
        uint16_t system;        // number of system
        uint16_t status;        // Bit field
        unsigned char str[512]; // Null terminated status string
    } ANET_ASTATUS;

    typedef struct ANET_CUSTOM  // Custom event
    {
        uint16_t device;        // Device number
        uint16_t port;          // Port number
        uint16_t system;        // System number
        uint16_t ID;            // ID of event (button ID)
        uint16_t type;          // Type of event
        uint16_t flag;          // Flag
        uint32_t value1;        // Value 1
        uint32_t value2;        // Value 2
        uint32_t value3;        // Value 3
        unsigned char dtype;    // type of following data
        uint16_t length;        // length of following string
        unsigned char data[255];// Custom data
    } ANET_CUSTOM;

    typedef struct ANET_BLINK       // Blink message (contains date and time)
    {
        unsigned char heartBeat;    // Time between heart beats in 10th of seconds
        unsigned char LED;          // Bit 0: Bus-LED: 0 off, 1 on; Bits 1-6 reserved; Bit 7: 1 = force reset device
        unsigned char month;        // Month 1 - 12
        unsigned char day;          // Day 1 - 31
        uint16_t year;              // Year
        unsigned char hour;         // Hours 0 - 23
        unsigned char minute;       // Minutes 0 - 59
        unsigned char second;       // Seconds 0 - 59
        unsigned char weekday;      // 0 = Monday, 1 = Thuesday, ...
        uint16_t extTemp;           // External temperature, if available (0x8000 invalid value)
        unsigned char dateTime[64]; // Rest is a string containing the date and time.

        void clear()
        {
            heartBeat = 0;
            LED = 0;
            month = 0;
            day = 0;
            year = 0;
            hour = 0;
            minute = 0;
            second = 0;
            weekday = 0;
            extTemp = 0;
            dateTime[0] = 0;
        }
    } ANET_BLINK;

    typedef struct ANET_FILETRANSFER    // File transfer
    {
        uint16_t ftype;         // 0 = not used, 1=IR, 2=Firmware, 3=TP file, 4=Axcess2
        uint16_t function;      // The function to be performed, or length of data block
        uint16_t info1;
        uint16_t info2;
        uint32_t unk;           // ?
        uint32_t unk1;          // ?
        uint32_t unk2;          // ?
        uint32_t unk3;          // ?
        unsigned char data[2048];// Function specific data
    } ANET_FILETRANSFER;

    typedef union
    {
        ANET_CHANNEL chan_state;
        ANET_MSG message_value;
        ANET_MSG_STRING message_string;
        ANET_LEVEL level;
        ANET_CHANNEL channel;
        ANET_RPCOUNT reqPortCount;
        ANET_APCOUNT sendPortNumber;
        ANET_ROUTCHAN reqOutpChannels;
        ANET_AOUTCHAN sendOutpChannels;
        ANET_ASTATCODE sendStatusCode;
        ANET_ASIZE sendSize;
        ANET_LEVEL reqLevels;
        ANET_LEVSUPPORT sendLevSupport;
        ANET_ADEVINFO srDeviceInfo;     // send/receive device info
        ANET_CUSTOM customEvent;
        ANET_BLINK blinkMessage;
        ANET_FILETRANSFER filetransfer;
    } ANET_DATA;

    typedef struct ANET_COMMAND // Structure of type command (type = 0x00, status = 0x000c)
    {
        char ID{2};             // 0x00:        Always 0x02
        uint16_t hlen{0};       // 0x01 - 0x02: Header length (length + 3 for total length!)
        char sep1{2};           // 0x03:        Seperator always 0x02
        char type{0};           // 0x04:        Type of header
        uint16_t unk1{1};       // 0x05 - 0x06: always 0x0001
        uint16_t device1{0};    // 0x07 - 0x08: receive: device, send: 0x000
        uint16_t port1{0};      // 0x09 - 0x0a: receive: port,   send: 0x001
        uint16_t system{0};     // 0x0b - 0x0c: receive: system, send: 0x001
        uint16_t device2{0};    // 0x0d - 0x0e: send: device,    receive: 0x0000
        uint16_t port2{0};      // 0x0f - 0x10: send: port,      receive: 0x0001
        char unk6{0x0f};        // 0x11:        Always 0x0f
        uint16_t count{0};      // 0x12 - 0x13: Counter
        uint16_t MC{0};         // 0x14 - 0x15: Message command identifier
        ANET_DATA data;         // 0x16 - n     Data block
        unsigned char checksum{0};  // last byte:   Checksum

        void clear()
        {
            ID = 0x02;
            hlen = 0;
            sep1 = 0x02;
            type = 0;
            unk1 = 1;
            device1 = 0;
            port1 = 0;
            system = 0;
            device2 = 0;
            port2 = 0;
            unk6 = 0x0f;
            count = 0;
            MC = 0;
            checksum = 0;
        }
    } ANET_COMMAND;

    typedef struct
    {
        unsigned char objectID;     // Unique 8-bit identifier that identifies this structure of information
        unsigned char parentID;     // Refers to an existing object ID. If 0, has this object to any parent object (parent).
        uint16_t manufacturerID;    // Value that uniquely identifies the manufacture of the device.
        uint16_t deviceID;          // Value that uniquely identifies the device type.
        char serialNum[16];         // Fixed length of 16 bytes.
        uint16_t firmwareID;        // Value that uniquely identifies the object code that the device requires.
        // NULL terminated text field
        char versionInfo[16];       // A closed with NULL text string that specifies the version of the reprogrammable component.
        char deviceInfo[32];        // A closed with NULL text string that specifies the name or model number of the device.
        char manufacturerInfo[32];  // A closed with NULL text string that specifies the name of the device manufacturer.
        unsigned char format;       // Value that indicates the type of device specific addressing information following.
        unsigned char len;          // Value that indicates the length of the following device-specific addressing information.
        unsigned char addr[8];      // Extended address as indicated by the type and length of the extended address.
    } DEVICE_INFO;

    typedef struct FTRANSFER
    {
        int percent{0};             // Status indicating the percent done
        int maxFiles{0};            // Total available files
        int lengthFile{0};          // Total length of currently transfered file
        int actFileNum{0};          // Number of currently transfered file.
        int actDelFile{0};          // Number of currently deleted file.
    } FTRANSFER;

    typedef struct FUNC_NETWORK_t
    {
        ulong handle{0};
        std::function<void(int)> func{nullptr};
    }FUNC_NETWORK_t;

    typedef struct FUNC_TIMER_t
    {
        ulong handle{0};
        std::function<void(const ANET_BLINK&)> func{nullptr};
    }FUNC_TIMER_t;

    class TAmxNet
    {
        public:
            TAmxNet();
            explicit TAmxNet(const std::string& sn);
            explicit TAmxNet(const std::string& sn, const std::string& nm);
            ~TAmxNet();

            void Run();
            void stop(bool soft=false);
            bool reconnect();

            void setCallback(std::function<void(const ANET_COMMAND&)> func) { callback = func; }
            void registerNetworkState(std::function<void(int)> registerNetwork, ulong handle);
            void registerTimer(std::function<void(const ANET_BLINK&)> registerBlink, ulong handle);
            void deregNetworkState(ulong handle);
            void deregTimer(ulong handle);

            bool sendCommand(const ANET_SEND& s);
            bool isStopped() { return stopped_; }
            bool isConnected() { if (mSocket) return mSocket->isConnected(); return false; }
            bool close() { if (mSocket) return mSocket->close(); return false; }
            bool isNetRun();
            void setPanelID(int id) { panelID = id; }
            void setSerialNum(const std::string& sn);
            bool setupStatus() { return receiveSetup; }
            void setPanName(const std::string& nm) { panName.assign(nm); }

            void setWaitTime(int secs);
            int getWaitTime() { return mWaitTime; }
            int swapWaitTime();

        protected:
            void start();

        private:
            enum R_TOKEN
            {
                RT_NONE,
                RT_ID,
                RT_LEN,
                RT_SEP1,
                RT_TYPE,
                RT_WORD1,
                RT_DEVICE,
                RT_WORD2,
                RT_WORD3,
                RT_WORD4,
                RT_WORD5,
                RT_SEP2,
                RT_COUNT,
                RT_MC,
                RT_DATA
            };

            void init();
            void handle_connect();
            void runWrite();
            void start_read();
            void handle_read(size_t n, R_TOKEN tk);
            void start_write();
            void handleFTransfer(ANET_SEND& s, ANET_FILETRANSFER& ft);
            uint16_t swapWord(uint16_t w);
            uint32_t swapDWord(uint32_t dw);
            unsigned char calcChecksum(const unsigned char *buffer, size_t len);
            uint16_t makeWord(unsigned char b1, unsigned char b2);
            uint32_t makeDWord(unsigned char b1, unsigned char b2, unsigned char b3, unsigned char b4);
            unsigned char *makeBuffer(const ANET_COMMAND& s);
            int msg97fill(ANET_COMMAND *com);
            bool isCommand(const std::string& cmd);
            bool isRunning() { return !(stopped_ || killed || prg_stopped); }
            int countFiles();
            void sendAllFuncNetwork(int state);
            void sendAllFuncTimer(const ANET_BLINK& blink);

            std::function<void(const ANET_COMMAND&)> callback;

            TSocket *mSocket{nullptr};  // Pointer to socket class needed for communication
            FILE *rcvFile{nullptr};
            FILE *sndFile{nullptr};
            size_t posRcv{0};
            size_t lenRcv{0};
            size_t posSnd{0};
            size_t lenSnd{0};
            std::thread mThread;
            std::thread mWriteThread;   // Thread used to write to the Netlinx.
            std::string input_buffer_;
            std::string panName;        // The technical name of the panel
            TVector<ANET_COMMAND> comStack; // commands to answer
            std::vector<DEVICE_INFO> devInfo;
            std::string oldCmd;
            std::string serNum;
            std::string sndFileName;
            std::string rcvFileName;
            ANET_COMMAND comm;          // received command
            ANET_COMMAND mSend;         // answer / request
            int panelID{0};             // Panel ID of currently legalized panel.
            int mWaitTime{3};           // [seconds]: Wait by default for 3 seconds
            int mOldWaitTime{3};        // [seconds]: The previous wait time in case of change
            FTRANSFER ftransfer;
            uint16_t reqDevStatus{0};
            uint16_t sendCounter{0};    // Counter increment on every send
            std::atomic<bool> stopped_{false};
            std::atomic<bool> mSendReady{false};
            bool protError{false};      // true = error on receive --> disconnect
            bool initSend{false};       // TRUE = all init messages are send.
            bool ready{false};          // TRUE = ready for communication
            std::atomic<bool> write_busy{false};
            std::atomic<bool> receiveSetup{false};
            bool isOpenSnd{false};
            bool isOpenRcv{false};
            bool _retry{false};
            char buff_[BUF_SIZE];       // Internal used buffer for network communication
            int mLastOnlineState{NSTATE_OFFLINE};
    };
}

#endif

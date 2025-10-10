/*
 * Copyright (C) 2024 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef TQINTERCOM_H
#define TQINTERCOM_H

#include <string>
//#include <QObject>
#include <QAudioFormat>
#include <QAbstractSocket>
#include <QAudioSink>
#include <QUdpSocket>
#include <QScopedPointer>

#include "taudioconvert.h"

typedef struct INTERCOM_t
{
    std::string ip;         // IP address of panel to connect to
    long rxPort{0};          // Network port number for receiver
    long txPort{0};          // Network port number for transmitter
    int mode{0};            // The mode (0 = talk, 1 = listen, 2 = both)
}INTERCOM_t;

class QAudioSource;
class QBuffer;
class QTimer;
class QTimerEvent;

class TMicrophone : public QIODevice, public TAudioConvert
{
    public:
        TMicrophone();
        ~TMicrophone();

        void start();
        void stop();

        qint64 readData(char *data, qint64 maxlen) override;
        qint64 writeData(const char *data, qint64 len) override;
        qint64 bytesAvailable() const override;
        qint64 size() const override { return mBuffer.size(); }

    private:
        qint64 mPos{0};
        QByteArray mBuffer;
};

class TQIntercom : public QObject, public TAudioConvert
{
    Q_OBJECT

    public:
        TQIntercom(QObject *parent);
        TQIntercom(QObject *parent, INTERCOM_t ic);
        ~TQIntercom();

        void start();
        void stop();
        void setMicrophoneLevel(int level);
        void setSpeakerLevel(int level);
        void setMute(bool mute);
        void setIntercom(INTERCOM_t ic);

    protected:
        bool connectTalker();   // Talker
        bool spawnServer();     // Listener

        void onInputStateChanged(QAbstractSocket::SocketState socketState);
        void onOutputStateChanged(QAbstractSocket::SocketState socketState);
        void stateChangedMessage(QAbstractSocket::SocketState socketState, bool in);
        void onInputErrorOccurred(QAbstractSocket::SocketError socketError);
        void onOutputErrorOccurred(QAbstractSocket::SocketError socketError);
        void socketErrorMessages(QAbstractSocket::SocketError socketError, const std::string& msg);
        void onReadPendingDatagrams();
        void onRecordPermissionGranted();
        void onMicStateChanged(QAudio::State state);

        qreal convertVolume(int volume);

    private:
        typedef struct _HEADER_t
        {
            uint16_t ident{0x8000};
            uint16_t counter{0};
            uint32_t position{0};
            uint16_t unk1{0xf8a8};
            uint16_t unk2{0xe554};
        }_HEADER_t;

        long getNextBlock(QByteArray *target, const QByteArray& data);

        INTERCOM_t mIntercom;
        int mMicLevel{100};
        int mSpkLevel{100};
        bool mInitialized{false};
        bool mTalkConnected{false};
        bool mRecordPermissionGranted{false};
        bool mMicOpen{false};
        _HEADER_t mHeader;

        QUdpSocket *mUdpTalker{nullptr};
        QUdpSocket *mUdpListener{nullptr};
        QAudioFormat mAudioFormat;
        QAudioSource *mMicrophone{nullptr}; // Talk --> from mic to remote
        QAudioSink *mRemote{nullptr};       // Listen --> from remote to speaker
        QByteArray mReadBuffer;
        TMicrophone *mMicDevice{nullptr};
        QTimer *mPushTimer{nullptr};
        QTimer *mPullTimer{nullptr};
        QAudioDevice mAudioMicDevice;
};

#endif // TQINTERCOM_H

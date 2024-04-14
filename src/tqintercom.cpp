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

#include <cstring>

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QAudioSink>
#include <QAudioSource>
#include <QMediaDevices>
#include <QMediaRecorder>
#include <QAudioInput>
#include <QBuffer>
#include <QTimer>
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#   if QT_VERSION_CHECK(6, 6, 0)
#       include <QApplication>
#       include <QPermissions>
#   endif   // QT_VERSION_CHECK(6, 6, 0)
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)

#include "tqintercom.h"
#include "tresources.h"
#include "tconfig.h"
#include "terror.h"

#define PACKET_SIZE     172
#define DATA_SIZE       160
#define HEADER_SIZE     12

using std::string;
using std::min;

TQIntercom::TQIntercom(QObject *parent)
    : QObject(parent)
{
    DECL_TRACER("TQIntercom::TQIntercom()");

    mSpkLevel = TConfig::getSystemVolume();
    mMicLevel = TConfig::getSystemGain();
    mPushTimer = new QTimer();
}

TQIntercom::TQIntercom(QObject *parent, INTERCOM_t ic)
    : QObject(parent)
{
    DECL_TRACER("TQIntercom::TQIntercom(INTERCOM_t ic)");

    mSpkLevel = TConfig::getSystemVolume();
    mMicLevel = TConfig::getSystemGain();
    mPushTimer = new QTimer();
    setIntercom(ic);
}

TQIntercom::~TQIntercom()
{
    DECL_TRACER("TQIntercom::~TQIntercom()");

    if (mRemote)
        delete mRemote;

    if (mMicrophone)
        delete mMicrophone;

    if (mUdpTalker)
    {
        if (mUdpTalker->isOpen())
            mUdpTalker->close();

        delete mUdpTalker;
    }

    if (mUdpListener)
    {
        if (mUdpListener->isOpen())
            mUdpListener->close();

        delete mUdpListener;
    }

    if (mPushTimer)
    {
        mPushTimer->stop();
        mPushTimer->disconnect();
        delete mPushTimer;
    }

    if (mPullTimer)
    {
        mPullTimer->stop();
        mPullTimer->disconnect();
        delete mPullTimer;
    }
}

void TQIntercom::setIntercom(INTERCOM_t ic)
{
    DECL_TRACER("TQIntercom::setIntercom(INTERCOM_t ic)");

    if (ic.ip.empty())
    {
        MSG_ERROR("No valid IP address!");
        return;
    }

    if (ic.rxPort < 0 || ic.rxPort > 0x0ffff)
    {
        MSG_ERROR("Receiver port is invalid! (" << ic.rxPort << ")");
        return;
    }

    if (ic.txPort < 0 || ic.txPort > 0x0ffff)
    {
        MSG_ERROR("Transmit port is invalid! (" << ic.txPort << ")");
        return;
    }

    if (ic.rxPort == 0 && ic.txPort == 0)
    {
        MSG_ERROR("No transmit and no receive port!");
        return;
    }

    if (ic.mode < 0 || ic.mode > 2)
    {
        MSG_ERROR("Invalid mode " << ic.mode << "!");
        return;
    }

    if (ic.mode == 0 && ic.rxPort == 0)     // listen / receive
    {
        MSG_ERROR("No network port for listening!");
        return;
    }

    if (ic.mode == 1 && ic.txPort == 0)     // talk / send
    {
        MSG_ERROR("No network port for talking!");
        return;
    }

    mIntercom = ic;
    mAudioFormat.setSampleRate(8000);
    mAudioFormat.setSampleFormat(QAudioFormat::Int16);
    mAudioFormat.setChannelCount(1);
    mAudioFormat.setChannelConfig(QAudioFormat::ChannelConfigMono);

    if (mIntercom.mode == 0 || mIntercom.mode == 2)     // talk
    {
        MSG_DEBUG("Connecting to \"" << mIntercom.ip << "\" on port " << mIntercom.txPort);

        if (TStreamError::checkFilter(HLOG_DEBUG))
        {
            const QList<QAudioDevice> audioDevices = QMediaDevices::audioInputs();

            for (const QAudioDevice &device : audioDevices)
            {
                MSG_DEBUG("In ID: " << device.id().toStdString());
                MSG_DEBUG("In Description: " << device.description().toStdString());
                MSG_DEBUG("In Is default: " << (device.isDefault() ? "Yes" : "No"));
            }
        }
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#   if QT_VERSION_CHECK(6, 6, 0)
        QMicrophonePermission microphonePermission;

        switch (qApp->checkPermission(microphonePermission))
        {
            case Qt::PermissionStatus::Undetermined:
                qApp->requestPermission(microphonePermission, this, &TQIntercom::onRecordPermissionGranted);
                return;

            case Qt::PermissionStatus::Denied:
                qWarning("Microphone permission is not granted!");
                return;

            case Qt::PermissionStatus::Granted:
                MSG_INFO("Microphone permission is granted.");
                break;
        }
#   endif  // QT_VERSION_CHECK(6, 6, 0)
#endif  // defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    }

    if (mIntercom.mode == 1 || mIntercom.mode == 2)     // listen
    {
        MSG_DEBUG("Receiving on port " << mIntercom.rxPort);

        if (TStreamError::checkFilter(HLOG_DEBUG))
        {
            const QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();

            for (const QAudioDevice &device : audioDevices)
            {
                MSG_DEBUG("Out ID: " << device.id().toStdString());
                MSG_DEBUG("Out Description: " << device.description().toStdString());
                MSG_DEBUG("Out Is default: " << (device.isDefault() ? "Yes" : "No"));

                if (device.isDefault())
                    mAudioMicDevice = device;
            }
        }
    }

    mInitialized = true;
}

void TQIntercom::setSpeakerLevel(int level)
{
    DECL_TRACER("TQIntercom::setSpeakerLevel(int level)");

    if (level < 0 || level > 100)
        return;

    mSpkLevel = level;
    qreal volume = QAudio::convertVolume(level, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale);

    if (mIntercom.mode == 0 || mIntercom.mode == 2)
    {
        if (mRemote)
            mRemote->setVolume(volume);
    }
}

void TQIntercom::setMicrophoneLevel(int level)
{
    DECL_TRACER("TQIntercom::setMicrophoneLevel(int level)");

    if (level < 0 || level > 100)
        return;

    mMicLevel = level;
    qreal gain = QAudio::convertVolume(level, QAudio::LinearVolumeScale, QAudio::LogarithmicVolumeScale);

    if (mIntercom.mode == 1 || mIntercom.mode == 2)
    {
        if (mMicrophone)
            mMicrophone->setVolume(gain);
    }
}

void TQIntercom::start()
{
    DECL_TRACER("TQIntercom::start()");

    if ((mIntercom.mode == 0 || mIntercom.mode == 2) && mIntercom.rxPort)   // listen
    {
        if (mPullTimer)
            mPullTimer->stop();
        else
            mPullTimer = new QTimer();

        if (mRemote)
            delete mRemote;

        mRemote = new QAudioSink(mAudioFormat, this);
        mRemote->setVolume(convertVolume(mSpkLevel));

        if (!spawnServer())
            return;

        if (mPullTimer)
        {
            mPullTimer->disconnect();
            QIODevice *io = mRemote->start();

            connect(mPullTimer, &QTimer::timeout, [this, io]()
            {
                if (!io || !io->isOpen() || mReadBuffer.isEmpty())
                    return;

                qint64 len = io->write(mReadBuffer);

                if (len > 0)
                    mReadBuffer.remove(0, len);

                MSG_DEBUG("Played " << len << " bytes. " << mReadBuffer.size() << " remaining.");
            });

            mPullTimer->start(10);
        }

    }

    if ((mIntercom.mode == 1 || mIntercom.mode == 2) && mIntercom.txPort) // talk
    {
        if (mPushTimer)
            mPushTimer->stop();
        else
            mPushTimer = new QTimer();

        if (mMicrophone)
            delete mMicrophone;

        if (mMicDevice)
        {
            if (mMicDevice->isOpen())
                mMicDevice->stop();

            delete mMicDevice;
        }


        mMicrophone = new QAudioSource(mAudioMicDevice, mAudioFormat, this);
        connect(mMicrophone, &QAudioSource::stateChanged, this, &TQIntercom::onMicStateChanged);
        mMicDevice = new TMicrophone();
        mMicDevice->start();

        if (!connectTalker())
            return;

        mMicrophone->start(mMicDevice);
        QIODevice *io = reinterpret_cast<QIODevice *>(mMicDevice);

        if (mPushTimer)
        {
            mPushTimer->disconnect();

            connect(mPushTimer, &QTimer::timeout, [this, io]()
            {
                if (mUdpTalker->state() != QUdpSocket::ConnectedState || io == nullptr)
                    return;

                int len = io->bytesAvailable();
                MSG_DEBUG(len << " bytes available");
                int chunks = len / DATA_SIZE;

                if (chunks > 0)
                {
                    mMicOpen = true;

                    for (int i = 0; i < chunks; ++i)
                    {
                        QByteArray buffer(DATA_SIZE, 0);
                        QByteArray wbuf(PACKET_SIZE, 0);
                        len = io->read(buffer.data(), DATA_SIZE);

                        if (len > 0 && mTalkConnected)
                        {
                            len = getNextBlock(&wbuf, buffer);
                            MSG_DEBUG("Writing bytes: " << len);
                            mUdpTalker->write(wbuf.data(), len);
                            TError::logHex(wbuf.data(), len);
                        }
                    }
                }
                else if (len == 0)
                {
                    if (!mMicOpen)
                    {
                        QByteArray buffer(DATA_SIZE, uLawEncodeDigital(0));
                        QByteArray wbuf(PACKET_SIZE, 0);
                        len = getNextBlock(&wbuf, buffer);
                        MSG_DEBUG("Writing bytes: " << len);
                        mUdpTalker->write(wbuf.data(), len);
                        TError::logHex(wbuf.data(), len);
                    }
                }
                else
                {
                    mMicOpen = true;
                    QByteArray buffer(len, 0);
                    QByteArray wbuf(HEADER_SIZE + len, 0);

                    if (mTalkConnected)
                    {
                        len = getNextBlock(&wbuf, buffer);
                        MSG_DEBUG("Writing bytes: " << len);
                        mUdpTalker->write(wbuf.data(), len);
                    }
                }
            });

            mPushTimer->start(10);
        }
    }
}

void TQIntercom::stop()
{
    DECL_TRACER("TQIntercom::stop()");

    if ((mIntercom.mode == 0 || mIntercom.mode == 2) && mIntercom.rxPort)   // listen
    {
        if (mRemote)
        {
            if (mPullTimer)
            {
                mPullTimer->stop();
                mPullTimer->disconnect();
            }

            mRemote->stop();
            delete mRemote;
            mRemote = nullptr;
        }

        if (mUdpListener)
        {
            mUdpListener->close();
            delete mUdpListener;
            mUdpListener = nullptr;
        }
    }

    if ((mIntercom.mode == 1 || mIntercom.mode == 2) && mIntercom.txPort) // talk
    {
        if (mPushTimer)
        {
            mPushTimer->stop();
            mPushTimer->disconnect();
        }

        if (mMicDevice)
        {
            mMicDevice->stop();
            mMicOpen = false;
        }

        if (mMicrophone)
        {
            mMicrophone->stop();
            delete mMicrophone;
            mMicrophone = nullptr;
        }

        if (mUdpTalker)
        {
            mUdpTalker->close();
            delete mUdpTalker;
            mUdpTalker = nullptr;
        }
    }
}

void TQIntercom::setMute(bool mute)
{
    DECL_TRACER("TQIntercom::setMute(bool mute)");

    if (mIntercom.mode == 1 || mIntercom.mode == 2)
    {
        if (mMicrophone)
            mMicrophone->setVolume(mute ? 0.0 : convertVolume(mMicLevel));
    }
}

bool TQIntercom::connectTalker()
{
    DECL_TRACER("TQIntercom::connectTalker()");

    // First we initialize the socket
    QHostAddress hostAddress(QString(mIntercom.ip.c_str()));

    if (mUdpTalker)
    {
        if (mUdpTalker->isOpen())
            mUdpTalker->close();

        delete mUdpTalker;
    }

    mUdpTalker = new QUdpSocket(this);
    connect(mUdpTalker, &QUdpSocket::stateChanged, this, &TQIntercom::onOutputStateChanged);
    connect(mUdpTalker, &QUdpSocket::errorOccurred, this, &TQIntercom::onOutputErrorOccurred);
    mUdpTalker->connectToHost(mIntercom.ip.c_str(), mIntercom.txPort);
    return true;
}

bool TQIntercom::spawnServer()
{
    DECL_TRACER("TQIntercom::spawnServer()");

    if (mUdpListener)
    {
        if (mUdpListener->isOpen())
            mUdpListener->close();

        delete mUdpListener;
    }

    mUdpListener = new QUdpSocket(this);
    mUdpListener->setReadBufferSize(PACKET_SIZE);

    if (!mUdpListener->bind(QHostAddress::Any, mIntercom.rxPort))
    {
        delete mUdpListener;
        mUdpListener = nullptr;
        MSG_WARNING("Couldn't bind to devices at port " << mIntercom.rxPort << "!");
        return false;
    }

    connect(mUdpListener, &QUdpSocket::stateChanged, this, &TQIntercom::onInputStateChanged);
    connect(mUdpListener, &QUdpSocket::errorOccurred, this, &TQIntercom::onInputErrorOccurred);
    connect(mUdpListener, &QUdpSocket::readyRead, this, &TQIntercom::onReadPendingDatagrams);

    return true;
}

qreal TQIntercom::convertVolume(int volume)
{
    DECL_TRACER("TQIntercom::converVolume(int volume)");

    return QAudio::convertVolume(static_cast<qreal>(volume) / qreal(100), QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);
}

void TQIntercom::onRecordPermissionGranted()
{
    DECL_TRACER("TQIntercom::onRecordPermissionGranted()");

    mRecordPermissionGranted = true;
}

void TQIntercom::onReadPendingDatagrams()
{
    DECL_TRACER("TQIntercom::onReadPendingDatagrams()");

    while (mUdpListener->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = mUdpListener->receiveDatagram();
        QByteArray data = datagram.data();

        if (mRemote)
        {
            for (qsizetype i = 0; i < data.size(); ++i)
            {
                if (i < HEADER_SIZE)
                    continue;

                uint16_t word = muLawToLinear(data[i]);
                uint8_t hbyte = word >> 8;
                uint8_t lbyte = word;
                mReadBuffer.append(hbyte);
                mReadBuffer.append(lbyte);
            }

            TError::logHex(data.data(), data.size());
        }
    }
}

void TQIntercom::onInputStateChanged(QAbstractSocket::SocketState socketState)
{
    DECL_TRACER("TQIntercom::onInputStateChanged(QAbstractSocket::SocketState socketState)");

    if (socketState == QAbstractSocket::ConnectedState)
    {
        QAudioDevice info(QMediaDevices::defaultAudioInput());

        if (!info.isFormatSupported(mAudioFormat))
        {
            MSG_WARNING("Raw audio device can not be captured!");
            return;
        }
    }

    stateChangedMessage(socketState, true);
}

void TQIntercom::onOutputStateChanged(QAbstractSocket::SocketState socketState)
{
    DECL_TRACER("TQIntercom::onOutputStateChanged(QAbstractSocket::SocketState socketState)");

    if (socketState == QAbstractSocket::ConnectedState)
        mTalkConnected = true;
    else
        mTalkConnected = false;

    stateChangedMessage(socketState, false);
}

void TQIntercom::onMicStateChanged(QAudio::State state)
{
    DECL_TRACER("TQIntercom::onMicStateChanged(QAudio::State state)");

    switch(state)
    {
        case QAudio::ActiveState:       MSG_DEBUG("Microphone is active"); break;
        case QAudio::SuspendedState:    MSG_DEBUG("Microphone is suspended"); break;
        case QAudio::StoppedState:      MSG_DEBUG("Microphone is stopped"); break;
        case QAudio::IdleState:         MSG_DEBUG("Microphone is idle"); break;
    }
}

void TQIntercom::stateChangedMessage(QAbstractSocket::SocketState socketState, bool in)
{
    if (!TStreamError::checkFilter(HLOG_DEBUG))
        return;

    DECL_TRACER("TQIntercom::stateChangedMessage(QAbstractSocket::SocketState socketState, bool in)");

    string dir = in ? "input" : "output";

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        switch(socketState)
        {
            case QAbstractSocket::UnconnectedState: MSG_DEBUG("State " << dir << ": Unconnected"); break;
            case QAbstractSocket::HostLookupState:  MSG_DEBUG("State " << dir << ": Looking up host"); break;
            case QAbstractSocket::ConnectingState:  MSG_DEBUG("State " << dir << ": Connecting"); break;
            case QAbstractSocket::ConnectedState:   MSG_DEBUG("State " << dir << ": Connected"); break;
            case QAbstractSocket::BoundState:       MSG_DEBUG("State " << dir << ": Bound"); break;
            case QAbstractSocket::ListeningState:   MSG_DEBUG("State " << dir << ": Listening"); break;
            case QAbstractSocket::ClosingState:     MSG_DEBUG("State " << dir << ": Closing"); break;
        }
    }
}

void TQIntercom::onInputErrorOccurred(QAbstractSocket::SocketError socketError)
{
    DECL_TRACER("TQIntercom::onInputErrorOccurred(QAbstractSocket::SocketError socketError)");

    socketErrorMessages(socketError, "Receive packet");
}

void TQIntercom::onOutputErrorOccurred(QAbstractSocket::SocketError socketError)
{
    DECL_TRACER("TQIntercom::onOutputErrorOccurred(QAbstractSocket::SocketError socketError)");

    socketErrorMessages(socketError, "Send packet");
}

void TQIntercom::socketErrorMessages(QAbstractSocket::SocketError socketError, const string& msg)
{
    DECL_TRACER("TQIntercom::socketErrorMessages(QAbstractSocket::SocketError socketError, const string& msg)");

    switch(socketError)
    {
        case QAbstractSocket::ConnectionRefusedError:           MSG_ERROR(msg << ": The connection was refused by the peer (or timed out)."); break;
        case QAbstractSocket::RemoteHostClosedError:            MSG_ERROR(msg << ": The remote host closed the connection."); break;
        case QAbstractSocket::HostNotFoundError:                MSG_ERROR(msg << ": The host address was not found."); break;
        case QAbstractSocket::SocketAccessError:                MSG_ERROR(msg << ": The socket operation failed because the application lacked the required privileges."); break;
        case QAbstractSocket::SocketResourceError:              MSG_ERROR(msg << ": The local system ran out of resources (e.g., too many sockets)."); break;
        case QAbstractSocket::SocketTimeoutError:               MSG_ERROR(msg << ": The socket operation timed out."); break;
        case QAbstractSocket::DatagramTooLargeError:            MSG_ERROR(msg << ": The datagram was larger than the operating system's limit."); break;
        case QAbstractSocket::NetworkError:                     MSG_ERROR(msg << ": An error occurred with the network (e.g., the network cable was accidentally plugged out)."); break;
        case QAbstractSocket::AddressInUseError:                MSG_ERROR(msg << ": The address specified to QAbstractSocket::bind() is already in use and was set to be exclusive."); break;
        case QAbstractSocket::SocketAddressNotAvailableError:   MSG_ERROR(msg << ": The address specified to QAbstractSocket::bind() does not belong to the host."); break;
        case QAbstractSocket::UnsupportedSocketOperationError:  MSG_ERROR(msg << ": The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support)."); break;
        case QAbstractSocket::ProxyAuthenticationRequiredError: MSG_ERROR(msg << ": The socket is using a proxy, and the proxy requires authentication."); break;
        case QAbstractSocket::SslHandshakeFailedError:          MSG_ERROR(msg << ": The SSL/TLS handshake failed, so the connection was closed"); break;
        case QAbstractSocket::UnfinishedSocketOperationError:   MSG_ERROR(msg << ": The last operation attempted has not finished yet (still in progress in the background)."); break;
        case QAbstractSocket::ProxyConnectionRefusedError:      MSG_ERROR(msg << ": Could not contact the proxy server because the connection to that server was denied"); break;
        case QAbstractSocket::ProxyConnectionClosedError:       MSG_ERROR(msg << ": The connection to the proxy server was closed unexpectedly (before the connection to the final peer was established)"); break;
        case QAbstractSocket::ProxyConnectionTimeoutError:      MSG_ERROR(msg << ": The connection to the proxy server timed out or the proxy server stopped responding in the authentication phase."); break;
        case QAbstractSocket::ProxyNotFoundError:               MSG_ERROR(msg << ": The proxy address set with setProxy() (or the application proxy) was not found."); break;
        case QAbstractSocket::ProxyProtocolError:               MSG_ERROR(msg << ": The connection negotiation with the proxy server failed, because the response from the proxy server could not be understood."); break;
        case QAbstractSocket::OperationError:                   MSG_ERROR(msg << ": An operation was attempted while the socket was in a state that did not permit it."); break;
        case QAbstractSocket::SslInternalError:                 MSG_ERROR(msg << ": The SSL library being used reported an internal error. This is probably the result of a bad installation or misconfiguration of the library."); break;
        case QAbstractSocket::SslInvalidUserDataError:          MSG_ERROR(msg << ": Invalid data (certificate, key, cypher, etc.) was provided and its use resulted in an error in the SSL library."); break;
        case QAbstractSocket::TemporaryError:                   MSG_ERROR(msg << ": A temporary error occurred (e.g., operation would block and socket is non-blocking)."); break;
        case QAbstractSocket::UnknownSocketError:               MSG_ERROR(msg << ": An unidentified error occurred."); break;
    }
}

long TQIntercom::getNextBlock(QByteArray* target, const QByteArray& data)
{
    DECL_TRACER("TQIntercom::getNextBlock(QByteArray* target, const QByteArray& data)");

    if (!target)
        return 0;

    long size = qMin(data.size(), DATA_SIZE);
    mHeader.counter++;
    mHeader.position += size;
    unsigned char bytes[10];

    if (target)
    {
        target->clear();

        uint16ToBytes(mHeader.ident, bytes);
        target->append(reinterpret_cast<char *>(bytes), 2);
        uint16ToBytes(mHeader.counter, bytes);
        target->append(reinterpret_cast<char *>(bytes), 2);
        uint32ToBytes(mHeader.position, bytes);
        target->append(reinterpret_cast<char *>(bytes), 4);
        uint16ToBytes(mHeader.unk1, bytes);
        target->append(reinterpret_cast<char *>(bytes), 2);
        uint16ToBytes(mHeader.unk2, bytes);
        target->append(reinterpret_cast<char *>(bytes), 2);
        target->append(data.right(size));
    }

    return size + HEADER_SIZE;
}

/*******************************************************************************
 * Methods of class _audioWrite start here
 ******************************************************************************/

TMicrophone::TMicrophone()
{
    DECL_TRACER("TMicrophone::TMicrophone(QAudioSource *source, QObject *parent)");
}

TMicrophone::~TMicrophone()
{
    DECL_TRACER("TMicrophone::~TMicrophone()");
}

void TMicrophone::start()
{
    DECL_TRACER("TMicrophone::start()");

    open(QIODevice::ReadWrite);

    if (!mBuffer.isEmpty())
        mBuffer.clear();
}

void TMicrophone::stop()
{
    DECL_TRACER("TMicrophone::stop()");

    mPos = 0;
    close();
    mBuffer.clear();
}

bool _first = true;

qint64 TMicrophone::readData(char* data, qint64 len)
{
//    DECL_TRACER("TMicrophone::readData(char* data, qint64 len)");

    qint64 total = 0;

    if (!mBuffer.isEmpty())
    {
        qint64 posBuffer = 0;
        qint64 posData = 0;
        qint64 size = qMin(len, mBuffer.size());
        qint16 hbyte, lbyte, word;
        bool bigEndian = isBigEndian();

        while (posData < size)
        {
            if (posBuffer >= mBuffer.size())
                break;

            hbyte = mBuffer[posBuffer];
            posBuffer++;
            lbyte = mBuffer[posBuffer];
            posBuffer++;

            if (bigEndian)
                word = ((hbyte << 8) & 0xff00) | lbyte;
            else
                word = ((lbyte << 8) & 0xff00) | hbyte;

            *(data+posData) = linearToMuLaw(word);
            posData++;
        }

        mBuffer.remove(0, posBuffer);
        total = posData;
    }

    return total;
}

qint64 TMicrophone::writeData(const char* data, qint64 len)
{
    DECL_TRACER("TMicrophone::writeData(const char* data, qint64 len)");

    if (len > 0)
        mBuffer.append(data, len);

    MSG_DEBUG("Wrote " << len << " bytes to buffer.");
    return len;
}

qint64 TMicrophone::bytesAvailable() const
{
//    DECL_TRACER("TMicrophone::bytesAvailable() const");

    return (mBuffer.size() / 2) + QIODevice::bytesAvailable();
}

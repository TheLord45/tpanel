/*
 * Copyright (C) 2021, 2022 by Andreas Theofilu <andreas@theosys.at>
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
#include <QPushButton>
#include <QLabel>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#   include <QSound>
#else
#include <QMediaPlayer>
#include <QAudioOutput>
#endif

#include "tqkeypad.h"
#include "ui_keypad.h"
#include "terror.h"
#include "tresources.h"
#include "tconfig.h"
#include "tpagemanager.h"

extern TPageManager *gPageManager;              //!< The pointer to the global defined main class.

TQKeypad::TQKeypad(const std::string& init, const std::string& prompt, QWidget *parent, bool priv)
    : QDialog(parent),
      TSystemSound(TConfig::getSystemPath(TConfig::SOUNDS)),
      ui(new Ui::TQKeypad),
      mPrivate(priv),
      mText(init)
{
    DECL_TRACER("TQKeypad::TQKeypad(const std::string& init, const std::string& prompt, QWidget *parent)");

    ui->setupUi(this);
    connect(ui->key_Enter, SIGNAL(pressed()), this, SLOT(accept()));
    connect(ui->key_Cancel, SIGNAL(pressed()), this, SLOT(reject()));

    connect(ui->key_0, SIGNAL(pressed()), this, SLOT(key0()));
    connect(ui->key_1, SIGNAL(pressed()), this, SLOT(key1()));
    connect(ui->key_2, SIGNAL(pressed()), this, SLOT(key2()));
    connect(ui->key_3, SIGNAL(pressed()), this, SLOT(key3()));
    connect(ui->key_4, SIGNAL(pressed()), this, SLOT(key4()));
    connect(ui->key_5, SIGNAL(pressed()), this, SLOT(key5()));
    connect(ui->key_6, SIGNAL(pressed()), this, SLOT(key6()));
    connect(ui->key_7, SIGNAL(pressed()), this, SLOT(key7()));
    connect(ui->key_8, SIGNAL(pressed()), this, SLOT(key8()));
    connect(ui->key_9, SIGNAL(pressed()), this, SLOT(key9()));
    connect(ui->key_Plus, SIGNAL(pressed()), this, SLOT(keyPlus()));
    connect(ui->key_Minus, SIGNAL(pressed()), this, SLOT(keyMinus()));
    connect(ui->key_Clear, SIGNAL(pressed()), this, SLOT(keyClear()));
    connect(ui->key_Dot, SIGNAL(pressed()), this, SLOT(keyDot()));
    connect(ui->key_DoubleDot, SIGNAL(pressed()), this, SLOT(keyDoubleDot()));
    connect(ui->key_Komma, SIGNAL(pressed()), this, SLOT(keyKomma()));

    ui->label_Prompt->setText(prompt.c_str());

    if (!mPrivate)
        ui->label_TextLine->setText(init.c_str());
    else
        ui->label_TextLine->setText(fillString('*', mText.length()).c_str());

    MSG_DEBUG("Dialog was initialized.");
}

TQKeypad::~TQKeypad()
{
    DECL_TRACER("TQKeypad::~TQKeypad()");
    delete ui;
}

void TQKeypad::doResize()
{
    DECL_TRACER("TQKeypad::doResize()");

#ifndef Q_OS_IOS
    QRect rect = this->geometry();
#if defined(Q_OS_ANDROID)
    QSize size = this->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    this->resize(size);
    this->move(scale(rect.left()), scale(rect.top()));
#endif  // Q_OS_ANDROID
    QWidget *parent = this->parentWidget();

    if (parent)
    {
        rect = parent->geometry();
        this->move(rect.center() - this->rect().center());
    }

#if defined(Q_OS_ANDROID)
    // Iterate through childs and resize them
    QObjectList childs = children();
    QList<QObject *>::Iterator iter;

    for (iter = childs.begin(); iter != childs.end(); ++iter)
    {
        QObject *obj = *iter;
        QString name = obj->objectName();

        if (name.startsWith("key_"))
        {
            QPushButton *bt = dynamic_cast<QPushButton *>(obj);
            size = bt->size();
            size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
            bt->resize(size);
            rect = bt->geometry();
            bt->move(scale(rect.left()), scale(rect.top()));
        }
        else    // It's a label
        {
            QLabel *lb = dynamic_cast<QLabel *>(obj);
            size = lb->size();
            size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
            lb->resize(size);
            rect = lb->geometry();
            lb->move(scale(rect.left()), scale(rect.top()));
        }
    }
#endif  // Q_OS_ANDROID
#endif  // ! Q_OS_IOS
}

void TQKeypad::setKey(Ui::KEYSP_t key)
{
    DECL_TRACER("TQKeypad::setKey(Ui::KEYSP_t key)");

    switch(key)
    {
        case Ui::KEYP_0:        mText += "0"; break;
        case Ui::KEYP_1:        mText += "1"; break;
        case Ui::KEYP_2:        mText += "2"; break;
        case Ui::KEYP_3:        mText += "3"; break;
        case Ui::KEYP_4:        mText += "4"; break;
        case Ui::KEYP_5:        mText += "5"; break;
        case Ui::KEYP_6:        mText += "6"; break;
        case Ui::KEYP_7:        mText += "7"; break;
        case Ui::KEYP_8:        mText += "8"; break;
        case Ui::KEYP_9:        mText += "9"; break;
        case Ui::KEYP_Plus:     mText += "+"; break;
        case Ui::KEYP_Minus:    mText += "-"; break;
        case Ui::KEYP_Komma:    mText += ","; break;
        case Ui::KEYP_Dot:      mText += "."; break;
        case Ui::KEYP_DoubleDot:mText += ":"; break;
        case Ui::KEYP_Clear:    mText.clear(); break;
    }

    if (mMaxLen > 0 && mText.length() > (size_t)mMaxLen)
        mText = mText.substr(0, mMaxLen);

    if (!mPrivate)
        ui->label_TextLine->setText(mText.c_str());
    else
        ui->label_TextLine->setText(fillString('*', mText.length()).c_str());

    if (getSystemSoundState())
    {
        std::string snd = getTouchFeedbackSound();
        MSG_DEBUG("Playing sound: " << snd);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QMediaPlayer *player = new QMediaPlayer;
        QAudioOutput *audioOutput = new QAudioOutput;
        player->setAudioOutput(audioOutput);
        player->setSource(QUrl::fromLocalFile(snd.c_str()));
        player->play();
        delete audioOutput;
        delete player;
#else
        QSound::play(snd.c_str());
#endif
    }

    if (gPageManager && gPageManager->getPassThrough() && !mText.empty() &&
        key != Ui::KEYP_Clear)
    {
        size_t pos = mText.length() - 1;
        gPageManager->sendKeyStroke(mText[pos]);
    }
}

void TQKeypad::setString(const std::string& str)
{
    DECL_TRACER("TQKeypad::setString(const string& str)");

    mText += str;

    if (mMaxLen > 0 && mText.length() > (size_t)mMaxLen)
        mText = mText.substr(0, mMaxLen);

    if (!mPrivate)
        ui->label_TextLine->setText(mText.c_str());
    else
        ui->label_TextLine->setText(fillString('*', mText.length()).c_str());
}

int TQKeypad::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}
